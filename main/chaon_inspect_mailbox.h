/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_INSPECT_MAILBOX_H
#define CHAON_INSPECT_MAILBOX_H

#include <stdbool.h>
#include <stdint.h>
#include "chaon_fat_ro.h"
#include "chaon_inspect.h"

/* Portable state only. ESP32 callers provide the critical section.
 * Never contains script text or executable instructions. */
typedef struct {
    uint32_t generation;
    bool queued;
    uint8_t index;
    chaon_fat_entry_t identity;
    chaon_inspection_t report;
} chaon_inspect_mailbox_t;

void chaon_inspect_mailbox_clear(chaon_inspect_mailbox_t *box);
bool chaon_inspect_mailbox_request(chaon_inspect_mailbox_t *box,
    const chaon_fat_catalog_t *catalog, uint8_t index,
    const chaon_fat_entry_t *expected);
bool chaon_inspect_mailbox_take(chaon_inspect_mailbox_t *box,
    uint32_t *generation, uint8_t *index, chaon_fat_entry_t *identity);
void chaon_inspect_mailbox_publish(chaon_inspect_mailbox_t *box,
    uint32_t generation, uint8_t index, chaon_inspection_t report);
bool chaon_inspect_mailbox_snapshot(const chaon_inspect_mailbox_t *box,
    uint8_t index, const chaon_fat_entry_t *identity,
    chaon_inspection_t *out);
#endif
