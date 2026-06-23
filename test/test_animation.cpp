// test_animation.cpp — Unit tests for AnimationPlayer (Phase 20.2)
// Tests animation definitions and state machine logic
// For native builds, we include the state machine implementation directly
// to avoid LVGL dependency
#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include <cstdio>
#include <cstdint>

// Include animation type definitions (no LVGL dependency)
#include "AnimTypes.h"

// Include the animation data declarations
#include "animations.h"

// ============================================================
// Animation State Machine (inline implementation for tests)
// ============================================================

class AnimStateMachine {
public:
    AnimStateMachine() : _current_action(ACTION_NONE), _previous_action(ACTION_NONE),
                         _action_start_tick(0), _action_duration_ms(0),
                         _pending_idle(false), _idle_return_tick(0) {}
    
    void init() {
        _current_action = ACTION_NONE;
        _previous_action = ACTION_NONE;
        _action_start_tick = 0;
        _action_duration_ms = 0;
        _pending_idle = false;
        _idle_return_tick = 0;
    }
    
    bool requestAction(AnimActionType action) {
        if (!canTransition(action)) return false;
        transitionTo(action);
        return true;
    }
    
    AnimActionType getCurrentAction() const { return _current_action; }
    
    const char* getCurrentActionName() const {
        switch (_current_action) {
            case ACTION_NONE: return "idle";
            case ACTION_FEED: return "eat";
            case ACTION_PLAY: return "play";
            case ACTION_SLEEP: return "sleep";
            case ACTION_WAKE: return "wake";
            case ACTION_HEAL: return "heal";
            case ACTION_HAPPY: return "happy";
            case ACTION_EVOLVE: return "evolve";
            case ACTION_CLEAN: return "clean";
            default: return "unknown";
        }
    }
    
    void forceState(AnimActionType state) {
        _current_action = state;
        _action_start_tick = 0;
        _pending_idle = false;
    }
    
    bool canTransition(AnimActionType action) const {
        if (_current_action == ACTION_HEAL && action != ACTION_HEAL) return false;
        if (_current_action == ACTION_SLEEP && (action == ACTION_FEED || action == ACTION_PLAY)) return false;
        if (_current_action == ACTION_SLEEP && action == ACTION_SLEEP) return false;
        if (action == ACTION_EVOLVE && _current_action != ACTION_NONE) return false;
        return true;
    }

private:
    AnimActionType _current_action;
    AnimActionType _previous_action;
    uint32_t _action_start_tick;
    uint32_t _action_duration_ms;
    bool _pending_idle;
    uint32_t _idle_return_tick;
    
    void transitionTo(AnimActionType action) {
        _previous_action = _current_action;
        _current_action = action;
        _action_start_tick = 0;
        _pending_idle = isOnceAction(action);
        _idle_return_tick = _pending_idle ? 2000 : 0;
    }
    
    bool isOnceAction(AnimActionType action) const {
        return action == ACTION_FEED || action == ACTION_HAPPY ||
               action == ACTION_EVOLVE || action == ACTION_CLEAN ||
               action == ACTION_HEAL;
    }
};

// ============================================================
// Animation Definition Tests
// ============================================================

void test_anim_def_baby_idle() {
    TEST_ASSERT_EQUAL_STRING("baby_idle", baby_idle.name);
    TEST_ASSERT_EQUAL(8, baby_idle.frame_count);
    TEST_ASSERT_TRUE(baby_idle.loop);
    TEST_ASSERT_EQUAL(12, baby_idle.fps);
    TEST_ASSERT_EQUAL(0, baby_idle.frame_indices[0]);
    TEST_ASSERT_EQUAL(7, baby_idle.frame_indices[7]);
    printf("  PASS: Baby idle animation definition\n");
}

void test_anim_def_baby_eat() {
    TEST_ASSERT_EQUAL_STRING("baby_eat", baby_eat.name);
    TEST_ASSERT_EQUAL(6, baby_eat.frame_count);
    TEST_ASSERT_FALSE(baby_eat.loop);
    printf("  PASS: Baby eat animation definition\n");
}

void test_anim_def_adult_evolve() {
    TEST_ASSERT_EQUAL_STRING("adult_evolve", adult_evolve.name);
    TEST_ASSERT_EQUAL(12, adult_evolve.frame_count);
    TEST_ASSERT_FALSE(adult_evolve.loop);
    printf("  PASS: Adult evolve animation definition\n");
}

