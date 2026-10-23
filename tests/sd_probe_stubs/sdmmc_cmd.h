/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/sdmmc_host.h"

typedef struct {
    struct {
        int64_t capacity;
        size_t sector_size;
    } csd;
} sdmmc_card_t;

esp_err_t sdmmc_read_sectors(sdmmc_card_t *card, void *destination,
    size_t start, size_t count);

static inline esp_err_t sdmmc_card_init(const sdmmc_host_t *host,
    sdmmc_card_t *card)
{
    (void)host;
    (void)card;
    return ESP_OK;
}
static inline esp_err_t sdmmc_get_status(sdmmc_card_t *card)
{
    (void)card;
    return ESP_OK;
}
