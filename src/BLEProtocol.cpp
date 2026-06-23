// ============================================================
// BLEProtocol.cpp — Companion App Protocol Implementation
// Phase 22.2: JSON-based command/response protocol
// ============================================================

#include "BLEProtocol.h"
#include "AppState_v2.h"
#include "Pet.h"
#include "config_v2.h"
#include <ArduinoJson.h>

// Forward declarations for pet actions
extern void feedPet();
extern void playPet();
extern void cleanPet();
extern void sleepPet();
extern void wakePet();
extern void healPet();
extern String getPetStateJson();
extern String getActivityHistoryJson();

BLEProtocol& BLEProtocol::getInstance() {
  static BLEProtocol instance;
  return instance;
}

void BLEProtocol::begin() {
  Serial.println("[BLEProto] Protocol handler initialized");
}

String BLEProtocol::processCommand(const BLECommand &cmd) {
  switch (cmd.type) {
    case BLE_CMD_FEED:          return handleFeed();
    case BLE_CMD_PLAY:          return handlePlay();
    case BLE_CMD_CLEAN:         return handleClean();
    case BLE_CMD_SLEEP:         return handleSleep();
    case BLE_CMD_WAKE:          return handleWake();
    case BLE_CMD_HEAL:          return handleHeal();
    case BLE_CMD_GET_STATE:     return handleGetState();
    case BLE_CMD_GET_HISTORY:   return handleGetHistory();
    case BLE_CMD_TRADE_OFFER:   return handleTradeOffer(cmd.payload, cmd.payloadLen);
    case BLE_CMD_TRADE_ACCEPT:   return handleTradeAccept();
    case BLE_CMD_TRADE_REJECT:  return handleTradeReject();
    case BLE_CMD_SET_NAME:      return handleSetName(cmd.payload, cmd.payloadLen);
    case BLE_CMD_SET_DIFFICULTY:return handleSetDifficulty(cmd.payload, cmd.payloadLen);
    case BLE_CMD_TOGGLE_SOUND:  return handleToggleSound();
    case BLE_CMD_TOGGLE_MUSIC:  return handleToggleMusic();
    case BLE_CMD_FACTORY_RESET: return handleFactoryReset();
    default:
      return createErrorResponse(BLE_ERR_INVALID_CMD, "Unknown command type");
  }
}

