#ifndef VOICEPROMPTS_H
#define VOICEPROMPTS_H

#include <Arduino.h>
#include "config_v2.h"

// ============================================================
// Phase 24.1: Voice Prompts System
// Pet "speaks" status updates via I2S audio (WAV clips).
// Voice packs are collections of WAV files mapped to events.
// Falls back to buzzer PWM tone when I2S is unavailable.
// ============================================================

#define VOICE_PACK_DIR      "/voice"
#define VOICE_PACK_ACTIVE_FILE "/voice/active.txt"
#define MAX_VOICE_PACKS     4
#define VOICE_PACK_NAME_LEN 32
#define VOICE_MAX_CLIPS     16

// Buzzer fallback configuration
#define VOICE_BUZZER_PIN    2     // Default buzzer GPIO
#define VOICE_BUZZER_CHANNEL 0    // LEDC channel for buzzer
#define VOICE_BUZZER_FREQ   2000  // Default tone frequency (Hz)
#define VOICE_BUZZER_DURATION 500 // Default buzzer duration (ms)

// Voice clip events
enum VoiceClipEvent {
    VC_CLIP_HAPPY = 0,
    VC_CLIP_SAD,
    VC_CLIP_HUNGRY,
    VC_CLIP_SICK,
    VC_CLIP_GREETING,
    VC_CLIP_LEVEL_UP,
    VC_CLIP_SLEEP,
    VC_CLIP_WAKE,
    VC_CLIP_THIRSTY,
    VC_CLIP_DIRTY,
    VC_CLIP_COUNT
};

// Voice pack info
struct VoicePackInfo {
    char name[VOICE_PACK_NAME_LEN];
    char filename[32];
    bool isDefault;
};

// Voice prompt configuration
struct VoiceConfig {
    bool enabled;
    uint8_t volume;         // 0-100
    char activePack[VOICE_PACK_NAME_LEN];
    uint8_t buzzerPin;      // GPIO for buzzer fallback
    bool i2sAvailable;      // Whether I2S initialized successfully
};

// Voice clip manifest entry (parsed from JSON)
struct VoiceClipEntry {
    char eventName[32];
    char filename[48];
    uint32_t durationMs;    // Duration in milliseconds
};

// Voice pack manifest (loaded from manifest.json)
struct VoicePackManifest {
    char name[VOICE_PACK_NAME_LEN];
    char version[16];
    VoiceClipEntry clips[VOICE_MAX_CLIPS];
    uint8_t clipCount;
};

// Initialize voice prompts system
void initVoicePrompts();

// Load voice pack by name (loads manifest)
bool loadVoicePack(const String &name);

// Play a voice clip by event
bool playVoiceClip(VoiceClipEvent event);

// Stop current voice playback
void stopVoiceClip();

// Check if voice is currently playing
bool isVoicePlaying();

// Set voice volume (0-100)
void setVoiceVolume(uint8_t vol);

// Get current voice volume
uint8_t getVoiceVolume();

// Enable/disable voice prompts
void setVoiceEnabled(bool enabled);

// Check if voice is enabled
bool isVoiceEnabled();

// Get list of available voice packs
int getVoicePackList(VoicePackInfo *packs, int maxPacks);

// Get active voice pack name
String getActiveVoicePack();

// Set active voice pack by name
bool setActiveVoicePack(const String &name);

// Get voice config as JSON
String getVoiceConfigJson();

// Play event-based voice clip (convenience)
void voiceEvent(VoiceClipEvent event);

// Task update (call in main loop)
void updateVoicePrompts();

// Set buzzer pin for fallback
void setBuzzerPin(uint8_t pin);

// Get current buzzer pin
uint8_t getBuzzerPin();

// Check if I2S is available
bool isI2SAvailable();

#endif // VOICEPROMPTS_H
