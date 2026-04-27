#include "task_monitor.h"

static SemaphoreHandle_t lvgl_mutex = NULL;

void task_monitor_init() {
    if (lvgl_mutex == NULL) {
        lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

void lvgl_acquire() {
    if (lvgl_mutex != NULL) {
        // Wait indefinitely for the mutex
        xSemaphoreTakeRecursive(lvgl_mutex, portMAX_DELAY);
    }
}

void lvgl_release() {
    if (lvgl_mutex != NULL) {
        xSemaphoreGiveRecursive(lvgl_mutex);
    }
}
