#define LV_CONF_SKIP
#include "screen_home.h"
#include "../../state/app_state.h"
#include "../widgets/stat_card.h"
#include "../ui_manager.h"

static lv_obj_t* create_task_row(lv_obj_t* parent, const TaskInfo& task);

lv_obj_t* create_screen_home() {
    AppState& state = AppState::getInstance();
    
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    // --- Header ---
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "FocusOS Dashboard");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);
    
    lv_obj_t* wifi_icon = lv_label_create(header);
    lv_label_set_text(wifi_icon, state.isWifiConnected ? "WiFi: ON" : "WiFi: OFF");
    lv_obj_set_style_text_color(wifi_icon, state.isWifiConnected ? lv_color_hex(0x10B981) : lv_color_hex(0xEF4444), 0);
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -60, 0);

    lv_obj_t* refresh_btn = lv_btn_create(header);
    lv_obj_set_size(refresh_btn, 40, 30);
    lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x334155), 0);
    lv_obj_t* refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, "R");
    lv_obj_center(refresh_lbl);

    // --- Stats Row ---
    lv_obj_t* stat_row = lv_obj_create(scr);
    lv_obj_set_size(stat_row, 480, 85);
    lv_obj_set_style_bg_opa(stat_row, 0, 0);
    lv_obj_set_style_border_width(stat_row, 0, 0);
    lv_obj_align(stat_row, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_flex_flow(stat_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stat_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(stat_row, 10, 0);

    create_stat_card(stat_row, "XP", String(state.xp).c_str(), "total xp", lv_color_hex(0xA855F7));
    create_stat_card(stat_row, "Spoons", (String(state.spoonsUsed) + " / " + String(state.spoonsTotal)).c_str(), "remaining", lv_color_hex(0x06B6D4));
    create_stat_card(stat_row, "Streak", (String(state.streakDays) + " days").c_str(), "on fire", lv_color_hex(0xF97316));
    create_stat_card(stat_row, "Level", (String("Lv ") + String(state.level)).c_str(), "ascension", lv_color_hex(0xEAB308));

    // --- Task List ---
    lv_obj_t* task_container = lv_obj_create(scr);
    lv_obj_set_size(task_container, 460, 140);
    lv_obj_align(task_container, LV_ALIGN_TOP_MID, 0, 135);
    lv_obj_set_style_bg_color(task_container, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_width(task_container, 0, 0);
    lv_obj_set_flex_flow(task_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(task_container, 5, 0);
    lv_obj_set_style_pad_gap(task_container, 8, 0);

    lv_obj_t* task_label = lv_label_create(scr);
    lv_label_set_text(task_label, "Tasks in Progress");
    lv_obj_set_style_text_color(task_label, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(task_label, LV_ALIGN_TOP_LEFT, 20, 135);
    lv_obj_set_style_pad_top(task_container, 25, 0);

    for (const auto& task : state.tasks) {
        create_task_row(task_container, task);
    }

    // --- Footer ---
    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, 480, 40);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0x1E293B), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t* add_task_btn = lv_btn_create(footer);
    lv_obj_set_size(add_task_btn, 120, 30);
    lv_obj_center(add_task_btn);
    lv_obj_set_style_bg_color(add_task_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_t* add_lbl = lv_label_create(add_task_btn);
    lv_label_set_text(add_lbl, "+ New Task");
    lv_obj_center(add_lbl);

    return scr;
}

static lv_obj_t* create_task_row(lv_obj_t* parent, const TaskInfo& task) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 55);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_all(row, 8, 0);
    
    // Priority Dot
    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, -10);
    
    lv_color_t p_color = lv_color_hex(0x3B82F6); // Low
    if (task.priority == 1) p_color = lv_color_hex(0xEAB308); // Med
    if (task.priority == 2) p_color = lv_color_hex(0xEF4444); // High
    lv_obj_set_style_bg_color(dot, p_color, 0);
    lv_obj_set_style_border_width(dot, 0, 0);

    // Names
    lv_obj_t* name = lv_label_create(row);
    lv_label_set_text(name, task.name.c_str());
    lv_obj_set_style_text_color(name, lv_color_white(), 0);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 20, 0);

    lv_obj_t* sub = lv_label_create(row);
    lv_label_set_text(sub, (String("Sub: ") + task.subtask).c_str());
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x64748B), 0);
    lv_obj_align(sub, LV_ALIGN_TOP_LEFT, 20, 18);

    // Progress Bar
    lv_obj_t* bar = lv_bar_create(row);
    lv_obj_set_size(bar, 120, 8);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 20, 0);
    lv_bar_set_value(bar, task.progress, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x3B82F6), LV_PART_INDICATOR);

    // Focus Button
    lv_obj_t* focus_btn = lv_btn_create(row);
    lv_obj_set_size(focus_btn, 70, 30);
    lv_obj_align(focus_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(focus_btn, lv_color_hex(0x10B981), 0);
    
    lv_obj_t* focus_lbl = lv_label_create(focus_btn);
    lv_label_set_text(focus_lbl, "Focus");
    lv_obj_center(focus_lbl);

    return row;
}
