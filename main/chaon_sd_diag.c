/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_sd_diag.h"
const char *chaon_sd_diag_label(chaon_sd_diag_t diagnostic)
{
    switch (diagnostic) {
        case CHAON_SD_INIT: return "SD INIT";
        case CHAON_SD_READ_OK: return "SD OK";
        case CHAON_SD_CATALOG_OK: return "SD DIR";
        case CHAON_SD_FS_UNSUPPORTED: return "SD FS N";
        case CHAON_SD_FS_ERROR: return "SD FS E";
        case CHAON_SD_HOST_ERROR: return "SD ERR H";
        case CHAON_SD_SLOT_ERROR: return "SD ERR B";
        case CHAON_SD_CARD_ERROR: return "SD ERR C";
        case CHAON_SD_SIZE_ERROR: return "SD ERR Z";
        case CHAON_SD_READ_ERROR: return "SD ERR R";
        case CHAON_SD_LOST_IO: return "SD IO S";
        case CHAON_SD_CLEANUP_ERROR: return "SD ERR D";
        case CHAON_SD_TASK_ERROR: return "SD ERR T";
        default: return "SD ERR U";
    }
}
