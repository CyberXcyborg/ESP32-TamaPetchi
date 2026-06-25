// ============================================================
// AnimTypes.h — Animation type definitions (no LVGL dependency)
// Shared between AnimationPlayer.h and animations.h
// ============================================================

#ifndef ANIM_TYPES_H
#define ANIM_TYPES_H

#include <Arduino.h>
#include "config_v2.h"

// Maximum frames per animation
#define ANIM_MAX_FRAMES  32

// Animation state
enum AnimState {
    ANIM_STOPPED = 0,
    ANIM_PLAYING,
    ANIM_PAUSED
};

// Animation action types
enum AnimActionType {
    ACTION_NONE = 0,
    ACTION_FEED,
    ACTION_PLAY,
    ACTION_CLEAN,
    ACTION_SLEEP,
    ACTION_WAKE,
    ACTION_HEAL,
    ACTION_HAPPY,
    ACTION_EVOLVE
};

// Animation definition (const data, stored in flash)
struct AnimDefinition {
    const char *name;
    const uint8_t *frame_indices;   // Array of sprite frame indices
    const uint16_t *frame_durations; // Duration per frame in ms (NULL = use fps)
    uint8_t frame_count;
    uint8_t fps;                    // Default FPS (used if frame_durations is null)
    bool loop;
};

// Animation set for a pet stage
struct AnimSet {
    const char *stage_name;
    const AnimDefinition *idle_loop;
    const AnimDefinition *eat_once;
    const AnimDefinition *play_loop;
    const AnimDefinition *sleep_loop;
    const AnimDefinition *sick_loop;
    const AnimDefinition *happy_once;
    const AnimDefinition *evolve_once;  // NULL for non-adult stages
};

#endif // ANIM_TYPES_H
