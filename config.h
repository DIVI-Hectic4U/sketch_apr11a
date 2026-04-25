#ifndef CONFIG_H
#define CONFIG_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/**
 * Part 0 — Hardware Reference (WT32-SC01 Plus)
 * --------------------------------------------
 * Display driver:   ST7796UI (8-bit parallel MCU8080)
 * Touch controller: FT6336U (I2C)
 * Display pins (8-bit parallel):
 *   TFT_D0-D7:  GPIO 9, 46, 3, 8, 18, 17, 16, 15
 *   TFT_WR:     GPIO 47
 *   TFT_RD:     GPIO -1 (not used)
 *   TFT_RS/DC:  GPIO 0
 *   TFT_CS:     GPIO -1 (not used)
 *   TFT_RST:    GPIO 4 (Shared with Touch)
 *   Backlight:  GPIO 45
 * Touch I2C:
 *   SDA:        GPIO 6
 *   SCL:        GPIO 5
 *   INT:        GPIO 7
 *   RST:        GPIO 4
 * Resolution:   480 (W) x 320 (H) - landscape default
 */

// Screen dimensions
static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 320;

// Display Task Priorities
#define LV_TICK_PRIORITY 1
#define LV_HANDLER_PRIORITY 2

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796    _panel;
  lgfx::Bus_Parallel8   _bus;
  lgfx::Light_PWM       _light;
  lgfx::Touch_FT5x06    _touch;

public:
  LGFX(void) {
    { // Bus — 8080 parallel
      auto cfg = _bus.config();
      cfg.freq_write = 4000000;
      cfg.pin_wr = 47;   // LCD_WR
      cfg.pin_rd = -1;   // not used
      cfg.pin_rs = 0;    // LCD_RS (D/C)
      cfg.pin_d0 = 9;    // LCD_DB0
      cfg.pin_d1 = 46;   // LCD_DB1
      cfg.pin_d2 = 3;    // LCD_DB2
      cfg.pin_d3 = 8;    // LCD_DB3
      cfg.pin_d4 = 18;   // LCD_DB4
      cfg.pin_d5 = 17;   // LCD_DB5
      cfg.pin_d6 = 16;   // LCD_DB6
      cfg.pin_d7 = 15;   // LCD_DB7
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel — ST7796
      auto cfg = _panel.config();
      cfg.pin_cs   = -1;
      cfg.pin_rst  = 4;  // LCD_RESET (GPIO 4)
      cfg.pin_busy = -1;
      cfg.panel_width  = 320;
      cfg.panel_height = 480;
      cfg.offset_rotation = 0;
      _panel.config(cfg);
    }
    { // Backlight
      auto cfg = _light.config();
      cfg.pin_bl = 45;   // BL_PWM
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    { // Touch — FT6336U via I2C
      auto cfg = _touch.config();
      cfg.i2c_port = 1;
      cfg.pin_sda  = 6;  // TP_SDA
      cfg.pin_scl  = 5;  // TP_SCL
      cfg.pin_int  = 7;  // TP_INT
      cfg.pin_rst  = -1; // Shared with LCD (handled by LCD RST)
      cfg.freq     = 400000;
      cfg.x_min = 0; cfg.x_max = 319;
      cfg.y_min = 0; cfg.y_max = 479;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

#endif // CONFIG_H