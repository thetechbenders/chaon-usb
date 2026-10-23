/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_PROGRAM_STAGE_H
#define CHAON_PROGRAM_STAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "chaon_fat_ro.h"
#include "chaon_script.h"

/* One fixed-capacity staged program. The enclosing SD service owns this
 * object for firmware lifetime and MUST serialize every call. A request
 * supersedes and wipes the previous slot; take transfers only a small token
 * to the SD task; publish accepts only that generation and exact raw entry.
 * Copy-out does not consume or authorize the program. No function here
 * performs I/O, allocates memory, invokes the VM, or emits HID reports. */
typedef enum {
    CHAON_PROGRAM_STAGE_EMPTY,
    CHAON_PROGRAM_STAGE_PENDING,
    CHAON_PROGRAM_STAGE_READY
} chaon_program_stage_state_t;

typedef struct {
    uint32_t generation;
    chaon_program_stage_state_t state;
    bool queued;
    uint8_t index;
    chaon_fat_entry_t identity;
    chaon_program_t program;
} chaon_program_stage_t;

void chaon_program_stage_clear(chaon_program_stage_t *stage);
/* Invalid requests also clear any previously valid staged program. */
bool chaon_program_stage_request(chaon_program_stage_t *stage,
    const chaon_fat_catalog_t *catalog, uint8_t index,
    const chaon_fat_entry_t *expected);
bool chaon_program_stage_take(chaon_program_stage_t *stage,
    uint32_t *generation, uint8_t *index, chaon_fat_entry_t *identity);
bool chaon_program_stage_publish(chaon_program_stage_t *stage,
    uint32_t generation, uint8_t index,
    const chaon_fat_entry_t *identity,
    const chaon_program_t *validated_program);
/* The caller owns `out`; firmware callers must use static/non-stack storage.
 * Success also returns the exact stage generation. Failure zeroes generation
 * and clears `out`. The stage remains owned by the SD service. */
bool chaon_program_stage_copy(const chaon_program_stage_t *stage,
    uint8_t index, const chaon_fat_entry_t *identity,
    uint32_t *generation, chaon_program_t *out);

#endif
