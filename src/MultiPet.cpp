#include "MultiPet.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

void loadMultiPetState(MultiPetState &state) {
  state.activePetIndex = 0;
  state.petCount = 0;
  for (int i = 0; i < MAX_PETS; i++) state.slots[i].occupied = false;

  if (!SPIFFS.exists(MULTI_PET_FILE)) return;

  File file = SPIFFS.open(MULTI_PET_FILE, "r");
  if (!file) return;

  DynamicJsonDocument jsonDoc(4096);
  if (deserializeJson(jsonDoc, file)) { file.close(); return; }
  file.close();

  state.activePetIndex = jsonDoc["activePetIndex"] | 0;
  JsonArray arr = jsonDoc["pets"].as<JsonArray>();

  int idx = 0;
  for (JsonObject obj : arr) {
    if (idx >= MAX_PETS) break;
    Pet &p = state.slots[idx].pet;
    p.hunger      = obj["hunger"] | 50;
    p.happiness   = obj["happiness"] | 50;
    p.health      = obj["health"] | 80;
    p.energy      = obj["energy"] | 100;
    p.cleanliness = obj["cleanliness"] | 80;
    p.age         = obj["age"] | 0;
    p.isAlive     = obj["isAlive"] | true;
    p.state       = obj["state"].as<String>();
    if (p.state.length() == 0) p.state = "normal";
    p.stage           = (PetStage)(int)obj["stage"] | BABY;
    p.isNight         = obj["isNight"] | false;
    p.virtualMinutes  = obj["virtualMinutes"] | 0UL;
    p.name            = obj["name"].as<String>();
    if (p.name.length() == 0) p.name = "Pet" + String(idx + 1);
    p.soundEnabled    = obj["soundEnabled"] | true;
    p.type            = (PetType)(int)obj["type"] | BLOB;
    p.feedCount       = obj["feedCount"] | 0;
    p.playCount       = obj["playCount"] | 0;
    p.hasBeenNamed    = obj["hasBeenNamed"] | false;
    p.elderAchieved   = obj["elderAchieved"] | false;
    state.slots[idx].occupied = true;
    idx++;
  }
  state.petCount = idx;
}

void saveMultiPetState(const MultiPetState &state) {
  File file = SPIFFS.open(MULTI_PET_FILE, "w");
  if (!file) return;

  DynamicJsonDocument jsonDoc(4096);
  jsonDoc["activePetIndex"] = state.activePetIndex;
  JsonArray arr = jsonDoc.createNestedArray("pets");

  for (int i = 0; i < MAX_PETS; i++) {
    if (!state.slots[i].occupied) continue;
    const Pet &p = state.slots[i].pet;
    JsonObject obj = arr.createNestedObject();
    obj["hunger"] = p.hunger; obj["happiness"] = p.happiness;
    obj["health"] = p.health; obj["energy"] = p.energy;
    obj["cleanliness"] = p.cleanliness; obj["age"] = p.age;
    obj["isAlive"] = p.isAlive; obj["state"] = p.state;
    obj["stage"] = (int)p.stage; obj["isNight"] = p.isNight;
    obj["virtualMinutes"] = p.virtualMinutes; obj["name"] = p.name;
    obj["soundEnabled"] = p.soundEnabled; obj["type"] = (int)p.type;
    obj["feedCount"] = p.feedCount; obj["playCount"] = p.playCount;
    obj["hasBeenNamed"] = p.hasBeenNamed; obj["elderAchieved"] = p.elderAchieved;
  }
  serializeJson(jsonDoc, file);
  file.close();
}

int createPet(MultiPetState &state, const String &name, PetType type) {
  for (int i = 0; i < MAX_PETS; i++) {
    if (!state.slots[i].occupied) {
      initPet(state.slots[i].pet);
      state.slots[i].pet.name = name.length() > 0 ? name : "Pet" + String(i + 1);
      state.slots[i].pet.type = type;
      state.slots[i].pet.hasBeenNamed = name.length() > 0;
      state.slots[i].occupied = true;
      state.petCount++;
      saveMultiPetState(state);
      return i;
    }
  }
  return -1;
}

bool switchPet(MultiPetState &state, int slotIndex) {
  if (slotIndex < 0 || slotIndex >= MAX_PETS) return false;
  if (!state.slots[slotIndex].occupied) return false;
  state.activePetIndex = slotIndex;
  saveMultiPetState(state);
  return true;
}

bool deletePet(MultiPetState &state, int slotIndex) {
  if (slotIndex < 0 || slotIndex >= MAX_PETS) return false;
  if (!state.slots[slotIndex].occupied) return false;
  if (state.petCount <= 1) return false; // Can't delete last pet

  state.slots[slotIndex].occupied = false;
  state.petCount--;

  if (state.activePetIndex == slotIndex) {
    for (int i = 0; i < MAX_PETS; i++) {
      if (state.slots[i].occupied) { state.activePetIndex = i; break; }
    }
  }
  saveMultiPetState(state);
  return true;
}

Pet* getPet(MultiPetState &state, int slotIndex) {
  if (slotIndex < 0 || slotIndex >= MAX_PETS) return nullptr;
  if (!state.slots[slotIndex].occupied) return nullptr;
  return &state.slots[slotIndex].pet;
}

Pet* getActivePet(MultiPetState &state) {
  return getPet(state, state.activePetIndex);
}

String getMultiPetJson(const MultiPetState &state) {
  DynamicJsonDocument jsonDoc(2048);
  jsonDoc["activePetIndex"] = state.activePetIndex;
  jsonDoc["petCount"] = state.petCount;
  jsonDoc["maxPets"] = MAX_PETS;
  JsonArray arr = jsonDoc.createNestedArray("pets");
  for (int i = 0; i < MAX_PETS; i++) {
    JsonObject slot = arr.createNestedObject();
    slot["index"] = i; slot["occupied"] = state.slots[i].occupied;
    if (state.slots[i].occupied) {
      const Pet &p = state.slots[i].pet;
      slot["name"] = p.name; slot["type"] = (int)p.type;
      slot["age"] = p.age; slot["isAlive"] = p.isAlive;
      slot["state"] = p.state; slot["stage"] = (int)p.stage;
    }
  }
  String result; serializeJson(jsonDoc, result);
  return result;
}
