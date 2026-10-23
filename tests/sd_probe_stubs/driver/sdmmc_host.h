/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef struct {
    int slot;
    int max_freq_khz;
} sdmmc_host_t;

typedef struct {
    int width;
    int clk;
    int cmd;
    int d0;
    unsigned flags;
} sdmmc_slot_config_t;

#define SDMMC_HOST_DEFAULT() ((sdmmc_host_t){0})
#define SDMMC_SLOT_CONFIG_DEFAULT() ((sdmmc_slot_config_t){0})
#define SDMMC_SLOT_FLAG_INTERNAL_PULLUP 1u

static inline esp_err_t sdmmc_host_init(void) { return ESP_OK; }
static inline esp_err_t sdmmc_host_init_slot(int slot,
    const sdmmc_slot_config_t *config)
{
    (void)slot;
    (void)config;
    return ESP_OK;
}
static inline esp_err_t sdmmc_host_deinit(void) { return ESP_OK; }
