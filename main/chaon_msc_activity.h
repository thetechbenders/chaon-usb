/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once
#include <stdbool.h>
#include <stdint.h>

enum { CHAON_MSC_READ = 1u, CHAON_MSC_WRITE = 2u };
/* Single atomic OR in USB task context. Reports attempts, including failures. */
void chaon_msc_activity_event(unsigned int event);
/* Sticky until power cycle. A deferred write failed after host acknowledgement. */
void chaon_msc_write_failure(void);
bool chaon_msc_write_failed(void);
/* Atomic drain; bursts coalesce, concurrent later events survive next drain. */
unsigned int chaon_msc_activity_take(void);

typedef struct {
    uint32_t last_event_ms, pulse_start_ms;
    unsigned int events;
    bool cycling;
} chaon_msc_activity_t;
/* App-only state: 75ms pulse, at least 125ms gap, no queued burst replay. */
unsigned int chaon_msc_activity_step(chaon_msc_activity_t *state,
    uint32_t now_ms, bool mounted, unsigned int events);
