#include "Achievements.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

void loadAchievements(Pet &pet) {
  if (!SPIFFS.exists(ACHIEVEMENTS_FILE)) {
    Serial.println("No achievements file found, starting fresh");
    return;
  }

  File file = SPIFFS.open(ACHIEVEMENTS_FILE, "r");
  if (!file) {
    Serial.println("Failed to open achievements file");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error = deserializeJson(jsonDoc, file);
  if (error) {
    Serial.println("Failed to parse achievements file");
  } else {
    pet.feedCount     = jsonDoc["feedCount"]     | 0;
    pet.playCount     = jsonDoc["playCount"]     | 0;
    pet.hasBeenNamed  = jsonDoc["hasBeenNamed"]  | false;
    pet.elderAchieved = jsonDoc["elderAchieved"] | false;
  }

  file.close();
}

void saveAchievements(const Pet &pet) {
  File file = SPIFFS.open(ACHIEVEMENTS_FILE, "w");
  if (!file) {
    Serial.println("Failed to open achievements file for writing");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["feedCount"]     = pet.feedCount;
  jsonDoc["playCount"]     = pet.playCount;
  jsonDoc["hasBeenNamed"]  = pet.hasBeenNamed;
  jsonDoc["elderAchieved"] = pet.elderAchieved;

  if (serializeJson(jsonDoc, file) == 0) {
    Serial.println("Failed to write achievements");
  }

  file.close();
}

void checkAchievements(Pet &pet) {
  if (!pet.isAlive) return;

  // Survived 1 hour (age >= 60 minutes)
  // Note: age is in minutes, so 60 = 1 hour

  // Survived 24 hours (age >= 1440 minutes)

  // Fed 10 times
  if (pet.feedCount >= 10) {
    Serial.println("Achievement: Fed 10x");
  }

  // Played 10 times
  if (pet.playCount >= 10) {
    Serial.println("Achievement: Played 10x");
  }

  // Named pet
  if (pet.hasBeenNamed) {
    Serial.println("Achievement: Named Pet");
  }

  // Reached elder stage
  if (pet.stage == ELDER && !pet.elderAchieved) {
    pet.elderAchieved = true;
    Serial.println("Achievement: Reached Elder!");
    saveAchievements(pet);
  }
}

String getAchievementsJson(const Pet &pet) {
  DynamicJsonDocument jsonDoc(1024);
  JsonArray arr = jsonDoc.createNestedArray("achievements");

  // survived_1h
  if (pet.age >= 60) arr.add(ACH_SURVIVED_1H);
  // survived_24h
  if (pet.age >= 1440) arr.add(ACH_SURVIVED_24H);
  // fed_10x
  if (pet.feedCount >= 10) arr.add(ACH_FED_10X);
  // played_10x
  if (pet.playCount >= 10) arr.add(ACH_PLAYED_10X);
  // named_pet
  if (pet.hasBeenNamed) arr.add(ACH_NAMED_PET);
  // reached_elder
  if (pet.elderAchieved || pet.stage == ELDER) arr.add(ACH_REACHED_ELDER);

  String result;
  serializeJson(jsonDoc, result);
  return result;
}
