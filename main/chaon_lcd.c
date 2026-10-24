/*
 * Native ESP-IDF SPI ST7735 transport for original LILYGO T-Dongle-S3.
 * Adapted from ST7735 controller command semantics, not Arduino firmware.
 * Reference pins: MOSI3 SCLK5 CS4 DC2 RST1 active-low BL38.
 * Landscape MADCTL, inversion, and offsets (x+1, y+26) need board proof.
 */
#include "chaon_lcd.h"
#include "chaon_screen.h"

#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_SPI SPI2_HOST
#define LCD_MOSI GPIO_NUM_3
#define LCD_CLK GPIO_NUM_5
#define LCD_CS GPIO_NUM_4
#define LCD_DC GPIO_NUM_2
#define LCD_RST GPIO_NUM_1
#define LCD_BL GPIO_NUM_38

static const char *TAG = "chaon_lcd";
static spi_device_handle_t display;
static bool lcd_initialized;
static bool backlight_enabled;
/* Static, DMA-compatible framebuffer. 25 KiB SRAM, NO PSRAM assumed. */
static uint8_t framebuffer[CHAON_SCREEN_BYTES] __attribute__((aligned(4)));

static esp_err_t tx(bool data, const void *bytes, size_t length)
{
    if (!display || !bytes || !length) return ESP_ERR_INVALID_ARG;
    gpio_set_level(LCD_DC, data ? 1 : 0);
    spi_transaction_t transfer = {
        .length = length * 8U,
        .tx_buffer = bytes,
    };
    return spi_device_polling_transmit(display, &transfer);
}

static esp_err_t command(uint8_t code, const uint8_t *params, size_t len)
{
    esp_err_t result = tx(false, &code, 1);
    if (result == ESP_OK && len != 0) result = tx(true, params, len);
    return result;
}

