/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_EXECUTION_SLOT_H
#define CHAON_EXECUTION_SLOT_H
#include "chaon_selection.h"

/* An event-scoped, single-use grant. Runtime controller integration MUST
 * issue this only upon the second debounced physical ARM/RUN gesture. */
typedef struct {
    bool issued;
    uint32_t staging_generation;
    uint32_t usb_epoch;
    uint8_t digest[CHAON_SOURCE_SHA256_BYTES];
} chaon_execution_grant_t;

typedef struct {
    bool ready;
    uint32_t staging_generation;
    uint32_t usb_epoch;
    uint8_t digest[CHAON_SOURCE_SHA256_BYTES];
    chaon_program_t program;
} chaon_execution_slot_t;

void chaon_execution_grant_clear(chaon_execution_grant_t *grant);
/* Accept only a controller-confirmed RUN edge. The controller remains
 * responsible for 30s arming expiry and fresh physical gesture checks. */
bool chaon_execution_grant_issue(chaon_execution_grant_t *grant,
    const chaon_selection_t *selected, bool physical_run_edge);
void chaon_execution_slot_clear(chaon_execution_slot_t *slot);
/* Preparing a slot consumes the grant; it NEVER calls HID/VM.
 * The selected program MUST still be valid at preparation and at take. */
bool chaon_execution_slot_prepare(chaon_execution_slot_t *slot,
    chaon_execution_grant_t *grant, const chaon_selection_t *selected,
    uint32_t generation, uint32_t usb_epoch,
    const uint8_t digest[CHAON_SOURCE_SHA256_BYTES],
    bool fresh_source_verified);
/* Take consumes slot only if selected source and epoch are still identical.
 * fresh_source_verified must originate from a new SD read, not the old hash. */
bool chaon_execution_slot_take(chaon_execution_slot_t *slot,
    const chaon_selection_t *selected, uint32_t generation,
    uint32_t usb_epoch, const uint8_t digest[CHAON_SOURCE_SHA256_BYTES],
    bool fresh_source_verified, chaon_program_t *out);
#endif
