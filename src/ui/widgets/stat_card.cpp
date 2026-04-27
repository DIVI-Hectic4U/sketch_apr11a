#define LV_CONF_SKIP
#include "stat_card.h"
#include <cstdio>

// ---------------------------------------------------------------------------
// Generic stat card
// ---------------------------------------------------------------------------
lv_obj_t* create_stat_card(lv_obj_t* parent, const char* title, const char* value,
                            const char* subtext, lv_color_t color) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 105, 75);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 8, 0);

    // Colored left accent border
    lv_obj_t* border = lv_obj_create(card);
    lv_obj_set_size(border, 4, LV_PCT(100));
    lv_obj_align(border, LV_ALIGN_LEFT_MID, -8, 0);
    lv_obj_set_style_bg_color(border, color, 0);
    lv_obj_set_style_border_width(border, 0, 0);
    lv_obj_set_style_radius(border, 2, 0);

    // Title
    lv_obj_t* title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 5, 0);

    // Value (larger, bold-looking via the same font for now)
    lv_obj_t* val_lbl = lv_label_create(card);
    lv_label_set_text(val_lbl, value);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(val_lbl, lv_color_white(), 0);
    lv_obj_align(val_lbl, LV_ALIGN_TOP_LEFT, 5, 18);

    // Subtext
    lv_obj_t* sub_lbl = lv_label_create(card);
    lv_label_set_text(sub_lbl, subtext);
    lv_obj_set_style_text_font(sub_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_align(sub_lbl, LV_ALIGN_TOP_LEFT, 5, 36);

    return card;
}

// ---------------------------------------------------------------------------
// Spoon card — includes a mini segmented bar chart below the value
// Filled segments = spoons remaining (green), used = dark grey
// ---------------------------------------------------------------------------
lv_obj_t* create_spoon_card(lv_obj_t* parent, int used, int total, lv_color_t color) {
    // Clamp to avoid negative display
    if (used < 0) used = 0;
    if (used > total) used = total;
    int remaining = total - used;

    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 105, 75);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 8, 0);

    // Colored left accent border
    lv_obj_t* border = lv_obj_create(card);
    lv_obj_set_size(border, 4, LV_PCT(100));
    lv_obj_align(border, LV_ALIGN_LEFT_MID, -8, 0);
    lv_obj_set_style_bg_color(border, color, 0);
    lv_obj_set_style_border_width(border, 0, 0);
    lv_obj_set_style_radius(border, 2, 0);

    // Title
    lv_obj_t* title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, "Spoons");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 5, 0);

    // Value: "used / total"
    char val_buf[16];
    snprintf(val_buf, sizeof(val_buf), "%d / %d", used, total);
    lv_obj_t* val_lbl = lv_label_create(card);
    lv_label_set_text(val_lbl, val_buf);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(val_lbl, lv_color_white(), 0);
    lv_obj_align(val_lbl, LV_ALIGN_TOP_LEFT, 5, 18);

    // Mini segmented bar: max 12 visible segments, 5px wide each, 2px gap
    // Cap at 12 to fit within the card width
    int segments = (total > 12) ? 12 : total;
    int seg_w = 5;
    int seg_h = 7;
    int seg_gap = 2;
    int total_bar_w = segments * seg_w + (segments - 1) * seg_gap;
    int x_start = 5;

    // Scale used/remaining to segment count if total > 12
    int filled_segs = (total > 12) ? (remaining * segments / total) : remaining;

    for (int i = 0; i < segments; i++) {
        lv_obj_t* seg = lv_obj_create(card);
        lv_obj_set_size(seg, seg_w, seg_h);
        lv_obj_set_style_border_width(seg, 0, 0);
        lv_obj_set_style_radius(seg, 1, 0);
        lv_obj_set_style_pad_all(seg, 0, 0);
        // Filled = remaining spoons (from the right), used = grey (from left)
        bool is_remaining = (i >= (segments - filled_segs));
        lv_obj_set_style_bg_color(seg,
            is_remaining ? lv_color_hex(0x06B6D4) : lv_color_hex(0x334155), 0);
        lv_obj_align(seg, LV_ALIGN_BOTTOM_LEFT,
                     x_start + i * (seg_w + seg_gap), 0);
    }

    return card;
}
