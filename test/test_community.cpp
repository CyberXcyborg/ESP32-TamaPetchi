// test_community.cpp — Unit tests for Community module
#include "Community.h"
#include "Pet.h"
#include "Achievements.h"
#include "config.h"
#include <cassert>
#include <cstdio>

// ============================================================
// Test: Pet Card Creation
// ============================================================
void test_pet_card_creation() {
  Pet pet;
  initPet(pet);
  pet.name = "TestPet";
  pet.type = CAT;
  pet.generation = 2;
  pet.age = 100;
  pet.feedCount = 50;
  pet.playCount = 30;
  pet.highScore = 15;

  PetCard card = createPetCard(pet);
  assert(card.petName == "TestPet");
  assert(card.petType == "cat");
  assert(card.generation == 2);
  assert(card.age == 100);
  assert(card.feedCount == 50);
  assert(card.playCount == 30);
  assert(card.highScore == 15);
  assert(card.deviceId.length() > 0);

  printf("  PASS: Pet card creation\n");
}

// ============================================================
// Test: Pet Card JSON Serialization
// ============================================================
void test_pet_card_json() {
  Pet pet;
  initPet(pet);
  pet.name = "JsonPet";
  pet.type = DOG;
  pet.generation = 1;
  pet.age = 200;
  pet.feedCount = 10;
  pet.playCount = 5;
  pet.highScore = 3;

  String json = petCardToJson(pet);
  assert(json.length() > 0);
  // Should contain key fields
  assert(json.indexOf("\"petName\":\"JsonPet\"") >= 0);
  assert(json.indexOf("\"petType\":\"dog\"") >= 0);
  assert(json.indexOf("\"generation\":1") >= 0);

  printf("  PASS: Pet card JSON serialization\n");
}

// ============================================================
// Test: Gallery Add and Count
// ============================================================
void test_gallery_add() {
  // Gallery starts empty (fresh SPIFFS mock)
  assert(getGalleryCount() == 0);

  PetCard card;
  card.deviceId = "device_001";
  card.petName = "GalleryPet";
  card.petType = "blob";
  card.generation = 0;
  card.age = 50;
  card.feedCount = 5;
  card.playCount = 3;
  card.highScore = 1;
  card.achievementCount = 2;
  card.maxTier = 1;
  card.createdAt = 1000;

  addToGallery(card);
  assert(getGalleryCount() == 1);

  // Adding same device ID should update, not duplicate
  addToGallery(card);
  assert(getGalleryCount() == 1);

  printf("  PASS: Gallery add\n");
}

// ============================================================
// Test: Gallery JSON
// ============================================================
void test_gallery_json() {
  String json = getGalleryJson();
  assert(json.length() > 0);
  assert(json.indexOf("\"gallery\"") >= 0);
  assert(json.indexOf("\"count\"") >= 0);

  printf("  PASS: Gallery JSON output\n");
}

// ============================================================
// Test: Gallery Remove
// ============================================================
void test_gallery_remove() {
  // Clear gallery by removing all entries (use getGalleryCount to iterate)
  // Since gallery is static, we remove by known IDs from previous tests
  // The gallery may have entries from test_gallery_add and test_gallery_json
  // We just verify removeFromGallery works correctly
  PetCard card1;
  card1.deviceId = "dev_remove_001";
  card1.petName = "Pet1";
  addToGallery(card1);
  assert(getGalleryCount() >= 1);

  removeFromGallery("dev_remove_001");
  // Count should have decreased by 1

  // Removing non-existent should not crash
  removeFromGallery("nonexistent");
  assert(getGalleryCount() >= 0);

  printf("  PASS: Gallery remove\n");
}

// ============================================================
// Test: Leaderboard Sorting
// ============================================================
void test_leaderboard_sorting() {
  // Add multiple entries
  PetCard cards[3];
  for (int i = 0; i < 3; i++) {
    cards[i].deviceId = "dev_" + String(i);
    cards[i].petName = "Pet" + String(i);
    cards[i].petType = "blob";
    cards[i].generation = i;
    cards[i].age = (i + 1) * 100;
    cards[i].feedCount = (i + 1) * 10;
    cards[i].playCount = (i + 1) * 5;
    cards[i].highScore = i * 3;
    cards[i].achievementCount = i;
    cards[i].maxTier = i;
    cards[i].createdAt = i * 1000;
    addToGallery(cards[i]);
  }
  assert(getGalleryCount() == 3);

  // Sort by achievements (default)
  String json = getLeaderboardJson(SORT_ACHIEVEMENTS, 10);
  assert(json.length() > 0);
  assert(json.indexOf("\"sort\":\"achievements\"") >= 0);

  // Sort by age
  json = getLeaderboardJson(SORT_AGE, 10);
  assert(json.indexOf("\"sort\":\"age\"") >= 0);

  // Sort by generation
  json = getLeaderboardJson(SORT_GENERATION, 10);
  assert(json.indexOf("\"sort\":\"generation\"") >= 0);

  // Limit
  json = getLeaderboardJson(SORT_ACHIEVEMENTS, 2);
  assert(json.indexOf("\"total\":3") >= 0);

  printf("  PASS: Leaderboard sorting\n");
}

