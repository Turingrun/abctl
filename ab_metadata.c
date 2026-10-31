#include "ab_metadata.h"
#include "crc32.h"

#include <string.h>

void ab_metadata_init(AvbABData *data)
{
    memset(data, 0, sizeof(*data));
    memcpy(data->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN);
    data->version_major = AVB_AB_MAJOR_VERSION;
    data->version_minor = AVB_AB_MINOR_VERSION;
    data->slots[0].priority = AVB_AB_MAX_PRIORITY;
    data->slots[1].priority = AVB_AB_MAX_PRIORITY - 1;
    data->slots[0].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    data->slots[1].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
}

bool ab_metadata_fields_valid(const AvbABData *data)
{
    for (size_t i = 0; i < 2; ++i) {
        const AvbABSlotData *slot = &data->slots[i];
        if (slot->priority > AVB_AB_MAX_PRIORITY ||
            slot->tries_remaining > AVB_AB_MAX_TRIES_REMAINING ||
            slot->successful_boot > 1)
            return false;
    }
    return true;
}

bool ab_metadata_valid(const AbValidation *validation)
{
    return validation->magic_ok && validation->version_ok &&
           validation->crc_ok && validation->fields_ok;
}

bool ab_metadata_decode(const uint8_t raw[AVB_AB_DATA_SIZE], AvbABData *data,
                        AbValidation *validation)
{
    memcpy(data, raw, AVB_AB_CRC_OFFSET);
    data->crc32 = ((uint32_t)raw[28] << 24) | ((uint32_t)raw[29] << 16) |
                  ((uint32_t)raw[30] << 8) | raw[31];
    validation->stored_crc = data->crc32;
    validation->computed_crc = ab_crc32(raw, AVB_AB_CRC_OFFSET);
    validation->magic_ok = memcmp(raw, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN) == 0;
    validation->version_ok = data->version_major <= AVB_AB_MAJOR_VERSION;
    validation->crc_ok = validation->stored_crc == validation->computed_crc;
    validation->fields_ok = ab_metadata_fields_valid(data);
    return ab_metadata_valid(validation);
}

void ab_metadata_encode(const AvbABData *data, uint8_t raw[AVB_AB_DATA_SIZE])
{
    memcpy(raw, data, AVB_AB_CRC_OFFSET);
    uint32_t crc = ab_crc32(raw, AVB_AB_CRC_OFFSET);
    raw[28] = (uint8_t)(crc >> 24);
    raw[29] = (uint8_t)(crc >> 16);
    raw[30] = (uint8_t)(crc >> 8);
    raw[31] = (uint8_t)crc;
}
