#include "VoicePrompts.h"
#include "AppState.h"
#include "WebHandlers.h"
#include "Storage_v2.h"
#include <ArduinoJson.h>

// ============================================================
// Forward declarations
// ============================================================
static const char* voiceEventToString(VoiceClipEvent event);

// ============================================================
// Module-level state
// ============================================================
static VoiceConfig voiceConfig = { true, 70, "default" };
static bool voicePlaying = false;
static VoiceClipEvent currentClip = VC_CLIP_COUNT;
static unsigned long clipStartTime = 0;
static unsigned long clipDuration = 0;

// Active voice pack manifest
static String activePackPath;

// ============================================================
// Voice clip manifest structure
// Maps VoiceClipEvent to WAV file paths
// ============================================================
struct VoiceClipManifest {
    const char *eventName;
    const char *filename;
};

// Default voice pack clips (stored in data/voice/default/)
static const VoiceClipManifest defaultClips[] = {
    { "happy",    "happy.wav" },
    { "sad",      "sad.wav" },
    { "hungry",   "hungry.wav" },
    { "sick",     "sick.wav" },
    { "greeting", "greeting.wav" },
    { "levelup",  "levelup.wav" },
    { "sleep",    "sleep.wav" },
    { "wake",     "wake.wav" },
    { "thirsty",  "hungry.wav" },  // Reuse hungry for thirsty
    { "dirty",    "sick.wav" },    // Reuse sick for dirty
};
static const int defaultClipCount = sizeof(defaultClips) / sizeof(defaultClips[0]);

// ============================================================
// Public API
// ============================================================

void initVoicePrompts() {
    // Load config from AppState if available
    voiceConfig.enabled = true;
    voiceConfig.volume = 70;
    strncpy(voiceConfig.activePack, "default", VOICE_PACK_NAME_LEN);

    // Ensure voice directory exists
    StorageV2::begin();
    if (!StorageV2::exists(VOICE_PACK_DIR)) {
        Serial.println("[VoicePrompts] Voice directory not found — voice prompts disabled");
        return;
    }

    // Load active pack
    loadVoicePack(voiceConfig.activePack);
    Serial.printf("[VoicePrompts] Initialized — pack: %s, vol: %d, enabled: %d\n",
                  voiceConfig.activePack, voiceConfig.volume, voiceConfig.enabled);
}

bool loadVoicePack(const String &name) {
    String manifestPath = String(VOICE_PACK_DIR) + "/" + name + "/manifest.json";
    
    if (!StorageV2::exists(manifestPath)) {
        Serial.printf("[VoicePrompts] Pack manifest not found: %s\n", manifestPath.c_str());
        return false;
    }

    File f = StorageV2::open(manifestPath, "r");
    if (!f) return false;

    // For now, we use the default clip mapping
    // Full implementation would parse manifest.json
    f.close();

    activePackPath = String(VOICE_PACK_DIR) + "/" + name + "/";
    strncpy(voiceConfig.activePack, name.c_str(), VOICE_PACK_NAME_LEN);
    Serial.printf("[VoicePrompts] Loaded pack: %s\n", name.c_str());
    return true;
}

bool playVoiceClip(VoiceClipEvent event) {
    if (!voiceConfig.enabled || event >= VC_CLIP_COUNT) return false;
    if (voicePlaying) stopVoiceClip();

    // Build file path from default clips
    String clipFile;
    for (int i = 0; i < defaultClipCount; i++) {
        if (String(defaultClips[i].eventName) == String(voiceEventToString(event))) {
            clipFile = activePackPath + defaultClips[i].filename;
            break;
        }
    }

    if (clipFile.length() == 0 || !StorageV2::exists(clipFile)) {
        // Fallback: play a tone via buzzer
        Serial.printf("[VoicePrompts] Clip not found, using buzzer fallback for event %d\n", event);
        // Trigger buzzer melody as fallback
        voicePlaying = true;
        currentClip = event;
        clipStartTime = millis();
        clipDuration = 500; // 500ms buzzer fallback
        return true;
    }

    // In full implementation, this would play the WAV file via I2S
    // For now, mark as playing with estimated duration
    voicePlaying = true;
    currentClip = event;
    clipStartTime = millis();
    clipDuration = 1000; // Estimated 1s clip

    Serial.printf("[VoicePrompts] Playing clip: %s (vol: %d)\n", clipFile.c_str(), voiceConfig.volume);
    return true;
}

