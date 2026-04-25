#define LV_CONF_SKIP
#include "stat_card.h"

lv_obj_t* create_stat_card(lv_obj_t* parent, const char* title, const char* value, const char* subtext, lv_color_t color) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 105, 70);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    
    // Accent border on the left
    lv_obj_t* border = lv_obj_create(card);
    lv_obj_set_size(border, 4, LV_PCT(100));
    lv_obj_align(border, LV_ALIGN_LEFT_MID, -8, 0);
    lv_obj_set_style_bg_color(border, color, 0);
    lv_obj_set_style_border_width(border, 0, 0);
    lv_obj_set_style_radius(border, 2, 0);
    
    // Title label
    lv_obj_t* title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 5, 0);
    
    // Value label
    lv_obj_t* value_label = lv_label_create(card);
    lv_label_set_text(value_label, value);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_14, 0); // Using 14 as default for now
    lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
    lv_obj_align(value_label, LV_ALIGN_TOP_LEFT, 5, 18);
    
    // Subtext label
    lv_obj_t* sub_label = lv_label_create(card);
    lv_label_set_text(sub_label, subtext);
    lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub_label, lv_color_hex(0x64748B), 0);
    lv_obj_align(sub_label, LV_ALIGN_TOP_LEFT, 5, 36);
    
    return card;
}
