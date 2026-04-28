#define LV_CONF_SKIP
#include "screen_subtasklist.h"
#include "../../state/app_state.h"
#include "../ui_manager.h"
#include "../../state/session_machine.h"

static lv_obj_t* list_cont = NULL;
static lv_obj_t* title_lbl = NULL;
static lv_obj_t* active_screen_obj = NULL;

static uint32_t _last_touch_ms = 0;
static bool debounce() {
    uint32_t now = millis();
    if (now - _last_touch_ms < 150) return false;
    _last_touch_ms = now;
    return true;
}

static void back_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    UIManager::getInstance().moveTo(Screen::TASK_LIST);
}

static void start_subtask_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    int sub_index = (int)(intptr_t)lv_event_get_user_data(e);
    AppState& state = AppState::getInstance();
    
    // Find selected task
    TaskInfo* task = nullptr;
    for (auto& t : state.tasks) {
        if (t.id == state.selectedTaskId) {
            task = &t;
            break;
        }
    }
    if (!task) return;

    if (sub_index < 0 || sub_index >= (int)task->subtasks.size()) return;
    
    SubtaskInfo& sub = task->subtasks[sub_index];
    state.selectedSubtaskId = sub.id;
    state.selectedSubtaskTitle = sub.title;
    state.currentSubtaskName = sub.title;
    
    SessionMachine::getInstance().start(task->suggestedDuration);
    UIManager::getInstance().moveTo(Screen::FOCUS);
}

void refresh_screen_subtasklist() {
    if (!active_screen_obj || !lv_obj_is_valid(active_screen_obj) || !list_cont) return;

    AppState& state = AppState::getInstance();
    
    // Find selected task
    TaskInfo* task = nullptr;
    for (auto& t : state.tasks) {
        if (t.id == state.selectedTaskId) {
            task = &t;
            break;
        }
    }
    if (!task) {
        lv_obj_clean(list_cont);
        lv_obj_t* empty = lv_label_create(list_cont);
        lv_label_set_text(empty, "Task no longer available");
        lv_obj_center(empty);
        return;
    }
    
    if (title_lbl) lv_label_set_text(title_lbl, task->name.c_str());

    lv_obj_clean(list_cont);

    int incomplete_count = 0;
    for (size_t i = 0; i < task->subtasks.size(); ++i) {
        if (task->subtasks[i].completed) continue;
        incomplete_count++;

        lv_obj_t* row = lv_obj_create(list_cont);
        lv_obj_set_size(row, LV_PCT(100), 60);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0xE2E8F0), 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(row, 5, 0);

        lv_obj_t* name = lv_label_create(row);
        lv_label_set_text(name, task->subtasks[i].title.c_str());
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(0x0F172A), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 10, 0);
        lv_label_set_long_mode(name, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(name, 320);

        lv_obj_t* start_btn = lv_btn_create(row);
        lv_obj_set_size(start_btn, 80, 40);
        lv_obj_align(start_btn, LV_ALIGN_RIGHT_MID, -5, 0);
        lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x10B981), 0);
        lv_obj_set_style_radius(start_btn, 8, 0);
        lv_obj_add_event_cb(start_btn, start_subtask_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* start_lbl = lv_label_create(start_btn);
        lv_label_set_text(start_lbl, "Start");
        lv_obj_set_style_text_color(start_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(start_lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(start_lbl);
    }

    if (incomplete_count == 0) {
        lv_obj_t* empty = lv_label_create(list_cont);
        lv_label_set_text(empty, "All subtasks completed!");
        lv_obj_set_style_text_color(empty, lv_color_hex(0x64748B), 0);
        lv_obj_center(empty);
    }
}

lv_obj_t* create_screen_subtasklist() {
    AppState& state = AppState::getInstance();
    
    // Find selected task
    TaskInfo* task = nullptr;
    for (auto& t : state.tasks) {
        if (t.id == state.selectedTaskId) {
            task = &t;
            break;
        }
    }
    if (!task) return lv_obj_create(NULL); // Should not happen

    lv_obj_t* scr = lv_obj_create(NULL);
    active_screen_obj = scr;
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

    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 35);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0x0F172A), 0);
    lv_obj_center(back_lbl);

    title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, task->name.c_str());
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x0F172A), 0);
    lv_obj_align(title_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title_lbl, 320);

    // SUBTASK LIST CONTAINER
    list_cont = lv_obj_create(scr);
    lv_obj_set_size(list_cont, 460, 260);
    lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_opa(list_cont, 0, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(list_cont, 10, 0);
    lv_obj_set_style_pad_all(list_cont, 10, 0);

    refresh_screen_subtasklist();

    return scr;
}
