/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_FAT_RO_H
#define CHAON_FAT_RO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Root-directory metadata preview ONLY. Never opens/executess file contents.
 * Callback is read-only by construction, with no write API or VFS mount.
 * Fixed allocations and I/O ceilings suit the original no-PSRAM S3.
 */
enum { CHAON_FAT_SECTOR_BYTES = 512, CHAON_FAT_MAX_FILES = 8,
       CHAON_FAT_MAX_ROOT_SECTORS = 64 };
typedef bool (*chaon_fat_read_t)(void *ctx, uint32_t lba,
                                 uint8_t out[CHAON_FAT_SECTOR_BYTES]);
typedef enum {
    CHAON_FAT_OK, CHAON_FAT_UNSUPPORTED, CHAON_FAT_INVALID,
    CHAON_FAT_IO_ERROR, CHAON_FAT_LIMIT
} chaon_fat_result_t;
typedef enum {
    CHAON_FAT_NONE, CHAON_FAT12, CHAON_FAT16, CHAON_FAT32
} chaon_fat_type_t;
typedef struct {
    char name[13]; /* sanitized printable upper-case DOS 8.3 name */
    bool is_directory;
} chaon_fat_entry_t;
typedef struct {
    chaon_fat_type_t type;
    uint8_t count;
    bool more;  /* at least one more entry was seen, or scan hit I/O bound */
    chaon_fat_entry_t entries[CHAON_FAT_MAX_FILES];
} chaon_fat_catalog_t;

/* Uses caller-provided 512-byte workspace; output zeroed on errors.
 * card_sectors constrains every read, including malicious BPB/MBR LBAs.
 * Only FAT12/16/32 8.3 entries in the ROOT are previewed.
 * No subdirectory traversal, LFN reconstruction or file content reads.
 */
chaon_fat_result_t chaon_fat_list_root(chaon_fat_read_t read_sector,
    void *ctx, uint32_t card_sectors, uint8_t work[CHAON_FAT_SECTOR_BYTES],
    chaon_fat_catalog_t *out);
#endif
