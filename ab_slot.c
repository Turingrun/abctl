#include "ab_slot.h"

bool ab_slot_bootable(const AvbABSlotData *slot)
{
    return slot->priority > 0 && (slot->successful_boot || slot->tries_remaining > 0);
}

bool ab_slot_canonical(const AvbABSlotData *slot)
{
    if (slot->priority > AVB_AB_MAX_PRIORITY ||
        slot->tries_remaining > AVB_AB_MAX_TRIES_REMAINING || slot->successful_boot > 1)
        return false;
    if (slot->priority == 0)
        return slot->tries_remaining == 0 && slot->successful_boot == 0;
    return slot->successful_boot ? slot->tries_remaining == 0 : slot->tries_remaining > 0;
}

int ab_slot_preferred(const AvbABData *data)
{
    bool a = ab_slot_bootable(&data->slots[0]);
    bool b = ab_slot_bootable(&data->slots[1]);
    if (a && b)
        return data->slots[1].priority > data->slots[0].priority ? 1 : 0;
    return a ? 0 : (b ? 1 : -1);
}

static void disable_slot(AvbABSlotData *slot)
{
    slot->priority = 0;
    slot->tries_remaining = 0;
    slot->successful_boot = 0;
}

static void normalize_slot(AvbABSlotData *slot)
{
    if (!ab_slot_canonical(slot))
        disable_slot(slot);
}

AbSlotResult ab_slot_apply(AvbABData *data, unsigned slot, AbSlotAction action)
{
    if (slot > 1 || !ab_metadata_fields_valid(data))
        return AB_SLOT_INVALID_ARGUMENT;
    AvbABData next = *data;
    normalize_slot(&next.slots[0]);
    normalize_slot(&next.slots[1]);
    AvbABSlotData *target = &next.slots[slot];
    switch (action) {
    case AB_SLOT_ACTIVE:
        target->priority = AVB_AB_MAX_PRIORITY;
        target->tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
        target->successful_boot = 0;
        if (next.slots[1 - slot].priority == AVB_AB_MAX_PRIORITY)
            next.slots[1 - slot].priority = AVB_AB_MAX_PRIORITY - 1;
        break;
    case AB_SLOT_SUCCESSFUL:
        if (!ab_slot_bootable(target))
            return AB_SLOT_NOT_BOOTABLE;
        target->tries_remaining = 0;
        target->successful_boot = 1;
        break;
    case AB_SLOT_UNBOOTABLE:
        disable_slot(target);
        break;
    default:
        return AB_SLOT_INVALID_ARGUMENT;
    }
    *data = next;
    return AB_SLOT_OK;
}

AbSlotResult ab_slot_update(AvbABData *data, unsigned slot, const AbSlotUpdate *update)
{
    if (slot > 1 || !ab_metadata_fields_valid(data) ||
        !(update->has_priority || update->has_tries || update->has_successful))
        return AB_SLOT_INVALID_ARGUMENT;
    AvbABSlotData next = data->slots[slot];
    if (update->has_priority)
        next.priority = update->priority;
    if (update->has_tries)
        next.tries_remaining = update->tries;
    if (update->has_successful)
        next.successful_boot = update->successful;
    if (!ab_slot_canonical(&next))
        return AB_SLOT_INVALID_COMBINATION;
    data->slots[slot] = next;
    return AB_SLOT_OK;
}
