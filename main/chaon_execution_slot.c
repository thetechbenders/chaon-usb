/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_execution_slot.h"
#include <string.h>

void chaon_execution_grant_clear(chaon_execution_grant_t *g)
{
    if (g) memset(g,0,sizeof *g);
}
bool chaon_execution_grant_issue(chaon_execution_grant_t *g,
    const chaon_selection_t *s, bool physical_run_edge)
{
    if (!g) return false;
    chaon_execution_grant_clear(g);
    if (!physical_run_edge || !s || !s->valid ||
        !s->staging_generation || !s->usb_epoch) return false;
    g->staging_generation=s->staging_generation;
    g->usb_epoch=s->usb_epoch;
    memcpy(g->digest,s->source_sha256,sizeof g->digest);
    g->issued=true;
    return true;
}
void chaon_execution_slot_clear(chaon_execution_slot_t *slot)
{
    if (slot) memset(slot,0,sizeof *slot);
}
bool chaon_execution_slot_prepare(chaon_execution_slot_t *slot,
    chaon_execution_grant_t *grant, const chaon_selection_t *selected,
    uint32_t generation, uint32_t usb_epoch,
    const uint8_t digest[CHAON_SOURCE_SHA256_BYTES],
    bool fresh_source_verified)
{
    if (!slot) { chaon_execution_grant_clear(grant); return false; }
    chaon_execution_slot_clear(slot);
    if (!grant) return false;
    const bool permitted=grant->issued && fresh_source_verified &&
        chaon_selection_matches(selected,generation,usb_epoch,digest) &&
        grant->staging_generation==generation &&
        grant->usb_epoch==usb_epoch &&
        memcmp(grant->digest,digest,sizeof grant->digest)==0;
    chaon_execution_grant_clear(grant); /* fail and success both consume */
    if (!permitted) return false;
    slot->program=selected->program;
    slot->staging_generation=generation;
    slot->usb_epoch=usb_epoch;
    memcpy(slot->digest,digest,sizeof slot->digest);
    slot->ready=true;
    return true;
}
bool chaon_execution_slot_take(chaon_execution_slot_t *slot,
    const chaon_selection_t *selected, uint32_t generation,
    uint32_t usb_epoch, const uint8_t digest[CHAON_SOURCE_SHA256_BYTES],
    bool fresh_source_verified, chaon_program_t *out)
{
    if (out) memset(out,0,sizeof *out);
    if (!slot) return false;
    const bool valid=out && slot->ready && fresh_source_verified &&
        chaon_selection_matches(selected,generation,usb_epoch,digest) &&
        slot->staging_generation==generation &&
        slot->usb_epoch==usb_epoch &&
        memcmp(slot->digest,digest,sizeof slot->digest)==0;
    if (valid) *out=slot->program;
    chaon_execution_slot_clear(slot);
    return valid;
}
