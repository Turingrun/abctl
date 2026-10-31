#include "crc32.h"

uint32_t ab_crc32(const uint8_t *bytes, size_t length)
{
    uint32_t crc = UINT32_MAX;

    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1U) ? UINT32_C(0xedb88320) : 0U);
    }
    return crc ^ UINT32_MAX;
}
