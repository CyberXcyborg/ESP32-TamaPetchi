// ============================================================
// SoundPackV2.cpp — WAV-based Sound Pack System
// Phase 21.3: Sound System v2
// ============================================================

#include "drivers/SoundPackV2.h"
#include "drivers/WavPlayer.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// ============================================================
// Singleton
// ============================================================
SoundPackManager& SoundPackManager::getInstance() {
    static SoundPackManager instance;
    return instance;
}

SoundPackManager::SoundPackManager()
    : _initialized(false), _volume(50) {
    memset(_activePack, 0, sizeof(_activePack));
    memset(&_manifest, 0, sizeof(_manifest));
}

SoundPackManager::~SoundPackManager() {
    // No cleanup needed
}

// ============================================================
// Public API
// ============================================================
bool SoundPackManager::begin() {
    if (_initialized) return true;

    // Initialize WAV player
    if (!WavPlayer::getInstance().begin()) {
        DEBUG_PRINTLN("[SoundPackV2] WavPlayer init failed");
        return false;
    }

    // Ensure soundpacks directory exists
    if (!LittleFS.exists(SOUNDPACK_V2_DIR)) {
        LittleFS.mkdir(SOUNDPACK_V2_DIR);
    }

    // Load previously active pack
    if (LittleFS.exists(SOUNDPACK_V2_ACTIVE_FILE)) {
        File f = LittleFS.open(SOUNDPACK_V2_ACTIVE_FILE, "r");
        if (f) {
            String name = f.readStringUntil('\n');
            name.trim();
            f.close();
            if (name.length() > 0) {
                strncpy(_activePack, name.c_str(), SOUNDPACK_V2_NAME_LEN - 1);
                loadPack(_activePack);
            }
        }
    }

    // If no active pack, try to load "default"
    if (strlen(_activePack) == 0) {
        strncpy(_activePack, "default", SOUNDPACK_V2_NAME_LEN - 1);
        if (!loadPack("default")) {
            DEBUG_PRINTLN("[SoundPackV2] No default pack found");
        }
    }

    WavPlayer::getInstance().setVolume(_volume);
    _initialized = true;
    DEBUG_PRINTLN("[SoundPackV2] Initialized");
    return true;
}

int SoundPackManager::getPackList(char names[][SOUNDPACK_V2_NAME_LEN], int maxPacks) {
    int count = 0;

    File dir = LittleFS.open(SOUNDPACK_V2_DIR);
    if (!dir) return 0;

    File entry = dir.openNextFile();
    while (entry && count < maxPacks) {
        if (entry.isDirectory()) {
            strncpy(names[count], entry.name(), SOUNDPACK_V2_NAME_LEN - 1);
            names[count][SOUNDPACK_V2_NAME_LEN - 1] = '\0';
            count++;
        }
        entry = dir.openNextFile();
    }
    dir.close();

    return count;
}

bool SoundPackManager::loadPack(const char* path) {
    if (!path) return false;

    char manifestPath[64];
    snprintf(manifestPath, sizeof(manifestPath), "%s/%s/manifest.json", SOUNDPACK_V2_DIR, path);

    SoundPackManifest manifest;
    if (!parseManifest(manifestPath, manifest)) {
        DEBUG_PRINTF("[SoundPackV2] Failed to parse manifest: %s\n", manifestPath);
        return false;
    }

    // Validate all WAV files exist
    for (int i = 0; i < manifest.soundCount; i++) {
        char wavPath[96];
        snprintf(wavPath, sizeof(wavPath), "%s/%s/%s", SOUNDPACK_V2_DIR, path, manifest.sounds[i].filename);
        if (!LittleFS.exists(wavPath)) {
            DEBUG_PRINTF("[SoundPackV2] Missing WAV: %s\n", wavPath);
            return false;
        }
    }

    // Pack is valid — activate it
    memcpy(&_manifest, &manifest, sizeof(SoundPackManifest));
    strncpy(_activePack, path, SOUNDPACK_V2_NAME_LEN - 1);

    // Save active pack
    File f = LittleFS.open(SOUNDPACK_V2_ACTIVE_FILE, "w");
    if (f) {
        f.println(path);
        f.close();
    }

    DEBUG_PRINTF("[SoundPackV2] Loaded pack: %s (%d sounds)\n", path, manifest.soundCount);
    return true;
}

