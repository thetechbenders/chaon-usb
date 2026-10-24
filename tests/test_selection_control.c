/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_control.h"

#include <assert.h>
#include <stdio.h>

static chaon_control_event_t poll(chaon_control_t *control, int64_t *time_ms,
                                  bool pressed, bool pick_ready,
                                  bool connected, bool catalogue)
{
    const chaon_control_event_t event = chaon_control_step_selection(control,
        connected, pressed, *time_ms, catalogue, catalogue ? 1 : 0,
        pick_ready);
    *time_ms += CHAON_CONTROL_POLL_MS;
    return event;
}

static chaon_control_event_t gesture(chaon_control_t *control,
                                     int64_t *time_ms, unsigned duration_ms,
                                     bool pick_ready)
{
    chaon_control_event_t result = CHAON_CONTROL_NONE;
    for (unsigned elapsed = 0; elapsed < duration_ms;
         elapsed += CHAON_CONTROL_POLL_MS) {
        const chaon_control_event_t event = poll(control, time_ms, true,
            pick_ready, true, true);
        if (event != CHAON_CONTROL_NONE) result = event;
    }
    for (unsigned sample = 0; sample < 4; ++sample) {
        const chaon_control_event_t event = poll(control, time_ms, false,
            pick_ready, true, true);
        if (event != CHAON_CONTROL_NONE) result = event;
    }
    return result;
}

static void prepare_pick(chaon_control_t *control, int64_t *time_ms)
{
    for (int sample = 0; sample < 4; ++sample)
        assert(poll(control, time_ms, false, false, true, true) ==
               CHAON_CONTROL_NONE);
    assert(gesture(control, time_ms, 1020, false) ==
           CHAON_CONTROL_BROWSER_ENTER);
    assert(gesture(control, time_ms, 1020, false) ==
           CHAON_CONTROL_BROWSER_SELECT);
    assert(control->browser_selected);
}

int main(void)
{
    chaon_control_t control = {0};
    int64_t time_ms = 0;

    prepare_pick(&control, &time_ms);
    /* Inspection completion cannot reuse the PICK release as ARM. */
    assert(poll(&control, &time_ms, false, true, true, true) ==
           CHAON_CONTROL_PICK_READY);
    assert(!control.browser_active && !control.browser_selected &&
           control.state == CHAON_STATUS_READY);
    assert(poll(&control, &time_ms, false, true, true, true) ==
           CHAON_CONTROL_NONE);
    assert(gesture(&control, &time_ms, 80, false) == CHAON_CONTROL_ARM);
    assert(gesture(&control, &time_ms, 80, false) == CHAON_CONTROL_EXECUTE);

    control = (chaon_control_t){0};
    time_ms = 0;
    prepare_pick(&control, &time_ms);
    assert(poll(&control, &time_ms, false, false, true, true) ==
           CHAON_CONTROL_NONE);
    assert(control.browser_selected && control.browser_active);
    assert(gesture(&control, &time_ms, 80, false) ==
           CHAON_CONTROL_BROWSER_SELECT);
    assert(!control.browser_selected);
    assert(gesture(&control, &time_ms, 3100, false) ==
           CHAON_CONTROL_BROWSER_CLOSE);

    control = (chaon_control_t){0};
    time_ms = 0;
    prepare_pick(&control, &time_ms);
    assert(poll(&control, &time_ms, true, true, true, true) ==
           CHAON_CONTROL_NONE);
    assert(control.browser_active && control.browser_selected);
    /* USB loss must never complete PICK or reinterpret release as ARM. */
    assert(poll(&control, &time_ms, false, true, false, true) !=
           CHAON_CONTROL_PICK_READY);
    assert(!control.browser_active);

    puts("ChaoN PICK-to-ARM/RUN separation tests: PASS");
    return 0;
}
