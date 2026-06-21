#ifndef PETAI_TYPES_H
#define PETAI_TYPES_H

#include <Arduino.h>

// ============================================================
// PetAI Type Definitions (Phase 16.1)
// Forward-declared types used by Pet struct and PetAI module.
// ============================================================

// Action types for pet memory log
enum PetAction {
  ACTION_FEED = 0,
  ACTION_PLAY,
  ACTION_CLEAN,
  ACTION_SLEEP,
  ACTION_HEAL,
  ACTION_GAME,
  ACTION_TRADE,
  ACTION_COUNT
};

#define PET_MEMORY_SIZE 10

struct PetMemory {
  PetAction actions[PET_MEMORY_SIZE];
  unsigned long timestamps[PET_MEMORY_SIZE];
  int head;
  int count;

  void init();
  void record(PetAction action, unsigned long ts);
  int frequency(PetAction action, unsigned long windowMs) const;
  PetAction mostFrequent(unsigned long windowMs) const;
  bool isEmpty() const { return count == 0; }
};

struct AIModifiers {
  int hungerRateMod;      // percentage modifier (100 = normal)
  int happinessRateMod;
  int energyRateMod;
  int healthRateMod;
};

#endif // PETAI_TYPES_H
