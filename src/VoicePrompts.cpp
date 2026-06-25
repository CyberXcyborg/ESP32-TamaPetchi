#include "VoicePrompts.h"
#include "Storage_v2.h"
#include <ArduinoJson.h>

#ifdef ENABLE_I2S_AUDIO
#include "drivers/WavPlayer.h"
#endif

// ============================================================
// Forward declarations
// ============================================================
static const char* voiceEventToString(VoiceClipEvent event);
static void playBuzzerTone(VoiceClipEvent event);

// ============================================================
// Module-level state
// ============================================================
static VoiceConfig voiceConfig = { true, 70, "default", VOICE_BUZZER_PIN, false };
static bool voicePlaying = false;
static VoiceClipEvent currentClip = VC_CLIP_COUNT;
static unsigned long clipStartTime = 0;
static unsigned long clipDuration = 0;
static bool usingBuzzer = false;

// Active voice pack manifest
static String activePackPath;
static VoicePackManifest activeManifest;
static bool manifestLoaded = false;

// Default voice pack clips (used when no manifest or as fallback)
static const VoiceClipEntry defaultClips[] = {
    { "happy",    "happy.wav",    1000 },
    { "sad",      "sad.wav",      1000 },
    { "hungry",   "hungry.wav",   1000 },
    { "sick",     "sick.wav",     1000 },
    { "greeting", "greeting.wav", 800 },
    { "levelup",  "levelup.wav",  1200 },
    { "sleep",    "sleep.wav",    1500 },
    { "wake",     "wake.wav",     800 },
    { "thirsty",  "hungry.wav",   1000 },  // Reuse hungry for thirsty
    { "dirty",    "sick.wav",     1000 },  // Reuse sick for dirty
};
static const int defaultClipCount = sizeof(defaultClips) / sizeof(defaultClips[0]);

// ============================================================
// Manifest JSON parsing
// ============================================================
static bool parseManifestJson(const String &jsonContent, VoicePackManifest &manifest) {
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, jsonContent);
    if (err) {
        Serial.printf("[VoicePrompts] JSON parse failed: %s\n", err.c_str());
        return false;
    }

    memset(&manifest, 0, sizeof(manifest));

    // Parse pack name
    const char* name = doc["name"] | "unknown";
    strncpy(manifest.name, name, VOICE_PACK_NAME_LEN - 1);

    // Parse version
    const char* version = doc["version"] | "1.0";
    strncpy(manifest.version, version, sizeof(manifest.version) - 1);

    // Parse clips object: { "happy": { "file": "happy.wav", "duration": 1000 }, ... }
    JsonObject clips = doc["clips"];
    if (clips) {
        for (JsonPair kv : clips) {
            if (manifest.clipCount >= VOICE_MAX_CLIPS) break;
            const char* eventName = kv.key().c_str();
            JsonObject clipData = kv.value().as<JsonObject>();
            if (clipData) {
                strncpy(manifest.clips[manifest.clipCount].eventName, eventName, 31);
                const char* filename = clipData["file"] | "";
                strncpy(manifest.clips[manifest.clipCount].filename, filename, 47);
                manifest.clips[manifest.clipCount].durationMs = clipData["duration"] | 1000;
            } else {
                // Simple string value: just a filename
                strncpy(manifest.clips[manifest.clipCount].eventName, eventName, 31);
                strncpy(manifest.clips[manifest.clipCount].filename, kv.value().as<const char*>(), 47);
                manifest.clips[manifest.clipCount].durationMs = 1000;
            }
            manifest.clipCount++;
        }
    }

    return manifest.clipCount > 0;
}

// ============================================================
// I2S Audio initialization
// ============================================================
static bool initIA2SAudio() {
#ifdef ENABLE_I2S_AUDIO
#ifdef ESP32
    if (!WavPlayer::getInstance().begin()) {
        Serial.println("[VoicePrompts] I2S init failed, will use buzzer fallback");
        return false;
    }
    WavPlayer::getInstance().setVolume(voiceConfig.volume);
    Serial.println("[VoicePrompts] I2S audio initialized");
    return true;
#else
    return false;
#endif
#else
    return false;
#endif
}

