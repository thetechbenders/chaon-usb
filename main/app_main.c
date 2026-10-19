/*
 * ChaoN original T-Dongle-S3 USB HID bring-up candidate.
 * Inert on insertion: press/release BOOT once to arm, again to type
 * a harmless, fixed string. LCD/APA102 state indications are advisory;
 * no display operation is allowed to bypass physical HID arming.
 */
#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"

#include "chaon_script.h"
#include "chaon_vm.h"
#include "chaon_status.h"
#include "chaon_lcd.h"

#define BUTTON_PIN GPIO_NUM_0
#define APA102_DATA GPIO_NUM_40
#define APA102_CLOCK GPIO_NUM_39
#define USB_DESCRIPTOR_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

enum { BUTTON_POLL_MS = 20, BUTTON_DEBOUNCE_MS = 40, ARM_TTL_MS = 30000 };
static const char *TAG = "chaon";
static const char demo_source[] = "DELAY 500\nTYPE \"ChaoN USB HID smoke test\"\n";
/* Avoid allocating ~3 KB program objects on FreeRTOS app_main stack. */
static chaon_program_t demo;

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, USB_DESCRIPTOR_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_descriptor), 0x81, 16, 10),
};
static const char language_id[2] = {0x09, 0x04};
static const char *hid_string_descriptor[] = {
    language_id, "Techbenders", "ChaoN USB HID", "CHN-S3-DEV", "ChaoN Keyboard"
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t req_len)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)req_len;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t size)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)size;
}

static void apa102_write_byte(uint8_t byte)
{
    for (unsigned bit = 0; bit < 8; ++bit) {
        gpio_set_level(APA102_CLOCK, 0);
        gpio_set_level(APA102_DATA, (byte & 0x80U) ? 1 : 0);
        gpio_set_level(APA102_CLOCK, 1);
        byte <<= 1;
    }
    gpio_set_level(APA102_CLOCK, 0);
}

static void led_set(uint8_t red, uint8_t green, uint8_t blue)
{
    for (int i = 0; i < 4; ++i) apa102_write_byte(0);
    apa102_write_byte(0xE4); /* 4/31 brightness */
    apa102_write_byte(blue); /* APA102 BGR */
    apa102_write_byte(green);
    apa102_write_byte(red);
    for (int i = 0; i < 4; ++i) apa102_write_byte(0xFF);
}

