#define LV_CONF_SKIP
#include "screen_tasklist.h"
#include "../../state/app_state.h"
#include "../ui_manager.h"
#include <algorithm>

static lv_obj_t* list_cont = NULL;
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
    UIManager::getInstance().moveTo(Screen::HOME);
}

static void focus_btn_event_cb(lv_event_t* e) {
    if (!debounce()) return;
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    AppState& state = AppState::getInstance();
    
    // Safety check for vector index
    if (index >= 0 && index < (int)state.tasks.size()) {
        state.selectedTaskId = state.tasks[index].id;
        state.currentTaskName = state.tasks[index].name;
        UIManager::getInstance().moveTo(Screen::SUBTASK_LIST);
    }
}

void refresh_screen_tasklist() {
    if (!active_screen_obj || !lv_obj_is_valid(active_screen_obj) || !list_cont) return;

    AppState& state = AppState::getInstance();
    lv_obj_clean(list_cont);

    // Create a copy of tasks to sort by priority
    struct IndexedTask {
        int originalIndex;
        TaskInfo* task;
    };
    std::vector<IndexedTask> sortedTasks;
    for (size_t i = 0; i < state.tasks.size(); ++i) {
        sortedTasks.push_back({(int)i, &state.tasks[i]});
    }

    // Sort: High(2) > Med(1) > Low(0)
    std::sort(sortedTasks.begin(), sortedTasks.end(), [](const IndexedTask& a, const IndexedTask& b) {
        return a.task->priority > b.task->priority;
    });

    for (auto& it : sortedTasks) {
        lv_obj_t* row = lv_obj_create(list_cont);
        lv_obj_set_size(row, LV_PCT(100), 60);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0xE2E8F0), 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(row, 5, 0);

        // Priority dot
        lv_obj_t* dot = lv_obj_create(row);
        lv_obj_set_size(dot, 12, 12);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_align(dot, LV_ALIGN_LEFT_MID, 5, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        
        lv_color_t p_color;
        switch (it.task->priority) {
            case 2:  p_color = lv_color_hex(0xEF4444); break; // High
            case 1:  p_color = lv_color_hex(0xEAB308); break; // Med
            default: p_color = lv_color_hex(0x3B82F6); break; // Low
        }
        lv_obj_set_style_bg_color(dot, p_color, 0);

        // Task name
        lv_obj_t* name = lv_label_create(row);
        lv_label_set_text(name, it.task->name.c_str());
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(0x0F172A), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 25, 0);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, 280);

        // Focus button
        lv_obj_t* focus_btn = lv_btn_create(row);
        lv_obj_set_size(focus_btn, 80, 40);
        lv_obj_align(focus_btn, LV_ALIGN_RIGHT_MID, -5, 0);
        lv_obj_set_style_bg_color(focus_btn, lv_color_hex(0x0F172A), 0);
        lv_obj_set_style_radius(focus_btn, 8, 0);
        lv_obj_add_event_cb(focus_btn, focus_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)it.originalIndex);

        lv_obj_t* focus_lbl = lv_label_create(focus_btn);
        lv_label_set_text(focus_lbl, "Focus");
        lv_obj_set_style_text_color(focus_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(focus_lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(focus_lbl);
    }

    if (state.tasks.empty()) {
        lv_obj_t* empty = lv_label_create(list_cont);
        lv_label_set_text(empty, "No tasks available");
        lv_obj_set_style_text_color(empty, lv_color_hex(0x64748B), 0);
        lv_obj_center(empty);
    }
}

lv_obj_t* create_screen_tasklist() {
    AppState& state = AppState::getInstance();

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

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Select Task");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // TASK LIST CONTAINER
    list_cont = lv_obj_create(scr);
    lv_obj_set_size(list_cont, 460, 260);
    lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_opa(list_cont, 0, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(list_cont, 10, 0);
    lv_obj_set_style_pad_all(list_cont, 10, 0);

    refresh_screen_tasklist();

    return scr;
}
