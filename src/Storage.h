#ifndef STORAGE_H
#define STORAGE_H

#include "Pet.h"

// ============================================================
// SPIFFS persistence
// ============================================================

void loadPetData(Pet &pet);
void savePetData(const Pet &pet);

#endif // STORAGE_H
