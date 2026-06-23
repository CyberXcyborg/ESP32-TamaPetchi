// ============================================================
// animations.h — Animation Definitions for All Pet Stages
// Const data stored in flash (PROGMEM on ESP32, normal on native)
// ============================================================

#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "AnimTypes.h"

// ============================================================
// Baby Stage Animations
// ============================================================

static const uint8_t baby_idle_frames[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint16_t baby_idle_durations[] = {83, 83, 83, 83, 83, 83, 83, 83}; // 12 FPS

static const uint8_t baby_eat_frames[] = {8, 9, 10, 11, 12, 13};
static const uint16_t baby_eat_durations[] = {62, 62, 62, 62, 62, 62}; // 16 FPS

static const uint8_t baby_sleep_frames[] = {14, 15, 16, 17};
static const uint16_t baby_sleep_durations[] = {250, 250, 250, 250}; // 4 FPS

static const uint8_t baby_happy_frames[] = {18, 19, 20, 21, 22, 23};
static const uint16_t baby_happy_durations[] = {62, 62, 62, 62, 62, 62}; // 16 FPS

// ============================================================
// Child Stage Animations
// ============================================================

static const uint8_t child_idle_frames[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint16_t child_idle_durations[] = {83, 83, 83, 83, 83, 83, 83, 83};

static const uint8_t child_eat_frames[] = {8, 9, 10, 11, 12, 13};
static const uint16_t child_eat_durations[] = {62, 62, 62, 62, 62, 62};

static const uint8_t child_play_frames[] = {14, 15, 16, 17, 18, 19, 20, 21};
static const uint16_t child_play_durations[] = {62, 62, 62, 62, 62, 62, 62, 62};

static const uint8_t child_sleep_frames[] = {22, 23, 24, 25};
static const uint16_t child_sleep_durations[] = {250, 250, 250, 250};

static const uint8_t child_sick_frames[] = {26, 27, 28, 29};
static const uint16_t child_sick_durations[] = {200, 200, 200, 200};

// ============================================================
// Adult Stage Animations
// ============================================================

static const uint8_t adult_idle_frames[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint16_t adult_idle_durations[] = {83, 83, 83, 83, 83, 83, 83, 83};

static const uint8_t adult_eat_frames[] = {8, 9, 10, 11, 12, 13};
static const uint16_t adult_eat_durations[] = {62, 62, 62, 62, 62, 62};

static const uint8_t adult_play_frames[] = {14, 15, 16, 17, 18, 19, 20, 21};
static const uint16_t adult_play_durations[] = {62, 62, 62, 62, 62, 62, 62, 62};

static const uint8_t adult_sleep_frames[] = {22, 23, 24, 25};
static const uint16_t adult_sleep_durations[] = {250, 250, 250, 250};

static const uint8_t adult_sick_frames[] = {26, 27, 28, 29};
static const uint16_t adult_sick_durations[] = {200, 200, 200, 200};

static const uint8_t adult_evolve_frames[] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
static const uint16_t adult_evolve_durations[] = {83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83};

// ============================================================
// Elder Stage Animations
// ============================================================

static const uint8_t elder_idle_frames[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint16_t elder_idle_durations[] = {100, 100, 100, 100, 100, 100, 100, 100}; // 10 FPS

static const uint8_t elder_eat_frames[] = {8, 9, 10, 11, 12, 13};
static const uint16_t elder_eat_durations[] = {83, 83, 83, 83, 83, 83};

static const uint8_t elder_play_frames[] = {14, 15, 16, 17, 18, 19, 20, 21};
static const uint16_t elder_play_durations[] = {83, 83, 83, 83, 83, 83, 83, 83};

static const uint8_t elder_sleep_frames[] = {22, 23, 24, 25};
static const uint16_t elder_sleep_durations[] = {300, 300, 300, 300}; // Slower

static const uint8_t elder_sick_frames[] = {26, 27, 28, 29};
static const uint16_t elder_sick_durations[] = {250, 250, 250, 250};

// ============================================================
// Animation Definitions
// ============================================================

static const AnimDefinition baby_idle = {
    "baby_idle", baby_idle_frames, baby_idle_durations, 8, 12, true
};
static const AnimDefinition baby_eat = {
    "baby_eat", baby_eat_frames, baby_eat_durations, 6, 16, false
};
static const AnimDefinition baby_sleep = {
    "baby_sleep", baby_sleep_frames, baby_sleep_durations, 4, 4, true
};
static const AnimDefinition baby_happy = {
    "baby_happy", baby_happy_frames, baby_happy_durations, 6, 16, false
};

static const AnimDefinition child_idle = {
    "child_idle", child_idle_frames, child_idle_durations, 8, 12, true
};
static const AnimDefinition child_eat = {
    "child_eat", child_eat_frames, child_eat_durations, 6, 16, false
};
static const AnimDefinition child_play = {
    "child_play", child_play_frames, child_play_durations, 8, 16, true
};
static const AnimDefinition child_sleep = {
    "child_sleep", child_sleep_frames, child_sleep_durations, 4, 4, true
};
static const AnimDefinition child_sick = {
    "child_sick", child_sick_frames, child_sick_durations, 4, 5, true
};

static const AnimDefinition adult_idle = {
    "adult_idle", adult_idle_frames, adult_idle_durations, 8, 12, true
};
static const AnimDefinition adult_eat = {
    "adult_eat", adult_eat_frames, adult_eat_durations, 6, 16, false
};
static const AnimDefinition adult_play = {
    "adult_play", adult_play_frames, adult_play_durations, 8, 16, true
};
static const AnimDefinition adult_sleep = {
    "adult_sleep", adult_sleep_frames, adult_sleep_durations, 4, 4, true
};
static const AnimDefinition adult_sick = {
    "adult_sick", adult_sick_frames, adult_sick_durations, 4, 5, true
};
static const AnimDefinition adult_evolve = {
    "adult_evolve", adult_evolve_frames, adult_evolve_durations, 12, 12, false
};

static const AnimDefinition elder_idle = {
    "elder_idle", elder_idle_frames, elder_idle_durations, 8, 10, true
};
static const AnimDefinition elder_eat = {
    "elder_eat", elder_eat_frames, elder_eat_durations, 6, 12, false
};
static const AnimDefinition elder_play = {
    "elder_play", elder_play_frames, elder_play_durations, 8, 12, true
};
static const AnimDefinition elder_sleep = {
    "elder_sleep", elder_sleep_frames, elder_sleep_durations, 4, 3, true
};
static const AnimDefinition elder_sick = {
    "elder_sick", elder_sick_frames, elder_sick_durations, 4, 4, true
};

// ============================================================
// Animation Sets (per stage)
// ============================================================

static const AnimSet baby_anim_set = {
    "Baby", &baby_idle, &baby_eat, nullptr, &baby_sleep, nullptr, &baby_happy, nullptr
};

static const AnimSet child_anim_set = {
    "Child", &child_idle, &child_eat, &child_play, &child_sleep, &child_sick, nullptr, nullptr
};

static const AnimSet adult_anim_set = {
    "Adult", &adult_idle, &adult_eat, &adult_play, &adult_sleep, &adult_sick, nullptr, &adult_evolve
};

static const AnimSet elder_anim_set = {
    "Elder", &elder_idle, &elder_eat, &elder_play, &elder_sleep, &elder_sick, nullptr, nullptr
};

// Helper to get animation set by stage
inline const AnimSet* getAnimSet(uint8_t stage) {
    switch (stage) {
        case 0: return &baby_anim_set;
        case 1: return &child_anim_set;
        case 2: return &adult_anim_set;
        case 3: return &elder_anim_set;
        default: return &baby_anim_set;
    }
}

// Helper to get animation definition by action
inline const AnimDefinition* getAnimForAction(const AnimSet *set, AnimActionType action) {
    if (!set) return nullptr;
    switch (action) {
        case ACTION_NONE:      return set->idle_loop;
        case ACTION_FEED:      return set->eat_once;
        case ACTION_PLAY:      return set->play_loop;
        case ACTION_SLEEP:     return set->sleep_loop;
        case ACTION_HEAL:      return set->idle_loop;  // Default to idle after heal
        case ACTION_HAPPY:     return set->happy_once;
        case ACTION_EVOLVE:    return set->evolve_once;
        case ACTION_CLEAN:     return set->idle_loop;  // Default to idle after clean
        case ACTION_WAKE:      return set->idle_loop;
        default:               return set->idle_loop;
    }
}

#endif // ANIMATIONS_H
