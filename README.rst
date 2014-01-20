drill: Make a file sparse without using extra disk space
--------------------------------------------------------
::

        drill [options] <filename>

Make a file sparse without using extra disk space, by just doing holes
in the file when possible.

You can think of this as doing a ``cp --sparse`` and renaming the dest
file as the original, without the need for extra disk space.

It uses linux and fs specific syscalls. It only works on Linux >= 2.6.38
with filesystems that support ``FALLOC_FL_PUNCH_HOLE`` fallocate(2) mode.
Some filesystems that support this are: btrfs, ext4, f2fs, ocfs2 and xfs.

Options:
  -s HOLE_SIZE  Size in kb of the minimum hole to dig (default: 32)
  -h            Show this help

Note that too small values for ``HOLE_SIZE`` might be ignored. And
too big values might use lot of RAM and not detect many holes.

Compiling
---------
To compile drill you just need a C99 compiler, a recent glibc, make and the
Linux kernel headers (from a kernel >= 2.6.38). To compile you can run::

        make

and it should just work.


Contributions
-------------
Of course, patches and bug reports are welcome. Please send them to me, Rodrigo
Campos, at rodrigo@sdfg.com.ar
