#ifndef STAT_CARD_H
#define STAT_CARD_H

#include <lvgl.h>

/**
 * Creates a stat card widget.
 * @param parent The parent object.
 * @param title The title of the card (e.g., "XP").
 * @param value The value text (e.g., "2,450").
 * @param subtext The subtext (e.g., "xp").
 * @param color The accent color for the left border.
 * @return The created card object.
 */
lv_obj_t* create_stat_card(lv_obj_t* parent, const char* title, const char* value, const char* subtext, lv_color_t color);

#endif // STAT_CARD_H
