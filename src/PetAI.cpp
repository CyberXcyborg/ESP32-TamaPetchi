#include "PetAI.h"
#include "config.h"
#include <ArduinoJson.h>

// ============================================================
// PetMemory Implementation
// ============================================================

void PetMemory::init() {
  head = 0;
  count = 0;
  memset(actions, 0, sizeof(actions));
  memset(timestamps, 0, sizeof(timestamps));
}

void PetMemory::record(PetAction action, unsigned long ts) {
  actions[head] = action;
  timestamps[head] = ts;
  head = (head + 1) % PET_MEMORY_SIZE;
  if (count < PET_MEMORY_SIZE) count++;
}

int PetMemory::frequency(PetAction action, unsigned long windowMs) const {
  int freq = 0;
  unsigned long now = millis();
  for (int i = 0; i < count; i++) {
    int idx = (head - 1 - i + PET_MEMORY_SIZE) % PET_MEMORY_SIZE;
    if (now - timestamps[idx] <= windowMs) {
      if (actions[idx] == action) freq++;
    }
  }
  return freq;
}

PetAction PetMemory::mostFrequent(unsigned long windowMs) const {
  int freq[ACTION_COUNT] = {0};
  unsigned long now = millis();
  for (int i = 0; i < count; i++) {
    int idx = (head - 1 - i + PET_MEMORY_SIZE) % PET_MEMORY_SIZE;
    if (now - timestamps[idx] <= windowMs) {
      freq[actions[idx]]++;
    }
  }
  int best = 0;
  for (int i = 1; i < ACTION_COUNT; i++) {
    if (freq[i] > freq[best]) best = i;
  }
  return (PetAction)best;
}

// ============================================================
// Lifecycle
// ============================================================

void initPetAI(Pet &pet) {
  pet.memory.init();
  pet.aiMods = {100, 100, 100, 100}; // neutral modifiers
  pet.lastAIUpdate = millis();
  pet.aiActivityLevel = 50;
}

void updatePetAI(Pet &pet) {
  if (!pet.isAlive) return;

  // Update AI every 2 minutes
  if (millis() - pet.lastAIUpdate < 120000UL) return;
  pet.lastAIUpdate = millis();

  // Compute modifiers from current state + personality + memory
  pet.aiMods = computeAIModifiers(pet);

  // Slowly evolve personality based on care patterns (every 10 AI ticks = ~20 min)
  static uint8_t evolveCounter = 0;
  if (++evolveCounter >= 10) {
    evolveCounter = 0;
    evolvePersonality(pet);
  }

  // Track activity level (0-100) based on recent interactions
  int recentActions = 0;
  for (int i = 0; i < ACTION_COUNT; i++) {
    recentActions += pet.memory.frequency((PetAction)i, 600000UL); // last 10 min window
  }
  pet.aiActivityLevel = constrain(recentActions * 10, 0, 100);
}

// ============================================================
// Memory Access
// ============================================================

void recordPetAction(Pet &pet, PetAction action) {
  pet.memory.record(action, millis());
  // Immediate stat tracking based on action
  switch (action) {
    case ACTION_FEED:
      pet.lastInteractionTime = millis();
      break;
    case ACTION_PLAY:
      pet.lastInteractionTime = millis();
      pet.totalPlayTime += PET_UPDATE_INTERVAL / 1000;
      break;
    case ACTION_SLEEP:
      pet.lastInteractionTime = millis();
      break;
    default:
      break;
  }
}

PetMemory& getPetMemory(Pet &pet) {
  return pet.memory;
}

// ============================================================
// Adaptive Rate Computation
// ============================================================

AIModifiers computeAIModifiers(const Pet &pet) {
  AIModifiers mods = {100, 100, 100, 100};

  // --- Adaptive Hunger Rate ---
  // Active pets (high activity level) get hungry faster
  // Sleeping pets get hungry slower
  if (pet.state == "sleeping") {
    mods.hungerRateMod = 60 - (pet.personalityHungry / 10); // 53-30% for sleepy pets
  } else {
    // Active lifestyle increases hunger
    int activityBonus = pet.aiActivityLevel * 30 / 100; // 0-30 extra
    int hungryTraitBonus = pet.personalityHungry * 20 / 100; // 0-20 from personality
    mods.hungerRateMod = 100 + activityBonus + hungryTraitBonus;
  }
  mods.hungerRateMod = constrain(mods.hungerRateMod, 40, 200);

  // --- Mood-Reactive Happiness Rate ---
  // Cheerful pets gain happiness faster when happy, lose it slower when sad
  if (pet.mood <= 1) { // ecstatic/happy
    mods.happinessRateMod = 80 - (pet.personalityCheerful / 10); // 70-30% decay
  } else if (pet.mood >= 5) { // upset/miserable
    mods.happinessRateMod = 120 + ((100 - pet.personalityCheerful) / 10); // 120-170% decay
  } else {
    mods.happinessRateMod = 100 + ((50 - pet.personalityCheerful) / 5); // 80-120%
  }
  mods.happinessRateMod = constrain(mods.happinessRateMod, 30, 200);

  // --- Energy Rate ---
  // Energetic pets lose energy slower when active, recover faster when sleeping
  if (pet.state == "sleeping") {
    mods.energyRateMod = 50; // Sleeping pets don't lose energy (this is regen)
  } else {
    int energyTrait = pet.personalityEnergetic * 30 / 100; // 0-30%
    mods.energyRateMod = 100 - energyTrait; // energetic pets lose less energy
  }
  mods.energyRateMod = constrain(mods.energyRateMod, 30, 180);

  // --- Health Rate (slower health loss for well-cared pets) ---
  // Hungry personality pets lose health faster when hungry
  if (pet.hunger < HEALTH_DECAY_THRESHOLD) {
    int hungryPenalty = pet.personalityHungry * 50 / 100; // 0-50% extra
    mods.healthRateMod = 100 + hungryPenalty;
  } else {
    // Well-fed pets with good mood lose health slower
    int careBonus = (pet.personalityCheerful + pet.personalityEnergetic) / 10; // 0-20
    mods.healthRateMod = 100 - careBonus;
  }
  mods.healthRateMod = constrain(mods.healthRateMod, 50, 200);

  return mods;
}

