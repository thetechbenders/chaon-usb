/*
 * ChaoN original T-Dongle-S3 USB HID bring-up candidate.
 * Inert on insertion: press/release BOOT once to arm, again to type
 * a harmless, fixed string. LCD/APA102 state indications are advisory;
 * no display operation is allowed to bypass physical HID arming.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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
#include "chaon_control.h"
#include "chaon_sd_probe.h"
#include "chaon_screen.h"
#include "chaon_selection.h"
#include "chaon_run_authority.h"
#include "chaon_execution_slot.h"

#define BUTTON_PIN GPIO_NUM_0
#define APA102_DATA GPIO_NUM_40
#define APA102_CLOCK GPIO_NUM_39
#define USB_DESCRIPTOR_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const char *TAG = "chaon";
static const char demo_source[] = "DELAY 500\nTYPE \"ChaoN USB HID smoke test\"\n";
/* Avoid allocating ~3 KB program objects on FreeRTOS app_main stack. */
static chaon_program_t demo;
static chaon_program_t staged_copy;
static chaon_selection_t selected_program;
static chaon_run_authority_t selected_authority;
static chaon_execution_grant_t selected_grant;
static chaon_execution_slot_t execution_slot;
static chaon_program_t execution_program;
static uint8_t selected_index;
static uint32_t verification_ticket;
static int64_t verification_started_ms;

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