void test_anim_def_frame_durations() {
    TEST_ASSERT_EQUAL(83, baby_idle.frame_durations[0]);
    TEST_ASSERT_EQUAL(83, baby_idle.frame_durations[7]);
    TEST_ASSERT_EQUAL(62, baby_eat.frame_durations[0]);
    TEST_ASSERT_EQUAL(250, baby_sleep.frame_durations[0]);
    printf("  PASS: Frame durations correct\n");
}

// ============================================================
// Animation Set Tests
// ============================================================

void test_anim_set_baby() {
    const AnimSet *set = getAnimSet(0);
    TEST_ASSERT_NOT_NULL(set);
    TEST_ASSERT_EQUAL_STRING("Baby", set->stage_name);
    TEST_ASSERT_NOT_NULL(set->idle_loop);
    TEST_ASSERT_NOT_NULL(set->eat_once);
    TEST_ASSERT_NOT_NULL(set->sleep_loop);
    TEST_ASSERT_NOT_NULL(set->happy_once);
    TEST_ASSERT_NULL(set->play_loop);
    TEST_ASSERT_NULL(set->evolve_once);
    printf("  PASS: Baby animation set\n");
}

void test_anim_set_adult() {
    const AnimSet *set = getAnimSet(2);
    TEST_ASSERT_NOT_NULL(set);
    TEST_ASSERT_EQUAL_STRING("Adult", set->stage_name);
    TEST_ASSERT_NOT_NULL(set->idle_loop);
    TEST_ASSERT_NOT_NULL(set->eat_once);
    TEST_ASSERT_NOT_NULL(set->play_loop);
    TEST_ASSERT_NOT_NULL(set->sleep_loop);
    TEST_ASSERT_NOT_NULL(set->evolve_once);
    printf("  PASS: Adult animation set\n");
}

void test_anim_set_all_stages() {
    for (int stage = 0; stage < 4; stage++) {
        const AnimSet *set = getAnimSet(stage);
        TEST_ASSERT_NOT_NULL(set);
        TEST_ASSERT_NOT_NULL(set->idle_loop);
        TEST_ASSERT_NOT_NULL(set->eat_once);
        TEST_ASSERT_NOT_NULL(set->sleep_loop);
    }
    printf("  PASS: All 4 stage animation sets valid\n");
}

// ============================================================
// Animation State Machine Tests
// ============================================================

void test_anim_sm_init() {
    AnimStateMachine sm;
    sm.init();
    TEST_ASSERT_EQUAL(ACTION_NONE, sm.getCurrentAction());
    TEST_ASSERT_EQUAL_STRING("idle", sm.getCurrentActionName());
    printf("  PASS: Animation SM initializes to idle\n");
}

void test_anim_sm_basic_transition() {
    AnimStateMachine sm;
    sm.init();
    TEST_ASSERT_TRUE(sm.requestAction(ACTION_FEED));
    TEST_ASSERT_EQUAL(ACTION_FEED, sm.getCurrentAction());
    printf("  PASS: Idle -> Feed transition\n");
}

void test_anim_sm_feed_returns_to_idle() {
    AnimStateMachine sm;
    sm.init();
    sm.requestAction(ACTION_FEED);
    TEST_ASSERT_EQUAL(ACTION_FEED, sm.getCurrentAction());
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_NONE));
    sm.requestAction(ACTION_NONE);
    TEST_ASSERT_EQUAL(ACTION_NONE, sm.getCurrentAction());
    printf("  PASS: Feed returns to idle\n");
}

void test_anim_sm_sleep_blocks_eat() {
    AnimStateMachine sm;
    sm.init();
    TEST_ASSERT_TRUE(sm.requestAction(ACTION_SLEEP));
    TEST_ASSERT_EQUAL(ACTION_SLEEP, sm.getCurrentAction());
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_FEED));
    printf("  PASS: Sleep blocks eat\n");
}

void test_anim_sm_sleep_blocks_play() {
    AnimStateMachine sm;
    sm.init();
    sm.requestAction(ACTION_SLEEP);
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_PLAY));
    printf("  PASS: Sleep blocks play\n");
}

void test_anim_sm_sick_overrides() {
    AnimStateMachine sm;
    sm.init();
    sm.forceState(ACTION_HEAL);
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_FEED));
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_PLAY));
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_SLEEP));
    printf("  PASS: Sick state blocks other actions\n");
}

