#ifndef CHAON_HID_H
#define CHAON_HID_H
#include <stdbool.h>
#include <stdint.h>
enum { CHAON_SHIFT = 0x02, CHAON_KEY_ENTER = 0x28, CHAON_KEY_TAB = 0x2B };
/** Translate printable US-ASCII to standard USB keyboard usage and modifier. */
bool chaon_hid_us_ascii(char ch, uint8_t *modifier, uint8_t *usage);
#endif
