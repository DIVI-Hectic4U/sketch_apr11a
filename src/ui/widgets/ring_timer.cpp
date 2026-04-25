#define LV_CONF_SKIP
#include "ring_timer.h"

lv_obj_t* create_ring_timer(lv_obj_t* parent) {
    lv_obj_t* arc = lv_arc_create(parent);
    lv_obj_set_size(arc, 220, 220);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_set_style_arc_width(arc, 15, 0);
    lv_obj_set_style_arc_width(arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x3B82F6), LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_center(arc);

    // Time label in center
    lv_obj_t* time_label = lv_label_create(arc);
    lv_label_set_text(time_label, "25:00");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0); // Montserrat 28 is target but 14 for safety
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_center(time_label);
    lv_obj_set_user_data(arc, time_label); // Store label ref

    return arc;
}

void update_ring_timer(lv_obj_t* arc, int value, const char* timeStr) {
    lv_arc_set_value(arc, value);
    lv_obj_t* time_label = (lv_obj_t*)lv_obj_get_user_data(arc);
    if (time_label) {
        lv_label_set_text(time_label, timeStr);
    }
}
