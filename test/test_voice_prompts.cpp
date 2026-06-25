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
    setVoiceVolume(0);
    TEST_ASSERT(getVoiceVolume() == 0, "Volume can be set to 0");
    setVoiceVolume(70); // Reset
    TEST_ASSERT(getVoiceVolume() == 70, "Volume reset to 70");
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
        TEST_ASSERT(packs[0].isDefault, "Default pack marked as default");
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
    TEST_ASSERT(json.indexOf("activePack") >= 0, "Config contains 'activePack'");
    TEST_ASSERT(json.indexOf("i2sAvailable") >= 0, "Config contains 'i2sAvailable'");
    TEST_ASSERT(json.indexOf("buzzerPin") >= 0, "Config contains 'buzzerPin'");
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

// State machine transition tests
void test_voice_state_machine() {
    printf("[test_voice_state_machine]\n");
    
    // State: IDLE -> PLAYING
    stopVoiceClip();
    TEST_ASSERT(!isVoicePlaying(), "Initial state: not playing");
    
    // Play a clip (will use buzzer fallback in test)
    bool started = playVoiceClip(VC_CLIP_HAPPY);
    TEST_ASSERT(started, "playVoiceClip returns true when enabled");
    TEST_ASSERT(isVoicePlaying(), "State: playing after playVoiceClip");
    
    // Stop
    stopVoiceClip();
    TEST_ASSERT(!isVoicePlaying(), "State: stopped after stopVoiceClip");
    
    // Play -> Stop -> Play sequence
    playVoiceClip(VC_CLIP_SAD);
    TEST_ASSERT(isVoicePlaying(), "State: playing again");
    stopVoiceClip();
    TEST_ASSERT(!isVoicePlaying(), "State: stopped again");
    
    // Boundary: invalid event
    TEST_ASSERT(!playVoiceClip(VC_CLIP_COUNT), "Invalid event (VC_CLIP_COUNT) rejected");
    TEST_ASSERT(!playVoiceClip((VoiceClipEvent)100), "Out-of-range event rejected");
}

// Buzzer fallback tests
void test_voice_buzzer_fallback() {
    printf("[test_voice_buzzer_fallback]\n");
    
    // Default buzzer pin
    TEST_ASSERT(getBuzzerPin() == 2, "Default buzzer pin is 2");
    
    // Change buzzer pin
    setBuzzerPin(4);
    TEST_ASSERT(getBuzzerPin() == 4, "Buzzer pin changed to 4");
    
    // Reset
    setBuzzerPin(2);
    TEST_ASSERT(getBuzzerPin() == 2, "Buzzer pin reset to 2");
    
    // I2S not available in native test
    TEST_ASSERT(!isI2SAvailable(), "I2S not available in native test");
}

// Volume control tests
void test_voice_volume_control() {
    printf("[test_voice_volume_control]\n");
    
    // Set various volumes
    setVoiceVolume(0);
    TEST_ASSERT(getVoiceVolume() == 0, "Volume 0 (mute)");
    
    setVoiceVolume(1);
    TEST_ASSERT(getVoiceVolume() == 1, "Volume 1 (min audible)");
    
    setVoiceVolume(50);
    TEST_ASSERT(getVoiceVolume() == 50, "Volume 50 (mid)");
    
    setVoiceVolume(99);
    TEST_ASSERT(getVoiceVolume() == 99, "Volume 99");
    
    setVoiceVolume(100);
    TEST_ASSERT(getVoiceVolume() == 100, "Volume 100 (max)");
    
    // Clamping
    setVoiceVolume(200);
    TEST_ASSERT(getVoiceVolume() == 100, "Volume clamped to 100");
    
    // Reset
    setVoiceVolume(70);
}

// Enable/disable tests
void test_voice_enable_disable_control() {
    printf("[test_voice_enable_disable_control]\n");
    
    // Enable when already enabled
    setVoiceEnabled(true);
    TEST_ASSERT(isVoiceEnabled(), "Enable when already enabled");
    
    // Disable
    setVoiceEnabled(false);
    TEST_ASSERT(!isVoiceEnabled(), "Disable works");
    
    // Play when disabled should fail
    TEST_ASSERT(!playVoiceClip(VC_CLIP_HAPPY), "Play fails when disabled");
    
    // Re-enable
    setVoiceEnabled(true);
    TEST_ASSERT(isVoiceEnabled(), "Re-enable works");
    
    // Play when enabled should succeed
    TEST_ASSERT(playVoiceClip(VC_CLIP_HAPPY), "Play succeeds when enabled");
    stopVoiceClip();
}

