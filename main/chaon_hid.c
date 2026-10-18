#include "chaon_hid.h"
bool chaon_hid_us_ascii(char ch, uint8_t *modifier, uint8_t *usage)
{
    if (!modifier || !usage) return false;
    *modifier = 0; *usage = 0;
    if (ch >= 'a' && ch <= 'z') { *usage = (uint8_t)(4 + ch - 'a'); return true; }
    if (ch >= 'A' && ch <= 'Z') {
        *usage = (uint8_t)(4 + ch - 'A'); *modifier = CHAON_SHIFT; return true;
    }
    if (ch >= '1' && ch <= '9') { *usage = (uint8_t)(30 + ch - '1'); return true; }
    if (ch == '0') { *usage = 39; return true; }
    static const char shifted_digits[] = "!@#$%^&*()";
    for (uint8_t i = 0; i < 10; ++i) {
        if (ch == shifted_digits[i]) {
            *modifier = CHAON_SHIFT;
            *usage = i == 9 ? 39 : (uint8_t)(30 + i);
            return true;
        }
    }
    switch (ch) {
        case ' ': *usage = 44; break;
        case '-': *usage = 45; break;
        case '_': *usage = 45; *modifier = CHAON_SHIFT; break;
        case '=': *usage = 46; break;
        case '+': *usage = 46; *modifier = CHAON_SHIFT; break;
        case '[': *usage = 47; break;
        case '{': *usage = 47; *modifier = CHAON_SHIFT; break;
        case ']': *usage = 48; break;
        case '}': *usage = 48; *modifier = CHAON_SHIFT; break;
        case '\\': *usage = 49; break;
        case '|': *usage = 49; *modifier = CHAON_SHIFT; break;
        case ';': *usage = 51; break;
        case ':': *usage = 51; *modifier = CHAON_SHIFT; break;
        case 39: *usage = 52; break;
        case '"': *usage = 52; *modifier = CHAON_SHIFT; break;
        case 96: *usage = 53; break;
        case '~': *usage = 53; *modifier = CHAON_SHIFT; break;
        case ',': *usage = 54; break;
        case '<': *usage = 54; *modifier = CHAON_SHIFT; break;
        case '.': *usage = 55; break;
        case '>': *usage = 55; *modifier = CHAON_SHIFT; break;
        case '/': *usage = 56; break;
        case '?': *usage = 56; *modifier = CHAON_SHIFT; break;
        default: return false;
    }
    return true;
}