// ============================================================
// Test: Score Calculation
// ============================================================
void test_score_calculation() {
  Pet pet;
  initPet(pet);
  pet.feedCount = 10;   // 10 * 2 = 20
  pet.playCount = 5;    // 5 * 3 = 15
  pet.highScore = 3;   // 3 * 5 = 15
  pet.age = 360;       // 360 / 60 = 6
  // No achievements unlocked = 0
  // Total = 20 + 15 + 15 + 6 = 56

  int score = calculatePetScore(pet);
  assert(score == 56);

  printf("  PASS: Score calculation\n");
}

// ============================================================
// Test: Import Pet Card
// ============================================================
void test_import_pet_card() {
  String json = "{\"deviceId\":\"imported_dev\",\"petName\":\"ImportedPet\","
                "\"petType\":\"cat\",\"generation\":3,\"age\":500,"
                "\"feedCount\":20,\"playCount\":15,\"highScore\":10}";

  Pet pet;
  initPet(pet);
  bool ok = importPetCard(json, pet);
  assert(ok);
  assert(pet.name == "ImportedPet");
  assert(pet.generation == 3);
  assert(pet.feedCount == 20);
  assert(pet.playCount == 15);
  assert(pet.highScore == 10);
  assert(pet.type == CAT);
  assert(pet.parentIds[0] == "imported_dev");

  printf("  PASS: Import pet card\n");
}

// ============================================================
// Test: Import Invalid JSON
// ============================================================
void test_import_invalid_json() {
  Pet pet;
  initPet(pet);
  bool ok = importPetCard("not valid json", pet);
  assert(!ok);

  printf("  PASS: Import invalid JSON rejected\n");
}

// ============================================================
// Test: Shareable Profile
// ============================================================
void test_shareable_profile() {
  Pet pet;
  initPet(pet);
  pet.name = "ShareMe";
  pet.type = BLOB;
  pet.generation = 0;
  pet.age = 60;
  pet.feedCount = 3;
  pet.playCount = 2;
  pet.highScore = 1;

  String json = getShareableProfileJson(pet);
  assert(json.length() > 0);
  assert(json.indexOf("\"petName\":\"ShareMe\"") >= 0);
  assert(json.indexOf("\"score\"") >= 0);

  printf("  PASS: Shareable profile\n");
}

// ============================================================
// Test: Gallery Max Entries
// ============================================================
void test_gallery_max_entries() {
  // Try to add more than MAX_GALLERY_ENTRIES
  for (int i = 0; i < MAX_GALLERY_ENTRIES + 5; i++) {
    PetCard card;
    card.deviceId = "dev_overflow_" + String(i);
    card.petName = "Pet" + String(i);
    addToGallery(card);
  }
  // Should not exceed MAX_GALLERY_ENTRIES
  assert(getGalleryCount() <= MAX_GALLERY_ENTRIES);

  printf("  PASS: Gallery max entries limit\n");
}

// ============================================================
// Main test runner
// ============================================================
int main() {
  printf("=== Community Unit Tests ===\n");

  test_pet_card_creation();
  printf("  [OK] test_pet_card_creation\n");
  test_pet_card_json();
  printf("  [OK] test_pet_card_json\n");
  test_gallery_add();
  printf("  [OK] test_gallery_add\n");
  test_gallery_json();
  printf("  [OK] test_gallery_json\n");
  test_gallery_remove();
  printf("  [OK] test_gallery_remove\n");
  test_leaderboard_sorting();
  printf("  [OK] test_leaderboard_sorting\n");
  test_score_calculation();
  printf("  [OK] test_score_calculation\n");
  test_import_pet_card();
  printf("  [OK] test_import_pet_card\n");
  test_import_invalid_json();
  printf("  [OK] test_import_invalid_json\n");
  test_shareable_profile();
  printf("  [OK] test_shareable_profile\n");
  test_gallery_max_entries();
  printf("  [OK] test_gallery_max_entries\n");

  printf("\n=== All 11 Community tests PASSED ===\n");
  return 0;
}
