#include "Storage.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

void savePetData(const Pet &pet) {
  File file = SPIFFS.open(PET_DATA_FILE, "w");
  if (!file) {
    Serial.println("Failed to open pet data file for writing");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["hunger"]      = pet.hunger;
  jsonDoc["happiness"]   = pet.happiness;
  jsonDoc["health"]      = pet.health;
  jsonDoc["energy"]      = pet.energy;
  jsonDoc["cleanliness"] = pet.cleanliness;
  jsonDoc["age"]         = pet.age;
  jsonDoc["isAlive"]     = pet.isAlive;
  jsonDoc["state"]       = pet.state;

  // Phase 2: persist new fields
  jsonDoc["stage"]          = (int)pet.stage;
  jsonDoc["isNight"]        = pet.isNight;
  jsonDoc["virtualMinutes"] = pet.virtualMinutes;

  if (serializeJson(jsonDoc, file) == 0) {
    Serial.println("Failed to write pet data to file");
  }

  file.close();
}

void loadPetData(Pet &pet) {
  if (!SPIFFS.exists(PET_DATA_FILE)) {
    Serial.println("No pet data file found, using defaults");
    return;
  }

  File file = SPIFFS.open(PET_DATA_FILE, "r");
  if (!file) {
    Serial.println("Failed to open pet data file for reading");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error = deserializeJson(jsonDoc, file);

  if (error) {
    Serial.println("Failed to parse pet data file");
  } else {
    pet.hunger      = jsonDoc["hunger"];
    pet.happiness   = jsonDoc["happiness"];
    pet.health      = jsonDoc["health"];
    pet.energy      = jsonDoc["energy"];
    pet.cleanliness = jsonDoc["cleanliness"];
    pet.age         = jsonDoc["age"];
    pet.isAlive     = jsonDoc["isAlive"];
    pet.state       = jsonDoc["state"].as<String>();

    // Phase 2: load new fields (with defaults for backward compatibility)
    pet.stage          = (PetStage)(int)jsonDoc["stage"] | (PetStage)0;
    pet.isNight        = jsonDoc["isNight"] | false;
    pet.virtualMinutes = jsonDoc["virtualMinutes"] | (6 * VIRTUAL_MINUTES_PER_HOUR);
  }

  file.close();
}
