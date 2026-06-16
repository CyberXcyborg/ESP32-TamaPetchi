#ifndef STORAGE_H
#define STORAGE_H

#include "Pet.h"

// ============================================================
// SPIFFS persistence
// ============================================================

void loadPetData(Pet &pet);
void savePetData(const Pet &pet);
void savePetDataForce(const Pet &pet);  // Phase 6: bypasses wear leveling for critical events

#endif // STORAGE_H
