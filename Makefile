# Run make clean before changing the compiler, sysroot, or flags.
CROSS_COMPILE ?= aarch64-none-linux-gnu-
SYSROOT ?= /home/ma/local_sysroot
CC = $(CROSS_COMPILE)gcc

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64
CFLAGS = -O2 -g -std=c11 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow

ifneq ($(SYSROOT),)
CPPFLAGS += --sysroot=$(SYSROOT) \
	-isystem $(SYSROOT)/usr/local/include \
	-isystem $(SYSROOT)/usr/include/aarch64-linux-gnu \
	-isystem $(SYSROOT)/usr/include
LDFLAGS += --sysroot=$(SYSROOT) \
	-B$(SYSROOT)/usr/lib/aarch64-linux-gnu \
	-L$(SYSROOT)/usr/lib/aarch64-linux-gnu \
	-L$(SYSROOT)/usr/local/lib
endif

SOURCES := abctl.c cli.c ab_metadata.c ab_slot.c misc_io.c crc32.c
OBJECTS := $(SOURCES:%.c=build/%.o)

.PHONY: all clean
all: abctl

abctl: $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

build/%.o: %.c $(wildcard *.h) Makefile | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@

clean:
	$(RM) $(OBJECTS) abctl