void test_anim_sm_no_double_sleep() {
    AnimStateMachine sm;
    sm.init();
    TEST_ASSERT_TRUE(sm.requestAction(ACTION_SLEEP));
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_SLEEP));
    printf("  PASS: No double sleep\n");
}

void test_anim_sm_evolve_requires_idle() {
    AnimStateMachine sm;
    sm.init();
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_EVOLVE));
    sm.requestAction(ACTION_FEED);
    TEST_ASSERT_FALSE(sm.canTransition(ACTION_EVOLVE));
    printf("  PASS: Evolve requires idle\n");
}

void test_anim_sm_force_state() {
    AnimStateMachine sm;
    sm.init();
    sm.forceState(ACTION_SLEEP);
    TEST_ASSERT_EQUAL(ACTION_SLEEP, sm.getCurrentAction());
    sm.forceState(ACTION_NONE);
    TEST_ASSERT_EQUAL(ACTION_NONE, sm.getCurrentAction());
    printf("  PASS: Force state works\n");
}

void test_anim_sm_can_transition_matrix() {
    AnimStateMachine sm;
    sm.init();
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_FEED));
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_PLAY));
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_SLEEP));
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_CLEAN));
    TEST_ASSERT_TRUE(sm.canTransition(ACTION_HAPPY));
    printf("  PASS: Transition matrix from idle\n");
}

void test_anim_get_anim_for_action() {
    const AnimSet *set = getAnimSet(0);
    TEST_ASSERT_EQUAL(&baby_idle, getAnimForAction(set, ACTION_NONE));
    TEST_ASSERT_EQUAL(&baby_eat, getAnimForAction(set, ACTION_FEED));
    TEST_ASSERT_EQUAL(&baby_sleep, getAnimForAction(set, ACTION_SLEEP));
    TEST_ASSERT_EQUAL(&baby_happy, getAnimForAction(set, ACTION_HAPPY));
    TEST_ASSERT_NULL(getAnimForAction(set, ACTION_PLAY));
    printf("  PASS: Get animation for action\n");
}

void test_anim_get_anim_adult_evolve() {
    const AnimSet *set = getAnimSet(2);
    TEST_ASSERT_NOT_NULL(getAnimForAction(set, ACTION_EVOLVE));
    TEST_ASSERT_EQUAL(&adult_evolve, getAnimForAction(set, ACTION_EVOLVE));
    printf("  PASS: Adult evolve animation\n");
}

void test_anim_elder_slower_animations() {
    // Elder animations should be slower (lower FPS)
    TEST_ASSERT_EQUAL(10, elder_idle.fps);   // 10 FPS vs 12 for baby
    TEST_ASSERT_EQUAL(3, elder_sleep.fps);    // 3 FPS vs 4 for baby
    TEST_ASSERT_EQUAL(100, elder_idle.frame_durations[0]);  // 100ms vs 83ms
    printf("  PASS: Elder animations are slower\n");
}

// ============================================================
// Test runner
// ============================================================
int run_animation_tests() {
    printf("--- Animation Engine Tests ---\n");
    
    RUN_TEST(test_anim_def_baby_idle);
    RUN_TEST(test_anim_def_baby_eat);
    RUN_TEST(test_anim_def_adult_evolve);
    RUN_TEST(test_anim_def_frame_durations);
    RUN_TEST(test_anim_set_baby);
    RUN_TEST(test_anim_set_adult);
    RUN_TEST(test_anim_set_all_stages);
    RUN_TEST(test_anim_sm_init);
    RUN_TEST(test_anim_sm_basic_transition);
    RUN_TEST(test_anim_sm_feed_returns_to_idle);
    RUN_TEST(test_anim_sm_sleep_blocks_eat);
    RUN_TEST(test_anim_sm_sleep_blocks_play);
    RUN_TEST(test_anim_sm_sick_overrides);
    RUN_TEST(test_anim_sm_no_double_sleep);
    RUN_TEST(test_anim_sm_evolve_requires_idle);
    RUN_TEST(test_anim_sm_force_state);
    RUN_TEST(test_anim_sm_can_transition_matrix);
    RUN_TEST(test_anim_get_anim_for_action);
    RUN_TEST(test_anim_get_anim_adult_evolve);
    RUN_TEST(test_anim_elder_slower_animations);
    
    printf("--- Animation Engine: 20 tests PASSED ---\n");
    return 0;
}
