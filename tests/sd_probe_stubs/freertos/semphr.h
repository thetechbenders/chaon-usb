/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once
#include <assert.h>
#include <stdint.h>
typedef struct { unsigned depth; } StaticSemaphore_t;
typedef StaticSemaphore_t *SemaphoreHandle_t;
#define portMAX_DELAY UINT32_MAX
static inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *b)
{ assert(b); b->depth=0; return b; }
static inline int xSemaphoreTake(SemaphoreHandle_t b, unsigned ticks)
{ (void)ticks; assert(b && !b->depth); b->depth=1; ++test_lock_entries; return 1; }
static inline int xSemaphoreGive(SemaphoreHandle_t b)
{ assert(b && b->depth==1); b->depth=0; return 1; }
