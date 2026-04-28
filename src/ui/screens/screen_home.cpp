#define LV_CONF_SKIP
#include "screen_home.h"
#include "../../state/app_state.h"
#include "../ui_manager.h"
#include "../../api/api_client.h"

static lv_obj_t* streak_val = NULL;
static lv_obj_t* xp_val = NULL;
static lv_obj_t* tasks_val = NULL;
static lv_obj_t* spoons_val = NULL;
static lv_obj_t* wifi_icon = NULL;
static lv_obj_t* active_screen_obj = NULL;

static uint32_t _last_touch_ms = 0;
static bool debounce() {
    uint32_t now = millis();
    if (now - _last_touch_ms < 150) return false;
    _last_touch_ms = now;
    return true;
}

static void wifi_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    UIManager::getInstance().moveTo(Screen::WIFI);
}

static void start_task_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    UIManager::getInstance().moveTo(Screen::TASK_LIST);
}

static void update_home_data(lv_timer_t* timer) {
    APIClient::getInstance().fetchDashboard();
}

void refresh_screen_home() {
    if (!active_screen_obj || !lv_obj_is_valid(active_screen_obj)) return;
    
    AppState& state = AppState::getInstance();
    
    if (streak_val) lv_label_set_text(streak_val, (String(state.streakDays) + " days").c_str());
    if (xp_val) lv_label_set_text(xp_val, String(state.xp).c_str());
    if (tasks_val) lv_label_set_text(tasks_val, String(state.tasks.size()).c_str());
    if (spoons_val) lv_label_set_text(spoons_val, String(state.spoonsTotal - state.spoonsUsed).c_str());
    
    if (wifi_icon) {
        lv_label_set_text(wifi_icon, state.isWifiConnected ? LV_SYMBOL_WIFI : LV_SYMBOL_WARNING);
        lv_obj_set_style_text_color(wifi_icon, state.isWifiConnected ? lv_color_hex(0x10B981) : lv_color_hex(0xEF4444), 0);
    }
}

lv_obj_t* create_screen_home() {
    AppState& state = AppState::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    active_screen_obj = scr;
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF8FAFC), 0); // Light Slate 50
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // HEADER
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "FocusOS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    lv_obj_t* wifi_btn = lv_btn_create(header);
    lv_obj_set_size(wifi_btn, 100, 35);
    lv_obj_align(wifi_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_width(wifi_btn, 1, 0);
    lv_obj_set_style_border_color(wifi_btn, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_radius(wifi_btn, 8, 0);
    lv_obj_set_style_shadow_width(wifi_btn, 0, 0);
    lv_obj_add_event_cb(wifi_btn, wifi_btn_event_cb, LV_EVENT_CLICKED, NULL);

    wifi_icon = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_icon, state.isWifiConnected ? LV_SYMBOL_WIFI : LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(wifi_icon, state.isWifiConnected ? lv_color_hex(0x10B981) : lv_color_hex(0xEF4444), 0);
    lv_obj_center(wifi_icon);

    // STATS CONTAINER
    lv_obj_t* stats_cont = lv_obj_create(scr);
    lv_obj_set_size(stats_cont, 400, 180);
    lv_obj_align(stats_cont, LV_ALIGN_TOP_MID, 0, 65);
    lv_obj_set_style_bg_opa(stats_cont, 0, 0);
    lv_obj_set_style_border_width(stats_cont, 0, 0);
    lv_obj_set_flex_flow(stats_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(stats_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(stats_cont, 12, 0);

    auto create_stat = [&](const char* label, String value, lv_color_t color, lv_obj_t** out_val) {
        lv_obj_t* row = lv_obj_create(stats_cont);
        lv_obj_set_size(row, 300, 30);
        lv_obj_set_style_bg_opa(row, 0, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(row, 0, 0);

        lv_obj_t* l = lv_label_create(row);
        lv_label_set_text(l, label);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0x64748B), 0);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);

        *out_val = lv_label_create(row);
        lv_label_set_text(*out_val, value.c_str());
        lv_obj_set_style_text_font(*out_val, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(*out_val, color, 0);
        lv_obj_align(*out_val, LV_ALIGN_RIGHT_MID, 0, 0);
    };

    create_stat("Streak", String(state.streakDays) + " days", lv_color_hex(0xF59E0B), &streak_val);
    create_stat("XP", String(state.xp), lv_color_hex(0x6366F1), &xp_val);
    create_stat("Tasks Left", String(state.tasks.size()), lv_color_hex(0x10B981), &tasks_val);
    create_stat("Spoons Left", String(state.spoonsTotal - state.spoonsUsed), lv_color_hex(0x06B6D4), &spoons_val);

    // START TASK BUTTON
    lv_obj_t* start_btn = lv_btn_create(scr);
    lv_obj_set_size(start_btn, 220, 50);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_radius(start_btn, 12, 0);
    lv_obj_add_event_cb(start_btn, start_task_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* start_lbl = lv_label_create(start_btn);
    lv_label_set_text(start_lbl, "Start Task");
    lv_obj_set_style_text_color(start_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(start_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(start_lbl);

    lv_timer_create(update_home_data, 60000, NULL);

    return scr;
}
