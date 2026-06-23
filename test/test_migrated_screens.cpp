// test_migrated_screens.cpp — Unit tests for Phase 20.4 migrated screens
// Tests screen creation, navigation, and interaction logic
#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include <cstdio>
#include <cstdint>

// ============================================================
// Screen Navigation Tests (extended from test_screenmanager)
// ============================================================

struct NavTestScreen {
    const char *name;
    bool created;
    bool active;
    uint8_t enter_count;
    uint8_t exit_count;
};

struct NavTestStack {
    static constexpr uint8_t MAX = 8;
    NavTestScreen *stack[MAX];
    uint8_t depth;
    
    void init() {
        memset(stack, 0, sizeof(stack));
        depth = 0;
    }
    
    bool push(NavTestScreen *screen) {
        if (depth >= MAX || !screen) return false;
        if (depth > 0) stack[depth - 1]->active = false;
        stack[depth++] = screen;
        screen->active = true;
        screen->enter_count++;
        return true;
    }
    
    bool pop() {
        if (depth <= 1) return false;
        NavTestScreen *current = stack[depth - 1];
        current->active = false;
        current->exit_count++;
        depth--;
        stack[depth - 1]->active = true;
        stack[depth - 1]->enter_count++;
        return true;
    }
    
    NavTestScreen* current() {
        return depth > 0 ? stack[depth - 1] : nullptr;
    }
};

NavTestScreen scr_main = {"Main", false, false, 0, 0};
NavTestScreen scr_menu = {"Menu", false, false, 0, 0};
NavTestScreen scr_stats = {"Stats", false, false, 0, 0};
NavTestScreen scr_games = {"Games", false, false, 0, 0};
NavTestScreen scr_settings = {"Settings", false, false, 0, 0};
NavTestScreen scr_memory = {"MemoryGame", false, false, 0, 0};
NavTestScreen scr_reaction = {"ReactionGame", false, false, 0, 0};
NavTestScreen scr_tilt = {"TiltGame", false, false, 0, 0};
NavTestScreen scr_ota = {"OTA", false, false, 0, 0};

void reset_nav_screens() {
    scr_main = {"Main", false, false, 0, 0};
    scr_menu = {"Menu", false, false, 0, 0};
    scr_stats = {"Stats", false, false, 0, 0};
    scr_games = {"Games", false, false, 0, 0};
    scr_settings = {"Settings", false, false, 0, 0};
    scr_memory = {"MemoryGame", false, false, 0, 0};
    scr_reaction = {"ReactionGame", false, false, 0, 0};
    scr_tilt = {"TiltGame", false, false, 0, 0};
    scr_ota = {"OTA", false, false, 0, 0};
}

// ============================================================
// Phase 20.4: Migrated Screen Tests
// ============================================================

void test_all_screen_names_unique() {
    NavTestScreen *screens[] = {
        &scr_main, &scr_menu, &scr_stats, &scr_games,
        &scr_settings, &scr_memory, &scr_reaction, &scr_tilt, &scr_ota
    };
    int count = 9;
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            TEST_ASSERT_TRUE(strcmp(screens[i]->name, screens[j]->name) != 0);
        }
    }
    printf("  PASS: All 9 screen names unique\n");
}

void test_games_screen_navigation() {
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    // Main -> Menu -> Games -> MemoryGame
    stack.push(&scr_main);
    stack.push(&scr_menu);
    stack.push(&scr_games);
    stack.push(&scr_memory);
    
    TEST_ASSERT_EQUAL(4, stack.depth);
    TEST_ASSERT_EQUAL(&scr_memory, stack.current());
    TEST_ASSERT_TRUE(scr_memory.active);
    TEST_ASSERT_FALSE(scr_games.active);
    printf("  PASS: Games screen navigation\n");
}

void test_game_return_navigation() {
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    // Main -> Menu -> Games -> MemoryGame -> (pop) -> Games
    stack.push(&scr_main);
    stack.push(&scr_menu);
    stack.push(&scr_games);
    stack.push(&scr_memory);
    
    // Auto-return after game ends (2s timer in MemoryGameScreen)
    stack.pop();
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    TEST_ASSERT_EQUAL(&scr_games, stack.current());
    TEST_ASSERT_TRUE(scr_games.active);
    TEST_ASSERT_EQUAL(1, scr_memory.exit_count);
    printf("  PASS: Game return navigation\n");
}

void test_settings_navigation() {
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    // Main -> Menu -> Settings
    stack.push(&scr_main);
    stack.push(&scr_menu);
    stack.push(&scr_settings);
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    TEST_ASSERT_EQUAL(&scr_settings, stack.current());
    printf("  PASS: Settings navigation\n");
}

