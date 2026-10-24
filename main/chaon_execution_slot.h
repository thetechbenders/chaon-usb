/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_EXECUTION_SLOT_H
#define CHAON_EXECUTION_SLOT_H
#include "chaon_selection.h"
/* Separate one-shot, execution-owned program slot; no HID or transport.
 * Caller must hold an external controller lock and supply fresh physical
 * ARM/RUN authority. A successful take consumes that authority elsewhere. */
typedef struct {
    bool ready;
    uint32_t usb_epoch;
    chaon_program_t program;
} chaon_execution_slot_t;
void chaon_execution_slot_clear(chaon_execution_slot_t *slot);
/* A prepared slot is inert: this function does not execute it. */
bool chaon_execution_slot_prepare(chaon_execution_slot_t *slot,
    const chaon_selection_t *selection, uint32_t generation,
    uint32_t usb_epoch, const uint8_t digest[CHAON_SOURCE_SHA256_BYTES],
    bool fresh_arm_run_authorization);
bool chaon_execution_slot_take(chaon_execution_slot_t *slot,
    uint32_t usb_epoch, chaon_program_t *out);
#endif
