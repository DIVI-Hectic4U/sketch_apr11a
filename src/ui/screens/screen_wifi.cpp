#define LV_CONF_SKIP
#include "screen_wifi.h"

lv_obj_t* create_screen_wifi() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e293b), 0);
    
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "WiFi Settings");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    
    return scr;
}