static void paint_status(const chaon_control_t *control, int64_t now_ms,
                         bool *lcd_ok)
{
    const chaon_status_t status = control->state;
    static bool sent_led = false;
    static chaon_rgb_t previous = {0, 0, 0};
    static bool have_frame = false;
    static chaon_screen_key_t previous_frame;
    static bool showed_catalog;
    static uint8_t last_catalog_index;
    static bool showed_browser;
    static bool showed_confirmation;
    static bool showed_selected;
    static uint32_t last_selected_generation;
    static bool showed_hold;
    static uint32_t last_hold_tick;
    static uint32_t last_confirmation_seconds;
    static uint8_t last_browser_index;
    static bool last_browser_picked;
    static chaon_inspection_t last_browser_inspection;

    const chaon_rgb_t rgb = chaon_status_led(status, (uint32_t)now_ms);
    if (!sent_led || rgb.red != previous.red ||
        rgb.green != previous.green || rgb.blue != previous.blue) {
        led_set(rgb.red, rgb.green, rgb.blue);
        previous = rgb;
        sent_led = true;
    }

    uint32_t seconds_left = 0;
    if (status == CHAON_STATUS_ARMED) {
        const int64_t remaining = CHAON_CONTROL_ARM_TTL_MS -
                                  (now_ms - control->armed_at_ms);
        if (remaining > 0) seconds_left = (uint32_t)((remaining + 999) / 1000);
    }
    if (control->confirm_active) {
        /* UI is observational only. No confirmation event reaches the VM. */
        const int64_t remaining = CHAON_CONTROL_CONFIRM_TTL_MS -
            (now_ms - control->confirm_entered_ms);
        const uint32_t seconds = remaining > 0 ?
            (uint32_t)((remaining + 999) / 1000) : 0;
        if (*lcd_ok && (!showed_confirmation ||
            seconds != last_confirmation_seconds)) {
            if (!chaon_lcd_show_confirmation(seconds)) {
                ESP_LOGE(TAG, "LCD_CONFIRM_FAIL; confirmation will cancel");
                *lcd_ok = false;
            }
        }
        showed_confirmation = true;
        last_confirmation_seconds = seconds;
        have_frame = false;
        showed_browser = false;
        showed_catalog = false;
        showed_selected = false;
        return;
    }
    showed_confirmation = false;
    /* Only a valid selected file produces hold-progress feedback.
     * The UI never grants execution authority or sends HID reports. */
    if (control->browser_active && control->browser_selected &&
        control->saw_press && control->stable_pressed &&
        now_ms >= control->pressed_at_ms && *lcd_ok) {
        chaon_fat_catalog_t cat;
        chaon_inspection_t info = { .state = CHAON_INSPECT_NONE };
        if (chaon_sd_probe_catalog_snapshot(&cat) &&
            cat.count == control->browser_count &&
            control->browser_index < cat.count &&
            chaon_sd_probe_inspect_snapshot(control->browser_index,
                &cat.entries[control->browser_index], &info) &&
            info.state == CHAON_INSPECT_VALID) {
            const int64_t elapsed = now_ms - control->pressed_at_ms;
            const uint32_t held = elapsed >= CHAON_CONTROL_CONFIRM_ENTER_MS ?
                CHAON_CONTROL_CONFIRM_ENTER_MS : (uint32_t)elapsed;
            const uint32_t tick = held / 200U;
            if (!showed_hold || tick != last_hold_tick) {
                if (!chaon_lcd_show_hold(held)) {
                    ESP_LOGE(TAG, "LCD_HOLD_FAIL; confirmation ineligible");
                    *lcd_ok = false;
                }
            }
            last_hold_tick = tick;
            showed_hold = true;
            showed_browser = false;
            have_frame = false;
            return;
        }
    }
    if (showed_hold) showed_browser = false;
    showed_hold = false;
    if (control->browser_active) {
        /* Browser never affects HID control; only the main task owns LCD.
         * The controller checks media availability each poll and closes
         * safely if its bounded catalogue becomes unavailable. */
        if (*lcd_ok) {
            chaon_fat_catalog_t cat;
            if (chaon_sd_probe_catalog_snapshot(&cat) &&
                control->browser_index < cat.count &&
                cat.count == control->browser_count) {
                chaon_inspection_t information =
                    { .state = CHAON_INSPECT_NONE };
                if (control->browser_selected)
                    (void)chaon_sd_probe_inspect_snapshot(
                        control->browser_index,
                        &cat.entries[control->browser_index], &information);
                if (!showed_browser ||
                    last_browser_index != control->browser_index ||
                    last_browser_picked != control->browser_selected ||
                    last_browser_inspection.state != information.state ||
                    last_browser_inspection.step_count != information.step_count ||
                    last_browser_inspection.error_line != information.error_line ||
                    last_browser_inspection.first_op != information.first_op) {
                    if (!chaon_lcd_show_browser_inspect(&cat,
                            control->browser_index, control->browser_selected,
                            &information)) {
                        ESP_LOGE(TAG, "LCD_BROWSER_FAIL; HID still gated");
                        *lcd_ok = false;
                    }
                }
                last_browser_inspection = information;
            }
        }
        last_browser_index = control->browser_index;
        last_browser_picked = control->browser_selected;
        showed_browser = true;
        have_frame = false; /* redraw normal READY fully on browser exit */
        showed_catalog = false;
        showed_selected = false;
        return;
    }
    showed_browser = false;
    const chaon_screen_key_t frame = {
        .status = status, .seconds = seconds_left,
        .sd = chaon_sd_probe_snapshot(),
    };
    const chaon_screen_update_t update =
        chaon_screen_update(have_frame ? &previous_frame : NULL, &frame);
    if (*lcd_ok && update != CHAON_SCREEN_NO_UPDATE) {
        const bool success = update == CHAON_SCREEN_FULL_UPDATE ?
            chaon_lcd_show(status, seconds_left, frame.sd) :
            chaon_lcd_show_sd(frame.sd);
        if (!success) {
            ESP_LOGE(TAG, "LCD_WRITE_FAIL; HID remains usable");
            *lcd_ok = false;
        }
    }
    if (*lcd_ok) {
        chaon_fat_catalog_t cat;
        const bool browsing = !selected_program.valid && status == CHAON_STATUS_READY &&
                              frame.sd == CHAON_SD_CATALOG_OK &&
                              chaon_sd_probe_catalog_snapshot(&cat);
        if (browsing) {
            const uint8_t index = cat.count ?
                (uint8_t)(((uint64_t)now_ms / 3000U) % cat.count) : 0;
            if (!showed_catalog || index != last_catalog_index ||
                update != CHAON_SCREEN_NO_UPDATE) {
                if (!chaon_lcd_show_catalog(&cat, index)) {
                    ESP_LOGE(TAG, "LCD_CATALOG_FAIL; HID remains usable");
                    *lcd_ok = false;
                }
            }
            last_catalog_index = index;
        }
        if ((status == CHAON_STATUS_READY || status == CHAON_STATUS_ARMED) && selected_program.valid) {
            chaon_fat_catalog_t one = {0};
            one.count = 1;
            one.entries[0] = selected_program.identity;
            if (!showed_selected || last_selected_generation != selected_program.staging_generation || update != CHAON_SCREEN_NO_UPDATE) {
                if (!chaon_lcd_show_catalog(&one, 0)) *lcd_ok = false;
            }
            showed_selected = true;
            last_selected_generation = selected_program.staging_generation;
        }
        if (!selected_program.valid) showed_selected = false;
        showed_catalog = browsing;
    }
    previous_frame = frame;
    have_frame = true;
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

    /* Advisory SD-only task. SD never supplies executable HID input. */
    chaon_sd_probe_start();

    /* The portable controller owns the physical arm/execute state machine.
     * Hardware I/O, LEDs, LCD and VM execution remain in this shell.
     */
    chaon_control_t control = {0};
    bool previous_connected = false;
    uint32_t usb_epoch = 1;

    for (;;) {
        const bool connected = tud_mounted();
        if (connected != previous_connected) {
            previous_connected = connected;
            if (++usb_epoch == 0) usb_epoch = 1;
            chaon_selection_clear(&selected_program);
            chaon_run_authority_cancel(&selected_authority);
            chaon_execution_grant_clear(&selected_grant);
            chaon_execution_slot_clear(&execution_slot);
            chaon_sd_probe_verify_cancel();
            verification_ticket=0;
        }
        const bool pressed = gpio_get_level(BUTTON_PIN) == 0;
        const int64_t now_ms = esp_timer_get_time() / 1000;

        chaon_fat_catalog_t current_catalog;
        const bool catalog_valid =
            chaon_sd_probe_catalog_snapshot(&current_catalog);
        chaon_inspection_t current_inspection = {
            .state = CHAON_INSPECT_NONE
        };
        const bool valid_selected_inspection =
            lcd_ok && connected && control.browser_active &&
            control.browser_selected && catalog_valid &&
            control.browser_index < current_catalog.count &&
            chaon_sd_probe_inspect_snapshot(control.browser_index,
                &current_catalog.entries[control.browser_index],
                &current_inspection) &&
            current_inspection.state == CHAON_INSPECT_VALID;
        const uint8_t picked_index = control.browser_index;
        const chaon_control_event_t event =
            chaon_control_step_confirmation(&control, connected, pressed, now_ms,
                catalog_valid, catalog_valid ? current_catalog.count : 0,
                valid_selected_inspection);
        /* This is the sole production producer of a selected-program
         * grant: the real debounced physical controller's ARM/RUN events. */
        (void)chaon_run_authority_step(&selected_authority,event,now_ms,
            connected,catalog_valid && lcd_ok && selected_program.valid,
            &selected_program,&selected_grant);
        if (event == CHAON_CONTROL_PICK_READY) {
            uint32_t generation=0;
            uint8_t digest[CHAON_SOURCE_SHA256_BYTES]={0};
            chaon_selection_clear(&selected_program);
            if (connected && catalog_valid && picked_index < current_catalog.count &&
                chaon_sd_probe_staged_program_copy_hashed(picked_index,
                    &current_catalog.entries[picked_index], &generation,
                    &staged_copy, digest) &&
                chaon_selection_load(&selected_program,
                    &current_catalog.entries[picked_index], &staged_copy,
                    generation, usb_epoch, digest)) {
                selected_index=picked_index;
                ESP_LOGI(TAG, "PICK READY: %s", selected_program.identity.name);
            } else {
                ESP_LOGW(TAG, "PICK failed: no valid immutable program");
            }
            memset(&staged_copy,0,sizeof staged_copy);
        } else if (event == CHAON_CONTROL_DISARM) {
            ESP_LOGI(TAG, "long hold disarmed; no HID execution");
        } else if (event == CHAON_CONTROL_BROWSER_ENTER) {
            chaon_selection_clear(&selected_program);
            chaon_run_authority_cancel(&selected_authority);
            chaon_execution_grant_clear(&selected_grant);
            chaon_execution_slot_clear(&execution_slot);
            chaon_sd_probe_verify_cancel();
            verification_ticket=0;
            ESP_LOGI(TAG, "browser entered; prior selection revoked");
        } else if (event == CHAON_CONTROL_BROWSER_SELECT) {
            if (control.browser_selected && catalog_valid &&
                control.browser_index < current_catalog.count &&
                chaon_sd_probe_inspect_request(control.browser_index,
                    &current_catalog.entries[control.browser_index])) {
                ESP_LOGI(TAG, "read-only inspection requested (NO HID)");
            } else {
                chaon_sd_probe_inspect_clear();
                ESP_LOGI(TAG, "inspection unavailable/cleared (NO HID)");
            }
        } else if (event == CHAON_CONTROL_BROWSER_CLOSE) {
            ESP_LOGI(TAG, "read-only browser exited");
        } else if (event == CHAON_CONTROL_CONFIRM_ENTER) {
            ESP_LOGI(TAG, "physical confirmation screen entered (INERT)");
        } else if (event == CHAON_CONTROL_CONFIRM_CANCEL) {
            ESP_LOGI(TAG, "physical confirmation cancelled (NO HID)");
        } else if (event == CHAON_CONTROL_CONFIRM_ACK) {
            ESP_LOGI(TAG, "physical confirmation acknowledged (NO HID)");
        } else if (event == CHAON_CONTROL_EXPIRE) {
            ESP_LOGI(TAG, "armed window expired");
        } else if (event == CHAON_CONTROL_ARM) {
            ESP_LOGI(TAG, "armed for 30 seconds");
        } else if (event == CHAON_CONTROL_EXECUTE) {
            if (selected_program.valid) {
                if (selected_grant.issued && connected && lcd_ok && !pressed) {
                    verification_ticket=chaon_sd_probe_verify_request(
                        selected_index,&selected_program.identity,
                        selected_program.staging_generation,
                        selected_program.source_sha256);
                    verification_started_ms=now_ms;
                    ESP_LOGI(TAG,"SD RUN verifying source (ticket=%lu)",
                             (unsigned long)verification_ticket);
                }
                if (!verification_ticket) {
                    chaon_execution_grant_clear(&selected_grant);
                    chaon_control_complete(&control,CHAON_RUN_CANCELLED,
                                           connected,pressed,now_ms);
                }
            } else {
            paint_status(&control, now_ms, &lcd_ok);
            const chaon_transport_t io = {
                .report = send_hid_report,
                .sleep_ms = sleep_ms,
                .cancelled = is_cancelled,
                .context = NULL,
            };
            const chaon_run_result_t result = chaon_run(&demo, &io);
            const int64_t completed_ms = esp_timer_get_time() / 1000;
            ESP_LOGI(TAG, "script result=%d", (int)result);
            chaon_control_complete(&control, result, tud_mounted(),
                                   gpio_get_level(BUTTON_PIN) == 0,
                                   completed_ms);
            }
        }
        /* Complete the verified one-shot transaction on the controller task.
         * There is no HID call on a stale/missing/mismatched reply. */
        if (verification_ticket) {
            const bool eligible = connected && lcd_ok && !pressed &&
                selected_program.valid &&
                control.state == CHAON_STATUS_RUNNING &&
                now_ms >= verification_started_ms &&
                now_ms - verification_started_ms <= 5000;
            const chaon_sd_verify_result_t verify = eligible ?
                chaon_sd_probe_verify_poll(verification_ticket) :
                CHAON_SD_VERIFY_FAIL;
            if (verify != CHAON_SD_VERIFY_WAIT) {
                verification_ticket=0;
                const bool fresh=verify == CHAON_SD_VERIFY_PASS && eligible;
                bool authorized = fresh &&
                    chaon_execution_slot_prepare(&execution_slot,
                        &selected_grant,&selected_program,
                        selected_program.staging_generation,usb_epoch,
                        selected_program.source_sha256,fresh) &&
                    chaon_execution_slot_take(&execution_slot,
                        &selected_program,selected_program.staging_generation,
                        usb_epoch,selected_program.source_sha256,fresh,
                        &execution_program);
                if (authorized && tud_mounted() &&
                    gpio_get_level(BUTTON_PIN) != 0) {
                    paint_status(&control,now_ms,&lcd_ok);
                    const chaon_transport_t io={
                        .report=send_hid_report,.sleep_ms=sleep_ms,
                        .cancelled=is_cancelled,.context=NULL
                    };
                    const chaon_run_result_t result=chaon_run(&execution_program,&io);
                    chaon_control_complete(&control,result,tud_mounted(),
                        gpio_get_level(BUTTON_PIN)==0,
                        esp_timer_get_time()/1000);
                    ESP_LOGI(TAG,"selected CHN result=%d",(int)result);
                } else {
                    chaon_control_complete(&control,CHAON_RUN_CANCELLED,
                        connected,pressed,esp_timer_get_time()/1000);
                    ESP_LOGW(TAG,"selected RUN denied: stale/invalid verification");
                }
                memset(&execution_program,0,sizeof execution_program);
                chaon_execution_slot_clear(&execution_slot);
                chaon_execution_grant_clear(&selected_grant);
                chaon_sd_probe_verify_cancel();
            }
        }
        /* Any deselection, navigation, USB loss or timeout invalidates an
         * in-flight result. The SD task checks its generation before
         * publishing, even if a slow card read completes afterward. */
        if (selected_program.valid) {
            const bool available = connected && lcd_ok && catalog_valid &&
                chaon_sd_probe_snapshot() == CHAON_SD_CATALOG_OK;
            chaon_selection_refresh(&selected_program, usb_epoch, available);
        }
        if (!control.browser_active && !selected_program.valid)
            chaon_sd_probe_inspect_clear();
        paint_status(&control, esp_timer_get_time() / 1000, &lcd_ok);
        vTaskDelay(pdMS_TO_TICKS(CHAON_CONTROL_POLL_MS));
    }
}