// ============================================================
// Buzzer fallback
// ============================================================
static void playBuzzerTone(VoiceClipEvent event) {
    usingBuzzer = true;
    voicePlaying = true;
    currentClip = event;
    clipStartTime = millis();

    // Map events to different frequencies for distinct "voices"
    uint32_t freq = VOICE_BUZZER_FREQ;
    uint32_t duration = VOICE_BUZZER_DURATION;

    switch (event) {
        case VC_CLIP_HAPPY:    freq = 800;  duration = 300; break;
        case VC_CLIP_SAD:      freq = 300;  duration = 600; break;
        case VC_CLIP_HUNGRY:   freq = 500;  duration = 400; break;
        case VC_CLIP_SICK:     freq = 250;  duration = 700; break;
        case VC_CLIP_GREETING: freq = 700;  duration = 250; break;
        case VC_CLIP_LEVEL_UP:  freq = 1000; duration = 500; break;
        case VC_CLIP_SLEEP:    freq = 200;  duration = 1000; break;
        case VC_CLIP_WAKE:     freq = 600;  duration = 350; break;
        case VC_CLIP_THIRSTY:  freq = 450;  duration = 400; break;
        case VC_CLIP_DIRTY:    freq = 350;  duration = 500; break;
        default:               freq = 400;  duration = 500; break;
    }

    clipDuration = duration;

#ifdef ESP32
    // Use LEDC for PWM tone on ESP32
    ledcSetup(VOICE_BUZZER_CHANNEL, freq, 8);  // 8-bit resolution
    ledcAttachPin(voiceConfig.buzzerPin, VOICE_BUZZER_CHANNEL);
    ledcWriteTone(VOICE_BUZZER_CHANNEL, freq);
#else
    // Native test: use Arduino tone() stub
    tone(voiceConfig.buzzerPin, freq, duration);
#endif
}

static void stopBuzzerTone() {
#ifdef ESP32
    ledcWriteTone(VOICE_BUZZER_CHANNEL, 0);
    ledcDetachPin(voiceConfig.buzzerPin);
#else
    noTone(voiceConfig.buzzerPin);
#endif
    usingBuzzer = false;
}

// ============================================================
// WAV playback via I2S
// ============================================================
static bool playWavClip(const char* filePath) {
#ifdef ENABLE_I2S_AUDIO
#ifdef ESP32
    if (!voiceConfig.i2sAvailable) return false;

    WavPlayer::getInstance().setVolume(voiceConfig.volume);
    int result = WavPlayer::getInstance().play(filePath);
    if (result == WAV_OK) {
        voicePlaying = true;
        usingBuzzer = false;
        clipStartTime = millis();
        // Duration will be tracked by isVoicePlaying() polling WavPlayer state
        clipDuration = 0; // Duration unknown, poll WavPlayer::isPlaying()
        return true;
    }
    return false;
#else
    (void)filePath;
    return false;
#endif
#else
    (void)filePath;
    return false;
#endif
}

// ============================================================
// Find clip entry by event name
// ============================================================
static const VoiceClipEntry* findClipEntry(VoiceClipEvent event) {
    const char* eventName = voiceEventToString(event);

    // First check loaded manifest
    if (manifestLoaded) {
        for (int i = 0; i < activeManifest.clipCount; i++) {
            if (strcmp(activeManifest.clips[i].eventName, eventName) == 0) {
                return &activeManifest.clips[i];
            }
        }
    }

    // Fall back to default clips
    for (int i = 0; i < defaultClipCount; i++) {
        if (strcmp(defaultClips[i].eventName, eventName) == 0) {
            return &defaultClips[i];
        }
    }

    return nullptr;
}

// ============================================================
// Public API
// ============================================================

void initVoicePrompts() {
    voiceConfig.enabled = true;
    voiceConfig.volume = 70;
    strncpy(voiceConfig.activePack, "default", VOICE_PACK_NAME_LEN);
    voiceConfig.buzzerPin = VOICE_BUZZER_PIN;
    voiceConfig.i2sAvailable = false;

    // Initialize storage
    StorageV2::begin();

    // Try to initialize I2S audio
    voiceConfig.i2sAvailable = initIA2SAudio();

    // Ensure voice directory exists
    if (!StorageV2::exists(VOICE_PACK_DIR)) {
        Serial.println("[VoicePrompts] Voice directory not found — voice prompts disabled");
        voiceConfig.enabled = false;
        return;
    }

    // Load active pack
    loadVoicePack(voiceConfig.activePack);

    Serial.printf("[VoicePrompts] Initialized — pack: %s, vol: %d, enabled: %d, i2s: %d\n",
                  voiceConfig.activePack, voiceConfig.volume, voiceConfig.enabled,
                  voiceConfig.i2sAvailable);
}

