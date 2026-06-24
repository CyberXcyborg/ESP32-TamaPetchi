// ============================================================
// test_phase22_5.cpp — Native tests for BLE Trade Game (Phase 22.5)
// Tests BLE trade game state machine, flow, payload building
// ============================================================

#include "Arduino.h"
#include "BLETradeGame.h"
#include <cstdio>
#include <cstring>
#include <cassert>

// ============================================================
// BLE Trade Game State Tests
// ============================================================

static void test_trade_game_init() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();
  assert(game.getState() == BLE_TRADE_IDLE);

  printf("  [PASS] test_trade_game_init\n");
}

static void test_trade_start() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  bool ok = game.startTrade();
  assert(ok);
  assert(game.getState() == BLE_TRADE_SCANNING);

  printf("  [PASS] test_trade_start\n");
}

static void test_trade_cancel() {
  BLETradeGame &game = BLETradeGame::getInstance();

  // Cancel current trade
  bool ok = game.cancelTrade();
  assert(ok);
  assert(game.getState() == BLE_TRADE_CANCELLED);

  printf("  [PASS] test_trade_cancel\n");
}

static void test_trade_payload_build() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  NFCTradePayload payload;
  bool ok = game.buildTradePayload(payload);
  assert(ok);
  assert(strncmp(payload.magic, "TAMA", 4) == 0);
  assert(payload.version == 1);

  printf("  [PASS] test_trade_payload_build\n");
}

static void test_trade_payload_process() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Build a valid payload with checksum
  NFCTradePayload payload = {};
  strncpy(payload.magic, "TAMA", 4);
  payload.version = 1;
  strncpy(payload.petName, "TradePet", sizeof(payload.petName) - 1);
  payload.petType = 1;
  payload.petStage = 2;
  payload.petAge = 1000;
  payload.hunger = 50;
  payload.happiness = 70;
  payload.health = 90;
  payload.energy = 60;

  // Calculate checksum (field-by-field to avoid struct padding issues)
  payload.checksum = 0;
  {
    const uint8_t *magic = (const uint8_t *)payload.magic;
    for (size_t i = 0; i < sizeof(payload.magic); i++) payload.checksum ^= magic[i];
    payload.checksum ^= payload.version;
    const uint8_t *name = (const uint8_t *)payload.petName;
    for (size_t i = 0; i < sizeof(payload.petName); i++) payload.checksum ^= name[i];
    payload.checksum ^= payload.petType;
    payload.checksum ^= payload.petStage;
    payload.checksum ^= (uint8_t)(payload.petAge & 0xFF);
    payload.checksum ^= (uint8_t)((payload.petAge >> 8) & 0xFF);
    payload.checksum ^= payload.hunger;
    payload.checksum ^= payload.happiness;
    payload.checksum ^= payload.health;
    payload.checksum ^= payload.energy;
  }

  bool ok = game.processReceivedPayload(payload);
  assert(ok);

  printf("  [PASS] test_trade_payload_process\n");
}

static void test_trade_payload_invalid() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Invalid magic
  NFCTradePayload payload = {};
  strncpy(payload.magic, "XXXX", 4);
  payload.version = 1;

  bool ok = game.processReceivedPayload(payload);
  assert(!ok);  // Should fail

  printf("  [PASS] test_trade_payload_invalid\n");
}

static void test_trade_payload_checksum_fail() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Valid magic but bad checksum
  NFCTradePayload payload = {};
  strncpy(payload.magic, "TAMA", 4);
  payload.version = 1;
  payload.checksum = 0xFF;  // Wrong checksum

  bool ok = game.processReceivedPayload(payload);
  assert(!ok);  // Should fail

  printf("  [PASS] test_trade_payload_checksum_fail\n");
}

