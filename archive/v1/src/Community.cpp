#include "Community.h"
#include "Achievements.h"
#include "Storage.h"
#include "config.h"
#include <ArduinoJson.h>
#ifndef UNIT_TEST
#include <SPIFFS.h>
#endif
#include <algorithm>

// ============================================================
// Module state
// ============================================================
static GalleryEntry gallery[MAX_GALLERY_ENTRIES];
static int galleryCount = 0;
static String localDeviceId;

// ============================================================
// Forward declarations
// ============================================================
#ifndef UNIT_TEST
static String readCommunityFile(const char *path);
#endif
static int calculatePetScoreRaw(const PetCard &card);

// ============================================================
// Device ID (derived from MAC)
// ============================================================
static String getDeviceId() {
  if (localDeviceId.length() == 0) {
#ifdef UNIT_TEST
    localDeviceId = "test_device_001";
#else
    uint64_t mac = ESP.getEfuseMac();
    localDeviceId = String((uint32_t)(mac >> 32), HEX) + String((uint32_t)mac, HEX);
    localDeviceId.toUpperCase();
#endif
  }
  return localDeviceId;
}

// ============================================================
// Score Calculation
// ============================================================
int calculatePetScore(const Pet &pet) {
  int score = 0;
  score += pet.feedCount * 2;
  score += pet.playCount * 3;
  score += pet.highScore * 5;
  score += pet.age / 60; // 1 point per hour alive
  // Achievement bonus
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked) {
      score += 10 * (achievementStates[i].tier + 1);
    }
  }
  return score;
}

static int calculatePetScoreRaw(const PetCard &card) {
  int score = 0;
  score += card.feedCount * 2;
  score += card.playCount * 3;
  score += card.highScore * 5;
  score += card.age / 60;
  score += card.achievementCount * 10 * (card.maxTier + 1);
  return score;
}

// ============================================================
// Init & Persistence
// ============================================================
void initCommunity() {
  getDeviceId();
#ifndef UNIT_TEST
  loadCommunity();
  Serial.printf("[Community] Initialized (device=%s)\n", localDeviceId.c_str());
#endif
}

void loadCommunity() {
#ifdef UNIT_TEST
  // In-memory only for tests
  galleryCount = 0;
#else
  if (!SPIFFS.exists(COMMUNITY_FILE)) {
    galleryCount = 0;
    return;
  }
  String data = readCommunityFile(COMMUNITY_FILE);
  if (data.length() == 0) { galleryCount = 0; return; }

  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, data.c_str())) { galleryCount = 0; return; }

  galleryCount = 0;
  JsonArray arr = doc["gallery"].as<JsonArray>();
  for (JsonObject entry : arr) {
    if (galleryCount >= MAX_GALLERY_ENTRIES) break;
    GalleryEntry &g = gallery[galleryCount];
    g.card.deviceId = entry["deviceId"] | "";
    g.card.petName = entry["petName"] | "";
    g.card.petType = entry["petType"] | "blob";
    g.card.generation = entry["generation"] | 0;
    g.card.age = entry["age"] | 0;
    g.card.feedCount = entry["feedCount"] | 0;
    g.card.playCount = entry["playCount"] | 0;
    g.card.highScore = entry["highScore"] | 0;
    g.card.achievementCount = entry["achievementCount"] | 0;
    g.card.maxTier = entry["maxTier"] | 0;
    g.card.createdAt = entry["createdAt"] | 0;
    g.score = entry["score"] | 0;
    galleryCount++;
  }
#endif
}

void saveCommunity() {
#ifndef UNIT_TEST
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("gallery");
  for (int i = 0; i < galleryCount; i++) {
    JsonObject entry = arr.createNestedObject();
    entry["deviceId"] = gallery[i].card.deviceId.c_str();
    entry["petName"] = gallery[i].card.petName.c_str();
    entry["petType"] = gallery[i].card.petType.c_str();
    entry["generation"] = gallery[i].card.generation;
    entry["age"] = gallery[i].card.age;
    entry["feedCount"] = gallery[i].card.feedCount;
    entry["playCount"] = gallery[i].card.playCount;
    entry["highScore"] = gallery[i].card.highScore;
    entry["achievementCount"] = gallery[i].card.achievementCount;
    entry["maxTier"] = gallery[i].card.maxTier;
    entry["createdAt"] = gallery[i].card.createdAt;
    entry["score"] = gallery[i].score;
  }
  String data;
  serializeJson(doc, data);
  File f = SPIFFS.open(COMMUNITY_FILE, "w");
  if (f) { f.print(data); f.close(); }
#endif
}

// ============================================================
// Pet Card Creation
// ============================================================
PetCard createPetCard(const Pet &pet) {
  PetCard card;
  card.deviceId = getDeviceId();
  card.petName = pet.name;
  card.petType = pet.type == BLOB ? "blob" : (pet.type == CAT ? "cat" : "dog");
  card.generation = pet.generation;
  card.age = pet.age;
  card.feedCount = pet.feedCount;
  card.playCount = pet.playCount;
  card.highScore = pet.highScore;
  card.createdAt = millis() / 1000;

  // Count achievements and find max tier
  card.achievementCount = 0;
  card.maxTier = 0;
  for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
    if (achievementStates[i].unlocked) {
      card.achievementCount++;
    }
    if (achievementStates[i].tier > card.maxTier) {
      card.maxTier = achievementStates[i].tier;
    }
  }
  return card;
}