int applyAIModifier(int baseRate, int personalityTrait, const AIModifiers &mods) {
  // Apply the appropriate modifier based on context
  // This is a helper for the decay calculation in updatePet()
  int mod = 100;
  if (baseRate > 0) {
    // Use hunger rate as default modifier for general decay
    mod = mods.hungerRateMod;
  }
  return (baseRate * mod * personalityTrait) / 10000;
}

// ============================================================
// Personality Evolution
// ============================================================

void evolvePersonality(Pet &pet) {
  if (!pet.isAlive) return;

  // Analyze care patterns from memory
  int feedFreq = pet.memory.frequency(ACTION_FEED, 3600000UL);   // last hour
  int playFreq = pet.memory.frequency(ACTION_PLAY, 3600000UL);
  int cleanFreq = pet.memory.frequency(ACTION_CLEAN, 3600000UL);
  int totalActions = 0;
  for (int i = 0; i < ACTION_COUNT; i++) {
    totalActions += pet.memory.frequency((PetAction)i, 3600000UL);
  }

  if (totalActions < 2) return; // Not enough data to evolve

  // Cheerful: increases with frequent play and good mood
  if (playFreq > feedFreq && pet.mood <= 2) {
    pet.personalityCheerful = min(100, pet.personalityCheerful + 1);
  } else if (pet.mood >= 5) {
    pet.personalityCheerful = max(20, pet.personalityCheerful - 1);
  }

  // Energetic: increases with frequent activity
  if (totalActions >= 5) {
    pet.personalityEnergetic = min(100, pet.personalityEnergetic + 1);
  } else if (totalActions <= 1) {
    pet.personalityEnergetic = max(20, pet.personalityEnergetic - 1);
  }

  // Hungry: increases with frequent feeding, decreases with infrequent feeding
  if (feedFreq >= 3) {
    pet.personalityHungry = min(100, pet.personalityHungry + 1);
  } else if (feedFreq == 0 && totalActions > 3) {
    pet.personalityHungry = max(10, pet.personalityHungry - 1);
  }
}

// ============================================================
// JSON Status
// ============================================================

String getPetAIStatusJson(const Pet &pet) {
  StaticJsonDocument<512> doc;

  doc["activityLevel"] = pet.aiActivityLevel;
  doc["mood"] = pet.mood;
  doc["moodName"] = getMoodName(pet.mood);

  JsonObject personality = doc.createNestedObject("personality");
  personality["cheerful"] = pet.personalityCheerful;
  personality["energetic"] = pet.personalityEnergetic;
  personality["hungry"] = pet.personalityHungry;

  JsonObject mods = doc.createNestedObject("modifiers");
  mods["hungerRate"] = pet.aiMods.hungerRateMod;
  mods["happinessRate"] = pet.aiMods.happinessRateMod;
  mods["energyRate"] = pet.aiMods.energyRateMod;
  mods["healthRate"] = pet.aiMods.healthRateMod;

  // Most frequent recent action
  PetAction recentMost = pet.memory.mostFrequent(600000UL);
  const char *actionNames[] = {"feed", "play", "clean", "sleep", "heal", "game", "trade"};
  doc["dominantAction"] = actionNames[recentMost];

  String result;
  serializeJson(doc, result);
  return result;
}

String getPetAIMemoryJson(const Pet &pet) {
  StaticJsonDocument<1024> doc;

  JsonArray mem = doc.createNestedArray("memory");
  const char *actionNames[] = {"feed", "play", "clean", "sleep", "heal", "game", "trade"};

  for (int i = 0; i < pet.memory.count; i++) {
    int idx = (pet.memory.head - 1 - i + PET_MEMORY_SIZE) % PET_MEMORY_SIZE;
    JsonObject entry = mem.createNestedObject();
    entry["action"] = actionNames[pet.memory.actions[idx]];
    entry["timestamp"] = pet.memory.timestamps[idx];
  }

  // Action frequencies (last hour)
  JsonObject freq = doc.createNestedObject("hourlyFrequency");
  for (int i = 0; i < ACTION_COUNT; i++) {
    freq[actionNames[i]] = pet.memory.frequency((PetAction)i, 3600000UL);
  }

  String result;
  serializeJson(doc, result);
  return result;
}
