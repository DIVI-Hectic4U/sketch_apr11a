#define LV_CONF_SKIP
#include "screen_boot.h"

lv_obj_t* create_screen_boot() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "FocusOS");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    
    lv_obj_t* sub = lv_label_create(scr);
    lv_label_set_text(sub, "Connecting...");
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x888888), 0);
    
    return scr;
}
