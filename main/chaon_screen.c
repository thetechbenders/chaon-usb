#include "chaon_screen.h"
#include <string.h>

/* Small original 5x7 upper-case display font, one byte per glyph column.
 * Restrict UI labels to supported ASCII: A-Z, 0-9, colon, hyphen and space.
 */
static const uint8_t letters[26][5] = {
    {0x7e,0x09,0x09,0x09,0x7e}, /* A */
    {0x7f,0x49,0x49,0x49,0x36}, /* B */
    {0x3e,0x41,0x41,0x41,0x22}, /* C */
    {0x7f,0x41,0x41,0x22,0x1c}, /* D */
    {0x7f,0x49,0x49,0x49,0x41}, /* E */
    {0x7f,0x09,0x09,0x09,0x01}, /* F */
    {0x3e,0x41,0x49,0x49,0x7a}, /* G */
    {0x7f,0x08,0x08,0x08,0x7f}, /* H */
    {0x00,0x41,0x7f,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3f,0x01}, /* J */
    {0x7f,0x08,0x14,0x22,0x41}, /* K */
    {0x7f,0x40,0x40,0x40,0x40}, /* L */
    {0x7f,0x02,0x0c,0x02,0x7f}, /* M */
    {0x7f,0x04,0x08,0x10,0x7f}, /* N */
    {0x3e,0x41,0x41,0x41,0x3e}, /* O */
    {0x7f,0x09,0x09,0x09,0x06}, /* P */
    {0x3e,0x41,0x51,0x21,0x5e}, /* Q */
    {0x7f,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7f,0x01,0x01}, /* T */
    {0x3f,0x40,0x40,0x40,0x3f}, /* U */
    {0x1f,0x20,0x40,0x20,0x1f}, /* V */
    {0x7f,0x20,0x18,0x20,0x7f}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
};
static const uint8_t digits[10][5] = {
    {0x3e,0x51,0x49,0x45,0x3e}, {0,0x42,0x7f,0x40,0},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31},
    {0x18,0x14,0x12,0x7f,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e},
};
static const uint8_t colon[5] = {0,0x36,0x36,0,0};
static const uint8_t dash[5] = {0x08,0x08,0x08,0x08,0x08};
static const uint8_t dot[5] = {0,0x60,0x60,0,0};
static const uint8_t underscore[5] = {0x40,0x40,0x40,0x40,0x40};
static const uint8_t blank[5] = {0,0,0,0,0};

static const uint8_t *glyph(char ch)
{
    if (ch >= 'A' && ch <= 'Z') return letters[ch - 'A'];
    if (ch >= '0' && ch <= '9') return digits[ch - '0'];
    if (ch == ':') return colon;
    if (ch == '-') return dash;
    if (ch == '.') return dot;
    if (ch == '_') return underscore;
    return blank;
}

static void pixel(uint8_t *frame, int x, int y, uint16_t color)
{
    if (x < 0 || x >= CHAON_SCREEN_WIDTH || y < 0 || y >= CHAON_SCREEN_HEIGHT)
        return;
    const size_t offset = (size_t)(y * CHAON_SCREEN_WIDTH + x) * 2U;
    frame[offset] = (uint8_t)(color >> 8);
    frame[offset + 1] = (uint8_t)color;
}

static void rect(uint8_t *frame, int x, int y, int w, int h, uint16_t color)
{
    for (int dy = 0; dy < h; ++dy)
        for (int dx = 0; dx < w; ++dx)
            pixel(frame, x + dx, y + dy, color);
}

static void print_text(uint8_t *frame, int x, int y, const char *text,
                       int scale, uint16_t color)
{
    while (*text && x < CHAON_SCREEN_WIDTH) {
        const uint8_t *columns = glyph(*text++);
        for (int col = 0; col < 5; ++col) {
            for (int row = 0; row < 7; ++row) {
                if (columns[col] & (1U << row))
                    rect(frame, x + col * scale, y + row * scale,
                         scale, scale, color);
            }
        }
        x += scale * 6;
    }
}

static uint16_t accent(chaon_status_t status)
{
    switch (status) {
        case CHAON_STATUS_READY: return 0xB7AD; /* #B4F66B RGB565 */
        case CHAON_STATUS_ARMED: return 0xFE20; /* yellow */
        case CHAON_STATUS_RUNNING: return 0x069B; /* cyan */
        case CHAON_STATUS_CANCELLED:
        case CHAON_STATUS_FAULT: return 0xF800; /* red */
        case CHAON_STATUS_WAIT_USB: return 0x738B; /* muted green-gray */
        default: return 0xF800;
    }
}

