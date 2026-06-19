#ifndef STORAGE_H
#define STORAGE_H

#include "Pet.h"

// ============================================================
// SPIFFS persistence
// ============================================================

void loadPetData(Pet &pet);
void savePetData(const Pet &pet);
void savePetDataForce(const Pet &pet);  // Phase 6: bypasses wear leveling for critical events

// Phase 10.5: Atomic write helper — write to .tmp, then rename
// Returns true on success, false on failure
bool atomicWrite(const char* filename, const String& content);

#endif // STORAGE_H
