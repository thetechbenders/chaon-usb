/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_msc_eject.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    chaon_msc_eject_t state = {0};
    assert(chaon_msc_eject_ready(&state, 0, true));
    assert(chaon_msc_eject_start_stop(&state, 0, true, true));
    assert(!chaon_msc_eject_ready(&state, 1, true));
    assert(!chaon_msc_eject_ready(&state, 0, false));
    assert(!chaon_msc_eject_ready(NULL, 0, true));
    assert(!chaon_msc_eject_start_stop(&state, 1, false, true));
    assert(chaon_msc_eject_start_stop(&state, 0, false, true));
    assert(!chaon_msc_eject_ready(&state, 0, true));
    assert(!chaon_msc_eject_start_stop(&state, 0, true, true));
    assert(chaon_msc_eject_start_stop(&state, 0, false, true));

    /* Full-sector, whole-volume and wraparound validation. */
    assert(chaon_msc_transfer_valid(0, 0, 512, 100, 512));
    assert(chaon_msc_transfer_valid(98, 0, 1024, 100, 512));
    assert(!chaon_msc_transfer_valid(99, 0, 1024, 100, 512));
    assert(!chaon_msc_transfer_valid(100, 0, 512, 100, 512));
    assert(!chaon_msc_transfer_valid(UINT32_MAX, 0, 512, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 0, 0, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 0, 1, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 0, 511, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 0, 513, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 1, 512, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 512, 512, 100, 512));
    assert(!chaon_msc_transfer_valid(0, 0, 512, 100, 0));
    assert(!chaon_msc_transfer_valid(0, 0, 512, 100, 256));
    assert(!chaon_msc_transfer_valid(0, 0, 512, 0, 512));
    assert(chaon_msc_transfer_valid(UINT32_MAX-1, 0, 1024, UINT32_MAX, 512) == false);
    assert(chaon_msc_transfer_valid(UINT32_MAX-1, 0, 512, UINT32_MAX, 512));

    puts("MSC eject and range guard tests passed");
    return 0;
}
