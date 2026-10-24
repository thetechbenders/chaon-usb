/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_execution_slot.h"
#include <string.h>
void chaon_execution_slot_clear(chaon_execution_slot_t *slot)
{
    if(slot) memset(slot,0,sizeof *slot);
}
bool chaon_execution_slot_prepare(chaon_execution_slot_t *slot,
    const chaon_selection_t *selection, uint32_t generation,
    uint32_t usb_epoch, const uint8_t digest[CHAON_SOURCE_SHA256_BYTES],
    bool fresh_arm_run_authorization)
{
    if(!slot) return false;
    chaon_execution_slot_clear(slot);
    if(!fresh_arm_run_authorization ||
       !chaon_selection_matches(selection,generation,usb_epoch,digest))
        return false;
    slot->program=selection->program;
    slot->usb_epoch=usb_epoch;
    slot->ready=true;
    return true;
}
bool chaon_execution_slot_take(chaon_execution_slot_t *slot,
    uint32_t usb_epoch, chaon_program_t *out)
{
    if(!out) { chaon_execution_slot_clear(slot); return false; }
    memset(out,0,sizeof *out);
    if(!slot || !slot->ready || !usb_epoch || slot->usb_epoch!=usb_epoch) {
        chaon_execution_slot_clear(slot);
        return false;
    }
    *out=slot->program;
    chaon_execution_slot_clear(slot);
    return true;
}
