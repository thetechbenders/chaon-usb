/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_msc_activity.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(chaon_msc_activity_take() == 0);
    assert(!chaon_msc_write_failed());
    for (unsigned i = 0; i < 100000; ++i) chaon_msc_activity_event(1);
    chaon_msc_activity_event(2);
    assert(chaon_msc_activity_take() == 3); /* no saturating counter/rollover */
    assert(chaon_msc_activity_take() == 0);
    chaon_msc_activity_event(2); /* later event survives previous drain */
    assert(chaon_msc_activity_take() == 2);
    chaon_msc_activity_event(4);
    assert(chaon_msc_activity_take() == 0);

    chaon_msc_activity_t s = {0};
    assert(chaon_msc_activity_step(&s, 0, true, 0) == 0);
    assert(chaon_msc_activity_step(&s, 10, true, 1) == 1);
    assert(chaon_msc_activity_step(&s, 35, true, 2) == 3);
    assert(chaon_msc_activity_step(&s, 84, true, 0) == 3);
    assert(chaon_msc_activity_step(&s, 85, true, 0) == 0);
    assert(chaon_msc_activity_step(&s, 210, true, 0) == 0);
    assert(!s.cycling); /* old burst is never replayed */
    assert(chaon_msc_activity_step(&s, 211, true, 2) == 2);
    assert(chaon_msc_activity_step(&s, 212, false, 3) == 0);
    assert(chaon_msc_activity_step(&s, 213, true, 0) == 0);

    s = (chaon_msc_activity_t){0};
    for (uint32_t t = 0; t < 10000; t += 25) {
        unsigned out = chaon_msc_activity_step(&s, t, true, 1);
        assert(out == (t % 200 < 75 ? 1u : 0u));
    }
    s = (chaon_msc_activity_t){0};
    uint32_t start = UINT32_MAX - 30u;
    assert(chaon_msc_activity_step(&s, start, true, 2) == 2);
    assert(chaon_msc_activity_step(&s, start + 74u, true, 0) == 2);
    assert(chaon_msc_activity_step(&s, start + 75u, true, 0) == 0);
    assert(chaon_msc_activity_step(&s, start + 200u, true, 0) == 0);
    assert(chaon_msc_activity_step(&s, start + 225u, true, 1) == 1);
    /* Dropped/coalesced signals cannot leave a permanent activity light. */
    assert(chaon_msc_activity_step(&s, start + 1000u, true, 0) == 0);
    chaon_msc_write_failure();
    assert(chaon_msc_write_failed());
    assert(chaon_msc_activity_take() == 0);
    assert(chaon_msc_write_failed()); /* draining or unmounting must not clear fault */
    puts("MSC activity tests passed");
}