// Voice event convenience function test
void test_voice_event() {
    printf("[test_voice_event]\n");
    
    // When enabled
    setVoiceEnabled(true);
    voiceEvent(VC_CLIP_GREETING);
    TEST_ASSERT(isVoicePlaying(), "voiceEvent plays when enabled");
    stopVoiceClip();
    
    // When disabled
    setVoiceEnabled(false);
    voiceEvent(VC_CLIP_GREETING);
    TEST_ASSERT(!isVoicePlaying(), "voiceEvent does nothing when disabled");
    
    // Re-enable
    setVoiceEnabled(true);
}

// Update/task function test
void test_voice_update() {
    printf("[test_voice_update]\n");
    
    // Should not crash
    updateVoicePrompts();
    TEST_ASSERT(true, "updateVoicePrompts does not crash");
    
    // Play and update
    playVoiceClip(VC_CLIP_HAPPY);
    updateVoicePrompts();
    stopVoiceClip();
    TEST_ASSERT(true, "updateVoicePrompts during playback does not crash");
}

// Manifest parsing test (mock JSON)
void test_voice_manifest_parsing() {
    printf("[test_voice_manifest_parsing]\n");
    
    // Test that loadVoicePack works with non-existent pack
    // (should fall back to default clips)
    bool result = loadVoicePack("default");
    TEST_ASSERT(result, "loadVoicePack('default') succeeds");
    
    // Test with non-existent pack name
    result = loadVoicePack("nonexistent_pack");
    TEST_ASSERT(result, "loadVoicePack with unknown name still succeeds (uses defaults)");
}

// Voice clip event enum tests
void test_voice_clip_enum() {
    printf("[test_voice_clip_enum]\n");
    
    // Verify enum values
    TEST_ASSERT(VC_CLIP_HAPPY == 0, "VC_CLIP_HAPPY is 0");
    TEST_ASSERT(VC_CLIP_SAD == 1, "VC_CLIP_SAD is 1");
    TEST_ASSERT(VC_CLIP_HUNGRY == 2, "VC_CLIP_HUNGRY is 2");
    TEST_ASSERT(VC_CLIP_SICK == 3, "VC_CLIP_SICK is 3");
    TEST_ASSERT(VC_CLIP_GREETING == 4, "VC_CLIP_GREETING is 4");
    TEST_ASSERT(VC_CLIP_LEVEL_UP == 5, "VC_CLIP_LEVEL_UP is 5");
    TEST_ASSERT(VC_CLIP_SLEEP == 6, "VC_CLIP_SLEEP is 6");
    TEST_ASSERT(VC_CLIP_WAKE == 7, "VC_CLIP_WAKE is 7");
    TEST_ASSERT(VC_CLIP_THIRSTY == 8, "VC_CLIP_THIRSTY is 8");
    TEST_ASSERT(VC_CLIP_DIRTY == 9, "VC_CLIP_DIRTY is 9");
    TEST_ASSERT(VC_CLIP_COUNT == 10, "VC_CLIP_COUNT is 10");
}

// Voice pack info struct test
void test_voice_pack_info_struct() {
    printf("[test_voice_pack_info_struct]\n");
    
    VoicePackInfo info;
    memset(&info, 0, sizeof(info));
    
    strncpy(info.name, "test_pack", VOICE_PACK_NAME_LEN);
    TEST_ASSERT(strcmp(info.name, "test_pack") == 0, "Pack name set correctly");
    
    strncpy(info.filename, "test_pack", 32);
    TEST_ASSERT(strcmp(info.filename, "test_pack") == 0, "Pack filename set correctly");
    
    info.isDefault = false;
    TEST_ASSERT(!info.isDefault, "Pack isDefault can be false");
    
    info.isDefault = true;
    TEST_ASSERT(info.isDefault, "Pack isDefault can be true");
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
    test_voice_state_machine();
    test_voice_buzzer_fallback();
    test_voice_volume_control();
    test_voice_enable_disable_control();
    test_voice_event();
    test_voice_update();
    test_voice_manifest_parsing();
    test_voice_clip_enum();
    test_voice_pack_info_struct();
    
    printf("Voice Prompt Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed;
}

#endif // UNIT_TEST