String petCardToJson(const Pet &pet) {
  PetCard card = createPetCard(pet);
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = card.deviceId.c_str();
  doc["petName"] = card.petName.c_str();
  doc["petType"] = card.petType.c_str();
  doc["generation"] = card.generation;
  doc["age"] = card.age;
  doc["feedCount"] = card.feedCount;
  doc["playCount"] = card.playCount;
  doc["highScore"] = card.highScore;
  doc["achievementCount"] = card.achievementCount;
  doc["maxTier"] = card.maxTier;
  doc["createdAt"] = card.createdAt;
  doc["score"] = calculatePetScore(pet);
  String result;
  serializeJson(doc, result);
  return result;
}

bool importPetCard(const String &json, Pet &pet) {
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, json.c_str())) return false;

  // Import as a "shared pet" — set lineage to indicate import
  pet.name = doc["petName"] | "Imported";
  pet.generation = doc["generation"] | 0;
  pet.feedCount = doc["feedCount"] | 0;
  pet.playCount = doc["playCount"] | 0;
  pet.highScore = doc["highScore"] | 0;
  String type = doc["petType"] | "blob";
  pet.type = (type == "cat") ? CAT : ((type == "dog") ? DOG : BLOB);
  pet.parentIds[0] = doc["deviceId"] | "unknown";
  pet.parentIds[1] = "";
  return true;
}

// ============================================================
// Gallery
// ============================================================
void addToGallery(const PetCard &card) {
  // Don't add duplicates
  for (int i = 0; i < galleryCount; i++) {
    if (gallery[i].card.deviceId == card.deviceId) {
      gallery[i].card = card;
      gallery[i].score = calculatePetScoreRaw(card);
      saveCommunity();
      return;
    }
  }
  if (galleryCount < MAX_GALLERY_ENTRIES) {
    gallery[galleryCount].card = card;
    gallery[galleryCount].score = calculatePetScoreRaw(card);
    galleryCount++;
  }
  saveCommunity();
}

void removeFromGallery(const String &deviceId) {
  for (int i = 0; i < galleryCount; i++) {
    if (gallery[i].card.deviceId == deviceId) {
      // Shift remaining entries
      for (int j = i; j < galleryCount - 1; j++) {
        gallery[j] = gallery[j + 1];
      }
      galleryCount--;
      saveCommunity();
      return;
    }
  }
}

String getGalleryJson() {
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("gallery");
  for (int i = 0; i < galleryCount; i++) {
    JsonObject entry = arr.createNestedObject();
    entry["deviceId"] = gallery[i].card.deviceId.c_str();
    entry["petName"] = gallery[i].card.petName.c_str();
    entry["petType"] = gallery[i].card.petType.c_str();
    entry["generation"] = gallery[i].card.generation;
    entry["age"] = gallery[i].card.age;
    entry["feedCount"] = gallery[i].card.feedCount;
    entry["playCount"] = gallery[i].card.playCount;
    entry["highScore"] = gallery[i].card.highScore;
    entry["achievementCount"] = gallery[i].card.achievementCount;
    entry["maxTier"] = gallery[i].card.maxTier;
    entry["createdAt"] = gallery[i].card.createdAt;
    entry["score"] = gallery[i].score;
  }
  doc["count"] = galleryCount;
  String result;
  serializeJson(doc, result);
  return result;
}

int getGalleryCount() { return galleryCount; }

// ============================================================
// Leaderboard
// ============================================================
String getLeaderboardJson(LeaderboardSort sort, int limit) {
  // Sort a copy
  GalleryEntry sorted[MAX_GALLERY_ENTRIES];
  int count = min(galleryCount, MAX_GALLERY_ENTRIES);
  memcpy(sorted, gallery, count * sizeof(GalleryEntry));

  // Simple insertion sort (small array)
  for (int i = 1; i < count; i++) {
    GalleryEntry key = sorted[i];
    int j = i - 1;
    while (j >= 0) {
      bool shouldSwap = false;
      switch (sort) {
        case SORT_AGE: shouldSwap = (sorted[j].card.age < key.card.age); break;
        case SORT_GENERATION: shouldSwap = (sorted[j].card.generation < key.card.generation); break;
        default: shouldSwap = (sorted[j].score < key.score); break;
      }
      if (!shouldSwap) break;
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }

  if (limit > count) limit = count;

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("leaderboard");
  for (int i = 0; i < limit; i++) {
    JsonObject entry = arr.createNestedObject();
    entry["rank"] = i + 1;
    entry["deviceId"] = sorted[i].card.deviceId.c_str();
    entry["petName"] = sorted[i].card.petName.c_str();
    entry["petType"] = sorted[i].card.petType.c_str();
    entry["score"] = sorted[i].score;
    entry["achievementCount"] = sorted[i].card.achievementCount;
    entry["maxTier"] = sorted[i].card.maxTier;
    entry["age"] = sorted[i].card.age;
    entry["generation"] = sorted[i].card.generation;
  }
  doc["sort"] = sort == SORT_AGE ? "age" : (sort == SORT_GENERATION ? "generation" : "achievements");
  doc["total"] = count;
  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Sharing
// ============================================================
String getShareableProfileJson(const Pet &pet) {
  return petCardToJson(pet);
}

bool sharePetProfile(const Pet &pet) {
  // Add own pet to gallery
  PetCard card = createPetCard(pet);
  addToGallery(card);
  Serial.printf("[Community] Shared pet profile: %s (score=%d)\n",
                pet.name.c_str(), calculatePetScore(pet));
  return true;
}

bool importSharedProfile(const String &json, Pet &pet) {
  return importPetCard(json, pet);
}

// ============================================================
// readFile helper for community data (SPIFFS only)
// ============================================================
#ifndef UNIT_TEST
static String readCommunityFile(const char *path) {
  File f = SPIFFS.open(path, "r");
  if (!f) return "";
  String data = f.readString();
  f.close();
  return data;
}
#endif
