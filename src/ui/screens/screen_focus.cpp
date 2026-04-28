#define LV_CONF_SKIP
#include "screen_focus.h"
#include "../../state/app_state.h"
#include "../../state/session_machine.h"
#include "../ui_manager.h"
#include "../../api/api_client.h"

static lv_obj_t* s_timer_lbl = NULL;
static lv_obj_t* s_state_lbl = NULL;
static lv_obj_t* s_pause_lbl = NULL;
static lv_timer_t* s_tick_timer = NULL;

static uint32_t _last_touch = 0;
static bool debounce() {
    uint32_t now = millis();
    if (now - _last_touch < 150) return false;
    _last_touch = now;
    return true;
}

static void toggle_cb(lv_event_t* e) {
    if (!debounce()) return;
    SessionMachine& sm = SessionMachine::getInstance();
    SessionState st = sm.getState();
    if (st == SessionState::FOCUS || st == SessionState::HYPERFOCUS) {
        sm.pause();
    } else if (st == SessionState::DISENGAGED || st == SessionState::BREAK) {
        sm.resume();
    }
}

static void end_task_cb(lv_event_t* e) {
    if (!debounce()) return;
    AppState& state = AppState::getInstance();
    
    // Mark subtask as complete in backend
    if (state.selectedSubtaskId != "") {
        APIClient::getInstance().completeSubtask(state.selectedSubtaskId);
    }
    
    // Stop session machine
    SessionMachine::getInstance().stop();
    
    // Clean up timer
    if (s_tick_timer) {
        lv_timer_del(s_tick_timer);
        s_tick_timer = NULL;
    }
    
    // Go back to subtask list
    UIManager::getInstance().moveTo(Screen::SUBTASK_LIST);
}

static void on_tick(lv_timer_t* timer) {
    SessionMachine& sm = SessionMachine::getInstance();
    SessionState st = sm.getState();

    // Update timer string
    if (s_timer_lbl) {
        lv_label_set_text(s_timer_lbl, sm.getTimeLabelString().c_str());
    }

    // Update state label and button text
    if (s_state_lbl) {
        const char* state_txt = "FOCUS";
        lv_color_t color = lv_color_hex(0x10B981); // Green
        
        switch (st) {
            case SessionState::HYPERFOCUS: 
                state_txt = "OVERTIME"; 
                color = lv_color_hex(0x6366F1); // Indigo
                break;
            case SessionState::BREAK:
                state_txt = "BREAK";
                color = lv_color_hex(0xF59E0B); // Amber
                break;
            case SessionState::DISENGAGED:
                state_txt = "PAUSED";
                color = lv_color_hex(0xEF4444); // Red
                break;
            default: break;
        }
        lv_label_set_text(s_state_lbl, state_txt);
        lv_obj_set_style_text_color(s_state_lbl, color, 0);
    }

    if (s_pause_lbl) {
        bool active = (st == SessionState::FOCUS || st == SessionState::HYPERFOCUS);
        lv_label_set_text(s_pause_lbl, active ? "Pause" : "Resume");
    }
}

lv_obj_t* create_screen_focus() {
    AppState& app = AppState::getInstance();
    SessionMachine& sm = SessionMachine::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF8FAFC), 0);
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
    lv_label_set_text(title, "Focus Session");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // BODY CONTAINER
    lv_obj_t* body = lv_obj_create(scr);
    lv_obj_set_size(body, 400, 180);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(body, 0, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(body, 15, 0);

    // Subtask Name
    lv_obj_t* sub_lbl = lv_label_create(body);
    lv_label_set_text(sub_lbl, app.selectedSubtaskTitle.c_str());
    lv_obj_set_style_text_font(sub_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x0F172A), 0);
    lv_label_set_long_mode(sub_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(sub_lbl, 350);
    lv_obj_set_style_text_align(sub_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Timer Label
    s_timer_lbl = lv_label_create(body);
    lv_label_set_text(s_timer_lbl, "00:00");
    lv_obj_set_style_text_font(s_timer_lbl, &lv_font_montserrat_14, 0); // Need to check if 48 is available, else 32
    lv_obj_set_style_text_color(s_timer_lbl, lv_color_hex(0x0F172A), 0);

    // State Label
    s_state_lbl = lv_label_create(body);
    lv_label_set_text(s_state_lbl, "FOCUS");
    lv_obj_set_style_text_font(s_state_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_state_lbl, lv_color_hex(0x10B981), 0);

    // CONTROLS CONTAINER
    lv_obj_t* controls = lv_obj_create(scr);
    lv_obj_set_size(controls, 480, 80);
    lv_obj_align(controls, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(controls, 0, 0);
    lv_obj_set_style_border_width(controls, 0, 0);
    lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE);

    // Pause Button
    lv_obj_t* pause_btn = lv_btn_create(controls);
    lv_obj_set_size(pause_btn, 120, 50);
    lv_obj_align(pause_btn, LV_ALIGN_CENTER, -70, 0);
    lv_obj_set_style_bg_color(pause_btn, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_width(pause_btn, 1, 0);
    lv_obj_set_style_border_color(pause_btn, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_radius(pause_btn, 12, 0);
    lv_obj_set_style_shadow_width(pause_btn, 0, 0);
    lv_obj_add_event_cb(pause_btn, toggle_cb, LV_EVENT_CLICKED, NULL);

    s_pause_lbl = lv_label_create(pause_btn);
    lv_label_set_text(s_pause_lbl, "Pause");
    lv_obj_set_style_text_color(s_pause_lbl, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(s_pause_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(s_pause_lbl);

    // End Task Button
    lv_obj_t* end_btn = lv_btn_create(controls);
    lv_obj_set_size(end_btn, 120, 50);
    lv_obj_align(end_btn, LV_ALIGN_CENTER, 70, 0);
    lv_obj_set_style_bg_color(end_btn, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_radius(end_btn, 12, 0);
    lv_obj_add_event_cb(end_btn, end_task_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* end_lbl = lv_label_create(end_btn);
    lv_label_set_text(end_lbl, "End Task");
    lv_obj_set_style_text_color(end_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(end_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(end_lbl);

    // Tick Timer (1s)
    s_tick_timer = lv_timer_create(on_tick, 1000, NULL);
    on_tick(s_tick_timer); // Initial call

    return scr;
}