bool chaon_screen_render(chaon_status_t status, uint32_t seconds_remaining,
                         chaon_sd_diag_t sd, uint8_t *frame, size_t length)
{
    if (!frame || length < CHAON_SCREEN_BYTES) return false;
    /* Fill the entire frame, no stale text from previous state. */
    for (size_t i = 0; i < CHAON_SCREEN_BYTES; i += 2) {
        frame[i] = 0x10;
        frame[i + 1] = 0x82; /* charcoal */
    }
    const uint16_t green = 0xB7AD;
    rect(frame, 0, 0, 3, CHAON_SCREEN_HEIGHT, green);
    print_text(frame, 9, 5, "CHAON", 2, green);
    print_text(frame, 112, 8, "USB HID", 1, 0xBDF7);
    rect(frame, 9, 24, 143, 1, 0x3166);
    const char *name = chaon_status_name(status);
    print_text(frame, 9, 30, name, 2, accent(status));
    if (status == CHAON_STATUS_ARMED) {
        /* Fixed two-digit countdown, clamped to 99; UI only. */
        const uint32_t value = seconds_remaining > 99 ? 99 : seconds_remaining;
        char count[4] = {(char)('0' + value / 10U),
                         (char)('0' + value % 10U), 'S', '\0'};
        print_text(frame, 119, 34, count, 1, 0xFFE0);
    }
    rect(frame, 9, 52, 143, 1, 0x3166);
    print_text(frame, 10, 62, chaon_status_hint(status), 1, 0xBDF7);
    return chaon_screen_render_sd(sd, frame, length);
}

bool chaon_screen_render_sd(chaon_sd_diag_t sd, uint8_t *frame, size_t length)
{
    if (!frame || length < CHAON_SCREEN_BYTES) return false;
    rect(frame, 0, CHAON_SCREEN_SD_Y, CHAON_SCREEN_WIDTH,
         CHAON_SCREEN_SD_HEIGHT, 0x1082);
    rect(frame, 0, CHAON_SCREEN_SD_Y, 3, CHAON_SCREEN_SD_HEIGHT, 0xB7AD);
    const uint16_t color = (sd == CHAON_SD_READ_OK || sd == CHAON_SD_CATALOG_OK) ? 0xB7AD :
                           sd == CHAON_SD_INIT ? 0xFE20 : 0xF800;
    print_text(frame, 10, CHAON_SCREEN_SD_Y, chaon_sd_diag_label(sd), 1, color);
    return true;
}

bool chaon_screen_render_catalog(const chaon_fat_catalog_t *cat,
                                 uint8_t index, uint8_t *frame, size_t length)
{
    if (!cat || !frame || length < CHAON_SCREEN_BYTES ||
        cat->count > CHAON_FAT_MAX_FILES ||
        (cat->count && index >= cat->count))
        return false;
    rect(frame, 0, CHAON_SCREEN_SD_Y, CHAON_SCREEN_WIDTH,
         CHAON_SCREEN_SD_HEIGHT, 0x1082);
    rect(frame, 0, CHAON_SCREEN_SD_Y, 3, CHAON_SCREEN_SD_HEIGHT, 0xB7AD);
    if (cat->count == 0) {
        print_text(frame, 10, CHAON_SCREEN_SD_Y, "SD EMPTY", 1, 0xB7AD);
        return true;
    }
    /* Fixed maximum: "SD 8:8 FILENAME.EXT" (19 chars) fits 160 px. */
    char label[28] = "SD 0:0 ";
    label[3] = (char)('1' + index);
    label[5] = (char)('0' + cat->count); /* count 1..8 */
    size_t offset = 7;
    const char *name = cat->entries[index].name;
    while (*name && offset < sizeof label - 1) label[offset++] = *name++;
    label[offset] = '\0';
    print_text(frame, 10, CHAON_SCREEN_SD_Y, label, 1, 0xB7AD);
    return true;
}

chaon_screen_update_t chaon_screen_update(const chaon_screen_key_t *previous,
                                         const chaon_screen_key_t *current)
{
    if (!current) return CHAON_SCREEN_NO_UPDATE;
    if (!previous || previous->status != current->status ||
        (current->status == CHAON_STATUS_ARMED &&
         previous->seconds != current->seconds)) {
        return CHAON_SCREEN_FULL_UPDATE;
    }
    return previous->sd != current->sd ? CHAON_SCREEN_SD_UPDATE :
                                       CHAON_SCREEN_NO_UPDATE;
}
