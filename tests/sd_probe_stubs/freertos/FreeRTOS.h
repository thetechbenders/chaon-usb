/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once

#include <assert.h>
#include <stdint.h>

typedef struct {
    unsigned depth;
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED {0}
#define portENTER_CRITICAL(mux) do { \
    assert((mux)->depth == 0); \
    ++(mux)->depth; \
    ++test_lock_entries; \
} while (0)
#define portEXIT_CRITICAL(mux) do { \
    assert((mux)->depth == 1); \
    --(mux)->depth; \
} while (0)
#define pdMS_TO_TICKS(ms) (ms)
#define pdPASS 1
#define tskIDLE_PRIORITY 0

extern unsigned test_lock_entries;