static bool send_hid_report(uint8_t modifier, uint8_t usage, void *ctx)
{
    (void)ctx;
    for (int i = 0; i < 40; ++i) {
        if (!tud_mounted()) return false;
        if (tud_hid_ready()) {
            uint8_t keys[6] = {0};
            keys[0] = usage;
            return tud_hid_keyboard_report(0, modifier, keys);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

static void sleep_ms(uint32_t ms, void *ctx)
{
    (void)ctx;
    vTaskDelay(pdMS_TO_TICKS(ms ? ms : 1));
}

static bool is_cancelled(void *ctx)
{
    (void)ctx;
    return !tud_mounted() || gpio_get_level(BUTTON_PIN) == 0;
}

static void init_gpio(void)
{
    const gpio_config_t button_cfg = {
        .pin_bit_mask = BIT64(BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const gpio_config_t led_cfg = {
        .pin_bit_mask = BIT64(APA102_DATA) | BIT64(APA102_CLOCK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button_cfg));
    ESP_ERROR_CHECK(gpio_config(&led_cfg));
    led_set(24, 33, 12); /* dim ChaoN green until USB mounts */
}

static void paint_status(chaon_status_t status, int64_t now_ms,
                         int64_t armed_at_ms, bool *lcd_ok)
{
    static bool sent_led = false;
    static chaon_rgb_t previous = {0, 0, 0};
    static int last_status = -1;
    static uint32_t last_seconds = UINT32_MAX;

    const chaon_rgb_t rgb = chaon_status_led(status, (uint32_t)now_ms);
    if (!sent_led || rgb.red != previous.red ||
        rgb.green != previous.green || rgb.blue != previous.blue) {
        led_set(rgb.red, rgb.green, rgb.blue);
        previous = rgb;
        sent_led = true;
    }

    uint32_t seconds_left = 0;
    if (status == CHAON_STATUS_ARMED) {
        const int64_t remaining = ARM_TTL_MS - (now_ms - armed_at_ms);
        if (remaining > 0) seconds_left = (uint32_t)((remaining + 999) / 1000);
    }
    if (*lcd_ok && (last_status != (int)status ||
                    last_seconds != seconds_left)) {
        if (!chaon_lcd_show(status, seconds_left)) {
            ESP_LOGE(TAG, "LCD_WRITE_FAIL; HID remains usable");
            *lcd_ok = false;
        }
    }
    last_status = (int)status;
    last_seconds = seconds_left;
}

void app_main(void)
{
    chaon_parse_error_t error = {0};
    if (chaon_parse(demo_source, &demo, &error) != CHAON_PARSE_OK) {
        ESP_LOGE(TAG, "embedded demo parse error at line %u", (unsigned)error.line);
        return;
    }
    init_gpio();

    tinyusb_config_t usb = TINYUSB_DEFAULT_CONFIG();
    usb.descriptor.device = NULL;
    usb.descriptor.full_speed_config = hid_configuration_descriptor;
    usb.descriptor.string = hid_string_descriptor;
    usb.descriptor.string_count =
        sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);
    ESP_ERROR_CHECK(tinyusb_driver_install(&usb));
    ESP_LOGI(TAG, "USB HID configured; no typing until physical confirmation");

    bool lcd_ok = chaon_lcd_open(); /* advisory peripheral only */
    if (!lcd_ok) ESP_LOGW(TAG, "LCD unavailable; HID stays operational");

    chaon_status_t state = CHAON_STATUS_WAIT_USB;
    bool stable_pressed = false;
    bool candidate_pressed = false;
    bool saw_press = false;
    bool suppress_release = false;
    int debounce_ms = 0;
    int64_t armed_at_ms = 0;
    int64_t cancelled_until_ms = 0;

    for (;;) {
        const bool connected = tud_mounted();
        const bool pressed = gpio_get_level(BUTTON_PIN) == 0;
        const int64_t now_ms = esp_timer_get_time() / 1000;

        if (!connected) {
            state = CHAON_STATUS_WAIT_USB;
            saw_press = false;
            suppress_release = pressed;
        } else if (state == CHAON_STATUS_WAIT_USB) {
            state = CHAON_STATUS_READY;
            saw_press = false;
            suppress_release = pressed; /* held on reconnect is not a gesture */
        }
        if (state == CHAON_STATUS_ARMED &&
            now_ms - armed_at_ms >= ARM_TTL_MS) {
            state = CHAON_STATUS_READY;
            saw_press = false;
            ESP_LOGI(TAG, "armed window expired");
        }
        if (state == CHAON_STATUS_CANCELLED &&
            now_ms >= cancelled_until_ms) {
            state = CHAON_STATUS_READY;
        }

        /* Debounced BOOT press/release. The button is GPIO0, a ROM strap pin. */
        if (pressed != candidate_pressed) {
            candidate_pressed = pressed;
            debounce_ms = 0;
        } else if (debounce_ms < BUTTON_DEBOUNCE_MS) {
            debounce_ms += BUTTON_POLL_MS;
        }
        if (debounce_ms >= BUTTON_DEBOUNCE_MS &&
            stable_pressed != candidate_pressed) {
            stable_pressed = candidate_pressed;
            if (stable_pressed) {
                saw_press = connected && !suppress_release &&
                            (state == CHAON_STATUS_READY ||
                             state == CHAON_STATUS_ARMED);
            } else {
                if (suppress_release) {
                    suppress_release = false;
                } else if (saw_press && connected) {
                    if (state == CHAON_STATUS_READY) {
                        state = CHAON_STATUS_ARMED;
                        armed_at_ms = now_ms;
                        ESP_LOGI(TAG, "armed for 30 seconds");
                    } else if (state == CHAON_STATUS_ARMED) {
                        state = CHAON_STATUS_RUNNING;
                        paint_status(state, now_ms, armed_at_ms, &lcd_ok);
                        const chaon_transport_t io = {
                            .report = send_hid_report,
                            .sleep_ms = sleep_ms,
                            .cancelled = is_cancelled,
                            .context = NULL,
                        };
                        const chaon_run_result_t result = chaon_run(&demo, &io);
                        const int64_t completed_ms = esp_timer_get_time() / 1000;
                        ESP_LOGI(TAG, "script result=%d", (int)result);
                        if (!tud_mounted()) {
                            state = CHAON_STATUS_WAIT_USB;
                        } else if (result == CHAON_RUN_CANCELLED) {
                            state = CHAON_STATUS_CANCELLED;
                            cancelled_until_ms = completed_ms + 2000;
                        } else if (result != CHAON_RUN_OK) {
                            state = CHAON_STATUS_FAULT;
                        } else {
                            state = CHAON_STATUS_READY;
                        }

                        /* Consume cancellation/press that occurred during VM
                         * execution. Never interpret its release as a new arm.
                         */
                        stable_pressed = gpio_get_level(BUTTON_PIN) == 0;
                        candidate_pressed = stable_pressed;
                        suppress_release = stable_pressed;
                        debounce_ms = 0;
                    }
                }
                saw_press = false;
            }
        }
        paint_status(state, esp_timer_get_time() / 1000,
                     armed_at_ms, &lcd_ok);
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}
