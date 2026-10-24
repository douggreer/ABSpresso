// Hardware bring-up for the SpotPear ESP32-S3-AI-BOX-V1.28.

#include "board.h"
#include "power.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "board";

// SpotPear routes touch and codec control over separate I2C pin pairs.
#define PIN_TOUCH_I2C_SDA 11
#define PIN_TOUCH_I2C_SCL 7
#define PIN_AUDIO_I2C_SDA 15
#define PIN_AUDIO_I2C_SCL 14

// 240x240 GC9A01 round display, 4-wire SPI.
#define LCD_HOST       SPI2_HOST
#define PIN_LCD_MOSI   2
#define PIN_LCD_SCK    4
#define PIN_LCD_CS     5
#define PIN_LCD_DC     47
#define PIN_LCD_RST    38
#define PIN_LCD_BL     42
#define LCD_PCLK_HZ    (40 * 1000 * 1000)
#define LCD_BUF_LINES  40

// CST816D touch controller. The Espressif CST816S-family driver responds on the tested board;
// the supplied datasheet does not document the coordinate-register protocol.
#define PIN_TOUCH_INT  12
#define PIN_TOUCH_RST  6

// Onboard ES8311 codec and NS4150B speaker amplifier.
#define PIN_I2S_MCLK   16
#define PIN_I2S_BCLK   9
#define PIN_I2S_WS     45
#define PIN_I2S_DOUT   8
#define PIN_PA_EN      46
#define ES8311_ADDR  0x18

static i2c_master_bus_handle_t s_touch_i2c_bus;
static i2c_master_bus_handle_t s_audio_i2c_bus;
static esp_lcd_panel_io_handle_t s_panel_io;
static esp_lcd_panel_handle_t s_panel;
static esp_lcd_touch_handle_t s_tp;
static bool s_rotated;
static i2s_chan_handle_t s_i2s_tx;
static i2c_master_dev_handle_t s_es8311;
static bool s_i2s_enabled;
static int s_rate, s_channels;
static int s_vol_q15 = 32768 / 2;

static esp_err_t touch_i2c_init(void)
{
    if (s_touch_i2c_bus) {
        return ESP_OK;
    }
    const i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_TOUCH_I2C_SDA,
        .scl_io_num = PIN_TOUCH_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&cfg, &s_touch_i2c_bus);
}

static esp_err_t audio_i2c_init(void)
{
    if (s_audio_i2c_bus) return ESP_OK;
    const i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = PIN_AUDIO_I2C_SDA,
        .scl_io_num = PIN_AUDIO_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&cfg, &s_audio_i2c_bus);
}

static void backlight_init(void)
{
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);
    const ledc_channel_config_t ch = {
        .gpio_num = PIN_LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&ch);
}

void board_set_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1023 * percent) / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static esp_err_t lcd_init(esp_lcd_panel_io_handle_t *out_io, esp_lcd_panel_handle_t *out_panel)
{
    const spi_bus_config_t bus_cfg = GC9A01_PANEL_BUS_SPI_CONFIG(
        PIN_LCD_SCK, PIN_LCD_MOSI, BOARD_LCD_H_RES * LCD_BUF_LINES * sizeof(uint16_t));
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO), TAG, "SPI bus init failed");

    esp_lcd_panel_io_spi_config_t lcd_io_cfg = GC9A01_PANEL_IO_SPI_CONFIG(PIN_LCD_CS, PIN_LCD_DC, NULL, NULL);
    lcd_io_cfg.pclk_hz = LCD_PCLK_HZ;
    lcd_io_cfg.trans_queue_depth = 10;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &lcd_io_cfg, out_io), TAG, "panel IO");

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_gc9a01(*out_io, &panel_cfg, out_panel), TAG, "panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(*out_panel), TAG, "panel reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(*out_panel), TAG, "panel init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(*out_panel, true), TAG, "panel on");
    return ESP_OK;
}

static esp_err_t touch_init(esp_lcd_touch_handle_t *out_tp)
{
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    io_cfg.scl_speed_hz = 400000;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_touch_i2c_bus, &io_cfg, &io), TAG, "touch IO");
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BOARD_LCD_H_RES,
        .y_max = BOARD_LCD_V_RES,
        .rst_gpio_num = PIN_TOUCH_RST,
        // No interrupt pin: with one, esp_lvgl_port switches LVGL to event-driven input and only
        // reads the controller when it interrupts. The CST816 doesn't reliably interrupt on
        // release or slow drags, so presses stuck and drags were missed. Polling every refresh
        // period is reliable and cheap.
        .int_gpio_num = GPIO_NUM_NC,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_cst816s(io, &tp_cfg, out_tp), TAG, "touch");

    ESP_LOGI(TAG, "touch controller initialized (CST816D via CST816S-family driver)");
    return ESP_OK;
}

