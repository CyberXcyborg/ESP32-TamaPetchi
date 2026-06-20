#ifndef COMMUNITY_H
#define COMMUNITY_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Community Features — Phase 13.3
// Pet profile sharing, community gallery, leaderboard
// ============================================================

// --- Pet Card (shareable profile) ---
struct PetCard {
  String deviceId;
  String petName;
  String petType;
  int generation;
  int age;
  int feedCount;
  int playCount;
  int highScore;
  int achievementCount;
  int maxTier; // highest achievement tier unlocked
  unsigned long createdAt;
};

// --- Gallery Entry ---
struct GalleryEntry {
  PetCard card;
  int score; // achievement score for ranking
};

// --- Leaderboard Sort Mode ---
enum LeaderboardSort {
  SORT_ACHIEVEMENTS = 0,
  SORT_AGE = 1,
  SORT_GENERATION = 2
};

// --- Community Data (persisted) ---
#define COMMUNITY_FILE "/community.json"
#define MAX_GALLERY_ENTRIES 20
#define MAX_SHARED_CARDS 10

// --- Init & Load/Save ---
void initCommunity();
void loadCommunity();
void saveCommunity();

// --- Pet Card Creation ---
PetCard createPetCard(const Pet &pet);
String petCardToJson(const Pet &pet);
bool importPetCard(const String &json, Pet &pet);

// --- Gallery ---
void addToGallery(const PetCard &card);
void removeFromGallery(const String &deviceId);
String getGalleryJson();
int getGalleryCount();

// --- Leaderboard ---
String getLeaderboardJson(LeaderboardSort sort = SORT_ACHIEVEMENTS, int limit = 10);

// --- Sharing ---
String getShareableProfileJson(const Pet &pet);
bool sharePetProfile(const Pet &pet);

// --- Import ---
bool importSharedProfile(const String &json, Pet &pet);

// --- Score Calculation ---
int calculatePetScore(const Pet &pet);

#endif // COMMUNITY_H
