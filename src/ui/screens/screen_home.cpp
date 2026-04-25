#define LV_CONF_SKIP
#include "screen_home.h"

lv_obj_t* create_screen_home() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0); // Slate 900
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Dashboard");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, 20);
    
    lv_obj_t* msg = lv_label_create(scr);
    lv_label_set_text(msg, "Part 3: Screen Architecture Active");
    lv_obj_center(msg);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x94A3B8), 0);
    
    return scr;
}
