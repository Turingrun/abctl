#ifndef AB_METADATA_H
#define AB_METADATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AVB_AB_MAGIC "\0AB0"
#define AVB_AB_MAGIC_LEN 4
#define AVB_AB_MAJOR_VERSION 1
#define AVB_AB_MINOR_VERSION 0
#define AVB_AB_DATA_SIZE 32
#define AVB_AB_CRC_OFFSET 28
#define AVB_AB_MAX_PRIORITY 15
#define AVB_AB_MAX_TRIES_REMAINING 7

typedef struct {
    uint8_t priority;
    uint8_t tries_remaining;
    uint8_t successful_boot;
    uint8_t reserved[1];
} AvbABSlotData;

/* crc32 is host-endian in memory; use encode/decode for on-disk bytes. */
typedef struct {
    uint8_t magic[AVB_AB_MAGIC_LEN];
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t reserved1[2];
    AvbABSlotData slots[2];
    uint8_t last_boot;
    uint8_t reserved2[11];
    uint32_t crc32;
} AvbABData;

_Static_assert(sizeof(AvbABSlotData) == 4, "slot layout mismatch");
_Static_assert(sizeof(AvbABData) == AVB_AB_DATA_SIZE, "metadata layout mismatch");
_Static_assert(offsetof(AvbABData, slots) == 8, "slots offset mismatch");
_Static_assert(offsetof(AvbABData, last_boot) == 16, "last_boot offset mismatch");
_Static_assert(offsetof(AvbABData, crc32) == AVB_AB_CRC_OFFSET, "CRC offset mismatch");

typedef struct {
    bool magic_ok;
    bool version_ok;
    bool crc_ok;
    bool fields_ok;
    uint32_t stored_crc;
    uint32_t computed_crc;
} AbValidation;

void ab_metadata_init(AvbABData *data);
/* Always decodes for diagnostics; returns false for invalid metadata. */
bool ab_metadata_decode(const uint8_t raw[AVB_AB_DATA_SIZE], AvbABData *data,
                        AbValidation *validation);
void ab_metadata_encode(const AvbABData *data, uint8_t raw[AVB_AB_DATA_SIZE]);
bool ab_metadata_fields_valid(const AvbABData *data);
bool ab_metadata_valid(const AbValidation *validation);

#endif