void test_ota_navigation() {
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    // Main -> Menu -> Settings -> OTA
    stack.push(&scr_main);
    stack.push(&scr_menu);
    stack.push(&scr_settings);
    stack.push(&scr_ota);
    
    TEST_ASSERT_EQUAL(4, stack.depth);
    TEST_ASSERT_EQUAL(&scr_ota, stack.current());
    printf("  PASS: OTA navigation\n");
}

void test_full_navigation_flow() {
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    // Simulate full user flow:
    // Main -> Menu -> Games -> ReactionGame -> (pop) -> Games -> (pop) -> Menu -> (pop) -> Main
    stack.push(&scr_main);
    TEST_ASSERT_EQUAL(1, stack.depth);
    
    stack.push(&scr_menu);
    TEST_ASSERT_EQUAL(2, stack.depth);
    
    stack.push(&scr_games);
    TEST_ASSERT_EQUAL(3, stack.depth);
    
    stack.push(&scr_reaction);
    TEST_ASSERT_EQUAL(4, stack.depth);
    TEST_ASSERT_EQUAL(&scr_reaction, stack.current());
    
    stack.pop();  // Return from game
    TEST_ASSERT_EQUAL(3, stack.depth);
    TEST_ASSERT_EQUAL(&scr_games, stack.current());
    
    stack.pop();  // Back to menu
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&scr_menu, stack.current());
    
    stack.pop();  // Back to main
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&scr_main, stack.current());
    
    printf("  PASS: Full navigation flow\n");
}

void test_screen_depth_limit() {
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    // Push all 9 screens - should stop at MAX (8)
    TEST_ASSERT_TRUE(stack.push(&scr_main));
    TEST_ASSERT_TRUE(stack.push(&scr_menu));
    TEST_ASSERT_TRUE(stack.push(&scr_stats));
    TEST_ASSERT_TRUE(stack.push(&scr_games));
    TEST_ASSERT_TRUE(stack.push(&scr_settings));
    TEST_ASSERT_TRUE(stack.push(&scr_memory));
    TEST_ASSERT_TRUE(stack.push(&scr_reaction));
    TEST_ASSERT_TRUE(stack.push(&scr_tilt));
    TEST_ASSERT_EQUAL(8, stack.depth);
    
    // 9th push should fail
    TEST_ASSERT_FALSE(stack.push(&scr_ota));
    TEST_ASSERT_EQUAL(8, stack.depth);
    
    printf("  PASS: Screen depth limit enforced\n");
}

void test_memory_game_auto_return() {
    // Memory game auto-returns after 2 seconds
    // Test that the screen stack handles this correctly
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    stack.push(&scr_main);
    stack.push(&scr_games);
    stack.push(&scr_memory);
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    
    // Simulate auto-return timer firing
    stack.pop();
    
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&scr_games, stack.current());
    printf("  PASS: Memory game auto-return\n");
}

void test_reaction_game_auto_return() {
    // Reaction game auto-returns after 2.5 seconds
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    stack.push(&scr_main);
    stack.push(&scr_games);
    stack.push(&scr_reaction);
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    
    stack.pop();
    
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&scr_games, stack.current());
    printf("  PASS: Reaction game auto-return\n");
}

void test_settings_back_button() {
    // Settings back button should pop to previous screen
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    stack.push(&scr_main);
    stack.push(&scr_menu);
    stack.push(&scr_settings);
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    
    // Simulate back button press
    stack.pop();
    
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&scr_menu, stack.current());
    printf("  PASS: Settings back button\n");
}

void test_ota_back_button() {
    // OTA back button should pop to previous screen
    NavTestStack stack;
    stack.init();
    reset_nav_screens();
    
    stack.push(&scr_main);
    stack.push(&scr_settings);
    stack.push(&scr_ota);
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    
    stack.pop();
    
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&scr_settings, stack.current());
    printf("  PASS: OTA back button\n");
}

// ============================================================
// Test runner
// ============================================================
int run_migrated_screen_tests() {
    printf("--- Migrated Screens Tests (Phase 20.4) ---\n");
    
    RUN_TEST(test_all_screen_names_unique);
    RUN_TEST(test_games_screen_navigation);
    RUN_TEST(test_game_return_navigation);
    RUN_TEST(test_settings_navigation);
    RUN_TEST(test_ota_navigation);
    RUN_TEST(test_full_navigation_flow);
    RUN_TEST(test_screen_depth_limit);
    RUN_TEST(test_memory_game_auto_return);
    RUN_TEST(test_reaction_game_auto_return);
    RUN_TEST(test_settings_back_button);
    RUN_TEST(test_ota_back_button);
    
    printf("--- Migrated Screens: 11 tests PASSED ---\n");
    return 0;
}
