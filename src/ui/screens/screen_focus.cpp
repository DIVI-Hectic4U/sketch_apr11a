#define LV_CONF_SKIP
#include "screen_focus.h"
#include "../../state/app_state.h"
#include "../../state/session_machine.h"
#include "../widgets/ring_timer.h"
#include "../ui_manager.h"

static lv_obj_t* timer_arc;

static void back_event_cb(lv_event_t * e) {
    UIManager::getInstance().moveTo(Screen::HOME);
}

static void toggle_timer_cb(lv_event_t * e) {
    SessionMachine& sm = SessionMachine::getInstance();
    if (sm.getState() == SessionState::RUNNING) {
        sm.pause();
        lv_label_set_text(lv_obj_get_child((lv_obj_t*)lv_event_get_target(e), 0), "Resume");
    } else {
        sm.resume();
        lv_label_set_text(lv_obj_get_child((lv_obj_t*)lv_event_get_target(e), 0), "Pause");
    }
}

static void stop_timer_cb(lv_event_t * e) {
    SessionMachine::getInstance().stop();
    UIManager::getInstance().moveTo(Screen::HOME);
}

lv_obj_t* create_screen_focus() {
    AppState& state = AppState::getInstance();
    SessionMachine& sm = SessionMachine::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x020617), 0); // Slate 950
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 50);
    lv_obj_set_style_bg_opa(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 80, 35);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x1E293B), 0);
    lv_obj_add_event_cb(back_btn, back_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "< Back");
    lv_obj_center(back_lbl);

    lv_obj_t* task_lbl = lv_label_create(header);
    lv_label_set_text(task_lbl, state.currentTaskName.c_str());
    lv_obj_set_style_text_color(task_lbl, lv_color_white(), 0);
    lv_obj_align(task_lbl, LV_ALIGN_CENTER, 0, 0);

    // Timer
    timer_arc = create_ring_timer(scr);
    lv_obj_align(timer_arc, LV_ALIGN_CENTER, 0, -10);

    // Subtask label below timer
    lv_obj_t* sub_lbl = lv_label_create(scr);
    lv_label_set_text(sub_lbl, state.currentSubtaskName.c_str());
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_align(sub_lbl, LV_ALIGN_CENTER, 0, 45);

    // Control Buttons
    lv_obj_t* controls = lv_obj_create(scr);
    lv_obj_set_size(controls, 480, 60);
    lv_obj_set_style_bg_opa(controls, 0, 0);
    lv_obj_set_style_border_width(controls, 0, 0);
    lv_obj_align(controls, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(controls, 20, 0);

    lv_obj_t* pause_btn = lv_btn_create(controls);
    lv_obj_set_size(pause_btn, 100, 40);
    lv_obj_set_style_bg_color(pause_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(pause_btn, toggle_timer_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* pause_lbl = lv_label_create(pause_btn);
    lv_label_set_text(pause_lbl, "Pause");
    lv_obj_center(pause_lbl);

    lv_obj_t* stop_btn = lv_btn_create(controls);
    lv_obj_set_size(stop_btn, 100, 40);
    lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xEF4444), 0);
    lv_obj_add_event_cb(stop_btn, stop_timer_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* stop_lbl = lv_label_create(stop_btn);
    lv_label_set_text(stop_lbl, "Stop");
    lv_obj_center(stop_lbl);

    // Initial timer update logic
    lv_timer_create([](lv_timer_t* t){
        SessionMachine& sm = SessionMachine::getInstance();
        if (lv_obj_is_valid(timer_arc)) {
            int progress = 100 - (sm.getRemainingSeconds() * 100 / (sm.getTotalSeconds() > 0 ? sm.getTotalSeconds() : 1));
            update_ring_timer(timer_arc, progress, sm.getTimeString().c_str());
        }
    }, 100, NULL);

    return scr;
}