static bool touch_poll(lv_point_t *at)
{
    esp_lcd_touch_point_data_t pt;
    uint8_t n = 0;
    if (!s_tp || esp_lcd_touch_read_data(s_tp) != ESP_OK || esp_lcd_touch_get_data(s_tp, &pt, &n, 1) != ESP_OK || !n) {
        return false;
    }
    if (at) {
        // Touch already reports the unmirrored logical coordinates. The panel's native X
        // correction is display-only; apply touch transforms only for the optional 180 rotation.
        at->x = s_rotated ? BOARD_LCD_H_RES - 1 - pt.x : pt.x;
        at->y = s_rotated ? BOARD_LCD_V_RES - 1 - pt.y : pt.y;
    }
    return true;
}

static void touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    lv_point_t at;
    const bool pressed = touch_poll(&at);
    // The power manager sees every touch (to keep the screen awake) and may hold one back.
    if (power_filter_touch(pressed)) {
        data->point = at;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool board_touch_pressed(void)
{
    return touch_poll(NULL);
}

static void lcd_cmd(uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(s_panel_io, cmd, NULL, 0);
}

void board_display_power(bool on)
{
    if (!s_panel) return;
    if (on) {
        lcd_cmd(0x11);  // sleep out
        vTaskDelay(pdMS_TO_TICKS(120));
        esp_lcd_panel_disp_on_off(s_panel, true);
    } else {
        esp_lcd_panel_disp_on_off(s_panel, false);
        lcd_cmd(0x10);  // sleep in
    }
}

void board_set_rotated(bool rotated)
{
    s_rotated = rotated;
    // The board's native upright orientation is mirrored on X. Rotate 180 degrees from it
    // by toggling both axes: (X=true, Y=false) becomes (X=false, Y=true).
    if (s_panel) esp_lcd_panel_mirror(s_panel, !rotated, rotated);
}

void board_prepare_deep_sleep(void)
{
    board_set_backlight(0);
    board_display_power(false);
    // Let the touch controller drop to its low-power scan; it still pulls INT low on a touch.
}

esp_err_t board_display_init(lv_display_t **out_disp)
{
    ESP_RETURN_ON_ERROR(touch_i2c_init(), TAG, "touch I2C init failed");
    backlight_init();

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_handle_t panel = NULL;
    ESP_RETURN_ON_ERROR(lcd_init(&io, &panel), TAG, "LCD");
    s_panel_io = io;
    s_panel = panel;

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_stack = 8192;
    port_cfg.task_affinity = 1;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_cfg), TAG, "LVGL port");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io,
        .panel_handle = panel,
        .buffer_size = BOARD_LCD_H_RES * LCD_BUF_LINES,
        .double_buffer = true,
        .hres = BOARD_LCD_H_RES,
        .vres = BOARD_LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        },
    };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    ESP_RETURN_ON_FALSE(disp, ESP_FAIL, TAG, "add display");

    esp_lcd_touch_handle_t tp = NULL;
    if (touch_init(&tp) == ESP_OK) {
        // Our own polled input device rather than lvgl_port_add_touch(): that one aborts on any
        // failed I2C read, where a missed read should just count as "not touched".
        lvgl_port_lock(0);
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, touch_read);
        s_tp = tp;
        lv_indev_set_display(indev, disp);
        // Let vertical lists take over after a small intentional drag. LVGL uses this value to
        // suppress short clicks once the pointer has moved, so book taps remain taps when the
        // finger stays nearly still.
        lv_indev_set_scroll_limit(indev, 6);
        lvgl_port_unlock();
    } else {
        ESP_LOGE(TAG, "Touch init failed; continuing without touch");
    }

    board_set_backlight(80);
    if (out_disp) {
        *out_disp = disp;
    }
    return ESP_OK;
}

static esp_err_t es8311_write(uint8_t reg, uint8_t val)
{
    const uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_es8311, buf, sizeof(buf), 100);
}

