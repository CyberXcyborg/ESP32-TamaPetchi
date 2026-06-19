#ifndef SOUNDPACK_H
#define SOUNDPACK_H

#include <Arduino.h>

// ============================================================
// Sound Pack System (Phase 11.3)
//
// Allows switching between different buzzer melody packs.
// Sound packs are stored as JSON in data/sounds/ directory.
// Each pack defines a name and melody assignments for events.
// ============================================================

#define SOUNDPACK_DIR       "/sounds"
#define SOUNDPACK_ACTIVE_FILE "/sounds/active.txt"
#define MAX_SOUNDPACKS       8
#define SOUNDPACK_NAME_LEN   32

// Sound pack info structure
struct SoundPackInfo {
  char name[SOUNDPACK_NAME_LEN];
  char filename[32];
  bool isDefault;
};

// Initialize sound pack system — call after SPIFFS init
void initSoundPack();

// Get list of available sound packs
int getSoundPackList(SoundPackInfo *packs, int maxPacks);

// Get active sound pack name
String getActiveSoundPack();

// Set active sound pack by name
bool setActiveSoundPack(const String &name);

// Load sound pack melodies into current melody config
bool loadSoundPackMelodies(const String &name);

// Get sound pack list as JSON
String getSoundPackListJson();

// Get active sound pack info as JSON
String getActiveSoundPackJson();

// Register sound pack API routes
void registerSoundPackRoutes();

#endif // SOUNDPACK_H
