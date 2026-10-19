#include "chaon_control.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

/* Emulate raw GPIO polling every 20 ms, including both debounce edges. */
static chaon_control_event_t hold(chaon_control_t *c, bool usb, bool pressed,
                                   int64_t *clock_ms)
{
    chaon_control_event_t event = CHAON_CONTROL_NONE;
    for (int sample = 0; sample < 4; ++sample) {
        const chaon_control_event_t step =
            chaon_control_step(c, usb, pressed, *clock_ms);
        if (step != CHAON_CONTROL_NONE) event = step;
        *clock_ms += CHAON_CONTROL_POLL_MS;
    }
    return event;
}

static chaon_control_event_t tap(chaon_control_t *c, int64_t *time_ms)
{
    assert(hold(c, true, true, time_ms) == CHAON_CONTROL_NONE);
    return hold(c, true, false, time_ms);
}

static void test_inert_and_expiry(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    assert(c.state == CHAON_STATUS_WAIT_USB);
    assert(hold(&c, false, false, &t) == CHAON_CONTROL_NONE);
    assert(hold(&c, true, false, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
    assert(c.state == CHAON_STATUS_ARMED);
    assert(chaon_control_step(&c, true, false,
                              c.armed_at_ms + CHAON_CONTROL_ARM_TTL_MS - 1)
           == CHAON_CONTROL_NONE);
    assert(chaon_control_step(&c, true, false,
                              c.armed_at_ms + CHAON_CONTROL_ARM_TTL_MS)
           == CHAON_CONTROL_EXPIRE);
    assert(c.state == CHAON_STATUS_READY);
    /* First post-expiry gesture only arms, never executes. */
    t = c.armed_at_ms + CHAON_CONTROL_ARM_TTL_MS + 100;
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
    assert(c.state == CHAON_STATUS_ARMED);
}

static void test_execute_and_cancel_release(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    hold(&c, true, false, &t);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
    assert(tap(&c, &t) == CHAON_CONTROL_EXECUTE);
    assert(c.state == CHAON_STATUS_RUNNING);
    chaon_control_complete(&c, CHAON_RUN_CANCELLED, true, true, t);
    assert(c.state == CHAON_STATUS_CANCELLED);
    const int64_t red_until = c.cancelled_until_ms;
    assert(chaon_control_step(&c, true, true, red_until - 1)
           == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_CANCELLED);
    assert(chaon_control_step(&c, true, true, red_until)
           == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY);
    t = red_until + CHAON_CONTROL_POLL_MS;
    /* Release of the held cancel button is consumed, not another ARM. */
    assert(hold(&c, true, false, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
}

static void test_held_connect_and_disconnect(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    hold(&c, false, true, &t);
    assert(hold(&c, true, true, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY);
    assert(hold(&c, true, false, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
    assert(hold(&c, false, false, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_WAIT_USB);
    assert(hold(&c, true, false, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
}

static void test_fault_blocks_arm_and_reset(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    hold(&c, true, false, &t);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
    assert(tap(&c, &t) == CHAON_CONTROL_EXECUTE);
    /* A failed final all-keys release must yield a non-arming FAULT. */
    chaon_control_complete(&c, CHAON_RUN_IO_ERROR, true, false, t);
    assert(c.state == CHAON_STATUS_FAULT);
    assert(tap(&c, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_FAULT);
    hold(&c, false, false, &t);
    assert(c.state == CHAON_STATUS_WAIT_USB);
    hold(&c, true, false, &t);
    assert(c.state == CHAON_STATUS_READY);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
}

static void test_success_returns_ready(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    hold(&c, true, false, &t);
    assert(tap(&c, &t) == CHAON_CONTROL_ARM);
    assert(tap(&c, &t) == CHAON_CONTROL_EXECUTE);
    chaon_control_complete(&c, CHAON_RUN_OK, true, false, t);
    assert(c.state == CHAON_STATUS_READY);
    assert(hold(&c, false, false, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_WAIT_USB);
}

int main(void)
{
    test_inert_and_expiry();
    test_execute_and_cancel_release();
    test_held_connect_and_disconnect();
    test_fault_blocks_arm_and_reset();
    test_success_returns_ready();
    puts("ChaoN BOOT state machine tests: PASS");
    return 0;
}
