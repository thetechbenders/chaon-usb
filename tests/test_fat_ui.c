/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_screen.h"
#include "chaon_fat_ro.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t framebuffer[CHAON_SCREEN_BYTES + 2];
static uint8_t untouched[CHAON_SCREEN_BYTES];
static uint8_t previous_strip[CHAON_SCREEN_SD_HEIGHT * CHAON_SCREEN_WIDTH * 2];
int main(void)
{
    chaon_fat_catalog_t cat = {0};
    assert(chaon_screen_render(CHAON_STATUS_READY, 0, CHAON_SD_CATALOG_OK,
                               framebuffer, CHAON_SCREEN_BYTES));
    memcpy(untouched, framebuffer, CHAON_SCREEN_BYTES);
    framebuffer[CHAON_SCREEN_BYTES] = 0xA5;
    framebuffer[CHAON_SCREEN_BYTES + 1] = 0x5A;
    assert(chaon_screen_render_catalog(&cat, 0, framebuffer,
                                       CHAON_SCREEN_BYTES));
    assert(!memcmp(untouched, framebuffer, CHAON_SCREEN_SD_Y * 160U * 2U));
    assert(framebuffer[CHAON_SCREEN_BYTES] == 0xA5);
    assert(framebuffer[CHAON_SCREEN_BYTES + 1] == 0x5A);
    memcpy(previous_strip, framebuffer + CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
           sizeof previous_strip);
    cat.more = true; /* cycle/sector cap: unknown contents, not empty */
    assert(chaon_screen_render_catalog(&cat, 0, framebuffer,
                                       CHAON_SCREEN_BYTES));
    assert(memcmp(previous_strip,
                  framebuffer + CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
                  sizeof previous_strip) != 0);
    cat.more = false;
    cat.count = 1;
    strcpy(cat.entries[0].name, "HELLO.TXT");
    assert(chaon_screen_render_catalog(&cat, 0, framebuffer,
                                       CHAON_SCREEN_BYTES));
    memcpy(previous_strip, framebuffer + CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
           sizeof previous_strip);
    cat.more = true; /* visibly mark truncated listings */
    assert(chaon_screen_render_catalog(&cat, 0, framebuffer,
                                       CHAON_SCREEN_BYTES));
    assert(memcmp(previous_strip,
                  framebuffer + CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
                  sizeof previous_strip) != 0);
    assert(!chaon_screen_render_catalog(&cat, 1, framebuffer,
                                        CHAON_SCREEN_BYTES));
    assert(!chaon_screen_render_catalog(&cat, 0, framebuffer, 12));
    /* Common FAT short aliases must render the tilde, not a blank. */
    cat.more = false;
    strcpy(cat.entries[0].name, "FILE~1.TXT");
    assert(chaon_screen_render_catalog(&cat, 0, framebuffer,
                                       CHAON_SCREEN_BYTES));
    memcpy(previous_strip,
           framebuffer + CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
           sizeof previous_strip);
    strcpy(cat.entries[0].name, "FILE 1.TXT");
    assert(chaon_screen_render_catalog(&cat, 0, framebuffer,
                                       CHAON_SCREEN_BYTES));
    assert(memcmp(previous_strip,
                  framebuffer + CHAON_SCREEN_SD_Y * CHAON_SCREEN_WIDTH * 2,
                  sizeof previous_strip) != 0);
    assert(!chaon_screen_render_catalog(NULL, 0, framebuffer,
                                        CHAON_SCREEN_BYTES));
    assert(!memcmp(untouched, framebuffer, CHAON_SCREEN_SD_Y * 160U * 2U));
    assert(framebuffer[CHAON_SCREEN_BYTES] == 0xA5);
    assert(framebuffer[CHAON_SCREEN_BYTES+1] == 0x5A);
    puts("ChaoN root catalogue display bounds tests: PASS");
    return 0;
}