static bool init_commands(void)
{
    /* Common ST7735 power/gamma register setup, initial hardware candidate. */
    static const uint8_t rate[] = {0x01,0x2c,0x2d};
    static const uint8_t rate3[] = {0x01,0x2c,0x2d,0x01,0x2c,0x2d};
    static const uint8_t power1[] = {0xa2,0x02,0x84};
    static const uint8_t power2[] = {0xc5};
    static const uint8_t power3[] = {0x0a,0x00};
    static const uint8_t power4[] = {0x8a,0x2a};
    static const uint8_t power5[] = {0x8a,0xee};
    static const uint8_t vcom[] = {0x0e};
    static const uint8_t madctl[] = {0xa8}; /* MY + MV + BGR */
    static const uint8_t colmod[] = {0x05}; /* RGB565 */
    static const uint8_t invctr[] = {0x07};
    if (command(0x01, NULL, 0) != ESP_OK) return false; /* SWRESET */
    vTaskDelay(pdMS_TO_TICKS(150));
    if (command(0x11, NULL, 0) != ESP_OK) return false; /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    const struct {
        uint8_t command;
        const uint8_t *bytes;
        size_t size;
    } sequence[] = {
        {0xb1, rate, sizeof rate}, {0xb2, rate, sizeof rate},
        {0xb3, rate3, sizeof rate3}, {0xb4, invctr, sizeof invctr},
        {0xc0, power1, sizeof power1}, {0xc1, power2, sizeof power2},
        {0xc2, power3, sizeof power3}, {0xc3, power4, sizeof power4},
        {0xc4, power5, sizeof power5}, {0xc5, vcom, sizeof vcom},
        {0x36, madctl, sizeof madctl}, {0x3a, colmod, sizeof colmod},
        {0x21, NULL, 0}, /* INVON, matching vendor example intention */
        {0x13, NULL, 0}, /* NORON */
        {0x29, NULL, 0}, /* DISPON */
    };
    for (size_t i = 0; i < sizeof sequence / sizeof sequence[0]; ++i) {
        if (command(sequence[i].command, sequence[i].bytes,
                    sequence[i].size) != ESP_OK) {
            ESP_LOGE(TAG, "LCD_INIT_FAIL command=0x%02x", sequence[i].command);
            return false;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    return true;
}

bool chaon_lcd_open(void)
{
    lcd_initialized = false;
    backlight_enabled = false;
    /* Preload the inactive level into the GPIO output latch BEFORE enabling
     * output, reducing the risk of a backlight-on glitch at pin activation.
     * ESP-IDF gpio_set_level() writes this latch even while configured input.
     */
    gpio_set_level(LCD_BL, 1);
    const gpio_config_t pins = {
        .pin_bit_mask = BIT64(LCD_DC) | BIT64(LCD_RST) | BIT64(LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&pins);
    if (err != ESP_OK) goto fail;
    gpio_set_level(LCD_BL, 1);
    const spi_bus_config_t bus = {
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CHAON_SCREEN_BYTES + 8,
    };
    err = spi_bus_initialize(LCD_SPI, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) goto fail;
    const spi_device_interface_config_t device_config = {
        .clock_speed_hz = 16000000,
        .mode = 0,
        .spics_io_num = LCD_CS,
        .queue_size = 1,
    };
    err = spi_bus_add_device(LCD_SPI, &device_config, &display);
    if (err != ESP_OK) goto fail;
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    if (!init_commands()) {
        ESP_LOGE(TAG, "LCD_INIT_FAIL register sequence");
        return false;
    }
    lcd_initialized = true;
    ESP_LOGI(TAG, "ST7735 initialized (landscape 160x80 candidate)");
    return true;

fail:
    ESP_LOGE(TAG, "LCD_INIT_FAIL esp_err=%s", esp_err_to_name(err));
    return false;
}

/* A contiguous full-width band shares the existing DMA framebuffer.
 * Only the main UI loop calls this code, never the SD task. */
static bool flush_band(unsigned y, unsigned height)
{
    const uint8_t columns[] = {0x00, 0x01, 0x00, 0xa0};
    const uint8_t rows[] = {
        0x00, (uint8_t)(26U + y), 0x00, (uint8_t)(25U + y + height)
    };
    esp_err_t result = command(0x2a, columns, sizeof columns);
    if (result == ESP_OK)
        result = command(0x2b, rows, sizeof rows);
    if (result == ESP_OK)
        result = command(0x2c, NULL, 0);
    if (result == ESP_OK)
        result = tx(true, framebuffer + y * CHAON_SCREEN_WIDTH * 2U,
                    height * CHAON_SCREEN_WIDTH * 2U);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "LCD_WRITE_FAIL esp_err=%s", esp_err_to_name(result));
        gpio_set_level(LCD_BL, 1);
        return false;
    }
    return true;
}

bool chaon_lcd_show(chaon_status_t status, uint32_t remaining_s,
                    chaon_sd_diag_t sd)
{
    if (!lcd_initialized) return false;
    if (!chaon_screen_render(status, remaining_s, sd,
                             framebuffer, sizeof framebuffer)) return false;
    if (!flush_band(0, CHAON_SCREEN_HEIGHT)) return false;
    if (!backlight_enabled) {
        gpio_set_level(LCD_BL, 0);
        backlight_enabled = true;
    }
    return true;
}

bool chaon_lcd_show_sd(chaon_sd_diag_t sd)
{
    if (!lcd_initialized || !backlight_enabled) return false;
    if (!chaon_screen_render_sd(sd, framebuffer, sizeof framebuffer)) return false;
    return flush_band(CHAON_SCREEN_SD_Y, CHAON_SCREEN_SD_HEIGHT);
}

bool chaon_lcd_show_catalog(const chaon_fat_catalog_t *cat, uint8_t index)
{
    if (!lcd_initialized || !backlight_enabled) return false;
    if (!chaon_screen_render_catalog(cat, index, framebuffer,
                                      sizeof framebuffer)) return false;
    return flush_band(CHAON_SCREEN_SD_Y, CHAON_SCREEN_SD_HEIGHT);
}

bool chaon_lcd_show_browser(const chaon_fat_catalog_t *cat,
                            uint8_t index, bool picked)
{
    if (!lcd_initialized || !backlight_enabled) return false;
    if (!chaon_screen_render_browser(cat, index, picked, framebuffer,
                                     sizeof framebuffer)) return false;
    return flush_band(0, CHAON_SCREEN_HEIGHT);
}

bool chaon_lcd_show_browser_inspect(const chaon_fat_catalog_t *cat,
    uint8_t index, bool picked, const chaon_inspection_t *info)
{
    if (!lcd_initialized || !backlight_enabled) return false;
    if (!chaon_screen_render_browser_inspect(cat, index, picked, info,
                                              framebuffer, sizeof framebuffer))
        return false;
    return flush_band(0, CHAON_SCREEN_HEIGHT);
}

bool chaon_lcd_show_confirmation(uint32_t remaining_seconds)
{
    if (!lcd_initialized) return false;
    if (!chaon_screen_render_confirmation(remaining_seconds,
                                          framebuffer, sizeof framebuffer))
        return false;
    if (!flush_band(0, CHAON_SCREEN_HEIGHT)) return false;
    if (!backlight_enabled) {
        gpio_set_level(LCD_BL, 0);
        backlight_enabled = true;
    }
    return true;
}

bool chaon_lcd_show_hold(uint32_t elapsed_ms)
{
    if (!lcd_initialized) return false;
    if (!chaon_screen_render_hold(elapsed_ms, framebuffer, sizeof framebuffer))
        return false;
    if (!flush_band(0, CHAON_SCREEN_HEIGHT)) return false;
    if (!backlight_enabled) {
        gpio_set_level(LCD_BL, 0);
        backlight_enabled = true;
    }
    return true;
}
