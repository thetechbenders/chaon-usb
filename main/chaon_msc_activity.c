/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_msc_activity.h"
#include <stdatomic.h>

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "MSC latch must always be lock-free");
static atomic_uint pending;
static atomic_bool write_failed;

void chaon_msc_write_failure(void)
{
    atomic_store_explicit(&write_failed, true, memory_order_relaxed);
}

bool chaon_msc_write_failed(void)
{
    return atomic_load_explicit(&write_failed, memory_order_relaxed);
}

void chaon_msc_activity_event(unsigned int event)
{
    atomic_fetch_or_explicit(&pending, event & 3u, memory_order_relaxed);
}

unsigned int chaon_msc_activity_take(void)
{
    return atomic_exchange_explicit(&pending, 0, memory_order_relaxed);
}

unsigned int chaon_msc_activity_step(chaon_msc_activity_t *s,
    uint32_t now, bool mounted, unsigned int events)
{
    if (!mounted) {
        *s = (chaon_msc_activity_t){0};
        return 0;
    }
    events &= 3u;
    if (events) {
        s->last_event_ms = now;
        s->events |= events;
        if (!s->cycling) {
            s->cycling = true;
            s->pulse_start_ms = now;
        }
    }
    if (!s->cycling) return 0;
    if ((uint32_t)(now - s->pulse_start_ms) >= 200u) {
        if ((uint32_t)(now - s->last_event_ms) >= 100u) {
            *s = (chaon_msc_activity_t){0};
            return 0;
        }
        s->pulse_start_ms = now;
        s->events = events; /* never replay old direction bits */
        if (!events) {
            s->cycling = false;
            return 0;
        }
    }
    return (uint32_t)(now - s->pulse_start_ms) < 75u ? s->events : 0;
}
