#ifndef PET_H
#define PET_H

#include <Arduino.h>

// ============================================================
// Pet Evolution Stages
// ============================================================
enum PetStage { BABY, CHILD, ADULT, ELDER };

// ============================================================
// Pet State
// ============================================================

struct Pet {
  int  hunger;
  int  happiness;
  int  health;
  int  energy;
  int  cleanliness;
  int  age;              // age in minutes
  bool isAlive;
  String state;          // normal, eating, playing, sleeping, sick, hungry, critical, dying, dead
  unsigned long stateChangeTime;
  unsigned long lastInteractionTime;

  // --- New fields for Phase 2 ---
  PetStage stage;        // evolution stage
  bool isNight;          // day/night cycle flag
  unsigned long virtualMinutes;  // virtual time in minutes since start
};

// --- Lifecycle ---
void initPet(Pet &pet);
void updatePet(Pet &pet);

// --- Evolution ---
void updateStage(Pet &pet);
int  getStageDecayMultiplier(Pet &pet);

// --- Day/Night ---
void updateDayNightCycle(Pet &pet);
bool isNightTime(unsigned long virtualMin);

// --- Randomness ---
int randomVariation(int base);

// --- Actions ---
void feedPet(Pet &pet);
void playPet(Pet &pet);
void cleanPet(Pet &pet);
void sleepPet(Pet &pet);
void healPet(Pet &pet);
void resetPet(Pet &pet);

#endif // PET_H
