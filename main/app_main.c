/*
 * ChaoN original T-Dongle-S3 USB HID bring-up candidate.
 * Inert on insertion: press/release BOOT once to arm, again to run the
 * explicit current source. LCD/APA102 state indications are advisory for the
 * fixed demo; selected SD execution additionally requires a healthy display
 * so its identity remains visible throughout READY and ARMED.
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
#include "class/msc/msc_device.h"
#include "chaon_usb_storage.h"

#include "chaon_script.h"
#include "chaon_vm.h"
#include "chaon_status.h"
#include "chaon_lcd.h"
#include "chaon_control.h"
#include "chaon_sd_probe.h"
#include "chaon_screen.h"
#include "chaon_selection.h"
#include "chaon_app_decision.h"

#define BUTTON_PIN GPIO_NUM_0
#define APA102_DATA GPIO_NUM_40
#define APA102_CLOCK GPIO_NUM_39
#define USB_DESCRIPTOR_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define MSC_DESCRIPTOR_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

static const char *TAG = "chaon";
static const char demo_source[] = "DELAY 500\nTYPE \"ChaoN USB HID smoke test\"\n";
/* Avoid allocating ~3 KB program objects on FreeRTOS app_main stack. */
static chaon_program_t demo;
static chaon_program_t staged_copy;
static chaon_selection_t selected_program;
static chaon_app_decision_t app_decision;
static chaon_program_t execution_program;
static uint8_t selected_index;
static uint32_t verification_ticket;

/* TinyUSB class-bit default PID scheme: MSC=0x4002, HID=0x4004.
 * Distinct PIDs are essential because Windows caches interface drivers by
 * USB identity; a runtime personality must not reuse the other's PID. */
#define CHAON_ESPRESSIF_VID 0x303A
#define CHAON_MSC_PID 0x4002
#define CHAON_HID_PID 0x4004
#define CHAON_DEVICE_DESC(pid) { \
    .bLength = sizeof(tusb_desc_device_t), \
    .bDescriptorType = TUSB_DESC_DEVICE, \
    .bcdUSB = 0x0200, \
    .bDeviceClass = 0, \
    .bDeviceSubClass = 0, \
    .bDeviceProtocol = 0, \
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE, \
    .idVendor = CHAON_ESPRESSIF_VID, \
    .idProduct = (pid), \
    .bcdDevice = 0x0100, \
    .iManufacturer = 1, \
    .iProduct = 2, \
    .iSerialNumber = 3, \
    .bNumConfigurations = 1, \
}
static const tusb_desc_device_t hid_device_descriptor =
    CHAON_DEVICE_DESC(CHAON_HID_PID);
