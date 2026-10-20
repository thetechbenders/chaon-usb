/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_fat_ro.h"
#include <string.h>

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static bool in_range(uint64_t start, uint64_t length, uint32_t capacity)
{
    return length > 0 && start < capacity && length <= (uint64_t)capacity - start;
}
static bool signature(const uint8_t *s)
{
    return s[510] == 0x55 && s[511] == 0xAA;
}
static bool power_two(uint32_t v)
{
    return v && !(v & (v - 1U));
}
static bool sector_read(chaon_fat_read_t io, void *ctx, uint32_t lba,
                        uint32_t capacity, uint8_t *work)
{
    return lba < capacity && io(ctx, lba, work);
}
static char safe_char(uint8_t c)
{
    if (c >= 'a' && c <= 'z') c = (uint8_t)(c - 'a' + 'A');
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '~') return (char)c;
    return '-'; /* don't leak control characters into display */
}
static void short_name(const uint8_t *raw, char output[13])
{
    size_t n = 0;
    for (size_t i = 0; i < 8 && raw[i] != ' '; ++i)
        output[n++] = safe_char(raw[i]);
    bool ext = false;
    for (size_t i = 8; i < 11; ++i)
        if (raw[i] != ' ') ext = true;
    if (ext) {
        output[n++] = '.';
        for (size_t i = 8; i < 11 && raw[i] != ' '; ++i)
            output[n++] = safe_char(raw[i]);
    }
    output[n] = '\0';
}
/* Returns true if directory enumerator should stop. */
static bool add_sector(const uint8_t *sector, chaon_fat_catalog_t *catalog,
                       bool *complete)
{
    for (size_t off = 0; off < 512; off += 32) {
        const uint8_t *entry = sector + off;
        if (entry[0] == 0x00) { *complete = true; return true; }
        if (entry[0] == 0xE5 || entry[0] == 0x20 ||
            entry[0] == 0x2E || entry[11] == 0x0F ||
            (entry[11] & 0x08) || (entry[11] & 0xC0))
            continue; /* deleted, invalid, dot, LFN, label, reserved */
        if (catalog->count == CHAON_FAT_MAX_FILES) {
            catalog->more = true;
            return true;
        }
        chaon_fat_entry_t *dest = &catalog->entries[catalog->count];
        short_name(entry, dest->name);
        if (!dest->name[0]) continue;
        dest->is_directory = (entry[11] & 0x10) != 0;
        ++catalog->count;
    }
    return false;
}

