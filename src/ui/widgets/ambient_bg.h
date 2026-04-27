#ifndef AMBIENT_BG_H
#define AMBIENT_BG_H

#include <lvgl.h>
#include "../../state/session_machine.h"

// ---------------------------------------------------------------------------
// ambient_bg — animated background color transitions (spec §5.4)
//
// Changes the screen background color on session state transitions using
// lv_obj_set_style_bg_color with animated color interpolation over 800ms.
// Uses lv_color_mix() in a timer callback over 16 steps (50ms each).
//
// Target colors per state:
//   FOCUS:       0xEAF3DE  soft green tint
//   HYPERFOCUS:  0xE6F1FB  soft blue tint
//   BREAK:       0xFAEEDA  soft amber tint
//   DISENGAGED:  0xFCEBEB  soft red tint
//   IDLE:        0x1A1A1A  dark neutral
// ---------------------------------------------------------------------------

/**
 * Initialise the ambient background on a screen object.
 * Must be called once after the screen is created.
 * @param screen  The lv_obj_t* screen to animate.
 */
void ambient_bg_init(lv_obj_t* screen);

/**
 * Trigger an animated transition to the target color for the given state.
 * Safe to call on every state change — ignores duplicate transitions.
 * @param newState  The new SessionState.
 */
void ambient_bg_transition(SessionState newState);

/**
 * Returns the target lv_color_t for a given SessionState.
 */
lv_color_t ambient_bg_color_for_state(SessionState s);

#endif // AMBIENT_BG_H
