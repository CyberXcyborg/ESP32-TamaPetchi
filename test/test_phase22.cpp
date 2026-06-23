// ============================================================
// test_phase22.cpp — Native tests for BLE & NFC (Phase 22)
// Tests BLEManager, BLEProtocol, BLEDiscovery, NFCManager
// ============================================================

#include "Arduino.h"
#include "BLEManager.h"
#include "BLEProtocol.h"
#include "BLEDiscovery.h"
#include "NFCManager.h"
#include <cstdio>
#include <cstring>
#include <cassert>

// ============================================================
// BLE Manager Tests
// ============================================================

static void test_ble_manager_init() {
  BLEManager &mgr = BLEManager::getInstance();
  assert(!mgr.isConnected());
  assert(mgr.getState() == BLE_OFF);

  bool ok = mgr.begin();
  assert(ok);
  assert(mgr.getState() == BLE_IDLE);

  printf("  [PASS] test_ble_manager_init\n");
}

static void test_ble_advertising() {
  BLEManager &mgr = BLEManager::getInstance();
  assert(mgr.getState() == BLE_IDLE);

  mgr.startAdvertising();
  assert(mgr.getState() == BLE_ADVERTISING);

  mgr.stopAdvertising();
  assert(mgr.getState() == BLE_IDLE);

  printf("  [PASS] test_ble_advertising\n");
}

static void test_ble_mtu() {
  BLEManager &mgr = BLEManager::getInstance();
  uint16_t mtu = mgr.getMTU();
  assert(mtu == 23);  // Default BLE MTU

  printf("  [PASS] test_ble_mtu\n");
}

static void test_ble_device_address() {
  BLEManager &mgr = BLEManager::getInstance();
  String addr = mgr.getDeviceAddress();
  assert(addr.length() > 0);

  printf("  [PASS] test_ble_device_address\n");
}

static void test_ble_notify_stubs() {
  BLEManager &mgr = BLEManager::getInstance();
  // These should not crash in stub mode
  mgr.notifyPetState("{\"test\":true}");
  mgr.notifyHistory("[]");
  mgr.notifyStatus("{}");
  mgr.notifyTrade("{}");

  printf("  [PASS] test_ble_notify_stubs\n");
}

// ============================================================
// BLE Protocol Tests
// ============================================================

static void test_ble_protocol_init() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  proto.begin();

  printf("  [PASS] test_ble_protocol_init\n");
}

static void test_ble_protocol_ack_response() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  String resp = proto.createAckResponse(BLE_CMD_FEED, "Fed pet");

  // Verify it's valid JSON with expected fields
  assert(resp.indexOf("\"t\":4") >= 0);  // ACK type (BLE_RESP_ACK = 4)
  assert(resp.indexOf("Fed pet") >= 0);

  printf("  [PASS] test_ble_protocol_ack_response\n");
}

static void test_ble_protocol_error_response() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  String resp = proto.createErrorResponse(BLE_ERR_INVALID_CMD, "Unknown command");

  assert(resp.indexOf("\"t\":5") >= 0);  // ERROR type
  assert(resp.indexOf("Unknown command") >= 0);

  printf("  [PASS] test_ble_protocol_error_response\n");
}

static void test_ble_protocol_notification() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  String resp = proto.createNotification("trade", "peer_device");

  assert(resp.indexOf("\"t\":7") >= 0);  // NOTIFICATION type
  assert(resp.indexOf("trade") >= 0);

  printf("  [PASS] test_ble_protocol_notification\n");
}

static void test_ble_protocol_command_feed() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  BLECommand cmd = {};
  cmd.type = BLE_CMD_FEED;
  String resp = proto.processCommand(cmd);
  assert(resp.indexOf("Fed pet") >= 0);

  printf("  [PASS] test_ble_protocol_command_feed\n");
}

static void test_ble_protocol_command_play() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  BLECommand cmd = {};
  cmd.type = BLE_CMD_PLAY;
  String resp = proto.processCommand(cmd);
  assert(resp.indexOf("Played") >= 0);

  printf("  [PASS] test_ble_protocol_command_play\n");
}

static void test_ble_protocol_command_get_state() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  BLECommand cmd = {};
  cmd.type = BLE_CMD_GET_STATE;
  String resp = proto.processCommand(cmd);
  assert(resp.indexOf("\"pet\"") >= 0);

  printf("  [PASS] test_ble_protocol_command_get_state\n");
}

static void test_ble_protocol_command_unknown() {
  BLEProtocol &proto = BLEProtocol::getInstance();
  BLECommand cmd = {};
  cmd.type = (BLECommandType)999;
  String resp = proto.processCommand(cmd);
  assert(resp.indexOf("\"t\":5") >= 0);  // Error

  printf("  [PASS] test_ble_protocol_command_unknown\n");
}

