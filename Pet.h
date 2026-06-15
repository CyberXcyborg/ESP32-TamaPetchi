#ifndef PET_H
#define PET_H

#include <Arduino.h>

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
  String state;          // normal, eating, playing, sleeping, sick, hungry, dead
  unsigned long stateChangeTime;
  unsigned long lastInteractionTime;
};

// --- Lifecycle ---
void initPet(Pet &pet);
void updatePet(Pet &pet);

// --- Sound Helpers ---
void soundFeed();
void soundPlay();
void soundDeath();
void soundWake();
void soundEnabledToggle();

// --- Actions ---
void feedPet(Pet &pet);
void playPet(Pet &pet);
void cleanPet(Pet &pet);
void sleepPet(Pet &pet);
void healPet(Pet &pet);
void resetPet(Pet &pet);

#endif // PET_H
