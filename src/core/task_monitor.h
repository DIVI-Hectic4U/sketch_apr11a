#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * Initializes the FreeRTOS task monitoring primitives.
 * Must be called before any LVGL tasks start.
 */
void task_monitor_init();

/**
 * Acquires the LVGL mutex. 
 * MUST be called before modifying any LVGL object from outside the main LVGL task.
 */
void lvgl_acquire();

/**
 * Releases the LVGL mutex.
 */
void lvgl_release();

#endif // TASK_MONITOR_H
