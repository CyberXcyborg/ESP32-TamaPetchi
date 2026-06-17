#ifndef MULTIPET_H
#define MULTIPET_H

#include "Pet.h"
#include "config.h"

// ============================================================
// Multi-Pet Support
// Manage up to 3 pets stored in SPIFFS
// ============================================================

struct PetSlot {
  bool occupied;
  Pet pet;
};

struct MultiPetState {
  PetSlot slots[MAX_PETS];
  int activePetIndex;
  int petCount;
};

void loadMultiPetState(MultiPetState &state);
void saveMultiPetState(const MultiPetState &state);
int createPet(MultiPetState &state, const String &name, PetType type);
bool switchPet(MultiPetState &state, int slotIndex);
bool deletePet(MultiPetState &state, int slotIndex);
Pet* getPet(MultiPetState &state, int slotIndex);
Pet* getActivePet(MultiPetState &state);
String getMultiPetJson(const MultiPetState &state);

#endif // MULTIPET_H
