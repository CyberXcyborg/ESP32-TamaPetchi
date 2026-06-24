// test/test_voice_prompts.cpp — Phase 24.1 Voice Prompts System Tests
#include "Arduino.h"
#include "VoicePrompts.h"
#include "config_v2.h"

#ifdef UNIT_TEST

#include <cstdint>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { tests_passed++; printf("  PASS: %s\n", msg); } \
    else { tests_failed++; printf("  FAIL: %s\n", msg); } \
} while(0)

void test_voice_init() {
    printf("[test_voice_init]\n");
    initVoicePrompts();
    TEST_ASSERT(isVoiceEnabled(), "Voice enabled by default");
    TEST_ASSERT(getVoiceVolume() == 70, "Default volume is 70");
}

void test_voice_volume() {
    printf("[test_voice_volume]\n");
    setVoiceVolume(50);
    TEST_ASSERT(getVoiceVolume() == 50, "Volume set to 50");
    setVoiceVolume(150); // Should be clamped
    TEST_ASSERT(getVoiceVolume() == 100, "Volume clamped to 100");
    setVoiceVolume(70); // Reset
}

void test_voice_enable_disable() {
    printf("[test_voice_enable_disable]\n");
    setVoiceEnabled(false);
    TEST_ASSERT(!isVoiceEnabled(), "Voice disabled");
    TEST_ASSERT(!playVoiceClip(VC_CLIP_HAPPY), "Cannot play when disabled");
    setVoiceEnabled(true);
    TEST_ASSERT(isVoiceEnabled(), "Voice re-enabled");
}

void test_voice_pack_list() {
    printf("[test_voice_pack_list]\n");
    VoicePackInfo packs[4];
    int count = getVoicePackList(packs, 4);
    TEST_ASSERT(count >= 1, "At least 1 voice pack available");
    if (count >= 1) {
        TEST_ASSERT(strcmp(packs[0].name, "default") == 0, "Default pack exists");
    }
}

void test_voice_active_pack() {
    printf("[test_voice_active_pack]\n");
    String active = getActiveVoicePack();
    TEST_ASSERT(active == "default", "Active pack is default");
    TEST_ASSERT(setActiveVoicePack("default"), "Can set active pack to default");
}

void test_voice_config_json() {
    printf("[test_voice_config_json]\n");
    String json = getVoiceConfigJson();
    TEST_ASSERT(json.length() > 0, "Config JSON not empty");
    TEST_ASSERT(json.indexOf("enabled") >= 0, "Config contains 'enabled'");
    TEST_ASSERT(json.indexOf("volume") >= 0, "Config contains 'volume'");
}

void test_voice_clip_events() {
    printf("[test_voice_clip_events]\n");
    // Test all clip events can be called without crash
    for (int i = 0; i < (int)VC_CLIP_COUNT; i++) {
        VoiceClipEvent evt = (VoiceClipEvent)i;
        // These may fail (no WAV files) but should not crash
        playVoiceClip(evt);
        stopVoiceClip();
    }
    TEST_ASSERT(true, "All clip events handled without crash");
}

void test_voice_stop() {
    printf("[test_voice_stop]\n");
    stopVoiceClip();
    TEST_ASSERT(!isVoicePlaying(), "Voice stopped");
}

int run_voice_prompt_tests() {
    printf("\n=== Voice Prompt Tests ===\n");
    test_voice_init();
    test_voice_volume();
    test_voice_enable_disable();
    test_voice_pack_list();
    test_voice_active_pack();
    test_voice_config_json();
    test_voice_clip_events();
    test_voice_stop();
    
    printf("Voice Prompt Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed;
}

#endif // UNIT_TEST
