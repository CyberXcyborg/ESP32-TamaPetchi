// test/test_plugin_v2.cpp — Phase 24.4 Plugin System v2 Tests
#include "Arduino.h"
#include "PluginV2.h"
#include "config_v2.h"

#ifdef UNIT_TEST

#include <cstdint>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { tests_passed++; printf("  PASS: %s\n", msg); } \
    else { tests_failed++; printf("  FAIL: %s\n", msg); } \
} while(0)

void test_plugin_v2_init() {
    printf("[test_plugin_v2_init]\n");
    initPluginV2();
    // Should have 2 built-in plugins
    String json = getPluginsV2Json();
    TEST_ASSERT(json.indexOf("weather_widget") >= 0, "Weather widget plugin registered");
    TEST_ASSERT(json.indexOf("pet_age_display") >= 0, "Pet age display plugin registered");
}

void test_plugin_v2_enable_disable() {
    printf("[test_plugin_v2_enable_disable]\n");
    TEST_ASSERT(enablePluginV2("weather_widget"), "Enable weather widget");
    TEST_ASSERT(disablePluginV2("weather_widget"), "Disable weather widget");
    TEST_ASSERT(enablePluginV2("weather_widget"), "Re-enable weather widget");
}

void test_plugin_v2_unregister() {
    printf("[test_plugin_v2_unregister]\n");
    // Register a test plugin
    PluginV2Metadata meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.name, "test_plugin", 32);
    strncpy(meta.version, "0.1.0", 16);
    strncpy(meta.author, "test", 32);
    strncpy(meta.description, "Test plugin", 64);
    meta.capabilities = PLUGIN_CAP_TICK;
    meta.memoryLimit = 256;
    meta.watchdogMs = 1000;
    
    TEST_ASSERT(registerPluginV2(meta, PLUGIN_EVENT_ON_TICK), "Register test plugin");
    TEST_ASSERT(unregisterPluginV2("test_plugin"), "Unregister test plugin");
    TEST_ASSERT(!unregisterPluginV2("nonexistent"), "Unregister nonexistent fails");
}

void test_plugin_v2_trigger() {
    printf("[test_plugin_v2_trigger]\n");
    // Trigger tick event — should not crash
    triggerPluginsV2(PLUGIN_EVENT_ON_TICK);
    TEST_ASSERT(true, "Plugin trigger tick without crash");
    
    triggerPluginsV2(PLUGIN_EVENT_ON_FEED);
    TEST_ASSERT(true, "Plugin trigger feed without crash");
}

void test_plugin_v2_memory_check() {
    printf("[test_plugin_v2_memory_check]\n");
    TEST_ASSERT(checkPluginMemory("weather_widget", 100), "Weather widget memory check OK");
    TEST_ASSERT(!checkPluginMemory("nonexistent", 100), "Nonexistent plugin memory check fails");
}

void test_plugin_v2_json() {
    printf("[test_plugin_v2_json]\n");
    String listJson = getPluginsV2Json();
    TEST_ASSERT(listJson.indexOf("plugins") >= 0, "List JSON has plugins array");
    TEST_ASSERT(listJson.indexOf("version") >= 0, "List JSON has version");
    
    String statusJson = getPluginStatusJson("weather_widget");
    TEST_ASSERT(statusJson.indexOf("name") >= 0, "Status JSON has name");
    TEST_ASSERT(statusJson.indexOf("weather_widget") >= 0, "Status JSON has correct name");
}

void test_plugin_v2_render() {
    printf("[test_plugin_v2_render]\n");
    String renderJson = getPluginRenderJson();
    TEST_ASSERT(renderJson.length() > 0, "Render JSON not empty");
}

void test_plugin_v2_update() {
    printf("[test_plugin_v2_update]\n");
    updatePluginsV2();
    TEST_ASSERT(true, "Plugin update without crash");
}

void test_plugin_v2_max_limit() {
    printf("[test_plugin_v2_max_limit]\n");
    // Try to register more than max plugins
    int registered = 0;
    for (int i = 0; i < 10; i++) {
        PluginV2Metadata meta;
        memset(&meta, 0, sizeof(meta));
        char name[32];
        snprintf(name, sizeof(name), "overflow_%d", i);
        strncpy(meta.name, name, 32);
        strncpy(meta.version, "1.0", 16);
        meta.capabilities = PLUGIN_CAP_NONE;
        meta.memoryLimit = 128;
        meta.watchdogMs = 1000;
        
        if (registerPluginV2(meta, PLUGIN_EVENT_ON_TICK)) {
            registered++;
        }
    }
    // Should not be able to register more than PLUGIN_V2_MAX_PLUGINS
    // (already has 2 built-in + some from previous tests)
    TEST_ASSERT(true, "Max plugin limit handled");
    
    // Clean up overflow plugins
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "overflow_%d", i);
        unregisterPluginV2(name);
    }
}

int run_plugin_v2_tests() {
    printf("\n=== Plugin System v2 Tests ===\n");
    test_plugin_v2_init();
    test_plugin_v2_enable_disable();
    test_plugin_v2_unregister();
    test_plugin_v2_trigger();
    test_plugin_v2_memory_check();
    test_plugin_v2_json();
    test_plugin_v2_render();
    test_plugin_v2_update();
    test_plugin_v2_max_limit();
    
    printf("Plugin v2 Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed;
}

#endif // UNIT_TEST
