
#include <linux/falloc.h>       /* FALLOC_FL_* flags */
#include <fcntl.h>              /* fallocate(), open() */
#include <sys/stat.h>           /* open() */
#include <stdio.h>              /* printf(), perror() */
#include <stdlib.h>             /* strtoul(), malloc() and friends */
#include <unistd.h>             /* lseek(), pread(), getopt(), sleep() */
#include <sys/types.h>          /* lseek() */
#include <string.h>             /* memcmp() */
#include <limits.h>             /* ULONG_MAX */

/*
 *  TODO:
 *      Add tests
 *      Use mmap ?
 */

void usage(void)
{
	char h[] = \
	"Usage: drill [options] <filename>\n"
	"\n"
	"Make a file sparse without using extra disk space, by just doing holes\n"
	"in the file when possible.\n"
	"\n"
	"You can think of this as doing a cp --sparse and renaming the dest\n"
	"file as the original, without the need for extra disk space.\n"
	"\n"
	"It uses linux and fs specific syscalls. It only works on Linux >= 2.6.38\n"
	"with filesystems that support FALLOC_FL_PUNCH_HOLE fallocate(2) mode.\n"
	"\n"
	"Options: \n"
	"  -s HOLE_SIZE  Size in kb of the minimum hole to dig (default: 32)\n"
	"  -h            Show this help\n"
	"\n"
	"Note that too small values for HOLE_SIZE might be ignored. And\n"
	"too big values might use lot of RAM and not detect many holes.\n"
	"\n"
	"Please report bugs to Rodrigo Campos <rodrigo@sdfg.com.ar>\n";

	printf("%s", h);
}

/* Returns 0 on failure, the value otherwise */
size_t parse_hole_size(char *optarg)
{
	size_t hole_size_kb = strtoul(optarg, NULL, 0);
	if (hole_size_kb == ULONG_MAX) {
		perror("Hole size");
		return 0;
	}
	if (hole_size_kb == 0) {
		printf("Error: hole size should be greater than 0\n");
		return 0;
	}

	/* This might overflow, but we don't really care */
	return hole_size_kb * 1024;
}

int dig_hole(int fd, off_t offset, off_t length)
{
	int r = fallocate(fd, FALLOC_FL_PUNCH_HOLE|FALLOC_FL_KEEP_SIZE,
	                  offset, length);
	if (r == -1)
		perror("fallocate failed");

	return r;
}

/*
 * Look for chunks of '\0's with size hole_size and when we find them, dig a
 * hole on that offset with that size
 */
int drill(int fd, size_t hole_size)
{
	int ret = 0;

	/* Create a buffer of '\0's to compare against */
	void *zeros = calloc(1, hole_size);
	if (zeros == NULL)
		return 1;

	/* buffer to read the file */
	ssize_t buf_len = hole_size;
	void *buf = malloc(buf_len);
	if (buf == NULL) {
		ret = 1;
		goto out;
	}

	off_t end = lseek(fd, 0, SEEK_END);
	if (end == -1) {
		perror("lseek failed");
		ret = 1;
		goto out;
	}

	for (off_t offset = 0; offset + hole_size <= end; offset += buf_len) {

		/* Try to read hole_size bytes */
		buf_len = pread(fd, buf, hole_size, offset);
		if (buf_len == -1) {
			perror("pread failed");
			ret = 1;
			goto out;
		}

		/* Always use buf_len, as we may read less than hole_size bytes */
		int not_zeros = memcmp(buf, zeros, buf_len);
		if (not_zeros)
			continue;

		ret = dig_hole(fd, offset, buf_len);
		if (ret)
			goto out;
	}
out:
	free(zeros);
	free(buf);
	return ret;
}

int main(int argc, char **argv)
{
	int opt;
	size_t hole_size = 1024 * 32;

	while ((opt = getopt(argc, argv, "s:h")) != -1) {
		switch(opt) {
		case 's':
			hole_size = parse_hole_size(optarg);
			if (hole_size == 0)
				return 1;
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return 1;
		}
	}

	if (optind == argc) {
		printf("Error: no filename specified\n");
		usage();
		return 1;
	}

	if (hole_size >= 100 * 1024 * 1024) {
		/* We allocate two buffers of size hole_size */
		size_t ram_mb = 2 * hole_size / 1024 / 1024;
		printf("WARNING: %zu MB RAM will be used\n", ram_mb);
		sleep(3);
	}

	char *fname = argv[argc - 1];
	int fd = open(fname, O_RDWR);
	if (fd == -1) {
		perror("Error opening file");
		return 1;
	}

	int ret = drill(fd, hole_size);

	int err = close(fd);
	if (err) {
		perror("write failed");
		ret = 1;
	}

	return ret;
}
