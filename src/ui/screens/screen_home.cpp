// screen_home.cpp — Part 4: HOME (Dashboard) screen
// Layout: 480x320 landscape
//  Row 0 (y=0,  h=40):  Header bar — title, WiFi status, refresh button
//  Row 1 (y=40, h=85):  Stat cards — XP / Spoons / Streak / Level
//  Row 2 (y=125, h=16): Section label "Tasks in Progress"
//  Row 3 (y=141, h=139): Scrollable task list
//  Row 4 (y=280, h=40): Footer — "New Task" button
// Total height used: 320px (exact fit)

#define LV_CONF_SKIP
#include "screen_home.h"
#include "../../state/app_state.h"
#include "../widgets/stat_card.h"
#include "../ui_manager.h"
#include "../../state/session_machine.h"
#include "../../api/api_client.h"

// --------------------------------------------------------------------------
// Debounce guard (150ms per spec §10.9)
// --------------------------------------------------------------------------
static uint32_t _last_touch_ms = 0;
static bool debounce() {
    uint32_t now = millis();
    if (now - _last_touch_ms < 150) return false;
    _last_touch_ms = now;
    return true;
}

// --------------------------------------------------------------------------
// Forward declarations
// --------------------------------------------------------------------------
static lv_obj_t* create_task_row(lv_obj_t* parent, TaskInfo& task);
static void update_home_data(lv_timer_t* timer);

// --------------------------------------------------------------------------
// Screen-level state
// --------------------------------------------------------------------------
static lv_obj_t*  s_spinner      = NULL;   // fetch spinner (hidden by default)
static lv_obj_t*  s_offline_pill = NULL;   // "Offline" badge on header
static lv_obj_t*  s_task_list    = NULL;   // scrollable task container
static lv_timer_t* s_refresh_timer = NULL; // 60-second periodic refresh

// --------------------------------------------------------------------------
// Event callbacks
// --------------------------------------------------------------------------
static void refresh_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;

    // Show spinner while fetching
    if (s_spinner) lv_obj_clear_flag(s_spinner, LV_OBJ_FLAG_HIDDEN);

    APIClient::getInstance().fetchDashboard();

    // Dashboard fetch is async; the api_client must call
    // UIManager::getInstance().moveTo(Screen::HOME) on completion
    // to rebuild the screen with fresh data. The spinner hides on rebuild.
}

static void wifi_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    UIManager::getInstance().moveTo(Screen::WIFI);
}

static void focus_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    int task_id = (int)(intptr_t)lv_event_get_user_data(e);

    AppState& state = AppState::getInstance();
    TaskInfo* task = nullptr;
    for (auto& t : state.tasks) {
        if (t.id == task_id) {
            task = &t;
            break;
        }
    }
    if (!task) return;

    state.currentTaskName    = task->name;
    state.currentSubtaskName = task->subtask;

    // Use calibrated duration from backend
    SessionMachine::getInstance().start(task->suggestedDuration); 
    UIManager::getInstance().moveTo(Screen::FOCUS);
}

// Periodic 60-second refresh timer callback
static void update_home_data(lv_timer_t* timer) {
    (void)timer;
    if (s_spinner) lv_obj_clear_flag(s_spinner, LV_OBJ_FLAG_HIDDEN);
    APIClient::getInstance().fetchDashboard();
}

