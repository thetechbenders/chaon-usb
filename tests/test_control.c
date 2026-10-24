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


/* Browser interaction uses the same debounced BOOT sampling as HID. */
static chaon_control_event_t gesture(chaon_control_t *c, bool available,
                                    uint8_t count, unsigned duration_ms,
                                    int64_t *clock_ms)
{
    chaon_control_event_t event = CHAON_CONTROL_NONE;
    unsigned samples = duration_ms / CHAON_CONTROL_POLL_MS;
    if (samples < 4) samples = 4;
    for (unsigned i = 0; i < samples; ++i) {
        const chaon_control_event_t e = chaon_control_step_browser(
            c, true, true, *clock_ms, available, count);
        if (e != CHAON_CONTROL_NONE) event = e;
        *clock_ms += CHAON_CONTROL_POLL_MS;
    }
    for (unsigned i = 0; i < 4; ++i) {
        const chaon_control_event_t e = chaon_control_step_browser(
            c, true, false, *clock_ms, available, count);
        if (e != CHAON_CONTROL_NONE) event = e;
        *clock_ms += CHAON_CONTROL_POLL_MS;
    }
    return event;
}
static void browser_ready(chaon_control_t *c, int64_t *t, uint8_t count)
{
    for (unsigned i=0;i<4;++i) {
        assert(chaon_control_step_browser(c, true, false, *t, true, count)
               == CHAON_CONTROL_NONE);
        *t += CHAON_CONTROL_POLL_MS;
    }
}
static void test_browse_selection_never_executes(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    browser_ready(&c, &t, 3);
    assert(gesture(&c, true, 3, 1020, &t) == CHAON_CONTROL_BROWSER_ENTER);
    assert(c.browser_active && c.state == CHAON_STATUS_READY);
    assert(c.browser_count == 3 && c.browser_index == 0 &&
           !c.browser_selected);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_BROWSER_NEXT);
    assert(c.browser_index == 1 && !c.browser_selected);
    assert(gesture(&c, true, 3, 1040, &t) == CHAON_CONTROL_BROWSER_SELECT);
    assert(c.browser_index == 1 && c.browser_selected);
    assert(gesture(&c, true, 3, 1060, &t) == CHAON_CONTROL_NONE);
    assert(c.browser_selected);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_BROWSER_SELECT);
    assert(!c.browser_selected);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_BROWSER_NEXT);
    assert(c.browser_index == 2);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_BROWSER_NEXT);
    assert(c.browser_index == 0); /* wrap; no extra filename read */
    assert(gesture(&c, true, 3, 3120, &t) == CHAON_CONTROL_BROWSER_CLOSE);
    assert(!c.browser_active && c.state == CHAON_STATUS_READY);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_ARM);
    assert(c.state == CHAON_STATUS_ARMED);
    /* Extended ARMED press is disarm, NOT execution. */
    assert(gesture(&c, true, 3, 1040, &t) == CHAON_CONTROL_DISARM);
    assert(c.state == CHAON_STATUS_READY);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_ARM);
    assert(gesture(&c, true, 3, 80, &t) == CHAON_CONTROL_EXECUTE);
}
static void test_browser_failure_closes_and_suppresses_release(void)
{
    chaon_control_t c = {0};
    int64_t t = 0;
    browser_ready(&c, &t, 8);
    assert(gesture(&c, true, 8, 1040, &t) == CHAON_CONTROL_BROWSER_ENTER);
    /* Press while browsing, then card disappears before release. */
    for (unsigned i=0;i<5;++i) {
        (void)chaon_control_step_browser(&c,true,true,t,true,8);
        t += CHAON_CONTROL_POLL_MS;
    }
    assert(chaon_control_step_browser(&c, true, true, t, false, 0)
           == CHAON_CONTROL_BROWSER_CLOSE);
    assert(!c.browser_active && c.state == CHAON_STATUS_READY);
    t += CHAON_CONTROL_POLL_MS;
    for (unsigned i=0;i<4;++i) {
        assert(chaon_control_step_browser(&c,true,false,t,false,0)
               == CHAON_CONTROL_NONE);
        t += CHAON_CONTROL_POLL_MS;
    }
    assert(c.state == CHAON_STATUS_READY); /* no phantom HID arm */
    assert(gesture(&c, false, 0, 1040, &t) == CHAON_CONTROL_NONE);
    assert(c.state == CHAON_STATUS_READY); /* no catalogue => no browser/arm */
    assert(gesture(&c, false, 0, 80, &t) == CHAON_CONTROL_ARM);
    assert(c.state == CHAON_STATUS_ARMED);
}
static void test_browser_count_and_idle(void)
{
    chaon_control_t c={0};int64_t t=0;
    browser_ready(&c,&t,2);
    assert(gesture(&c,true,2,1100,&t)==CHAON_CONTROL_BROWSER_ENTER);
    assert(chaon_control_step_browser(&c,true,false,t,true,3)
           == CHAON_CONTROL_BROWSER_CLOSE);
    assert(!c.browser_active && c.state==CHAON_STATUS_READY);
    t+=CHAON_CONTROL_POLL_MS;
    assert(gesture(&c,true,3,1030,&t)==CHAON_CONTROL_BROWSER_ENTER);
    t=c.browser_activity_ms+CHAON_CONTROL_BROWSER_IDLE_MS;
    assert(chaon_control_step_browser(&c,true,false,t,true,3)
           == CHAON_CONTROL_BROWSER_CLOSE);
    assert(!c.browser_active);
    t+=CHAON_CONTROL_POLL_MS;
    assert(gesture(&c,true,3,80,&t)==CHAON_CONTROL_ARM);
}
static void test_browser_usb_recovery_and_fault(void)
{
    chaon_control_t c={0};int64_t t=0;
    browser_ready(&c,&t,2);
    assert(gesture(&c,true,2,1050,&t)==CHAON_CONTROL_BROWSER_ENTER);
    assert(chaon_control_step_browser(&c,false,true,t,true,2)
           == CHAON_CONTROL_NONE);
    t+=CHAON_CONTROL_POLL_MS;
    assert(!c.browser_active && c.state==CHAON_STATUS_WAIT_USB);
    for(unsigned i=0;i<4;++i) {
        assert(chaon_control_step_browser(&c,true,true,t,true,2)
               == CHAON_CONTROL_NONE);
        t+=CHAON_CONTROL_POLL_MS;
    }
    assert(c.state==CHAON_STATUS_READY && !c.browser_active);
    for(unsigned i=0;i<4;++i) {
        assert(chaon_control_step_browser(&c,true,false,t,true,2)
               == CHAON_CONTROL_NONE);
        t+=CHAON_CONTROL_POLL_MS;
    }
    assert(c.state==CHAON_STATUS_READY);
    assert(gesture(&c,true,2,80,&t)==CHAON_CONTROL_ARM);
    assert(gesture(&c,true,2,80,&t)==CHAON_CONTROL_EXECUTE);
    chaon_control_complete(&c,CHAON_RUN_IO_ERROR,true,false,t);
    assert(c.state==CHAON_STATUS_FAULT);
    assert(gesture(&c,true,2,1050,&t)==CHAON_CONTROL_NONE);
    assert(!c.browser_active && c.state==CHAON_STATUS_FAULT);
}
static void test_browser_exact_hold_boundaries(void)
{
    chaon_control_t c={0};int64_t t=0;
    browser_ready(&c,&t,2);
    /* 880 ms remains a short press in READY, 900 ms never EXECUTES
     * when already ARMED. Times refer to debounced press/release. */
    assert(gesture(&c,true,2,880,&t)==CHAON_CONTROL_ARM);
    assert(c.state==CHAON_STATUS_ARMED);
    assert(gesture(&c,true,2,900,&t)==CHAON_CONTROL_DISARM);
    assert(c.state==CHAON_STATUS_READY);
    assert(gesture(&c,true,2,900,&t)==CHAON_CONTROL_BROWSER_ENTER);
    assert(c.browser_active);
    assert(gesture(&c,true,2,880,&t)==CHAON_CONTROL_BROWSER_NEXT);
    assert(c.browser_index==1 && !c.browser_selected);
    assert(gesture(&c,true,2,900,&t)==CHAON_CONTROL_BROWSER_SELECT);
    assert(c.browser_selected);
    assert(gesture(&c,true,2,2980,&t)==CHAON_CONTROL_NONE);
    assert(c.browser_selected && c.browser_active);
    assert(gesture(&c,true,2,80,&t)==CHAON_CONTROL_BROWSER_SELECT);
    assert(!c.browser_selected && c.browser_active);
    assert(gesture(&c,true,2,3000,&t)==CHAON_CONTROL_BROWSER_CLOSE);
    assert(!c.browser_active && c.state==CHAON_STATUS_READY);
    /* Broken / malicious oversized catalogue fails to open or ARM. */
    assert(gesture(&c,true,255,900,&t)==CHAON_CONTROL_NONE);
    assert(c.state==CHAON_STATUS_READY);
}

