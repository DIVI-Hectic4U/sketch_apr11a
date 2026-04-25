#ifndef DISPLAY_H
#define DISPLAY_H

#include <lvgl.h>

/**
 * Initializes the LovyanGFX display driver and the LVGL graphics library.
 * Performs a hardware reset, I2C initialization, and memory allocation for LVGL.
 */
void display_init();

#endif // DISPLAY_H
