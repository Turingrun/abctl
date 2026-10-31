#ifndef MISC_IO_H
#define MISC_IO_H

#include <stdbool.h>
#include <stdint.h>

#define MISC_METADATA_OFFSET 2048
#define MISC_METADATA_SIZE 32

/* Return -1 with errno on failure. The lock lives until close. */
int misc_io_open(const char *path, bool writable);
int misc_io_read(int fd, uint8_t raw[MISC_METADATA_SIZE]);
/* Writes, fsyncs and compares readback. Caller validates CRC separately.
 * After failure the target may already have been modified. */
int misc_io_write_verified(int fd, const uint8_t raw[MISC_METADATA_SIZE],
                           uint8_t readback[MISC_METADATA_SIZE]);
int misc_io_close(int fd);

#endif
