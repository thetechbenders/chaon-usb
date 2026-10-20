/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_sd_diag.h"
#include "chaon_screen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const chaon_sd_diag_t diagnostics[] = {
    CHAON_SD_INIT, CHAON_SD_READ_OK, CHAON_SD_HOST_ERROR,
    CHAON_SD_SLOT_ERROR, CHAON_SD_CARD_ERROR, CHAON_SD_SIZE_ERROR,
    CHAON_SD_READ_ERROR, CHAON_SD_LOST_IO, CHAON_SD_CATALOG_OK,
    CHAON_SD_FS_UNSUPPORTED, CHAON_SD_FS_ERROR, CHAON_SD_CLEANUP_ERROR,
    CHAON_SD_TASK_ERROR, (chaon_sd_diag_t)999
};
static const char *const labels[] = {
    "SD INIT", "SD OK", "SD ERR H", "SD ERR B", "SD ERR C",
    "SD ERR Z", "SD ERR R", "SD IO S", "SD DIR", "SD FS N",
    "SD FS E", "SD ERR D", "SD ERR T", "SD ERR U"
};
enum { COUNT = sizeof diagnostics / sizeof diagnostics[0],
       STRIP_OFFSET = CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
       STRIP_BYTES = CHAON_SCREEN_SD_HEIGHT * CHAON_SCREEN_WIDTH * 2 };
static uint8_t guarded[CHAON_SCREEN_BYTES + 2];
static uint8_t expected[CHAON_SCREEN_BYTES];
static uint8_t original[CHAON_SCREEN_BYTES];
static uint8_t strips[COUNT][STRIP_BYTES];

static void mapping_and_rendering(void)
{
    uint8_t *frame = guarded + 1;
    memset(guarded, 0xA5, sizeof guarded);
    assert(!chaon_screen_render_sd(CHAON_SD_READ_OK, NULL, CHAON_SCREEN_BYTES));
    assert(!chaon_screen_render_sd(CHAON_SD_READ_OK, frame,
                                   CHAON_SCREEN_BYTES - 1));
    for (size_t i = 0; i < sizeof guarded; ++i) assert(guarded[i] == 0xA5);

    const chaon_status_t modes[] = {
        CHAON_STATUS_WAIT_USB, CHAON_STATUS_READY, CHAON_STATUS_ARMED,
        CHAON_STATUS_RUNNING, CHAON_STATUS_CANCELLED, CHAON_STATUS_FAULT
    };
    for (unsigned mode = 0; mode < sizeof modes / sizeof modes[0]; ++mode) {
        assert(chaon_screen_render(modes[mode], 30, CHAON_SD_INIT,
                                   original, sizeof original));
        memcpy(frame, original, sizeof original);
        for (unsigned i = 0; i < COUNT; ++i) {
            assert(strcmp(chaon_sd_diag_label(diagnostics[i]), labels[i]) == 0);
            assert(strlen(labels[i]) * 6U + 10U <= CHAON_SCREEN_WIDTH);
            assert(chaon_screen_render_sd(diagnostics[i], frame,
                                          CHAON_SCREEN_BYTES));
            /* Neither primary text nor countdown/BOOT hint may change. */
            assert(memcmp(frame, original, STRIP_OFFSET) == 0);
            assert(guarded[0] == 0xA5);
            assert(guarded[CHAON_SCREEN_BYTES + 1] == 0xA5);
            assert(chaon_screen_render(modes[mode], 30, diagnostics[i],
                                       expected, sizeof expected));
            /* A partial transition must erase stale longer error labels. */
            assert(memcmp(frame, expected, CHAON_SCREEN_BYTES) == 0);
            if (mode == 0) memcpy(strips[i], frame + STRIP_OFFSET, STRIP_BYTES);
        }
    }
    /* All documented codes produce visibly distinct text, not just colors. */
    for (unsigned i = 0; i < COUNT; ++i)
        for (unsigned j = i + 1; j < COUNT; ++j)
            assert(memcmp(strips[i], strips[j], STRIP_BYTES) != 0);
}

static void redraw_transitions(void)
{
    chaon_screen_key_t previous = {
        CHAON_STATUS_READY, 0, CHAON_SD_INIT
    };
    assert(chaon_screen_update(NULL, &previous) == CHAON_SCREEN_FULL_UPDATE);
    assert(chaon_screen_update(&previous, NULL) == CHAON_SCREEN_NO_UPDATE);
    const chaon_sd_diag_t sequence[] = {
        CHAON_SD_READ_OK, CHAON_SD_LOST_IO, CHAON_SD_INIT,
        CHAON_SD_CARD_ERROR, CHAON_SD_INIT, CHAON_SD_READ_OK,
        CHAON_SD_READ_ERROR, CHAON_SD_CLEANUP_ERROR, CHAON_SD_TASK_ERROR
    };
    for (unsigned i = 0; i < sizeof sequence / sizeof sequence[0]; ++i) {
        chaon_screen_key_t current = previous;
        current.sd = sequence[i];
        assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_SD_UPDATE);
        previous = current;
        assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_NO_UPDATE);
    }
    chaon_screen_key_t current = previous;
    current.status = CHAON_STATUS_ARMED;
    current.seconds = 30;
    assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_FULL_UPDATE);
    previous = current;
    current.seconds = 29;
    assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_FULL_UPDATE);
    previous = current;
    current.sd = CHAON_SD_READ_OK;
    assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_SD_UPDATE);
    current.status = CHAON_STATUS_READY;
    assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_FULL_UPDATE);
    previous = current;
    current.seconds = 0; /* irrelevant outside ARMED */
    assert(chaon_screen_update(&previous, &current) == CHAON_SCREEN_NO_UPDATE);
}

int main(void)
{
    mapping_and_rendering();
    redraw_transitions();
    puts("ChaoN SD diagnostic mapping/render/transition tests: PASS");
    return 0;
}