static void test_trade_receive_offer() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Simulate receiving a trade offer
  bool ok = game.receiveTradeOffer("AA:BB:CC:DD:EE:FF", "Buddy", "1234");
  assert(ok);
  assert(game.getState() == BLE_TRADE_OFFER_RECEIVED);

  const BLETradeSession &session = game.getSession();
  assert(strcmp(session.peerAddress, "AA:BB:CC:DD:EE:FF") == 0);
  assert(strcmp(session.remotePetName, "Buddy") == 0);
  assert(!session.isInitiator);

  printf("  [PASS] test_trade_receive_offer\n");
}

static void test_trade_accept_offer() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Receive offer first
  game.receiveTradeOffer("AA:BB:CC:DD:EE:FF", "Buddy", "1234");
  assert(game.getState() == BLE_TRADE_OFFER_RECEIVED);

  // Accept
  bool ok = game.acceptTrade();
  assert(ok);
  assert(game.getState() == BLE_TRADE_NFC_PENDING);

  printf("  [PASS] test_trade_accept_offer\n");
}

static void test_trade_reject_offer() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Receive offer first
  game.receiveTradeOffer("AA:BB:CC:DD:EE:FF", "Buddy", "1234");

  // Reject
  bool ok = game.rejectTrade();
  assert(ok);
  assert(game.getState() == BLE_TRADE_IDLE);

  printf("  [PASS] test_trade_reject_offer\n");
}

static void test_trade_accept_without_offer() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // Try to accept without receiving offer
  bool ok = game.acceptTrade();
  assert(!ok);  // Should fail

  printf("  [PASS] test_trade_accept_without_offer\n");
}

static void test_trade_state_string() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  assert(game.getStateString() == "Idle");
  game.startTrade();
  assert(game.getStateString() == "Scanning");
  game.cancelTrade();
  assert(game.getStateString() == "Cancelled");

  printf("  [PASS] test_trade_state_string\n");
}

static void test_trade_error_handling() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  String err = game.getLastError();
  assert(err.length() == 0);

  // Cause an error
  game.acceptTrade();  // No offer received
  err = game.getLastError();
  assert(err.length() > 0);

  // Clear error
  game.clearError();
  err = game.getLastError();
  assert(err.length() == 0);

  printf("  [PASS] test_trade_error_handling\n");
}

static void test_trade_session_data() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  game.receiveTradeOffer("11:22:33:44:55:66", "Max", "5678");
  const BLETradeSession &session = game.getSession();

  assert(strcmp(session.peerAddress, "11:22:33:44:55:66") == 0);
  assert(strcmp(session.remotePetName, "Max") == 0);
  assert(strcmp((char*)session.tradePin, "5678") == 0);
  assert(session.state == BLE_TRADE_OFFER_RECEIVED);
  assert(!session.isInitiator);

  printf("  [PASS] test_trade_session_data\n");
}

static void test_trade_peer_selection() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  // In stub mode, no peers discovered
  assert(game.getDiscoveredPeerCount() == 0);
  const BLEPeer *peer = game.getDiscoveredPeer(0);
  assert(peer == nullptr);

  printf("  [PASS] test_trade_peer_selection\n");
}

static void test_trade_double_start() {
  BLETradeGame &game = BLETradeGame::getInstance();
  game.begin();

  bool ok = game.startTrade();
  assert(ok);

  // Try to start another trade while one is active
  ok = game.startTrade();
  assert(!ok);  // Should fail

  game.cancelTrade();

  printf("  [PASS] test_trade_double_start\n");
}

// ============================================================
// Test Runner
// ============================================================

int run_phase22_5_tests() {
  printf("\n--- Phase 22.5: BLE Trade Game Tests ---\n");

  test_trade_game_init();
  test_trade_start();
  test_trade_cancel();
  test_trade_payload_build();
  test_trade_payload_process();
  test_trade_payload_invalid();
  test_trade_payload_checksum_fail();
  test_trade_receive_offer();
  test_trade_accept_offer();
  test_trade_reject_offer();
  test_trade_accept_without_offer();
  test_trade_state_string();
  test_trade_error_handling();
  test_trade_session_data();
  test_trade_peer_selection();
  test_trade_double_start();

  printf("--- Phase 22.5: 16/16 tests passed ---\n");
  return 0;
}
