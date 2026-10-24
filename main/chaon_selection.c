/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_selection.h"
#include <string.h>

void chaon_selection_clear(chaon_selection_t *selection)
{
    if (selection) memset(selection, 0, sizeof *selection);
}

bool chaon_selection_load(chaon_selection_t *selection,
    const chaon_fat_entry_t *identity, const chaon_program_t *validated,
    uint32_t generation, uint32_t usb_epoch, uint64_t fingerprint)
{
    if (!selection) return false;
    chaon_selection_clear(selection);
    if (!identity || identity->is_directory || !validated ||
        validated->count == 0 || validated->count > CHAON_MAX_STEPS ||
        generation == 0 || usb_epoch == 0 || fingerprint == 0 ||
        memcmp(identity->raw_name + 8, "CHN", 3) != 0)
        return false;
    /* The validator is responsible for full per-op instruction validation.
     * This structure cannot be filled from untrusted raw source. */
    selection->identity = *identity;
    selection->program = *validated;
    selection->staging_generation = generation;
    selection->usb_epoch = usb_epoch;
    selection->source_fingerprint = fingerprint;
    selection->source_fresh = true;
    selection->valid = true;
    return true;
}

void chaon_selection_refresh(chaon_selection_t *selection,
                             uint32_t usb_epoch, bool source_still_fresh)
{
    if (!selection || !selection->valid) return;
    if (!source_still_fresh || !usb_epoch || selection->usb_epoch != usb_epoch)
        chaon_selection_clear(selection);
}

bool chaon_selection_matches(const chaon_selection_t *selection,
    uint32_t generation, uint32_t usb_epoch, uint64_t fingerprint)
{
    return selection && selection->valid && selection->source_fresh &&
        generation && usb_epoch && fingerprint &&
        selection->staging_generation == generation &&
        selection->usb_epoch == usb_epoch &&
        selection->source_fingerprint == fingerprint;
}
