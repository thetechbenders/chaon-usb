/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_msc_eject.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

int main(void)
{
    chaon_msc_eject_t s = {0};
    assert(chaon_msc_eject_ready(&s, 0, true));
    assert(!chaon_msc_eject_ready(&s, 1, true));
    assert(!chaon_msc_eject_ready(&s, 0, false));
    assert(!chaon_msc_eject_ready(NULL, 0, true));
    assert(!chaon_msc_eject_write_begin(&s, 1, true));
    assert(!chaon_msc_eject_write_begin(&s, 0, false));
    assert(!chaon_msc_eject_write_end(&s)); /* No underflow. */

    /* Eject never claims success with a deferred write outstanding. */
    assert(chaon_msc_eject_write_begin(&s, 0, true));
    assert(s.pending_writes == 1);
    assert(!chaon_msc_eject_start_stop(&s, 0, false, true));
    assert(chaon_msc_eject_ready(&s, 0, true));
    assert(chaon_msc_eject_write_end(&s));
    assert(s.pending_writes == 0);
    assert(chaon_msc_eject_start_stop(&s, 0, false, true));
    assert(s.ejected);
    assert(!chaon_msc_eject_ready(&s, 0, true));
    assert(!chaon_msc_eject_write_begin(&s, 0, true));
    assert(!chaon_msc_eject_start_stop(&s, 0, true, true));
    assert(!chaon_msc_eject_start_stop(&s, 0, true, false));
    assert(chaon_msc_eject_start_stop(&s, 0, false, true)); /* Idempotent. */

    s = (chaon_msc_eject_t){0};
    assert(!chaon_msc_eject_start_stop(&s, 1, false, true));
    assert(chaon_msc_eject_ready(&s, 0, true));
    assert(!chaon_msc_eject_start_stop(NULL, 0, false, true));
    assert(!chaon_msc_eject_write_end(NULL));
    assert(!chaon_msc_eject_write_begin(NULL, 0, true));

    /* Counter overflow is rejected rather than wrapping to zero. */
    s.pending_writes = UINT32_MAX;
    assert(!chaon_msc_eject_write_begin(&s, 0, true));
    assert(!chaon_msc_eject_start_stop(&s, 0, false, true));
    puts("MSC eject-state policy tests passed");
    return 0;
}