static void test_ble_protocol_trade_offer_parse() {
  BLEProtocol &proto = BLEProtocol::getInstance();

  // Valid trade offer
  const char *payload = "{\"peerId\":\"device123\",\"petName\":\"Fluffy\",\"pin\":\"1234\"}";
  String peerId, petName, pin;
  bool ok = proto.parseTradeOffer(payload, strlen(payload), peerId, petName, pin);
  assert(ok);
  assert(peerId == "device123");
  assert(petName == "Fluffy");
  assert(pin == "1234");

  printf("  [PASS] test_ble_protocol_trade_offer_parse\n");
}

static void test_ble_protocol_set_name_parse() {
  BLEProtocol &proto = BLEProtocol::getInstance();

  const char *payload = "{\"name\":\"Buddy\"}";
  String newName;
  bool ok = proto.parseSetName(payload, strlen(payload), newName);
  assert(ok);
  assert(newName == "Buddy");

  printf("  [PASS] test_ble_protocol_set_name_parse\n");
}

// ============================================================
// BLE Discovery Tests
// ============================================================

static void test_ble_discovery_init() {
  BLEDiscovery &disc = BLEDiscovery::getInstance();
  assert(disc.getState() == BLE_DISC_OFF);

  bool ok = disc.begin();
  assert(ok);
  assert(disc.getState() == BLE_DISC_IDLE);

  printf("  [PASS] test_ble_discovery_init\n");
}

static void test_ble_discovery_scan() {
  BLEDiscovery &disc = BLEDiscovery::getInstance();

  disc.clearResults();
  assert(disc.getPeerCount() == 0);

  bool ok = disc.startScan(100);
  assert(ok);
  // Stub completes scan immediately on update
  disc.update();

  printf("  [PASS] test_ble_discovery_scan\n");
}

static void test_ble_discovery_rssi_threshold() {
  BLEDiscovery &disc = BLEDiscovery::getInstance();

  disc.setRssiThreshold(-70);
  assert(disc.getRssiThreshold() == -70);

  disc.setRssiThreshold(-90);
  assert(disc.getRssiThreshold() == -90);

  printf("  [PASS] test_ble_discovery_rssi_threshold\n");
}

static void test_ble_discovery_find_peer() {
  BLEDiscovery &disc = BLEDiscovery::getInstance();
  disc.clearResults();

  // In stub mode, no peers
  const BLEPeer *peer = disc.findPeerByAddress("AA:BB:CC:DD:EE:FF");
  assert(peer == nullptr);

  const BLEPeer *nearest = disc.findNearestTamaPetchi();
  assert(nearest == nullptr);

  printf("  [PASS] test_ble_discovery_find_peer\n");
}

// ============================================================
// NFC Manager Tests
// ============================================================

static void test_nfc_manager_init() {
  NFCManager &nfc = NFCManager::getInstance();
  assert(nfc.getState() == NFC_OFF);
  assert(!nfc.isReady());

  bool ok = nfc.begin();
  assert(ok);
  assert(nfc.isReady());
  assert(nfc.getState() == NFC_IDLE);

  printf("  [PASS] test_nfc_manager_init\n");
}

static void test_nfc_tag_read_stub() {
  NFCManager &nfc = NFCManager::getInstance();
  NFCTagData tag;

  // Stub returns false (no tag present)
  bool ok = nfc.readTag(tag);
  assert(!ok);

  printf("  [PASS] test_nfc_tag_read_stub\n");
}

static void test_nfc_tag_write_stub() {
  NFCManager &nfc = NFCManager::getInstance();
  NFCTagData tag = {};
  tag.uid[0] = 0x04;
  tag.uidLen = 4;
  tag.dataLen = 10;
  tag.valid = true;

  // Stub returns true
  bool ok = nfc.writeTag(tag);
  assert(ok);

  printf("  [PASS] test_nfc_tag_write_stub\n");
}

static void test_nfc_trade_payload_serialize() {
  NFCTradePayload payload = {};
  strncpy(payload.magic, "TAMA", 4);
  payload.version = 1;
  strncpy(payload.petName, "Fluffy", sizeof(payload.petName) - 1);
  payload.petType = 1;   // CAT
  payload.petStage = 2;  // ADULT
  payload.petAge = 1440;
  payload.hunger = 50;
  payload.happiness = 80;
  payload.health = 90;
  payload.energy = 70;

  // Calculate checksum (exclude checksum byte and trailing padding)
  payload.checksum = 0;
  const uint8_t *p = (const uint8_t *)&payload;
  for (size_t i = 0; i < sizeof(NFCTradePayload) - 2; i++) {
    payload.checksum ^= p[i];
  }

  uint8_t buffer[64];
  uint16_t len = sizeof(buffer);
  bool ok = NFCManager::serializeTradePayload(payload, buffer, len);
  assert(ok);
  assert(len == sizeof(NFCTradePayload));

  printf("  [PASS] test_nfc_trade_payload_serialize\\n");
}

