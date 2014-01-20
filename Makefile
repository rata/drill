
CFLAGS += -Wall -Wstrict-prototypes -O3
ALL_CFLAGS += $(CFLAGS) -std=c99 -pedantic

# LFS support (LFS_LDFLAGS are not needed on Linux)
ALL_CFLAGS += $(shell getconf LFS_CFLAGS 2>/dev/null)

# This is needed for fallocate() and pread()
ALL_CFLAGS += -D_GNU_SOURCE

ifneq ($(V), 1)
        NICE_CC = @echo "  CC  $@"; $(CC)
else
        NICE_CC = $(CC)
endif

default: drill

%.o: %.c
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

drill: drill.o
	$(NICE_CC) $^ $(LDFLAGS) -o $@

clean:
	@echo "  CLEAN"
	@rm -f drill drill.o


.PHONY: default clean
