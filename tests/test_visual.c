#include "chaon_status.h"
#include "chaon_screen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int equal(chaon_rgb_t a, chaon_rgb_t b)
{
    return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

int main(void)
{
    const chaon_rgb_t ready = chaon_status_led(CHAON_STATUS_READY, 0);
    assert(ready.red == 180 && ready.green == 246 && ready.blue == 107);
    assert(equal(ready, chaon_status_led(CHAON_STATUS_READY, 100000)));
    assert(equal(chaon_status_led(CHAON_STATUS_ARMED, 0),
                 (chaon_rgb_t){255,196,0}));
    assert(equal(chaon_status_led(CHAON_STATUS_ARMED, 250),
                 (chaon_rgb_t){0,0,0}));
    assert(equal(chaon_status_led(CHAON_STATUS_ARMED, 500),
                 (chaon_rgb_t){255,196,0}));
    assert(equal(chaon_status_led(CHAON_STATUS_CANCELLED, 50),
                 (chaon_rgb_t){255,0,0}));
    assert(equal(chaon_status_led(CHAON_STATUS_FAULT, 125),
                 (chaon_rgb_t){0,0,0}));
    assert(strcmp(chaon_status_name(CHAON_STATUS_READY), "READY") == 0);
    assert(strcmp(chaon_status_hint(CHAON_STATUS_CANCELLED), "KEYS RELEASED") == 0);

    static uint8_t frame[CHAON_SCREEN_BYTES + 2];
    frame[0] = 0xCC;
    frame[CHAON_SCREEN_BYTES] = 0xA5;
    frame[CHAON_SCREEN_BYTES + 1] = 0x5A;
    assert(!chaon_screen_render(CHAON_STATUS_READY, 0, CHAON_SD_INIT, NULL, sizeof frame));
    assert(!chaon_screen_render(CHAON_STATUS_READY, 0, CHAON_SD_INIT, frame,
                                CHAON_SCREEN_BYTES - 1));
    const chaon_status_t states[] = {
        CHAON_STATUS_WAIT_USB, CHAON_STATUS_READY, CHAON_STATUS_ARMED,
        CHAON_STATUS_RUNNING, CHAON_STATUS_CANCELLED, CHAON_STATUS_FAULT
    };
    for (unsigned i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        assert(chaon_screen_render(states[i], 30, CHAON_SD_INIT, frame, CHAON_SCREEN_BYTES));
        assert(frame[CHAON_SCREEN_BYTES] == 0xA5);
        assert(frame[CHAON_SCREEN_BYTES + 1] == 0x5A);
        assert(frame[0] != 0xCC); /* top left stripe */
    }
    assert(chaon_screen_render(CHAON_STATUS_READY, 0, CHAON_SD_INIT, frame, CHAON_SCREEN_BYTES));
    uint8_t ready_frame[CHAON_SCREEN_BYTES];
    memcpy(ready_frame, frame, sizeof ready_frame);
    assert(chaon_screen_render(CHAON_STATUS_ARMED, 12, CHAON_SD_INIT, frame, CHAON_SCREEN_BYTES));
    assert(memcmp(ready_frame, frame, CHAON_SCREEN_BYTES) != 0);
    puts("ChaoN status and screen tests: PASS");
    return 0;
}
