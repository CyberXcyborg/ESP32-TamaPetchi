// ============================================================
// SoundPackV2.h — WAV-based Sound Pack System
// Phase 21.3: Sound System v2
// ============================================================
// Sound packs are directories of WAV files with JSON manifest.
// Multiple packs in /soundpacks/ directory.
// Active pack selectable in SettingsScreen.
// ============================================================

#ifndef SOUND_PACK_V2_H
#define SOUND_PACK_V2_H

#include <Arduino.h>
#include "config_v2.h"

#define SOUNDPACK_V2_DIR        "/soundpacks"
#define SOUNDPACK_V2_ACTIVE_FILE "/soundpacks/active.txt"
#define SOUNDPACK_V2_NAME_LEN   32
#define SOUNDPACK_MAX_PACKS     8
#define SOUNDPACK_MAX_SOUNDS    32

// Sound pack manifest structure
struct SoundPackManifest {
    char name[SOUNDPACK_V2_NAME_LEN];
    char version[16];
    char displayName[SOUNDPACK_V2_NAME_LEN];
    int soundCount;
    struct SoundMapping {
        char name[SOUNDPACK_V2_NAME_LEN];
        char filename[48];
    } sounds[SOUNDPACK_MAX_SOUNDS];
};

class SoundPackManager {
public:
    static SoundPackManager& getInstance();

    // Initialize — load default/previous pack
    bool begin();

    // Get list of available sound packs
    int getPackList(char names[][SOUNDPACK_V2_NAME_LEN], int maxPacks);

    // Load a sound pack by directory name
    bool loadPack(const char* path);

    // Play a sound by name from active pack
    bool play(const char* soundName);

    // Stop current playback
    void stop();

    // Set global volume (0-100)
    void setVolume(uint8_t vol);

    // Get global volume
    uint8_t getVolume() const;

    // Get active pack name
    String getActivePack() const;

    // Get pack list as JSON
    String getPackListJson() const;

    // Get active pack info as JSON
    String getActivePackJson() const;

    // Check if initialized
    bool isInitialized() const;

private:
    SoundPackManager();
    ~SoundPackManager();
    SoundPackManager(const SoundPackManager&) = delete;
    SoundPackManager& operator=(const SoundPackManager&) = delete;

    // Parse manifest JSON from file
    bool parseManifest(const char* path, SoundPackManifest& manifest);

    // Find sound in active pack
    bool findSound(const char* name, char* outPath, size_t pathLen);

    bool _initialized;
    uint8_t _volume;
    char _activePack[SOUNDPACK_V2_NAME_LEN];
    SoundPackManifest _manifest;
};

#endif // SOUND_PACK_V2_H
