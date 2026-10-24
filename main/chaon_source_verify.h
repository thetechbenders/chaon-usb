/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SOURCE_VERIFY_H
#define CHAON_SOURCE_VERIFY_H
#include "chaon_fat_ro.h"
#include "chaon_source_sha256.h"
/* Synchronous, read-only verifier. Must run in SD I/O task, never inside a
 * critical section or while USB callbacks are executing. */
bool chaon_source_verify(chaon_fat_read_t read_sector, void *context,
    uint32_t card_sectors, uint8_t sector[CHAON_FAT_SECTOR_BYTES],
    uint8_t index, const chaon_fat_entry_t *expected,
    const uint8_t digest[CHAON_SHA256_SIZE],
    uint8_t source_buffer[2048]);
#endif