static void test_browser_timeout_while_pressed(void)
{
    chaon_control_t c={0};int64_t t=0;
    browser_ready(&c,&t,1);
    assert(gesture(&c,true,1,1030,&t)==CHAON_CONTROL_BROWSER_ENTER);
    /* Stable pressed as the 60 second inactivity deadline arrives. */
    t=c.browser_activity_ms+CHAON_CONTROL_BROWSER_IDLE_MS-160;
    for(unsigned i=0;i<5;++i) {
        (void)chaon_control_step_browser(&c,true,true,t,true,1);
        t+=CHAON_CONTROL_POLL_MS;
    }
    t=c.browser_activity_ms+CHAON_CONTROL_BROWSER_IDLE_MS;
    assert(chaon_control_step_browser(&c,true,true,t,true,1)
           == CHAON_CONTROL_BROWSER_CLOSE);
    assert(!c.browser_active);
    t+=CHAON_CONTROL_POLL_MS;
    for(unsigned i=0;i<4;++i) {
        assert(chaon_control_step_browser(&c,true,false,t,true,1)
               == CHAON_CONTROL_NONE);
        t+=CHAON_CONTROL_POLL_MS;
    }
    assert(c.state==CHAON_STATUS_READY);
    assert(gesture(&c,true,1,80,&t)==CHAON_CONTROL_ARM);
}

int main(void)
{
    test_inert_and_expiry();
    test_execute_and_cancel_release();
    test_held_connect_and_disconnect();
    test_fault_blocks_arm_and_reset();
    test_success_returns_ready();
    test_browse_selection_never_executes();
    test_browser_failure_closes_and_suppresses_release();
    test_browser_count_and_idle();
    test_browser_usb_recovery_and_fault();
    test_browser_timeout_while_pressed();
    test_browser_exact_hold_boundaries();
    puts("ChaoN BOOT/browser state machine tests: PASS");
    return 0;
}
