#ifndef ABCTL_CRC32_H
#define ABCTL_CRC32_H

#include <stddef.h>
#include <stdint.h>

/* CRC-32/ISO-HDLC, equivalent to SDK avb_crc32() / crc32(0, ...). */
uint32_t ab_crc32(const uint8_t *bytes, size_t length);

#endif
