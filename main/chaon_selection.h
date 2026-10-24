/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SELECTION_H
#define CHAON_SELECTION_H
#include <stdbool.h>
#include <stdint.h>
#include "chaon_fat_ro.h"
#include "chaon_script.h"

/* One fixed-capacity selected program. No VM or HID access in this module.
 * Caller must serialize mutations and keep this off small FreeRTOS stacks.
 * Fingerprint must describe the exact validated source bytes, not FAT metadata.
 */
typedef struct {
    bool valid;
    bool source_fresh;
    uint32_t staging_generation;
    uint32_t usb_epoch;
    uint64_t source_fingerprint;
    chaon_fat_entry_t identity;
    chaon_program_t program;
} chaon_selection_t;

void chaon_selection_clear(chaon_selection_t *selection);
/* Selection alone NEVER confers authorization to execute. */
bool chaon_selection_load(chaon_selection_t *selection,
    const chaon_fat_entry_t *identity, const chaon_program_t *validated,
    uint32_t generation, uint32_t usb_epoch, uint64_t fingerprint);
/* Invalidate the selected program whenever the USB epoch changes or source
 * freshness is lost. No automatic restoration on reconnect. */
void chaon_selection_refresh(chaon_selection_t *selection, uint32_t usb_epoch,
                             bool source_still_fresh);
/* Candidate authorization check ONLY; the caller must separately enforce
 * a fresh physical ARM/RUN token and atomic execution-owned transfer. */
bool chaon_selection_matches(const chaon_selection_t *selection,
    uint32_t generation, uint32_t usb_epoch, uint64_t fingerprint);
#endif