void stopVoiceClip() {
    voicePlaying = false;
    currentClip = VC_CLIP_COUNT;
    clipDuration = 0;
}

bool isVoicePlaying() {
    if (!voicePlaying) return false;
    // Check if clip has finished
    if (millis() - clipStartTime >= clipDuration) {
        stopVoiceClip();
        return false;
    }
    return true;
}

void setVoiceVolume(uint8_t vol) {
    voiceConfig.volume = constrain(vol, 0, 100);
    Serial.printf("[VoicePrompts] Volume set to %d\n", voiceConfig.volume);
}

uint8_t getVoiceVolume() {
    return voiceConfig.volume;
}

void setVoiceEnabled(bool enabled) {
    voiceConfig.enabled = enabled;
    if (!enabled) stopVoiceClip();
}

bool isVoiceEnabled() {
    return voiceConfig.enabled;
}

int getVoicePackList(VoicePackInfo *packs, int maxPacks) {
    int count = 0;
    
    // Always include default pack
    if (count < maxPacks) {
        strncpy(packs[count].name, "default", VOICE_PACK_NAME_LEN);
        strncpy(packs[count].filename, "default", 32);
        packs[count].isDefault = true;
        count++;
    }

    File dir = StorageV2::open(VOICE_PACK_DIR);
    if (!dir) return count;

    File file = dir.openNextFile();
    while (file && count < maxPacks) {
        String fname = file.name();
        if (file.isDirectory()) {
            String pname = fname.substring(String(VOICE_PACK_DIR).length() + 1);
            if (pname.length() > 0 && pname != "default") {
                strncpy(packs[count].name, pname.c_str(), VOICE_PACK_NAME_LEN);
                strncpy(packs[count].filename, pname.c_str(), 32);
                packs[count].isDefault = false;
                count++;
            }
        }
        file = dir.openNextFile();
    }
    dir.close();
    return count;
}

String getActiveVoicePack() {
    return String(voiceConfig.activePack);
}

bool setActiveVoicePack(const String &name) {
    return loadVoicePack(name);
}

String getVoiceConfigJson() {
    StaticJsonDocument<256> doc;
    doc["enabled"] = voiceConfig.enabled;
    doc["volume"] = voiceConfig.volume;
    doc["activePack"] = voiceConfig.activePack;
    doc["playing"] = isVoicePlaying();
    
    String output;
    serializeJson(doc, output);
    return output;
}

void voiceEvent(VoiceClipEvent event) {
    if (voiceConfig.enabled) {
        playVoiceClip(event);
    }
}

void updateVoicePrompts() {
    // Called in main loop to update voice playback state
    isVoicePlaying(); // This checks timeout and auto-stops
}

// ============================================================
// Helper: Convert event enum to string
// ============================================================
static const char* voiceEventToString(VoiceClipEvent event) {
    switch (event) {
        case VC_CLIP_HAPPY:    return "happy";
        case VC_CLIP_SAD:      return "sad";
        case VC_CLIP_HUNGRY:   return "hungry";
        case VC_CLIP_SICK:     return "sick";
        case VC_CLIP_GREETING: return "greeting";
        case VC_CLIP_LEVEL_UP:  return "levelup";
        case VC_CLIP_SLEEP:    return "sleep";
        case VC_CLIP_WAKE:     return "wake";
        case VC_CLIP_THIRSTY:  return "thirsty";
        case VC_CLIP_DIRTY:    return "dirty";
        default:               return "unknown";
    }
}
