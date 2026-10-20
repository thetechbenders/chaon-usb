/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SD_DIAG_H
#define CHAON_SD_DIAG_H
/* Advisory observations, never HID instructions. CARD_ERROR includes absence:
 * there is no verified card-detect signal. Codes identify failing stages. */
typedef enum {
    CHAON_SD_INIT,
    CHAON_SD_READ_OK,
    CHAON_SD_HOST_ERROR,
    CHAON_SD_SLOT_ERROR,
    CHAON_SD_CARD_ERROR,
    CHAON_SD_SIZE_ERROR,
    CHAON_SD_READ_ERROR,
    CHAON_SD_LOST_IO,
    CHAON_SD_CATALOG_OK,
    CHAON_SD_FS_UNSUPPORTED,
    CHAON_SD_FS_ERROR,
    CHAON_SD_CLEANUP_ERROR,
    CHAON_SD_TASK_ERROR
} chaon_sd_diag_t;
const char *chaon_sd_diag_label(chaon_sd_diag_t diagnostic);
#endif
