/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SELECTION_H
#define CHAON_SELECTION_H
#include <stdbool.h>
#include <stdint.h>
#include "chaon_fat_ro.h"
#include "chaon_script.h"

enum { CHAON_SOURCE_SHA256_BYTES = 32 };

/* Static fixed-capacity ownership. No HID authority or VM calls.
 * A trusted SD validation service computes SHA-256 over the exact source
 * bytes and supplies the digest alongside the bounded parsed program. */
typedef struct {
    bool valid;
    uint32_t staging_generation;
    uint32_t usb_epoch;
    uint8_t source_sha256[CHAON_SOURCE_SHA256_BYTES];
    chaon_fat_entry_t identity;
    chaon_program_t program;
} chaon_selection_t;

void chaon_selection_clear(chaon_selection_t *selection);
bool chaon_selection_load(chaon_selection_t *selection,
    const chaon_fat_entry_t *identity, const chaon_program_t *validated,
    uint32_t generation, uint32_t usb_epoch,
    const uint8_t sha256[CHAON_SOURCE_SHA256_BYTES]);
void chaon_selection_refresh(chaon_selection_t *selection, uint32_t usb_epoch,
                             bool source_still_fresh);
/* Metadata binding check, NOT sufficient authority to execute. */
bool chaon_selection_matches(const chaon_selection_t *selection,
    uint32_t generation, uint32_t usb_epoch,
    const uint8_t sha256[CHAON_SOURCE_SHA256_BYTES]);
#endif