// Slave mode, MCLK from the MCLK pin at 256 x fs, 16-bit I2S. With a fixed 256 x fs ratio the
// clock dividers are the same at every sample rate (see Espressif's es8311 coefficient table).
static esp_err_t es8311_init(void)
{
    static const uint8_t seq[][2] = {
        {0x00, 0x1F}, {0x00, 0x00}, {0x00, 0x80},  // reset, power on, slave mode
        {0x01, 0x3F},                              // all clocks on, MCLK from pin
        {0x02, 0x00}, {0x03, 0x10}, {0x04, 0x10},  // pre-div/mult 1, single speed, ADC/DAC OSR
        {0x05, 0x00}, {0x06, 0x03}, {0x07, 0x00}, {0x08, 0xFF},
        {0x09, 0x0C}, {0x0A, 0x0C},                // SDP in/out: I2S, 16-bit
        {0x0D, 0x01}, {0x0E, 0x02},                // power up analog
        {0x12, 0x00}, {0x13, 0x10},                // power up DAC, enable output drive
        {0x1C, 0x6A}, {0x37, 0x08},                // bypass ADC/DAC equalisers
        {0x31, 0x00},                              // unmute
        {0x32, 0xBF},                              // DAC 0 dB; volume is applied in software
    };
    for (size_t i = 0; i < sizeof(seq) / sizeof(seq[0]); i++) {
        ESP_RETURN_ON_ERROR(es8311_write(seq[i][0], seq[i][1]), TAG, "ES8311 reg %02x", seq[i][0]);
        if (i == 0) vTaskDelay(pdMS_TO_TICKS(20));
    }
    return ESP_OK;
}

esp_err_t board_audio_init(void)
{
    ESP_RETURN_ON_ERROR(audio_i2c_init(), TAG, "audio I2C init failed");
    const bool codec = i2c_master_probe(s_audio_i2c_bus, ES8311_ADDR, 50) == ESP_OK;
    ESP_RETURN_ON_FALSE(codec, ESP_FAIL, TAG, "ES8311 not found on audio I2C bus");
    ESP_LOGI(TAG, "audio: ES8311 + NS4150B");
    const gpio_config_t pa = {.pin_bit_mask = BIT64(PIN_PA_EN), .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&pa), TAG, "amplifier enable GPIO");
    gpio_set_level(PIN_PA_EN, 0);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 512;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_i2s_tx, NULL), TAG, "I2S channel");
    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = PIN_I2S_MCLK,
            .bclk = PIN_I2S_BCLK,
            .ws = PIN_I2S_WS,
            .dout = PIN_I2S_DOUT,
            .din = GPIO_NUM_NC,
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_i2s_tx, &std_cfg), TAG, "I2S std mode");
    s_rate = 44100;
    s_channels = 2;

    if (codec) {
        const i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = ES8311_ADDR,
            .scl_speed_hz = 100000,
        };
        ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_audio_i2c_bus, &dev_cfg, &s_es8311), TAG, "ES8311 device");
        ESP_RETURN_ON_ERROR(es8311_init(), TAG, "ES8311 init");
    }
    return ESP_OK;
}

esp_err_t board_audio_open(int sample_rate, int channels, int bits)
{
    ESP_RETURN_ON_FALSE(s_i2s_tx && bits == 16 && (channels == 1 || channels == 2), ESP_ERR_INVALID_ARG, TAG,
                        "unsupported format %d/%d/%d", sample_rate, channels, bits);
    if (s_i2s_enabled && sample_rate == s_rate && channels == s_channels) {
        return ESP_OK;
    }
    board_audio_close();
    const i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
    const i2s_std_slot_config_t slot = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT, channels == 1 ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO);
    ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_clock(s_i2s_tx, &clk), TAG, "clock");
    ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_slot(s_i2s_tx, &slot), TAG, "slot");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_i2s_tx), TAG, "enable");
    s_rate = sample_rate;
    s_channels = channels;
    s_i2s_enabled = true;
    if (s_es8311) gpio_set_level(PIN_PA_EN, 1);
    ESP_LOGI(TAG, "audio out %d Hz x%d", sample_rate, channels);
    return ESP_OK;
}

void board_audio_close(void)
{
    if (s_i2s_enabled) {
        if (s_es8311) gpio_set_level(PIN_PA_EN, 0);
        i2s_channel_disable(s_i2s_tx);
        s_i2s_enabled = false;
    }
}

esp_err_t board_audio_write(int16_t *samples, int len)
{
    if (!s_i2s_enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    // The DAC has no hardware volume, so scale in place.
    const int n = len / 2;
    for (int i = 0; i < n; i++) {
        samples[i] = (int16_t)((samples[i] * s_vol_q15) >> 15);
    }
    size_t written = 0;
    return i2s_channel_write(s_i2s_tx, samples, len, &written, portMAX_DELAY);
}

void board_audio_set_volume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    // Rough perceptual curve: square of the linear control.
    s_vol_q15 = (32768 * volume * volume) / 10000;
}
