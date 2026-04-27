#define LV_CONF_SKIP
#include "screen_focus.h"
#include "../../state/app_state.h"
#include "../../state/session_machine.h"
#include "../widgets/ring_timer.h"
#include "../widgets/ambient_bg.h"
#include "../ui_manager.h"
#include "../../api/api_client.h"

// ---- Module-level handles ------------------------------------------------
static RingTimerHandle* s_ring       = NULL;
static lv_obj_t*        s_state_pill = NULL;
static lv_obj_t*        s_toast      = NULL;
static lv_obj_t*        s_pause_lbl  = NULL;
static lv_obj_t*        s_actual_lbl = NULL;
static lv_obj_t*        s_spoon_bar  = NULL;
static lv_timer_t*      s_tick_timer = NULL;
static SessionState     s_lastState  = SessionState::IDLE;

// ---- Debounce -----------------------------------------------------------
static uint32_t _last_touch = 0;
static bool debounce() {
    uint32_t now = millis();
    if (now - _last_touch < 150) return false;
    _last_touch = now;
    return true;
}

// ---- Toast (§5.5) -------------------------------------------------------
static void toast_continue_cb(lv_event_t* e) {
    if (!debounce()) return;
    SessionMachine::getInstance().dismissToast();
    if (s_toast && lv_obj_is_valid(s_toast))
        lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
}

