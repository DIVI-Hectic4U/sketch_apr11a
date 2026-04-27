// ambient_bg.cpp — Part 5.4: Animated background color transitions
// Uses lv_color_mix() across 16 steps at 50ms each = 800ms total (spec §5.4).
// A static lv_timer drives the interpolation; only one transition runs at a time.

#define LV_CONF_SKIP
#include "ambient_bg.h"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static lv_obj_t*   s_screen      = NULL;
static lv_color_t  s_colorFrom   = {0};
static lv_color_t  s_colorTo     = {0};
static int         s_step        = 0;          // 0 = inactive, 1–16 = animating
static lv_timer_t* s_anim_timer  = NULL;
static SessionState s_lastState  = SessionState::IDLE;

// ---------------------------------------------------------------------------
// Target colors per spec §5.4
// ---------------------------------------------------------------------------
lv_color_t ambient_bg_color_for_state(SessionState s) {
    switch (s) {
        case SessionState::FOCUS:       return lv_color_hex(0xEAF3DE); // soft green
        case SessionState::HYPERFOCUS:  return lv_color_hex(0xE6F1FB); // soft blue
        case SessionState::BREAK:       return lv_color_hex(0xFAEEDA); // soft amber
        case SessionState::DISENGAGED:  return lv_color_hex(0xFCEBEB); // soft red
        default:                        return lv_color_hex(0x0F172A); // slate-950 (home bg)
    }
}

// ---------------------------------------------------------------------------
// Animation step timer callback
// Blends from s_colorFrom → s_colorTo over 16 steps, 50ms each.
// lv_color_mix(c1, c2, ratio): ratio 0 → pure c2, 255 → pure c1
// At step k, mix ratio = (k * 255) / 16
// ---------------------------------------------------------------------------
static void _anim_step_cb(lv_timer_t* timer) {
    (void)timer;
    if (!s_screen || !lv_obj_is_valid(s_screen)) {
        s_step = 0;
        return;
    }

    s_step++;
    if (s_step > 16) {
        lv_obj_set_style_bg_color(s_screen, s_colorTo, 0);
        lv_timer_del(s_anim_timer);
        s_anim_timer = NULL;
        s_step = 0;
        Serial.println("[BG] Animation complete");
        return;
    }

    uint8_t ratio = (uint8_t)(((16 - s_step) * 255) / 16);
    lv_color_t blended = lv_color_mix(s_colorFrom, s_colorTo, ratio);
    lv_obj_set_style_bg_color(s_screen, blended, 0);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ambient_bg_init(lv_obj_t* screen) {
    s_screen    = screen;
    s_lastState = SessionState::IDLE;
    s_step      = 0;
    if (s_anim_timer) {
        lv_timer_del(s_anim_timer);
        s_anim_timer = NULL;
    }
    // Set initial dark-neutral background immediately (no animation on init)
    lv_obj_set_style_bg_color(s_screen, ambient_bg_color_for_state(SessionState::IDLE), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
}

void ambient_bg_transition(SessionState newState) {
    if (!s_screen || !lv_obj_is_valid(s_screen)) return;
    
    // We only skip if the state is the same AND an animation isn't already running
    if (newState == s_lastState && s_anim_timer == NULL) return; 

    Serial.printf("[BG] Transition to state %d\n", (int)newState);

    if (s_anim_timer) {
        lv_timer_del(s_anim_timer);
        s_anim_timer = NULL;
    }

    s_colorFrom = lv_obj_get_style_bg_color(s_screen, LV_PART_MAIN);
    s_colorTo   = ambient_bg_color_for_state(newState);
    s_lastState = newState;
    s_step      = 0;

    s_anim_timer = lv_timer_create(_anim_step_cb, 50, NULL);
}
