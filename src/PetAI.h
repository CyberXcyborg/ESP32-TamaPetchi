#ifndef PETAI_H
#define PETAI_H

#include <Arduino.h>
#include "Pet.h"
#include "PetAI_Types.h"

// ============================================================
// PetAI — Adaptive Behavior Engine (Phase 16.1)
//
// Adds AI-like automation features:
// - Adaptive hunger rate: pet gets hungry faster when active, slower when sleeping
// - Mood-reactive behavior: cheerful pets gain happiness faster, hungry pets lose health faster
// - Personality evolution: traits shift based on care patterns
// - Pet memory: track last 10 actions and adapt responses
// ============================================================

// --- Lifecycle ---
void initPetAI(Pet &pet);
void updatePetAI(Pet &pet);

// --- Memory ---
void recordPetAction(Pet &pet, PetAction action);
PetMemory& getPetMemory(Pet &pet);

// --- Adaptive Rates ---
AIModifiers computeAIModifiers(const Pet &pet);
int applyAIModifier(int baseRate, int personalityTrait, const AIModifiers &mods);

// --- Personality Evolution ---
void evolvePersonality(Pet &pet);

// --- AI Status JSON ---
String getPetAIStatusJson(const Pet &pet);
String getPetAIMemoryJson(const Pet &pet);

#endif // PETAI_H
