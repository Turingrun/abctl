#ifndef AB_SLOT_H
#define AB_SLOT_H

#include "ab_metadata.h"

typedef enum {
    AB_SLOT_ACTIVE,
    AB_SLOT_SUCCESSFUL,
    AB_SLOT_UNBOOTABLE
} AbSlotAction;

typedef struct {
    bool has_priority;
    bool has_tries;
    bool has_successful;
    uint8_t priority;
    uint8_t tries;
    uint8_t successful;
} AbSlotUpdate;

typedef enum {
    AB_SLOT_OK,
    AB_SLOT_INVALID_ARGUMENT,
    AB_SLOT_NOT_BOOTABLE,
    AB_SLOT_INVALID_COMBINATION
} AbSlotResult;

bool ab_slot_bootable(const AvbABSlotData *slot);
bool ab_slot_canonical(const AvbABSlotData *slot);
/* Returns 0 (A), 1 (B), or -1 (none). Caller must validate metadata first. */
int ab_slot_preferred(const AvbABData *data);
/* Failed operations leave data unchanged. Standard actions normalize both slots. */
AbSlotResult ab_slot_apply(AvbABData *data, unsigned slot, AbSlotAction action);
AbSlotResult ab_slot_update(AvbABData *data, unsigned slot, const AbSlotUpdate *update);

#endif
