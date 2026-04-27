// ring_timer.cpp — Three-layer circular arc widget (spec §5.2)
// Layer 1: bg_arc        — dark grey full ring (always 0→360)
// Layer 2: base_arc      — state-colored progress arc (0→360 over session)
// Layer 3: overflow_arc  — blue HyperFocus arc, drawn on top
// All angle updates go through lv_anim_t (spec §5.2 requirement).

#define LV_CONF_SKIP
#include "ring_timer.h"

// ---------------------------------------------------------------------------
// lv_anim exec callbacks — arc uses int32_t value 0–360
// ---------------------------------------------------------------------------
static void _anim_base_exec(void* obj, int32_t val) {
    // base_arc stores 0–360 in its range; use lv_arc_set_angles directly
    // to avoid the arc's built-in clamping which uses a 0–100 value range.
    lv_arc_set_end_angle((lv_obj_t*)obj, (uint16_t)val);
}

static void _anim_overflow_exec(void* obj, int32_t val) {
    lv_arc_set_end_angle((lv_obj_t*)obj, (uint16_t)val);
}

// ---------------------------------------------------------------------------
// Helper: make a full-circle background ring (non-interactive, no knob)
// ---------------------------------------------------------------------------
static lv_obj_t* _make_arc(lv_obj_t* parent, int size, int arcWidth,
                             lv_color_t bgColor, lv_color_t indicatorColor,
                             bool clickable = false) {
    lv_obj_t* arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_arc_set_rotation(arc, 270);           // start at top (12 o'clock)
    lv_arc_set_bg_angles(arc, 0, 360);       // full-circle background track
    lv_arc_set_angles(arc, 0, 0);            // indicator starts empty
    lv_arc_set_range(arc, 0, 360);           // use degrees directly

    lv_obj_set_style_arc_width(arc, arcWidth, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, arcWidth, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, bgColor, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, indicatorColor, LV_PART_INDICATOR);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);  // no knob
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);

    if (!clickable) {
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_center(arc);
    return arc;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
RingTimerHandle* create_ring_timer(lv_obj_t* parent, int size) {
    // Container (transparent, same size as arcs) — acts as the widget root
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, size, size);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    RingTimerHandle* h = new RingTimerHandle();

    // Layer 1 — background ring (dark grey, always full)
    h->bg_arc = _make_arc(container, size, 14,
                           lv_color_hex(0x1E293B),   // bg track color
                           lv_color_hex(0x1E293B));  // indicator same as bg
    lv_arc_set_angles(h->bg_arc, 0, 360); // always fully drawn

    // Layer 2 — base arc (FOCUS green by default)
    h->base_arc = _make_arc(container, size, 14,
                             lv_color_hex(0x0F1929),   // track (translucent)
                             lv_color_hex(0x3D9A2B));  // FOCUS green

    // Layer 3 — overflow arc (HyperFocus blue), drawn on top, starts empty
    h->overflow_arc = _make_arc(container, size - 0, 14,
                                 lv_color_hex(0x00000000),  // transparent track
                                 lv_color_hex(0x1A7AD4));   // HyperFocus blue

    // Make overflow arc track fully transparent so only indicator shows
    lv_obj_set_style_arc_opa(h->overflow_arc, LV_OPA_TRANSP, LV_PART_MAIN);

    // Center time label inside the ring
    h->time_label = lv_label_create(container);
    lv_label_set_text(h->time_label, "25:00");
    lv_obj_set_style_text_font(h->time_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(h->time_label, lv_color_hex(0x1E293B), 0);
    lv_obj_center(h->time_label);

    // Store the handle pointer so the parent screen can retrieve it
    lv_obj_set_user_data(container, h);

    return h;
}

void ring_timer_animate_base(RingTimerHandle* h, int targetAngle, uint32_t durationMs) {
    if (h && h->base_arc) {
        lv_arc_set_end_angle(h->base_arc, (uint16_t)targetAngle);
    }
}

void ring_timer_animate_overflow(RingTimerHandle* h, int targetAngle, uint32_t durationMs) {
    if (h && h->overflow_arc) {
        lv_arc_set_end_angle(h->overflow_arc, (uint16_t)targetAngle);
    }
}

void ring_timer_set_label(RingTimerHandle* h, const char* text) {
    if (h && h->time_label && lv_obj_is_valid(h->time_label)) {
        lv_label_set_text(h->time_label, text);
    }
}

void ring_timer_set_base_color(RingTimerHandle* h, lv_color_t color) {
    if (h && h->base_arc) {
        lv_obj_set_style_arc_color(h->base_arc, color, LV_PART_INDICATOR);
    }
}
