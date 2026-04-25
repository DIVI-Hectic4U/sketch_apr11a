#define LV_CONF_SKIP
#include "display.h"
#include "config.h"
#include <Wire.h>
#include "src/ui/ui_manager.h"

static lv_display_t * disp;
static lv_indev_t   * indev;
static uint8_t      * buf1;
static uint8_t      * buf2;

LGFX lcd;

/* Display flushing for LVGL 9 */
void my_disp_flush(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.pushPixels((uint16_t *)px_map, w * h, true);
    lcd.endWrite();

    lv_display_flush_ready(display);
}

/* Reading input device (touch) for LVGL 9 */
void my_touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    uint16_t touchX, touchY;
    bool touched = lcd.getTouch(&touchX, &touchY);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

void lv_tick_task(void *pvParameters) {
    while (1) {
        lv_tick_inc(5);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void lvgl_handler_task(void *pvParameters) {
    while (1) {
        lv_timer_handler(); 
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void display_init() {
    // Hard reset
    Serial.println("Performing Hardware Reset...");
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);  delay(100);
    digitalWrite(4, HIGH); delay(200);

    // Init I2C for touch
    Wire.begin(6, 5);
    Wire.setClock(400000);

    // Init LCD
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(255);
    Serial.println("LCD Initialized.");

    // Flash test
    lcd.fillScreen(TFT_RED);
    delay(500);
    lcd.fillScreen(TFT_GREEN);
    delay(500);
    lcd.fillScreen(TFT_BLACK);

    // LVGL Init
    lv_init();
    Serial.println("LVGL Initialized.");

    // Buffer allocation
    size_t buffer_size = screenWidth * 40 * sizeof(lv_color16_t);
    buf1 = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    buf2 = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    if (!buf1) buf1 = (uint8_t *)malloc(buffer_size);
    if (!buf2) buf2 = (uint8_t *)malloc(buffer_size);

    if (!buf1 || !buf2) {
        Serial.println("FATAL: Memory allocation failed!");
        while(1) delay(1000);
    }

    // Register Display
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.println("LVGL Display Driver Registered.");

    // Register Touch
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    Serial.println("LVGL Input Driver Registered.");

    // Start FreeRTOS Tasks
    xTaskCreatePinnedToCore(lv_tick_task, "lv_tick", 4096, NULL, LV_TICK_PRIORITY, NULL, 0);
    xTaskCreatePinnedToCore(lvgl_handler_task, "lv_handler", 8192, NULL, LV_HANDLER_PRIORITY, NULL, 1);
    Serial.println("LVGL Tasks Started.");
}
