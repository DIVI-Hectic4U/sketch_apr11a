#ifndef RING_TIMER_H
#define RING_TIMER_H

#include <lvgl.h>

/**
 * Creates a ring timer widget (giant circular arc).
 * @param parent The parent object.
 * @return The created arc object.
 */
lv_obj_t* create_ring_timer(lv_obj_t* parent);

/**
 * Updates the ring timer display.
 * @param arc The arc object.
 * @param value 0-100 progress.
 * @param timeStr Time string (e.g., "24:59").
 */
void update_ring_timer(lv_obj_t* arc, int value, const char* timeStr);

#endif // RING_TIMER_H