static const tusb_desc_device_t msc_device_descriptor =
    CHAON_DEVICE_DESC(CHAON_MSC_PID);

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, USB_DESCRIPTOR_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_descriptor), 0x81, 16, 10),
};
/* Storage-only descriptor: no keyboard interface and no HID endpoint. */
static const uint8_t msc_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, MSC_DESCRIPTOR_LEN, 0, 100),
    TUD_MSC_DESCRIPTOR(0, 4, 0x02, 0x82, 64),
};
static const char *msc_string_descriptor[] = {
    (const char[2]){0x09, 0x04}, "Techbenders",
    "ChaoN USB Storage", "CHN-S3-DEV", "ChaoN Storage"
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
    static bool showed_selected;
    static uint32_t last_selected_generation;
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
        const chaon_app_display_t display = chaon_app_decision_display(
            &app_decision, status, selected_program.valid,
            frame.sd == CHAON_SD_CATALOG_OK);
        const bool browsing = display == CHAON_APP_DISPLAY_CATALOG &&
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
        if (display == CHAON_APP_DISPLAY_SELECTED) {
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

    /* Exactly one USB personality is selected BEFORE USB enumeration.
     * The host cannot own card sectors concurrently with ChaoN's SD task. */
    const chaon_usb_mode_t mode = chaon_usb_boot_mode();
    if (mode == CHAON_USB_BLOCKED) {
        ESP_LOGE(TAG, "SD handoff failed; do not enumerate HID or MSC");
        led_set(64, 0, 0);
        for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    tinyusb_config_t usb = TINYUSB_DEFAULT_CONFIG();
    usb.descriptor.device = &msc_device_descriptor;
    if (mode == CHAON_USB_MSC) {
        tinyusb_msc_storage_handle_t storage = NULL;
        const bool media_ok = chaon_usb_storage_start(&storage);
        usb.descriptor.full_speed_config = msc_configuration_descriptor;
        usb.descriptor.string = msc_string_descriptor;
        usb.descriptor.string_count =
            sizeof(msc_string_descriptor) / sizeof(msc_string_descriptor[0]);
        if (!media_ok) {
            /* The pinned upstream MSC callbacks assume an allocated LUN.
             * Never install USB after a partial storage creation failure. */
            ESP_LOGE(TAG, "MSC startup failed; USB blocked, ROM recovery available");
            led_set(64, 0, 0);
            for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
        }
        ESP_ERROR_CHECK(tinyusb_driver_install(&usb));
        ESP_LOGI(TAG, "STORAGE_ONLY: SD exported to host; HID not enumerated");
        /* Do not start SD probe, HID controller, or VM while host owns MSC. */
        for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    usb.descriptor.device = &hid_device_descriptor;
    usb.descriptor.full_speed_config = hid_configuration_descriptor;
    usb.descriptor.string = hid_string_descriptor;
    usb.descriptor.string_count =
        sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);
    ESP_ERROR_CHECK(tinyusb_driver_install(&usb));
    ESP_LOGI(TAG, "ONE_BOOT_HID: marker consumed; physical ARM/RUN still required");

    bool lcd_ok = chaon_lcd_open(); /* advisory peripheral only */
    if (!lcd_ok) ESP_LOGW(TAG, "LCD unavailable; HID stays operational");

    /* Read-only SD task stages and verifies; it never sends HID reports. */
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
            chaon_app_decision_reset(&app_decision);
            chaon_sd_probe_verify_cancel();
            verification_ticket=0;
        }
        const bool pressed = gpio_get_level(BUTTON_PIN) == 0;
        const int64_t now_ms = esp_timer_get_time() / 1000;

        chaon_fat_catalog_t current_catalog;
        const bool catalog_valid =
            chaon_sd_probe_catalog_snapshot(&current_catalog);
        const bool selected_source_available = selected_program.valid &&
            connected && lcd_ok && catalog_valid &&
            selected_index < current_catalog.count &&
            memcmp(&current_catalog.entries[selected_index],
                &selected_program.identity,
                sizeof selected_program.identity) == 0;
        if (selected_program.valid && !selected_source_available) {
            chaon_selection_clear(&selected_program);
            chaon_app_decision_invalidate_selected(&app_decision);
            chaon_sd_probe_verify_cancel();
            verification_ticket = 0;
            if (control.state == CHAON_STATUS_RUNNING)
                chaon_control_complete(&control, CHAON_RUN_CANCELLED,
                    connected, pressed, now_ms);
        }
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
            chaon_control_step_selection(&control, connected, pressed, now_ms,
                catalog_valid, catalog_valid ? current_catalog.count : 0,
                valid_selected_inspection);
        /* This production decision layer consumes only the real debounced
         * controller's ARM/RUN events. */
        const chaon_app_action_t event_action = chaon_app_decision_event(
            &app_decision, event, now_ms, connected,
            selected_source_available, &selected_program);
        if (event == CHAON_CONTROL_PICK_READY) {
            uint32_t generation=0;
            uint8_t digest[CHAON_SOURCE_SHA256_BYTES]={0};
            bool loaded = false;
            chaon_selection_clear(&selected_program);
            if (connected && catalog_valid && picked_index < current_catalog.count &&
                chaon_sd_probe_staged_program_copy_hashed(picked_index,
                    &current_catalog.entries[picked_index], &generation,
                    &staged_copy, digest) &&
                chaon_selection_load(&selected_program,
                    &current_catalog.entries[picked_index], &staged_copy,
                    generation, usb_epoch, digest)) {
                selected_index=picked_index;
                loaded = true;
                ESP_LOGI(TAG, "PICK READY: %s", selected_program.identity.name);
            } else {
                ESP_LOGW(TAG, "PICK failed: no valid immutable program");
            }
            chaon_app_decision_pick_result(&app_decision, loaded);
            memset(&staged_copy,0,sizeof staged_copy);
        } else if (event == CHAON_CONTROL_DISARM) {
            ESP_LOGI(TAG, "long hold disarmed; no HID execution");
        } else if (event == CHAON_CONTROL_BROWSER_ENTER) {
            chaon_selection_clear(&selected_program);
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
        } else if (event == CHAON_CONTROL_EXPIRE) {
            ESP_LOGI(TAG, "armed window expired");
        } else if (event == CHAON_CONTROL_ARM) {
            ESP_LOGI(TAG, "armed for 30 seconds");
        } else if (event == CHAON_CONTROL_EXECUTE) {
            if (event_action == CHAON_APP_ACTION_VERIFY_SELECTED) {
                verification_ticket=chaon_sd_probe_verify_request(
                    selected_index,&selected_program.identity,
                    selected_program.staging_generation,
                    selected_program.source_sha256);
                ESP_LOGI(TAG,"SD RUN verifying source (ticket=%lu)",
                         (unsigned long)verification_ticket);
                if (!verification_ticket) {
                    (void)chaon_app_decision_verification(&app_decision,
                        CHAON_APP_VERIFY_FAIL, now_ms, connected, false,
                        pressed, &selected_program, &execution_program);
                    chaon_selection_clear(&selected_program);
                    chaon_control_complete(&control,CHAON_RUN_CANCELLED,
                                           connected,pressed,now_ms);
                }
            } else if (event_action == CHAON_APP_ACTION_RUN_DEMO) {
                paint_status(&control, now_ms, &lcd_ok);
                const chaon_transport_t io = {
                    .report = send_hid_report,
                    .sleep_ms = sleep_ms,
                    .cancelled = is_cancelled,
                    .context = NULL,
                };
                const chaon_run_result_t result = chaon_run(&demo, &io);
                const int64_t completed_ms = esp_timer_get_time() / 1000;
                chaon_app_decision_cancel_transaction(&app_decision);
                ESP_LOGI(TAG, "script result=%d", (int)result);
                chaon_control_complete(&control, result, tud_mounted(),
                                       gpio_get_level(BUTTON_PIN) == 0,
                                       completed_ms);
            } else if (event_action == CHAON_APP_ACTION_REJECT_SELECTED) {
                chaon_selection_clear(&selected_program);
                chaon_control_complete(&control, CHAON_RUN_CANCELLED,
                                       connected, pressed, now_ms);
                ESP_LOGW(TAG, "selected RUN denied before verification");
            }
        }
        /* Complete the verified one-shot transaction on the controller task.
         * There is no HID call on a stale/missing/mismatched reply. */
        if (verification_ticket) {
            const chaon_sd_verify_result_t verify =
                chaon_sd_probe_verify_poll(verification_ticket);
            const chaon_app_verify_t app_verify =
                verify == CHAON_SD_VERIFY_PASS ? CHAON_APP_VERIFY_PASS :
                verify == CHAON_SD_VERIFY_WAIT ? CHAON_APP_VERIFY_WAIT :
                CHAON_APP_VERIFY_FAIL;
            const chaon_app_action_t verify_action =
                chaon_app_decision_verification(&app_decision, app_verify,
                    now_ms, connected,
                    selected_source_available &&
                        control.state == CHAON_STATUS_RUNNING,
                    pressed, &selected_program, &execution_program);
            if (verify_action != CHAON_APP_ACTION_NONE) {
                verification_ticket=0;
                if (verify_action == CHAON_APP_ACTION_RUN_SELECTED &&
                    tud_mounted() &&
                    gpio_get_level(BUTTON_PIN) != 0) {
                    paint_status(&control,now_ms,&lcd_ok);
                    const chaon_transport_t io={
                        .report=send_hid_report,.sleep_ms=sleep_ms,
                        .cancelled=is_cancelled,.context=NULL
                    };
                    const chaon_run_result_t result=chaon_run(&execution_program,&io);
                    chaon_app_decision_cancel_transaction(&app_decision);
                    chaon_control_complete(&control,result,tud_mounted(),
                        gpio_get_level(BUTTON_PIN)==0,
                        esp_timer_get_time()/1000);
                    ESP_LOGI(TAG,"selected CHN result=%d",(int)result);
                } else {
                    chaon_selection_clear(&selected_program);
                    chaon_control_complete(&control,CHAON_RUN_CANCELLED,
                        connected,pressed,esp_timer_get_time()/1000);
                    ESP_LOGW(TAG,"selected RUN denied: stale/invalid verification");
                }
                memset(&execution_program,0,sizeof execution_program);
                chaon_sd_probe_verify_cancel();
            }
        }
        if (!control.browser_active && !selected_program.valid)
            chaon_sd_probe_inspect_clear();
        paint_status(&control, esp_timer_get_time() / 1000, &lcd_ok);
        vTaskDelay(pdMS_TO_TICKS(CHAON_CONTROL_POLL_MS));
    }
}