// --------------------------------------------------------------------------
// Task row builder
// --------------------------------------------------------------------------
static lv_obj_t* create_task_row(lv_obj_t* parent, TaskInfo& task) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(row, 58, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Priority dot
    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, -10);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_color_t p_color;
    switch (task.priority) {
        case 2:  p_color = lv_color_hex(0xEF4444); break; // High — red
        case 1:  p_color = lv_color_hex(0xEAB308); break; // Med  — yellow
        default: p_color = lv_color_hex(0x3B82F6); break; // Low  — blue
    }
    lv_obj_set_style_bg_color(dot, p_color, 0);

    // Task name (truncated at 22 chars to fit left of the Focus button)
    char name_buf[24];
    if (task.name.length() > 22) {
        strncpy(name_buf, task.name.c_str(), 19);
        name_buf[19] = '.'; name_buf[20] = '.'; name_buf[21] = '.';
        name_buf[22] = '\0';
    } else {
        strncpy(name_buf, task.name.c_str(), sizeof(name_buf));
        name_buf[sizeof(name_buf) - 1] = '\0';
    }
    lv_obj_t* name_lbl = lv_label_create(row);
    lv_label_set_text(name_lbl, name_buf);
    lv_obj_set_style_text_color(name_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 18, 2);

    // Active subtask name
    lv_obj_t* sub_lbl = lv_label_create(row);
    lv_label_set_text(sub_lbl, task.subtask.c_str());
    lv_obj_set_style_text_font(sub_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_align(sub_lbl, LV_ALIGN_TOP_LEFT, 18, 20);

    // Progress bar — clamp 0-100
    int prog = task.progress;
    if (prog < 0) prog = 0;
    if (prog > 100) prog = 100;
    lv_obj_t* bar = lv_bar_create(row);
    lv_obj_set_size(bar, 130, 6);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 18, -2);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, prog, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x3B82F6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);

    // Focus button
    lv_obj_t* focus_btn = lv_btn_create(row);
    lv_obj_set_size(focus_btn, 70, 34);
    lv_obj_align(focus_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(focus_btn, lv_color_hex(0x10B981), 0);
    lv_obj_set_style_bg_color(focus_btn, lv_color_hex(0x059669), LV_STATE_PRESSED);
    lv_obj_set_style_radius(focus_btn, 6, 0);
    lv_obj_add_event_cb(focus_btn, focus_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)task.id);

    lv_obj_t* focus_lbl = lv_label_create(focus_btn);
    lv_label_set_text(focus_lbl, LV_SYMBOL_PLAY " Focus");
    lv_obj_set_style_text_color(focus_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(focus_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(focus_lbl);

    return row;
}

// --------------------------------------------------------------------------
// Main screen builder
// --------------------------------------------------------------------------
lv_obj_t* create_screen_home() {
    // Stop any stale periodic timer from a previous HOME instance
    if (s_refresh_timer) {
        lv_timer_del(s_refresh_timer);
        s_refresh_timer = NULL;
    }

    AppState& state = AppState::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // -----------------------------------------------------------------------
    // HEADER (y=0, h=40)
    // -----------------------------------------------------------------------
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // App logo / title
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, LV_SYMBOL_BULLET " FocusOS");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    // WiFi status badge — shows "Offline" pill when disconnected
    s_offline_pill = lv_label_create(header);
    if (!state.isWifiConnected) {
        lv_label_set_text(s_offline_pill, " Offline ");
        lv_obj_set_style_text_color(s_offline_pill, lv_color_hex(0xFEF3C7), 0);
        lv_obj_set_style_bg_color(s_offline_pill, lv_color_hex(0xD97706), 0);
        lv_obj_set_style_bg_opa(s_offline_pill, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_offline_pill, 6, 0);
        lv_obj_set_style_pad_ver(s_offline_pill, 2, 0);
        lv_obj_set_style_pad_hor(s_offline_pill, 4, 0);
    } else {
        lv_label_set_text(s_offline_pill, ""); // Hide when online
    }
    lv_obj_align(s_offline_pill, LV_ALIGN_LEFT_MID, 115, 0);

    // WiFi icon button → WiFi Settings screen
    lv_obj_t* wifi_btn = lv_btn_create(header);
    lv_obj_set_size(wifi_btn, 80, 30);
    lv_obj_align(wifi_btn, LV_ALIGN_RIGHT_MID, -50, 0);
    lv_obj_set_style_bg_opa(wifi_btn, 0, 0);
    lv_obj_set_style_border_width(wifi_btn, 0, 0);
    lv_obj_add_event_cb(wifi_btn, wifi_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* wifi_lbl = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_lbl, state.isWifiConnected
                                ? (LV_SYMBOL_WIFI " " + state.currentSSID).c_str()
                                : LV_SYMBOL_WARNING " No WiFi");
    lv_obj_set_style_text_color(wifi_lbl,
        state.isWifiConnected ? lv_color_hex(0x10B981) : lv_color_hex(0xF59E0B), 0);
    lv_obj_set_style_text_font(wifi_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(wifi_lbl);

    // Refresh button
    lv_obj_t* refresh_btn = lv_btn_create(header);
    lv_obj_set_size(refresh_btn, 36, 30);
    lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_opa(refresh_btn, 0, 0);
    lv_obj_set_style_border_width(refresh_btn, 0, 0);
    lv_obj_add_event_cb(refresh_btn, refresh_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(refresh_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_center(refresh_lbl);

    // Fetch spinner (shown during API call, hidden otherwise)
    s_spinner = lv_spinner_create(scr);
    lv_obj_set_size(s_spinner, 24, 24);
    lv_obj_align(s_spinner, LV_ALIGN_TOP_RIGHT, -44, 8);
    lv_obj_set_style_arc_color(s_spinner, lv_color_hex(0x3B82F6), LV_PART_INDICATOR);
    lv_obj_add_flag(s_spinner, LV_OBJ_FLAG_HIDDEN); // hidden until fetch starts

    // -----------------------------------------------------------------------
    // STAT ROW (y=40, h=85) — XP | Spoons | Streak | Level
    // -----------------------------------------------------------------------
    lv_obj_t* stat_row = lv_obj_create(scr);
    lv_obj_set_size(stat_row, 480, 85);
    lv_obj_align(stat_row, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_opa(stat_row, 0, 0);
    lv_obj_set_style_border_width(stat_row, 0, 0);
    lv_obj_set_style_pad_hor(stat_row, 8, 0);
    lv_obj_set_style_pad_ver(stat_row, 5, 0);
    lv_obj_set_flex_flow(stat_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stat_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(stat_row, LV_OBJ_FLAG_SCROLLABLE);

    // XP card
    char xp_val[16];
    snprintf(xp_val, sizeof(xp_val), "%d xp", state.xp);
    create_stat_card(stat_row, "XP", xp_val, "total", lv_color_hex(0xA855F7));

    // Spoon card — with mini segmented bar
    create_spoon_card(stat_row, state.spoonsUsed, state.spoonsTotal, lv_color_hex(0x06B6D4));

    // Streak card
    char streak_val[16];
    snprintf(streak_val, sizeof(streak_val), "%d days", state.streakDays);
    create_stat_card(stat_row, "Streak", streak_val, "on fire", lv_color_hex(0xF97316));

    // Level card
    char lvl_val[8];
    snprintf(lvl_val, sizeof(lvl_val), "Lv %d", state.level);
    create_stat_card(stat_row, "Level", lvl_val, "ascension", lv_color_hex(0xEAB308));

    // -----------------------------------------------------------------------
    // SECTION LABEL (y=125, h=16)
    // -----------------------------------------------------------------------
    lv_obj_t* section_lbl = lv_label_create(scr);
    lv_label_set_text(section_lbl, "Tasks in Progress");
    lv_obj_set_style_text_color(section_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(section_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(section_lbl, LV_ALIGN_TOP_LEFT, 12, 127);

    // -----------------------------------------------------------------------
    // TASK LIST (y=145, h=133) — scrollable, max 10 tasks
    // -----------------------------------------------------------------------
    s_task_list = lv_obj_create(scr);
    lv_obj_set_size(s_task_list, 460, 133);
    lv_obj_align(s_task_list, LV_ALIGN_TOP_MID, 0, 145);
    lv_obj_set_style_bg_color(s_task_list, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_width(s_task_list, 0, 0);
    lv_obj_set_style_pad_all(s_task_list, 4, 0);
    lv_obj_set_style_pad_gap(s_task_list, 6, 0);
    lv_obj_set_flex_flow(s_task_list, LV_FLEX_FLOW_COLUMN);
    // Scrollbar — thin, subtle
    lv_obj_set_scrollbar_mode(s_task_list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_width(s_task_list, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(s_task_list, lv_color_hex(0x334155), LV_PART_SCROLLBAR);

    int render_count = 0;
    for (size_t i = 0; i < state.tasks.size() && render_count < 10; ++i, ++render_count) {
        create_task_row(s_task_list, state.tasks[i]);
    }

    if (state.tasks.empty()) {
        lv_obj_t* no_task = lv_label_create(s_task_list);
        lv_label_set_text(no_task, "No active tasks. Tap below to add one.");
        lv_obj_set_style_text_color(no_task, lv_color_hex(0x475569), 0);
        lv_obj_set_style_text_font(no_task, &lv_font_montserrat_14, 0);
        lv_obj_center(no_task);
    }

    // -----------------------------------------------------------------------
    // FOOTER (y=280, h=40)
    // -----------------------------------------------------------------------
    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, 480, 40);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_pad_all(footer, 4, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* add_btn = lv_btn_create(footer);
    lv_obj_set_size(add_btn, 130, 30);
    lv_obj_center(add_btn);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x2563EB), LV_STATE_PRESSED);
    lv_obj_set_style_radius(add_btn, 6, 0);

    lv_obj_t* add_lbl = lv_label_create(add_btn);
    lv_label_set_text(add_lbl, LV_SYMBOL_PLUS " New Task");
    lv_obj_set_style_text_color(add_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(add_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(add_lbl);

    // -----------------------------------------------------------------------
    // Periodic 60-second data refresh timer
    // -----------------------------------------------------------------------
    s_refresh_timer = lv_timer_create(update_home_data, 60000, NULL);

    return scr;
}
