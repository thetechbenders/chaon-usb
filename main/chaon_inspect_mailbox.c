/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_inspect_mailbox.h"
#include <string.h>

static bool same_file(const chaon_fat_entry_t *a,
                      const chaon_fat_entry_t *b)
{
    return a && b && a->is_directory == b->is_directory &&
        a->first_cluster == b->first_cluster &&
        a->file_size == b->file_size &&
        memcmp(a->raw_name, b->raw_name, sizeof a->raw_name) == 0;
}

void chaon_inspect_mailbox_clear(chaon_inspect_mailbox_t *box)
{
    if (!box) return;
    ++box->generation;
    box->queued = false;
    box->index = 0;
    memset(&box->identity, 0, sizeof box->identity);
    box->report = (chaon_inspection_t){ .state=CHAON_INSPECT_NONE };
}

bool chaon_inspect_mailbox_request(chaon_inspect_mailbox_t *box,
    const chaon_fat_catalog_t *catalog, uint8_t index,
    const chaon_fat_entry_t *expected)
{
    if (!box || !catalog || !expected || index >= catalog->count ||
        catalog->count > CHAON_FAT_MAX_FILES || expected->is_directory ||
        !same_file(&catalog->entries[index], expected))
        return false;
    chaon_inspect_mailbox_clear(box);
    box->index = index;
    box->identity = *expected;
    box->report.state = CHAON_INSPECT_PENDING;
    box->queued = true;
    return true;
}

bool chaon_inspect_mailbox_take(chaon_inspect_mailbox_t *box,
    uint32_t *generation, uint8_t *index, chaon_fat_entry_t *identity)
{
    if (!box || !generation || !index || !identity || !box->queued)
        return false;
    *generation = box->generation;
    *index = box->index;
    *identity = box->identity;
    box->queued = false;
    return true;
}

void chaon_inspect_mailbox_publish(chaon_inspect_mailbox_t *box,
    uint32_t generation, uint8_t index, chaon_inspection_t report)
{
    if (!box || box->generation != generation || box->index != index ||
        box->report.state != CHAON_INSPECT_PENDING)
        return;
    box->report = report;
}

bool chaon_inspect_mailbox_snapshot(const chaon_inspect_mailbox_t *box,
    uint8_t index, const chaon_fat_entry_t *identity,
    chaon_inspection_t *out)
{
    if (!box || !out || box->report.state == CHAON_INSPECT_NONE ||
        box->index != index || !same_file(&box->identity, identity))
        return false;
    *out = box->report;
    return true;
}