static void test_nfc_trade_payload_deserialize() {
  NFCTradePayload original = {};
  strncpy(original.magic, "TAMA", 4);
  original.version = 1;
  strncpy(original.petName, "Fluffy", sizeof(original.petName) - 1);
  original.petType = 1;
  original.petStage = 2;
  original.petAge = 1440;
  original.hunger = 50;
  original.happiness = 80;
  original.health = 90;
  original.energy = 70;

  // Calculate checksum (exclude checksum byte and padding)
  original.checksum = 0;
  const uint8_t *p = (const uint8_t *)&original;
  for (size_t i = 0; i < sizeof(NFCTradePayload) - 2; i++) {
    original.checksum ^= p[i];
  }

  uint8_t buffer[64];
  uint16_t len = sizeof(buffer);
  NFCManager::serializeTradePayload(original, buffer, len);

  NFCTradePayload decoded;
  bool ok = NFCManager::deserializeTradePayload(buffer, len, decoded);
  assert(ok);
  assert(strncmp(decoded.magic, "TAMA", 4) == 0);
  assert(decoded.version == 1);
  assert(decoded.petType == 1);
  assert(decoded.petStage == 2);
  assert(decoded.petAge == 1440);

  printf("  [PASS] test_nfc_trade_payload_deserialize\n");
}

static void test_nfc_trade_payload_invalid_magic() {
  uint8_t buffer[64];
  memset(buffer, 0, sizeof(buffer));
  buffer[0] = 'X';  // Invalid magic

  NFCTradePayload decoded;
  bool ok = NFCManager::deserializeTradePayload(buffer, sizeof(buffer), decoded);
  assert(!ok);  // Should fail - invalid magic

  printf("  [PASS] test_nfc_trade_payload_invalid_magic\n");
}

static void test_nfc_trade_offer_lifecycle() {
  NFCManager &nfc = NFCManager::getInstance();

  NFCTradePayload payload = {};
  strncpy(payload.magic, "TAMA", 4);
  payload.version = 1;
  strncpy(payload.petName, "Fluffy", sizeof(payload.petName) - 1);
  payload.petType = 1;

  // Start trade offer
  bool ok = nfc.startTradeOffer(payload);
  assert(ok);
  assert(nfc.getState() == NFC_TRADE_PENDING);

  // Read trade offer (stub returns false)
  NFCTradePayload received;
  ok = nfc.readTradeOffer(received);
  assert(!ok);  // No tag in stub

  // Cancel
  nfc.cancelTrade();
  assert(nfc.getState() == NFC_IDLE);

  printf("  [PASS] test_nfc_trade_offer_lifecycle\n");
}

static void test_nfc_error_handling() {
  NFCManager &nfc = NFCManager::getInstance();

  String err = nfc.getLastError();
  assert(err.length() == 0);  // No error yet

  nfc.clearError();
  err = nfc.getLastError();
  assert(err.length() == 0);

  printf("  [PASS] test_nfc_error_handling\n");
}

// ============================================================
// Test Runner
// ============================================================

int run_phase22_tests() {
  printf("\\n--- Phase 22: BLE & NFC Tests ---\\n");

  // BLE Manager tests
  test_ble_manager_init();
  test_ble_advertising();
  test_ble_mtu();
  test_ble_device_address();
  test_ble_notify_stubs();

  // BLE Protocol tests
  test_ble_protocol_init();
  test_ble_protocol_ack_response();
  test_ble_protocol_error_response();
  test_ble_protocol_notification();
  test_ble_protocol_command_feed();
  test_ble_protocol_command_play();
  test_ble_protocol_command_get_state();
  test_ble_protocol_command_unknown();
  test_ble_protocol_trade_offer_parse();
  test_ble_protocol_set_name_parse();

  // BLE Discovery tests
  test_ble_discovery_init();
  test_ble_discovery_scan();
  test_ble_discovery_rssi_threshold();
  test_ble_discovery_find_peer();

  // NFC Manager tests
  test_nfc_manager_init();
  test_nfc_tag_read_stub();
  test_nfc_tag_write_stub();
  test_nfc_trade_payload_serialize();
  test_nfc_trade_payload_deserialize();
  test_nfc_trade_payload_invalid_magic();
  test_nfc_trade_offer_lifecycle();
  test_nfc_error_handling();

  // Total: 22 tests
  printf("--- Phase 22: 22/22 tests passed ---\\n");
  return 0;
}
