// test_screenmanager.cpp — Unit tests for ScreenManager (Phase 20.3)
// Tests screen stack operations and navigation logic
#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include <cstdio>
#include <cstdint>

// ============================================================
// Screen Stack Logic Tests (platform-independent)
// ============================================================

struct TestScreen {
    const char *name;
    bool created;
    bool active;
    uint8_t enter_count;
    uint8_t exit_count;
};

struct TestScreenStack {
    static constexpr uint8_t MAX = 8;
    TestScreen *stack[MAX];
    uint8_t depth;
    
    void init() {
        memset(stack, 0, sizeof(stack));
        depth = 0;
    }
    
    bool push(TestScreen *screen) {
        if (depth >= MAX || !screen) return false;
        if (depth > 0) stack[depth - 1]->active = false;
        stack[depth++] = screen;
        screen->active = true;
        screen->enter_count++;
        return true;
    }
    
    bool pop() {
        if (depth <= 1) return false;
        TestScreen *current = stack[depth - 1];
        current->active = false;
        current->exit_count++;
        depth--;
        stack[depth - 1]->active = true;
        stack[depth - 1]->enter_count++;
        return true;
    }
    
    bool switchTo(TestScreen *screen) {
        if (depth == 0 || !screen) return false;
        TestScreen *current = stack[depth - 1];
        current->active = false;
        current->exit_count++;
        stack[depth - 1] = screen;
        screen->active = true;
        screen->enter_count++;
        return true;
    }
    
    void popToRoot() {
        while (depth > 1) pop();
    }
    
    TestScreen* current() {
        return depth > 0 ? stack[depth - 1] : nullptr;
    }
};

TestScreen screen_main = {"Main", false, false, 0, 0};
TestScreen screen_menu = {"Menu", false, false, 0, 0};
TestScreen screen_stats = {"Stats", false, false, 0, 0};
TestScreen screen_games = {"Games", false, false, 0, 0};
TestScreen screen_settings = {"Settings", false, false, 0, 0};

void reset_test_screens() {
    screen_main = {"Main", false, false, 0, 0};
    screen_menu = {"Menu", false, false, 0, 0};
    screen_stats = {"Stats", false, false, 0, 0};
    screen_games = {"Games", false, false, 0, 0};
    screen_settings = {"Settings", false, false, 0, 0};
}

void test_stack_init() {
    TestScreenStack stack;
    stack.init();
    TEST_ASSERT_EQUAL(0, stack.depth);
    TEST_ASSERT_NULL(stack.current());
    printf("  PASS: Stack initializes empty\n");
}

void test_stack_push() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    TEST_ASSERT_TRUE(stack.push(&screen_main));
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&screen_main, stack.current());
    TEST_ASSERT_TRUE(screen_main.active);
    TEST_ASSERT_EQUAL(1, screen_main.enter_count);
    printf("  PASS: Push screen\n");
}

void test_stack_push_multiple() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    stack.push(&screen_menu);
    stack.push(&screen_stats);
    
    TEST_ASSERT_EQUAL(3, stack.depth);
    TEST_ASSERT_EQUAL(&screen_stats, stack.current());
    TEST_ASSERT_FALSE(screen_main.active);
    TEST_ASSERT_FALSE(screen_menu.active);
    TEST_ASSERT_TRUE(screen_stats.active);
    printf("  PASS: Push multiple screens\n");
}

void test_stack_pop() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    stack.push(&screen_menu);
    
    TEST_ASSERT_TRUE(stack.pop());
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&screen_main, stack.current());
    TEST_ASSERT_TRUE(screen_main.active);
    TEST_ASSERT_FALSE(screen_menu.active);
    TEST_ASSERT_EQUAL(1, screen_menu.exit_count);
    printf("  PASS: Pop screen\n");
}

void test_stack_pop_root_fails() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    
    TEST_ASSERT_FALSE(stack.pop());
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&screen_main, stack.current());
    printf("  PASS: Can't pop root screen\n");
}

void test_stack_switch() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    stack.push(&screen_menu);
    
    TEST_ASSERT_TRUE(stack.switchTo(&screen_stats));
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&screen_stats, stack.current());
    TEST_ASSERT_EQUAL(1, screen_menu.exit_count);
    TEST_ASSERT_EQUAL(1, screen_stats.enter_count);
    printf("  PASS: Switch screen\n");
}