static void toast_break_cb(lv_event_t* e) {
    if (!debounce()) return;
    SessionMachine::getInstance().takeBreak();
    if (s_toast && lv_obj_is_valid(s_toast))
        lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t* build_toast(lv_obj_t* scr) {
    lv_obj_t* t = lv_obj_create(scr);
    lv_obj_set_size(t, 460, 60);
    lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(t, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_color(t, lv_color_hex(0x1A7AD4), 0);
    lv_obj_set_style_border_width(t, 1, 0);
    lv_obj_set_style_radius(t, 8, 0);
    lv_obj_set_style_pad_all(t, 8, 0);
    lv_obj_add_flag(t, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(t, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(t, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* msg = lv_label_create(t);
    lv_label_set_text(msg, "Planned time's up! Keep going?");
    lv_obj_set_style_text_color(msg, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_14, 0);
    lv_obj_align(msg, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* cont_btn = lv_btn_create(t);
    lv_obj_set_size(cont_btn, 80, 34);
    lv_obj_align(cont_btn, LV_ALIGN_RIGHT_MID, -90, 0);
    lv_obj_set_style_bg_color(cont_btn, lv_color_hex(0x1A7AD4), 0);
    lv_obj_set_style_radius(cont_btn, 6, 0);
    lv_obj_add_event_cb(cont_btn, toast_continue_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* cl = lv_label_create(cont_btn);
    lv_label_set_text(cl, "Continue");
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_14, 0);
    lv_obj_center(cl);

    lv_obj_t* brk_btn = lv_btn_create(t);
    lv_obj_set_size(brk_btn, 90, 34);
    lv_obj_align(brk_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(brk_btn, lv_color_hex(0xBA7517), 0);
    lv_obj_set_style_radius(brk_btn, 6, 0);
    lv_obj_add_event_cb(brk_btn, toast_break_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* bl = lv_label_create(brk_btn);
    lv_label_set_text(bl, LV_SYMBOL_PAUSE " Break");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    lv_obj_center(bl);

    return t;
}

// ---- Event callbacks ----------------------------------------------------
static void back_cb(lv_event_t* e) {
    if (!debounce()) return;
    if (s_tick_timer) { lv_timer_del(s_tick_timer); s_tick_timer = NULL; }
    UIManager::getInstance().moveTo(Screen::HOME);
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

static void break_btn_cb(lv_event_t* e) {
    if (!debounce()) return;
    SessionMachine::getInstance().takeBreak();
}

static void stop_cb(lv_event_t* e) {
    if (!debounce()) return;
    if (s_tick_timer) { lv_timer_del(s_tick_timer); s_tick_timer = NULL; }
    SessionMachine::getInstance().stop();
    UIManager::getInstance().moveTo(Screen::HOME);
}

// ---- Tick callback (100ms) ----------------------------------------------
static void on_tick(lv_timer_t*) {
    SessionMachine& sm = SessionMachine::getInstance();
    SessionState st = sm.getState();

    // Animate ring arcs (600ms smoothing per tick interval)
    ring_timer_animate_base(s_ring, sm.getBaseArcAngle(), 600);
    ring_timer_animate_overflow(s_ring, sm.getOverflowArcAngle(), 600);
    ring_timer_set_label(s_ring, sm.getTimeLabelString().c_str());

    // State pill text + color
    if (s_state_pill && lv_obj_is_valid(s_state_pill)) {
        const char* pill_txt = "FOCUS";
        lv_color_t pill_col  = lv_color_hex(0x3D9A2B);
        switch (st) {
            case SessionState::HYPERFOCUS: pill_txt="HYPERFOCUS"; pill_col=lv_color_hex(0x1A7AD4); break;
            case SessionState::BREAK:      pill_txt="BREAK";      pill_col=lv_color_hex(0xBA7517); break;
            case SessionState::DISENGAGED: pill_txt="PAUSED";     pill_col=lv_color_hex(0xE24B4A); break;
            default: break;
        }
        lv_label_set_text(s_state_pill, pill_txt);
        lv_obj_set_style_bg_color(s_state_pill, pill_col, 0);
    }

    // Ambient background transition on state change
    if (st != s_lastState) {
        ambient_bg_transition(st);
        // Update ring base color
        lv_color_t rc = lv_color_hex(0x3D9A2B);
        if (st == SessionState::BREAK)      rc = lv_color_hex(0xBA7517);
        if (st == SessionState::DISENGAGED) rc = lv_color_hex(0xE24B4A);
        ring_timer_set_base_color(s_ring, rc);
        s_lastState = st;
    }

    // Pause/Resume button label
    if (s_pause_lbl && lv_obj_is_valid(s_pause_lbl)) {
        bool active = (st == SessionState::FOCUS || st == SessionState::HYPERFOCUS);
        lv_label_set_text(s_pause_lbl, active ? LV_SYMBOL_PAUSE " Pause" : LV_SYMBOL_PLAY " Resume");
    }

    // Actual elapsed time in info panel
    if (s_actual_lbl && lv_obj_is_valid(s_actual_lbl)) {
        int e_sec = sm.getElapsedSeconds();
        char buf[12];
        snprintf(buf, sizeof(buf), "%d min", e_sec / 60);
        lv_label_set_text(s_actual_lbl, buf);
    }

    // Spoon bar
    if (s_spoon_bar && lv_obj_is_valid(s_spoon_bar)) {
        AppState& app = AppState::getInstance();
        int pct = (app.spoonsTotal > 0)
            ? ((app.spoonsTotal - app.spoonsUsed) * 100 / app.spoonsTotal)
            : 0;
        lv_bar_set_value(s_spoon_bar, pct, LV_ANIM_ON);
    }

    // Toast visibility (§5.5)
    if (s_toast && lv_obj_is_valid(s_toast)) {
        if (sm.isToastActive())
            lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
    }

    // Heartbeat for debugging
    static uint32_t lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 2000) {
        Serial.printf("[UI] Focus Tick - State: %d, Elapsed: %d\n", (int)st, sm.getElapsedSeconds());
        lastHeartbeat = millis();
    }
}

// ---- Screen builder -----------------------------------------------------
lv_obj_t* create_screen_focus() {
    if (s_tick_timer) { lv_timer_del(s_tick_timer); s_tick_timer = NULL; }

    AppState& app = AppState::getInstance();
    SessionMachine& sm = SessionMachine::getInstance();
    s_lastState = sm.getState();

    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Init ambient bg
    ambient_bg_init(scr);
    ambient_bg_transition(s_lastState);

    // ---- HEADER (h=40) --------------------------------------------------
    lv_obj_t* hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 480, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x334155), 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* back_btn = lv_btn_create(hdr);
    lv_obj_set_size(back_btn, 70, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_radius(back_btn, 6, 0);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* bl = lv_label_create(back_btn);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    lv_obj_center(bl);

    lv_obj_t* title = lv_label_create(hdr);
    lv_label_set_text(title, "Focus Session");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // State pill
    s_state_pill = lv_label_create(hdr);
    lv_label_set_text(s_state_pill, "FOCUS");
    lv_obj_set_style_text_color(s_state_pill, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_state_pill, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(s_state_pill, lv_color_hex(0x3D9A2B), 0);
    lv_obj_set_style_bg_opa(s_state_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_state_pill, 10, 0);
    lv_obj_set_style_pad_hor(s_state_pill, 8, 0);
    lv_obj_set_style_pad_ver(s_state_pill, 2, 0);
    lv_obj_align(s_state_pill, LV_ALIGN_RIGHT_MID, -8, 0);

    // ---- BODY (y=40, h=240) — left ring | right info --------------------
    // Left panel: ring timer (240x240 within 240-wide column)
    s_ring = create_ring_timer(scr, 200);

    // Position ring container on the left half
    lv_obj_t* ring_root = lv_obj_get_parent(s_ring->bg_arc);
    lv_obj_align(ring_root, LV_ALIGN_TOP_LEFT, 10, 48);

    // Right info panel (x=245, y=40, w=230, h=240)
    lv_obj_t* info = lv_obj_create(scr);
    lv_obj_set_size(info, 225, 225);
    lv_obj_align(info, LV_ALIGN_TOP_RIGHT, -5, 48);
    lv_obj_set_style_bg_color(info, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(info, LV_OPA_40, 0); // glass effect
    lv_obj_set_style_border_width(info, 1, 0);
    lv_obj_set_style_border_color(info, lv_color_white(), 0);
    lv_obj_set_style_border_opa(info, LV_OPA_50, 0);
    lv_obj_set_style_radius(info, 12, 0);
    lv_obj_set_style_pad_all(info, 12, 0);
    lv_obj_set_style_pad_gap(info, 6, 0);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    // Task name
    lv_obj_t* task_lbl = lv_label_create(info);
    lv_label_set_text(task_lbl, app.currentTaskName.c_str());
    lv_obj_set_style_text_color(task_lbl, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_text_font(task_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(task_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(task_lbl, 200);

    // Subtask name (scrolling if too long)
    lv_obj_t* sub_lbl = lv_label_create(info);
    lv_obj_set_width(sub_lbl, 200);
    lv_label_set_long_mode(sub_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(sub_lbl, app.currentSubtaskName.c_str());
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(sub_lbl, &lv_font_montserrat_14, 0);

    // Separator
    lv_obj_t* sep1 = lv_obj_create(info);
    lv_obj_set_size(sep1, 200, 1);
    lv_obj_set_style_bg_color(sep1, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(sep1, LV_OPA_20, 0);
    lv_obj_set_style_border_width(sep1, 0, 0);

    // Planned duration
    lv_obj_t* plan_lbl = lv_label_create(info);
    char plan_buf[24];
    snprintf(plan_buf, sizeof(plan_buf), "Planned:  %d min", sm.getPlannedSeconds() / 60);
    lv_label_set_text(plan_lbl, plan_buf);
    lv_obj_set_style_text_color(plan_lbl, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_text_font(plan_lbl, &lv_font_montserrat_14, 0);

    // Actual elapsed (live)
    s_actual_lbl = lv_label_create(info);
    lv_label_set_text(s_actual_lbl, "Actual:   0 min");
    lv_obj_set_style_text_color(s_actual_lbl, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_text_font(s_actual_lbl, &lv_font_montserrat_14, 0);

    // Energy cost
    lv_obj_t* energy_lbl = lv_label_create(info);
    lv_label_set_text(energy_lbl, "Energy:   -1 spoon");
    lv_obj_set_style_text_color(energy_lbl, lv_color_hex(0x06B6D4), 0);
    lv_obj_set_style_text_font(energy_lbl, &lv_font_montserrat_14, 0);

    // Separator
    lv_obj_t* sep2 = lv_obj_create(info);
    lv_obj_set_size(sep2, 200, 1);
    lv_obj_set_style_bg_color(sep2, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(sep2, LV_OPA_20, 0);
    lv_obj_set_style_border_width(sep2, 0, 0);

    // Start/Pause button
    lv_obj_t* pause_btn = lv_btn_create(info);
    lv_obj_set_size(pause_btn, 200, 34);
    lv_obj_set_style_bg_color(pause_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_set_style_bg_color(pause_btn, lv_color_hex(0x2563EB), LV_STATE_PRESSED);
    lv_obj_set_style_radius(pause_btn, 6, 0);
    lv_obj_add_event_cb(pause_btn, toggle_cb, LV_EVENT_CLICKED, NULL);
    s_pause_lbl = lv_label_create(pause_btn);
    lv_label_set_text(s_pause_lbl, LV_SYMBOL_PAUSE " Pause");
    lv_obj_set_style_text_font(s_pause_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_pause_lbl, lv_color_white(), 0);
    lv_obj_center(s_pause_lbl);

    // Take a Break button
    lv_obj_t* brk_btn = lv_btn_create(info);
    lv_obj_set_size(brk_btn, 200, 34);
    lv_obj_set_style_bg_color(brk_btn, lv_color_hex(0xBA7517), 0);
    lv_obj_set_style_bg_color(brk_btn, lv_color_hex(0x92580F), LV_STATE_PRESSED);
    lv_obj_set_style_radius(brk_btn, 6, 0);
    lv_obj_add_event_cb(brk_btn, break_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* brk_lbl = lv_label_create(brk_btn);
    lv_label_set_text(brk_lbl, LV_SYMBOL_STOP " Take a Break");
    lv_obj_set_style_text_font(brk_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(brk_lbl, lv_color_white(), 0);
    lv_obj_center(brk_lbl);

    // ---- FOOTER (y=280, h=40) — spoon bar + effort bonus ----------------
    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, 480, 40);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_pad_all(footer, 6, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* spoon_lbl = lv_label_create(footer);
    lv_label_set_text(spoon_lbl, "Spoons today:");
    lv_obj_set_style_text_color(spoon_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(spoon_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(spoon_lbl, LV_ALIGN_LEFT_MID, 8, 0);

    s_spoon_bar = lv_bar_create(footer);
    lv_obj_set_size(s_spoon_bar, 120, 8);
    lv_obj_align(s_spoon_bar, LV_ALIGN_LEFT_MID, 105, 0);
    int spoon_pct = (app.spoonsTotal > 0)
        ? ((app.spoonsTotal - app.spoonsUsed) * 100 / app.spoonsTotal) : 0;
    lv_bar_set_value(s_spoon_bar, spoon_pct, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_spoon_bar, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(s_spoon_bar, lv_color_hex(0x06B6D4), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_spoon_bar, 4, 0);
    lv_obj_set_style_radius(s_spoon_bar, 4, LV_PART_INDICATOR);

    char spoon_count[12];
    snprintf(spoon_count, sizeof(spoon_count), "%d/%d",
             app.spoonsTotal - app.spoonsUsed, app.spoonsTotal);
    lv_obj_t* spoon_count_lbl = lv_label_create(footer);
    lv_label_set_text(spoon_count_lbl, spoon_count);
    lv_obj_set_style_text_color(spoon_count_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(spoon_count_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(spoon_count_lbl, LV_ALIGN_LEFT_MID, 232, 0);

    lv_obj_t* effort_lbl = lv_label_create(footer);
    lv_label_set_text(effort_lbl, "Effort: 1.0x");
    lv_obj_set_style_text_color(effort_lbl, lv_color_hex(0xEAB308), 0);
    lv_obj_set_style_text_font(effort_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(effort_lbl, LV_ALIGN_RIGHT_MID, -70, 0);

    // Stop button (small, right of footer)
    lv_obj_t* stop_btn = lv_btn_create(scr);
    lv_obj_set_size(stop_btn, 60, 28);
    lv_obj_align(stop_btn, LV_ALIGN_BOTTOM_RIGHT, -8, -6);
    lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xEF4444), 0);
    lv_obj_set_style_radius(stop_btn, 6, 0);
    lv_obj_add_event_cb(stop_btn, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* stop_lbl = lv_label_create(stop_btn);
    lv_label_set_text(stop_lbl, LV_SYMBOL_STOP " End");
    lv_obj_set_style_text_font(stop_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(stop_lbl);

    // ---- Non-blocking HyperFocus toast (§5.5) ---------------------------
    s_toast = build_toast(scr);

    // ---- 100ms tick timer -----------------------------------------------
    s_tick_timer = lv_timer_create(on_tick, 100, NULL);

    return scr;
}
