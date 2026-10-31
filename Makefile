CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -O2 -g
LDFLAGS ?=
LDLIBS ?=

PROJECT_CPPFLAGS = -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -I.
PROJECT_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow
SOURCES = abctl.c cli.c ab_metadata.c ab_slot.c misc_io.c crc32.c
OBJECTS = $(SOURCES:%.c=build/%.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean
all: abctl

abctl: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: %.c | build
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) $(PROJECT_CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p $@

clean:
	$(RM) $(OBJECTS) $(DEPS) abctl

-include $(DEPS)
