/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_fat_ro.h"
#include "chaon_script.h"
#include <string.h>

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static chaon_fat_file_result_t clear_error(uint8_t out[2048],
    size_t *length, chaon_fat_file_result_t error)
{
    if (out) memset(out, 0, CHAON_MAX_SOURCE);
    if (length) *length = 0;
    return error;
}

chaon_fat_file_result_t chaon_fat_read_script(chaon_fat_read_t read_sector,
    void *ctx, uint32_t card_sectors, uint8_t work[CHAON_FAT_SECTOR_BYTES],
    uint8_t index, const chaon_fat_entry_t *expected,
    uint8_t *out, size_t out_capacity, size_t *out_length)
{
    if (out_length) *out_length = 0;
    if (!out || !out_length || out_capacity < CHAON_MAX_SOURCE)
        return CHAON_FAT_FILE_INVALID;
    memset(out, 0, CHAON_MAX_SOURCE);
    if (!read_sector || !work || !expected || !card_sectors ||
        index >= CHAON_FAT_MAX_FILES)
        return CHAON_FAT_FILE_INVALID;

    /* Original script v0 format is selected ONLY by the unambiguous raw
     * DOS extension, never by a sanitized UI string or an LFN alias. */
    if (expected->is_directory ||
        memcmp(expected->raw_name + 8, "CHN", 3) != 0)
        return CHAON_FAT_FILE_UNSUPPORTED;
    if (expected->file_size == 0 || expected->file_size >= CHAON_MAX_SOURCE)
        return CHAON_FAT_FILE_TOO_LARGE;

    /* Card geometry and selected file identity MUST be revalidated using
     * fresh disk reads before any data-sector read. */
    chaon_fat_catalog_t fresh;
    const chaon_fat_result_t scan = chaon_fat_list_root(read_sector, ctx,
        card_sectors, work, &fresh);
    if (scan == CHAON_FAT_IO_ERROR) return CHAON_FAT_FILE_IO_ERROR;
    if (scan != CHAON_FAT_OK && scan != CHAON_FAT_LIMIT)
        return CHAON_FAT_FILE_INVALID;
    if (fresh.type != CHAON_FAT32) return CHAON_FAT_FILE_UNSUPPORTED;
    if (index >= fresh.count) return CHAON_FAT_FILE_STALE;
    const chaon_fat_entry_t *e = &fresh.entries[index];
    if (e->is_directory || e->is_directory != expected->is_directory ||
        memcmp(e->raw_name, expected->raw_name, 11) != 0 ||
        e->file_size != expected->file_size ||
        e->first_cluster != expected->first_cluster)
        return CHAON_FAT_FILE_STALE;

    uint32_t cluster = e->first_cluster & 0x0FFFFFFFU;
    uint32_t remaining = e->file_size;
    size_t written = 0;
    uint32_t visited[4] = {0};
    unsigned hops = 0;
    while (remaining) {
        if (cluster < 2 || (uint64_t)cluster >=
                (uint64_t)fresh.cluster_count + 2U || hops >= 4)
            return clear_error(out, out_length, CHAON_FAT_FILE_INVALID);
        for (unsigned i=0;i<hops;++i)
            if (visited[i] == cluster)
                return clear_error(out, out_length, CHAON_FAT_FILE_INVALID);
        visited[hops++] = cluster;
        const uint64_t start = (uint64_t)fresh.data_start +
            (uint64_t)(cluster - 2U) * fresh.sectors_per_cluster;
        if (start >= fresh.volume_end ||
            (uint64_t)fresh.sectors_per_cluster >
                (uint64_t)fresh.volume_end - start ||
            start >= card_sectors ||
            fresh.sectors_per_cluster > card_sectors - start)
            return clear_error(out, out_length, CHAON_FAT_FILE_INVALID);
        for (unsigned s=0;s<fresh.sectors_per_cluster && remaining;++s) {
            if (!read_sector(ctx, (uint32_t)(start+s), work))
                return clear_error(out, out_length, CHAON_FAT_FILE_IO_ERROR);
            const size_t count = remaining < CHAON_FAT_SECTOR_BYTES ?
                                 remaining : CHAON_FAT_SECTOR_BYTES;
            if (written + count >= CHAON_MAX_SOURCE)
                return clear_error(out, out_length, CHAON_FAT_FILE_TOO_LARGE);
            memcpy(out+written, work, count);
            written += count;
            remaining -= (uint32_t)count;
        }
        if (!remaining) break;
        const uint64_t offset = (uint64_t)cluster * 4U;
        if (offset / 512U >= fresh.fat_sectors ||
            (uint64_t)fresh.active_fat + offset / 512U >= card_sectors ||
            (uint64_t)fresh.active_fat + offset / 512U >= fresh.volume_end)
            return clear_error(out, out_length, CHAON_FAT_FILE_INVALID);
        if (!read_sector(ctx, (uint32_t)(fresh.active_fat + offset / 512U), work))
            return clear_error(out, out_length, CHAON_FAT_FILE_IO_ERROR);
        const uint32_t next = le32(work + offset % 512U) & 0x0FFFFFFFU;
        if (next >= 0x0FFFFFF0U || next < 2)
            return clear_error(out, out_length, CHAON_FAT_FILE_INVALID);
        cluster = next;
    }
    out[written] = 0;
    *out_length = written;
    return CHAON_FAT_FILE_OK;
}
