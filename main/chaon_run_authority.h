/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_RUN_AUTHORITY_H
#define CHAON_RUN_AUTHORITY_H
#include "chaon_control.h"
#include "chaon_execution_slot.h"
/* Portable adapter between debounced controller events and selected-program
 * grant issuance. The main control task is the sole event owner. */
typedef struct {
    bool armed;
    int64_t armed_ms;
    uint32_t generation;
    uint32_t usb_epoch;
    uint8_t digest[CHAON_SOURCE_SHA256_BYTES];
} chaon_run_authority_t;
void chaon_run_authority_cancel(chaon_run_authority_t *a);
bool chaon_run_authority_step(chaon_run_authority_t *a,
    chaon_control_event_t event, int64_t now_ms, bool usb_mounted,
    bool source_available, const chaon_selection_t *selection,
    chaon_execution_grant_t *out_grant);
#endif