bool loadVoicePack(const String &name) {
    String manifestPath = String(VOICE_PACK_DIR) + "/" + name + "/manifest.json";

    if (!StorageV2::exists(manifestPath)) {
        Serial.printf("[VoicePrompts] Pack manifest not found: %s\n", manifestPath.c_str());
        // Use default clips as fallback
        manifestLoaded = false;
        activePackPath = String(VOICE_PACK_DIR) + "/" + name + "/";
        strncpy(voiceConfig.activePack, name.c_str(), VOICE_PACK_NAME_LEN);
        return true; // Still succeed with default clips
    }

    // Read and parse manifest
    String jsonContent = StorageV2::read(manifestPath.c_str());
    if (jsonContent.length() == 0) {
        Serial.printf("[VoicePrompts] Empty manifest: %s\n", manifestPath.c_str());
        manifestLoaded = false;
        return false;
    }

    VoicePackManifest newManifest;
    if (!parseManifestJson(jsonContent, newManifest)) {
        Serial.printf("[VoicePrompts] Failed to parse manifest: %s\n", manifestPath.c_str());
        manifestLoaded = false;
        return false;
    }

    // Validate WAV files exist (only check, don't fail on missing in case I2S not available)
    activeManifest = newManifest;
    manifestLoaded = true;

    activePackPath = String(VOICE_PACK_DIR) + "/" + name + "/";
    strncpy(voiceConfig.activePack, name.c_str(), VOICE_PACK_NAME_LEN);
    Serial.printf("[VoicePrompts] Loaded pack: %s (%d clips)\n", name.c_str(), activeManifest.clipCount);
    return true;
}

bool playVoiceClip(VoiceClipEvent event) {
    if (!voiceConfig.enabled || event >= VC_CLIP_COUNT) return false;
    if (voicePlaying) stopVoiceClip();

    currentClip = event;

    // Find the clip entry
    const VoiceClipEntry* entry = findClipEntry(event);
    if (!entry) {
        Serial.printf("[VoicePrompts] No clip entry for event %d\n", event);
        // Fall back to buzzer
        playBuzzerTone(event);
        return true;
    }

    // Try I2S WAV playback first
    if (voiceConfig.i2sAvailable) {
        String clipFile = activePackPath + entry->filename;
        if (StorageV2::exists(clipFile.c_str())) {
            if (playWavClip(clipFile.c_str())) {
                Serial.printf("[VoicePrompts] Playing WAV: %s (vol: %d)\n", clipFile.c_str(), voiceConfig.volume);
                return true;
            }
        }
    }

    // Fallback: buzzer tone
    Serial.printf("[VoicePrompts] Using buzzer fallback for event %d (%s)\n", event, entry->eventName);
    playBuzzerTone(event);
    return true;
}

void stopVoiceClip() {
    if (usingBuzzer) {
        stopBuzzerTone();
    } else {
#ifdef ENABLE_I2S_AUDIO
        WavPlayer::getInstance().stop();
#endif
    }
    voicePlaying = false;
    currentClip = VC_CLIP_COUNT;
    clipDuration = 0;
    usingBuzzer = false;
}

bool isVoicePlaying() {
    if (!voicePlaying) return false;

    if (usingBuzzer) {
        // Time-based state machine for buzzer
        if (millis() - clipStartTime >= clipDuration) {
            stopVoiceClip();
            return false;
        }
        return true;
    } else {
#ifdef ENABLE_I2S_AUDIO
        // Poll WavPlayer for I2S playback status
        if (!WavPlayer::getInstance().isPlaying()) {
            voicePlaying = false;
            currentClip = VC_CLIP_COUNT;
            return false;
        }
        return true;
#else
        return false;
#endif
    }
}

void setVoiceVolume(uint8_t vol) {
    voiceConfig.volume = constrain(vol, 0, 100);
    Serial.printf("[VoicePrompts] Volume set to %d\n", voiceConfig.volume);
#ifdef ENABLE_I2S_AUDIO
    if (voiceConfig.i2sAvailable) {
        WavPlayer::getInstance().setVolume(voiceConfig.volume);
    }
#endif
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
    doc["i2sAvailable"] = voiceConfig.i2sAvailable;
    doc["buzzerPin"] = voiceConfig.buzzerPin;

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

void setBuzzerPin(uint8_t pin) {
    voiceConfig.buzzerPin = pin;
    Serial.printf("[VoicePrompts] Buzzer pin set to %d\n", pin);
}

uint8_t getBuzzerPin() {
    return voiceConfig.buzzerPin;
}

bool isI2SAvailable() {
    return voiceConfig.i2sAvailable;
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