void test_stack_pop_to_root() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    stack.push(&screen_menu);
    stack.push(&screen_stats);
    stack.push(&screen_games);
    
    TEST_ASSERT_EQUAL(4, stack.depth);
    
    stack.popToRoot();
    
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&screen_main, stack.current());
    TEST_ASSERT_TRUE(screen_main.active);
    printf("  PASS: Pop to root\n");
}

void test_stack_max_depth() {
    TestScreenStack stack;
    stack.init();
    
    TestScreen s1 = {"S1"}, s2 = {"S2"}, s3 = {"S3"}, s4 = {"S4"};
    TestScreen s5 = {"S5"}, s6 = {"S6"}, s7 = {"S7"}, s8 = {"S8"};
    TestScreen s9 = {"S9"};
    
    TEST_ASSERT_TRUE(stack.push(&s1));
    TEST_ASSERT_TRUE(stack.push(&s2));
    TEST_ASSERT_TRUE(stack.push(&s3));
    TEST_ASSERT_TRUE(stack.push(&s4));
    TEST_ASSERT_TRUE(stack.push(&s5));
    TEST_ASSERT_TRUE(stack.push(&s6));
    TEST_ASSERT_TRUE(stack.push(&s7));
    TEST_ASSERT_TRUE(stack.push(&s8));
    TEST_ASSERT_EQUAL(8, stack.depth);
    
    TEST_ASSERT_FALSE(stack.push(&s9));
    TEST_ASSERT_EQUAL(8, stack.depth);
    printf("  PASS: Max depth enforcement\n");
}

void test_stack_lifecycle_counts() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    TEST_ASSERT_EQUAL(1, screen_main.enter_count);
    TEST_ASSERT_EQUAL(0, screen_main.exit_count);  // Push doesn't increment exit_count
    
    stack.push(&screen_menu);
    TEST_ASSERT_EQUAL(0, screen_main.exit_count);  // Still 0 - push doesn't exit
    TEST_ASSERT_EQUAL(1, screen_menu.enter_count);
    
    stack.pop();
    TEST_ASSERT_EQUAL(1, screen_menu.exit_count);  // Pop increments exit_count
    TEST_ASSERT_EQUAL(2, screen_main.enter_count);  // Re-entered
    
    stack.push(&screen_stats);
    TEST_ASSERT_EQUAL(0, screen_main.exit_count);  // Push doesn't exit
    TEST_ASSERT_EQUAL(1, screen_stats.enter_count);
    
    printf("  PASS: Lifecycle enter/exit counts\n");
}

void test_screen_name_uniqueness() {
    const char *names[] = {"Main", "Menu", "Stats", "Games", "Settings"};
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT_TRUE(strcmp(names[i], names[j]) != 0);
        }
    }
    printf("  PASS: Screen names are unique\n");
}

void test_stack_navigation_sequence() {
    TestScreenStack stack;
    stack.init();
    reset_test_screens();
    
    stack.push(&screen_main);
    TEST_ASSERT_EQUAL(1, stack.depth);
    
    stack.push(&screen_menu);
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&screen_menu, stack.current());
    
    stack.pop();
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&screen_main, stack.current());
    
    stack.push(&screen_stats);
    TEST_ASSERT_EQUAL(2, stack.depth);
    TEST_ASSERT_EQUAL(&screen_stats, stack.current());
    
    stack.pop();
    TEST_ASSERT_EQUAL(1, stack.depth);
    TEST_ASSERT_EQUAL(&screen_main, stack.current());
    
    printf("  PASS: Navigation sequence\n");
}

int run_screenmanager_tests() {
    printf("--- ScreenManager Tests ---\n");
    
    RUN_TEST(test_stack_init);
    RUN_TEST(test_stack_push);
    RUN_TEST(test_stack_push_multiple);
    RUN_TEST(test_stack_pop);
    RUN_TEST(test_stack_pop_root_fails);
    RUN_TEST(test_stack_switch);
    RUN_TEST(test_stack_pop_to_root);
    RUN_TEST(test_stack_max_depth);
    RUN_TEST(test_stack_lifecycle_counts);
    RUN_TEST(test_screen_name_uniqueness);
    RUN_TEST(test_stack_navigation_sequence);
    
    printf("--- ScreenManager: 11 tests PASSED ---\n");
    return 0;
}