bool SoundPackManager::play(const char* soundName) {
    if (!_initialized || !soundName) return false;

    char wavPath[96];
    if (!findSound(soundName, wavPath, sizeof(wavPath))) {
        DEBUG_PRINTF("[SoundPackV2] Sound not found: %s\n", soundName);
        return false;
    }

    return WavPlayer::getInstance().play(wavPath) == WAV_OK;
}

void SoundPackManager::stop() {
    WavPlayer::getInstance().stop();
}

void SoundPackManager::setVolume(uint8_t vol) {
    _volume = (vol > 100) ? 100 : vol;
    WavPlayer::getInstance().setVolume(_volume);
}

uint8_t SoundPackManager::getVolume() const {
    return _volume;
}

String SoundPackManager::getActivePack() const {
    return String(_activePack);
}

String SoundPackManager::getPackListJson() const {
    char names[SOUNDPACK_MAX_PACKS][SOUNDPACK_V2_NAME_LEN];
    int count = getPackList(names, SOUNDPACK_MAX_PACKS);

    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.createNestedArray("packs");

    for (int i = 0; i < count; i++) {
        JsonObject pack = arr.createNestedObject();
        pack["name"] = names[i];
        pack["isActive"] = (strcmp(names[i], _activePack) == 0);
    }

    doc["active"] = _activePack;
    doc["count"] = count;

    String result;
    serializeJson(doc, result);
    return result;
}

String SoundPackManager::getActivePackJson() const {
    StaticJsonDocument<256> doc;
    doc["name"] = _activePack;
    doc["success"] = true;
    doc["volume"] = _volume;
    doc["soundCount"] = _manifest.soundCount;

    String result;
    serializeJson(doc, result);
    return result;
}

bool SoundPackManager::isInitialized() const {
    return _initialized;
}

// ============================================================
// Private Methods
// ============================================================
bool SoundPackManager::parseManifest(const char* path, SoundPackManifest& manifest) {
    File f = LittleFS.open(path, "r");
    if (!f) return false;

    String content = f.readString();
    f.close();

    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) return false;

    memset(&manifest, 0, sizeof(manifest));

    const char* name = doc["name"] | "unknown";
    strncpy(manifest.name, name, SOUNDPACK_V2_NAME_LEN - 1);

    const char* version = doc["version"] | "1.0";
    strncpy(manifest.version, version, sizeof(manifest.version) - 1);

    const char* displayName = doc["displayName"] | name;
    strncpy(manifest.displayName, displayName, SOUNDPACK_V2_NAME_LEN - 1);

    JsonObject sounds = doc["sounds"];
    if (sounds) {
        for (JsonPair kv : sounds) {
            if (manifest.soundCount >= SOUNDPACK_MAX_SOUNDS) break;
            strncpy(manifest.sounds[manifest.soundCount].name, kv.key().c_str(), SOUNDPACK_V2_NAME_LEN - 1);
            strncpy(manifest.sounds[manifest.soundCount].filename, kv.value().as<const char*>(), 47);
            manifest.soundCount++;
        }
    }

    return true;
}

bool SoundPackManager::findSound(const char* name, char* outPath, size_t pathLen) {
    for (int i = 0; i < _manifest.soundCount; i++) {
        if (strcmp(_manifest.sounds[i].name, name) == 0) {
            snprintf(outPath, pathLen, "%s/%s/%s", SOUNDPACK_V2_DIR, _activePack, _manifest.sounds[i].filename);
            return true;
        }
    }
    return false;
}