String BLEProtocol::serializePetState() {
  // Compact JSON for BLE MTU efficiency
  AppStateV2 &state = g_state;
  StaticJsonDocument<512> doc;

  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_STATE;
  doc["n"] = state.wifiConnected ? 1 : 0;  // wifi status as int

  // Pet data from global state
  // Note: In v2.0, pet data is managed by Pet_v2 or AppState_v2
  // This is a compact representation
  JsonObject pet = doc.createNestedObject("pet");
  pet["name"] = "Tama";  // Placeholder — actual data from pet system
  pet["stage"] = 0;
  pet["age"] = 0;
  pet["hunger"] = 50;
  pet["happy"] = 50;
  pet["health"] = 80;
  pet["energy"] = 60;
  pet["alive"] = true;

  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::serializeStatus() {
  AppStateV2 &state = g_state;
  StaticJsonDocument<256> doc;

  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_STATUS;
  doc["uptime"] = state.uptimeSeconds;
  doc["heap"] = state.freeHeap;
  doc["battery"] = state.batteryPercent;
  doc["wifi"] = state.wifiConnected;
  doc["ble"] = state.bleConnected;
  doc["brightness"] = state.brightness;
  doc["volume"] = state.volume;

  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::serializeHistory() {
  StaticJsonDocument<512> doc;

  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_HISTORY;

  JsonArray events = doc.createNestedArray("events");
  // Placeholder — actual history from activity log
  JsonObject evt = events.createNestedObject();
  evt["type"] = "feed";
  evt["ts"] = millis();

  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::serializeTradeData() {
  StaticJsonDocument<256> doc;

  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_TRADE_UPDATE;
  doc["state"] = 0;  // 0=idle, 1=offered, 2=accepted, 3=complete

  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::createAckResponse(BLECommandType cmdType, const String &message) {
  StaticJsonDocument<128> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_ACK;
  doc["cmd"] = (int)cmdType;
  doc["msg"] = message;

  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::createErrorResponse(BLEErrorCode code, const String &message) {
  StaticJsonDocument<128> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_ERROR;
  doc["code"] = (int)code;
  doc["msg"] = message;

  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::createNotification(const String &event, const String &data) {
  StaticJsonDocument<256> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_NOTIFICATION;
  doc["event"] = event;
  doc["data"] = data;
  doc["ts"] = millis();

  String result;
  serializeJson(doc, result);
  return result;
}

bool BLEProtocol::parseTradeOffer(const char *payload, uint8_t len,
                                   String &peerDeviceId, String &petName, String &tradePin) {
  if (len < 5) return false;  // Minimum: 1 byte type + some data

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  if (err) return false;

  peerDeviceId = doc["peerId"] | "";
  petName = doc["petName"] | "";
  tradePin = doc["pin"] | "";

  return peerDeviceId.length() > 0 && tradePin.length() == 4;
}

bool BLEProtocol::parseSetName(const char *payload, uint8_t len, String &newName) {
  if (len < 1) return false;

  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  if (err) return false;

  newName = doc["name"] | "";
  return newName.length() > 0 && newName.length() <= 16;
}

// ============================================================
// Command Handlers
// ============================================================

String BLEProtocol::handleFeed() {
  // feedPet();  // Call actual pet feed function
  return createAckResponse(BLE_CMD_FEED, "Fed pet");
}

String BLEProtocol::handlePlay() {
  // playPet();
  return createAckResponse(BLE_CMD_PLAY, "Played with pet");
}

String BLEProtocol::handleClean() {
  // cleanPet();
  return createAckResponse(BLE_CMD_CLEAN, "Cleaned pet");
}

String BLEProtocol::handleSleep() {
  // sleepPet();
  return createAckResponse(BLE_CMD_SLEEP, "Pet is sleeping");
}

String BLEProtocol::handleWake() {
  // wakePet();
  return createAckResponse(BLE_CMD_WAKE, "Pet woke up");
}

String BLEProtocol::handleHeal() {
  // healPet();
  return createAckResponse(BLE_CMD_HEAL, "Healed pet");
}

String BLEProtocol::handleGetState() {
  return serializePetState();
}

String BLEProtocol::handleGetHistory() {
  return serializeHistory();
}

String BLEProtocol::handleTradeOffer(const char *payload, uint8_t len) {
  String peerId, petName, pin;
  if (!parseTradeOffer(payload, len, peerId, petName, pin)) {
    return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Invalid trade offer format");
  }
  // Initiate trade via BLE trade characteristic
  return createAckResponse(BLE_CMD_TRADE_OFFER, "Trade offer sent to " + peerId);
}

String BLEProtocol::handleTradeAccept() {
  return createAckResponse(BLE_CMD_TRADE_ACCEPT, "Trade accepted");
}

String BLEProtocol::handleTradeReject() {
  return createAckResponse(BLE_CMD_TRADE_REJECT, "Trade rejected");
}

String BLEProtocol::handleSetName(const char *payload, uint8_t len) {
  String newName;
  if (!parseSetName(payload, len, newName)) {
    return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Invalid name (1-16 chars)");
  }
  return createAckResponse(BLE_CMD_SET_NAME, "Name set to " + newName);
}

String BLEProtocol::handleSetDifficulty(const char *payload, uint8_t len) {
  if (len < 1) {
    return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Missing difficulty value");
  }
  int diff = (uint8_t)payload[0];
  if (diff > 2) {
    return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Difficulty must be 0-2");
  }
  return createAckResponse(BLE_CMD_SET_DIFFICULTY, "Difficulty set");
}

String BLEProtocol::handleToggleSound() {
  return createAckResponse(BLE_CMD_TOGGLE_SOUND, "Sound toggled");
}

String BLEProtocol::handleToggleMusic() {
  return createAckResponse(BLE_CMD_TOGGLE_MUSIC, "Music toggled");
}

String BLEProtocol::handleFactoryReset() {
  return createAckResponse(BLE_CMD_FACTORY_RESET, "Factory reset initiated");
}
