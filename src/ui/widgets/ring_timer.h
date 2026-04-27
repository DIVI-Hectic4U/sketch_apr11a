#ifndef RING_TIMER_H
#define RING_TIMER_H

#include <lvgl.h>

// ---------------------------------------------------------------------------
// RingTimer — three-layer circular progress widget (spec §5.2)
//
// Layer 1 (bg_arc):       Full circle, dark grey, always visible.
// Layer 2 (base_arc):     Fills 0→360° over plannedDuration.
//                          Color changes per session state.
// Layer 3 (overflow_arc): Drawn on top; blue, grows during HyperFocus.
//                          Max = HYPERFOCUS_CAP / planned * 360°
//
// A child lv_label centered in the ring shows the per-state time string.
// Arc angles are animated via lv_anim_t (not set directly per §5.2).
// ---------------------------------------------------------------------------

// Opaque handle returned by create_ring_timer()
struct RingTimerHandle {
    lv_obj_t* bg_arc;        // background ring (always full, dark grey)
    lv_obj_t* base_arc;      // FOCUS progress arc
    lv_obj_t* overflow_arc;  // HYPERFOCUS overflow arc (on top)
    lv_obj_t* time_label;    // center label
};

/**
 * Creates the three-arc ring timer in parent at the given size.
 * Returns a heap-allocated RingTimerHandle (caller must not free it;
 * it is freed when the parent screen is deleted).
 */
RingTimerHandle* create_ring_timer(lv_obj_t* parent, int size = 160);

/**
 * Animates the base arc to targetAngle (0–360) over durationMs.
 * Uses lv_anim_t per spec §5.2 — do NOT call lv_arc_set_value directly.
 */
void ring_timer_animate_base(RingTimerHandle* h, int targetAngle, uint32_t durationMs);

/**
 * Animates the overflow arc to targetAngle (0–360) over durationMs.
 */
void ring_timer_animate_overflow(RingTimerHandle* h, int targetAngle, uint32_t durationMs);

/**
 * Updates the center time label text.
 */
void ring_timer_set_label(RingTimerHandle* h, const char* text);

/**
 * Changes the base arc color (called on state transition).
 * focusColor:  0x3D9A2B green (FOCUS)
 * breakColor:  0xBA7517 amber (BREAK, frozen)
 * disengaged:  0xE24B4A red   (DISENGAGED, draining)
 */
void ring_timer_set_base_color(RingTimerHandle* h, lv_color_t color);

#endif // RING_TIMER_H
