#ifndef STAT_CARD_H
#define STAT_CARD_H

#include <lvgl.h>

/**
 * Creates a generic stat card widget with a colored left-border accent.
 * @param parent   Parent LVGL object (should be a flex-row container).
 * @param title    Short title string, e.g. "XP".
 * @param value    Primary value string, e.g. "2,450 xp".
 * @param subtext  Muted subtext, e.g. "total xp".
 * @param color    Left-border accent color.
 * @return Pointer to the created card lv_obj_t.
 */
lv_obj_t* create_stat_card(lv_obj_t* parent,
                            const char* title,
                            const char* value,
                            const char* subtext,
                            lv_color_t color);

/**
 * Creates the Spoon card, which includes a mini segmented bar
 * showing used vs. remaining spoons beneath the numeric value.
 * @param parent     Parent LVGL object.
 * @param used       Number of spoons consumed today.
 * @param total      Total daily spoon budget.
 * @param color      Left-border accent color (teal).
 * @return Pointer to the created card lv_obj_t.
 */
lv_obj_t* create_spoon_card(lv_obj_t* parent,
                             int used,
                             int total,
                             lv_color_t color);

#endif // STAT_CARD_H