chaon_fat_result_t chaon_fat_list_root(chaon_fat_read_t io, void *ctx,
    uint32_t capacity, uint8_t work[CHAON_FAT_SECTOR_BYTES],
    chaon_fat_catalog_t *out)
{
    if (!out) return CHAON_FAT_INVALID;
    memset(out, 0, sizeof(*out));
    if (!io || !work || !capacity) return CHAON_FAT_INVALID;
    if (!sector_read(io, ctx, 0, capacity, work))
        return CHAON_FAT_IO_ERROR;
    if (!signature(work)) return CHAON_FAT_UNSUPPORTED;

    uint32_t partition = 0, partition_size = capacity;
    if (le16(work + 11) != 512) {
        /* Not a FAT superfloppy BPB: inspect primary MBR, never extended
         * partition tables or GPT. Require a known FAT partition type. */
        bool found = false;
        for (size_t i = 0; i < 4; ++i) {
            const uint8_t *e = work + 446 + 16 * i;
            if (e[0] != 0 && e[0] != 0x80) continue;
            const uint8_t type = e[4];
            if (type != 0x01 && type != 0x04 && type != 0x06 &&
                type != 0x0B && type != 0x0C && type != 0x0E) continue;
            uint32_t start = le32(e + 8), size = le32(e + 12);
            if (!in_range(start, size, capacity))
                return CHAON_FAT_INVALID;
            partition = start; partition_size = size;
            found = true;
            break;
        }
        if (!found) return CHAON_FAT_UNSUPPORTED;
        if (!sector_read(io, ctx, partition, capacity, work))
            return CHAON_FAT_IO_ERROR;
    }
    if (!signature(work) || le16(work + 11) != 512)
        return CHAON_FAT_INVALID;
    if (work[0] != 0xEB && work[0] != 0xE9)
        return CHAON_FAT_INVALID;
    const uint32_t cluster_sectors = work[13];
    const uint32_t reserved = le16(work + 14);
    const uint32_t nfat = work[16];
    const uint32_t root_entries = le16(work + 17);
    const uint32_t total = le16(work + 19) ? le16(work + 19) :
                           le32(work + 32);
    const uint32_t fat16 = le16(work + 22);
    const uint32_t fat_sectors = fat16 ? fat16 : le32(work + 36);
    if (!power_two(cluster_sectors) || cluster_sectors > 128 ||
        !reserved || !nfat || nfat > 2 || !fat_sectors ||
        total > partition_size || !total)
        return CHAON_FAT_INVALID;
    const uint64_t root_sectors = ((uint64_t)root_entries * 32U + 511U)/512U;
    const uint64_t fat_start = (uint64_t)partition + reserved;
    const uint64_t root_relative = (uint64_t)reserved +
                                  (uint64_t)nfat * fat_sectors;
    const uint64_t data_relative = root_relative + root_sectors;
    if (data_relative >= total || !in_range(partition, total, capacity))
        return CHAON_FAT_INVALID;
    const uint64_t clusters = ((uint64_t)total - data_relative) /
                              cluster_sectors;
    if (!clusters || clusters > UINT32_MAX - 1U)
        return CHAON_FAT_INVALID;
    const chaon_fat_type_t type = clusters < 4085 ? CHAON_FAT12 :
                                  clusters < 65525 ? CHAON_FAT16 :
                                  CHAON_FAT32;
    if (type == CHAON_FAT32) {
        if (root_entries || fat16) return CHAON_FAT_INVALID;
    } else if (!root_entries || !fat16) {
        return CHAON_FAT_INVALID;
    }
    out->type = type;
    uint64_t sectors_left = CHAON_FAT_MAX_ROOT_SECTORS;
    bool complete = false;
    if (type != CHAON_FAT32) {
        if (!root_sectors) return CHAON_FAT_INVALID;
        for (uint64_t n = 0; n < root_sectors && sectors_left; ++n) {
            const uint64_t lba = (uint64_t)partition + root_relative + n;
            if (lba > UINT32_MAX ||
                !sector_read(io, ctx, (uint32_t)lba, capacity, work))
                goto io_error;
            --sectors_left;
            if (add_sector(work, out, &complete)) return CHAON_FAT_OK;
        }
        if (root_sectors > CHAON_FAT_MAX_ROOT_SECTORS) out->more = true;
        return CHAON_FAT_OK;
    }
    const uint32_t root_cluster = le32(work + 44) & 0x0FFFFFFFU;
    if (root_cluster < 2 || root_cluster >= clusters + 2)
        return CHAON_FAT_INVALID;
    uint32_t cluster = root_cluster;
    while (sectors_left) {
        const uint64_t first = (uint64_t)partition + data_relative +
                               (uint64_t)(cluster - 2) * cluster_sectors;
        if (!in_range(first, cluster_sectors, capacity) ||
            first + cluster_sectors > (uint64_t)partition + total)
            return CHAON_FAT_INVALID;
        for (uint32_t n = 0; n < cluster_sectors && sectors_left; ++n) {
            if (!sector_read(io, ctx, (uint32_t)(first + n), capacity, work))
                goto io_error;
            --sectors_left;
            if (add_sector(work, out, &complete)) return CHAON_FAT_OK;
        }
        if (!sectors_left) { out->more = true; return CHAON_FAT_LIMIT; }
        const uint64_t fat_offset = (uint64_t)cluster * 4U;
        const uint64_t fat_lba = fat_start + fat_offset / 512U;
        if (fat_offset / 512U >= fat_sectors || fat_lba > UINT32_MAX)
            return CHAON_FAT_INVALID;
        if (!sector_read(io, ctx, (uint32_t)fat_lba, capacity, work))
            goto io_error;
        const uint32_t next = le32(work + fat_offset % 512U) & 0x0FFFFFFFU;
        if (next >= 0x0FFFFFF8U) return CHAON_FAT_OK;
        if (next < 2 || next >= clusters + 2 || next == 0x0FFFFFF7U)
            return CHAON_FAT_INVALID;
        cluster = next;
    }
    out->more = true;
    return CHAON_FAT_LIMIT;
io_error:
    memset(out, 0, sizeof(*out));
    return CHAON_FAT_IO_ERROR;
}
