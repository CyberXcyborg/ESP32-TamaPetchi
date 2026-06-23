// ============================================================
// BLE_NFC_Native.cpp — Native stubs for BLE & NFC modules
// Provides test implementations of BLEManager, BLEProtocol,
// BLEDiscovery, and NFCManager for native unit tests.
// ============================================================

#include "BLEManager.h"
#include "BLEProtocol.h"
#include "BLEDiscovery.h"
#include "NFCManager.h"
#include "AppState_v2.h"
#include <ArduinoJson.h>
#include <cstring>
#include <cstdio>

// ============================================================
// BLE Manager Stub Implementation
// ============================================================

BLEManager::BLEManager()
    : _state(BLE_OFF)
    , _mtu(23)
    , _lastActivity(0)
    , _cmdHead(0)
    , _cmdTail(0)
    , _cmdCount(0)
{}

BLEManager& BLEManager::getInstance() {
  static BLEManager instance;
  return instance;
}

bool BLEManager::begin() {
  _state = BLE_IDLE;
  g_state.bleConnected = false;
  return true;
}

void BLEManager::end() {
  _state = BLE_OFF;
  g_state.bleConnected = false;
}

void BLEManager::update() {}

BLEState BLEManager::getState() const { return _state; }
bool BLEManager::isConnected() const { return _state == BLE_CONNECTED; }
uint8_t BLEManager::getConnectedCount() const {
  return (_state == BLE_CONNECTED) ? 1 : 0;
}

void BLEManager::notifyPetState(const String &jsonState) {
  (void)jsonState;
}

void BLEManager::notifyHistory(const String &jsonHistory) {
  (void)jsonHistory;
}

void BLEManager::notifyStatus(const String &jsonStatus) {
  (void)jsonStatus;
}

void BLEManager::notifyTrade(const String &jsonTradeData) {
  (void)jsonTradeData;
}

bool BLEManager::hasCommand() const { return _cmdCount > 0; }

BLECommand BLEManager::getCommand() {
  BLECommand cmd = {};
  if (_cmdCount == 0) return cmd;
  cmd = _cmdQueue[_cmdTail];
  _cmdTail = (_cmdTail + 1) % CMD_QUEUE_SIZE;
  _cmdCount--;
  return cmd;
}

void BLEManager::clearCommand() {
  _cmdHead = 0;
  _cmdTail = 0;
  _cmdCount = 0;
}

void BLEManager::startAdvertising() { _state = BLE_ADVERTISING; }
void BLEManager::stopAdvertising() { _state = BLE_IDLE; }
uint16_t BLEManager::getMTU() const { return _mtu; }
String BLEManager::getDeviceAddress() const { return "AA:BB:CC:DD:EE:FF"; }

void BLEManager::enqueueCommand(const BLECommand &cmd) {
  if (_cmdCount >= CMD_QUEUE_SIZE) return;
  _cmdQueue[_cmdHead] = cmd;
  _cmdHead = (_cmdHead + 1) % CMD_QUEUE_SIZE;
  _cmdCount++;
}

// ============================================================
// BLE Protocol Stub Implementation
// ============================================================

BLEProtocol& BLEProtocol::getInstance() {
  static BLEProtocol instance;
  return instance;
}

void BLEProtocol::begin() {}

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
    case BLE_CMD_TRADE_ACCEPT:  return handleTradeAccept();
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
  StaticJsonDocument<512> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_STATE;
  JsonObject pet = doc.createNestedObject("pet");
  pet["name"] = "Tama";
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
  StaticJsonDocument<256> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_STATUS;
  doc["uptime"] = g_state.uptimeSeconds;
  doc["heap"] = g_state.freeHeap;
  doc["battery"] = g_state.batteryPercent;
  doc["wifi"] = g_state.wifiConnected;
  doc["ble"] = g_state.bleConnected;
  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::serializeHistory() {
  StaticJsonDocument<512> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_HISTORY;
  JsonArray events = doc.createNestedArray("events");
  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::serializeTradeData() {
  StaticJsonDocument<256> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_TRADE_UPDATE;
  doc["state"] = 0;
  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::createAckResponse(BLECommandType cmdType, const String &message) {
  StaticJsonDocument<128> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_ACK;
  doc["cmd"] = (int)cmdType;
  doc["msg"] = message.c_str();
  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::createErrorResponse(BLEErrorCode code, const String &message) {
  StaticJsonDocument<128> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_ERROR;
  doc["code"] = (int)code;
  doc["msg"] = message.c_str();
  String result;
  serializeJson(doc, result);
  return result;
}

String BLEProtocol::createNotification(const String &event, const String &data) {
  StaticJsonDocument<256> doc;
  doc["v"] = BLE_PROTOCOL_VERSION;
  doc["t"] = BLE_RESP_NOTIFICATION;
  doc["event"] = event.c_str();
  doc["data"] = data.c_str();
  doc["ts"] = millis();
  String result;
  serializeJson(doc, result);
  return result;
}

bool BLEProtocol::parseTradeOffer(const char *payload, uint8_t len,
                                   String &peerDeviceId, String &petName, String &tradePin) {
  if (len < 5) return false;
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

String BLEProtocol::handleFeed() { return createAckResponse(BLE_CMD_FEED, "Fed pet"); }
String BLEProtocol::handlePlay() { return createAckResponse(BLE_CMD_PLAY, "Played with pet"); }
String BLEProtocol::handleClean() { return createAckResponse(BLE_CMD_CLEAN, "Cleaned pet"); }
String BLEProtocol::handleSleep() { return createAckResponse(BLE_CMD_SLEEP, "Pet is sleeping"); }
String BLEProtocol::handleWake() { return createAckResponse(BLE_CMD_WAKE, "Pet woke up"); }
String BLEProtocol::handleHeal() { return createAckResponse(BLE_CMD_HEAL, "Healed pet"); }
String BLEProtocol::handleGetState() { return serializePetState(); }
String BLEProtocol::handleGetHistory() { return serializeHistory(); }

String BLEProtocol::handleTradeOffer(const char *payload, uint8_t len) {
  String peerId, petName, pin;
  if (!parseTradeOffer(payload, len, peerId, petName, pin)) {
    return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Invalid trade offer format");
  }
  return createAckResponse(BLE_CMD_TRADE_OFFER, "Trade offer sent to " + peerId);
}

String BLEProtocol::handleTradeAccept() { return createAckResponse(BLE_CMD_TRADE_ACCEPT, "Trade accepted"); }
String BLEProtocol::handleTradeReject() { return createAckResponse(BLE_CMD_TRADE_REJECT, "Trade rejected"); }

String BLEProtocol::handleSetName(const char *payload, uint8_t len) {
  String newName;
  if (!parseSetName(payload, len, newName)) {
    return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Invalid name (1-16 chars)");
  }
  return createAckResponse(BLE_CMD_SET_NAME, "Name set to " + newName);
}

String BLEProtocol::handleSetDifficulty(const char *payload, uint8_t len) {
  if (len < 1) return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Missing difficulty value");
  int diff = (uint8_t)payload[0];
  if (diff > 2) return createErrorResponse(BLE_ERR_INVALID_PAYLOAD, "Difficulty must be 0-2");
  return createAckResponse(BLE_CMD_SET_DIFFICULTY, "Difficulty set");
}

String BLEProtocol::handleToggleSound() { return createAckResponse(BLE_CMD_TOGGLE_SOUND, "Sound toggled"); }
String BLEProtocol::handleToggleMusic() { return createAckResponse(BLE_CMD_TOGGLE_MUSIC, "Music toggled"); }
String BLEProtocol::handleFactoryReset() { return createAckResponse(BLE_CMD_FACTORY_RESET, "Factory reset initiated"); }

// ============================================================
// BLE Discovery Stub Implementation
// ============================================================

BLEDiscovery::BLEDiscovery()
    : _state(BLE_DISC_OFF)
    , _peerCount(0)
    , _rssiThreshold(BLE_RSSI_THRESHOLD)
    , _lastScanTime(0)
    , _initialized(false)
{
  memset(_peers, 0, sizeof(_peers));
}

BLEDiscovery& BLEDiscovery::getInstance() {
  static BLEDiscovery instance;
  return instance;
}

bool BLEDiscovery::begin() {
  _state = BLE_DISC_IDLE;
  _initialized = true;
  return true;
}

void BLEDiscovery::end() {
  _state = BLE_DISC_OFF;
  _initialized = false;
}

void BLEDiscovery::update() {
  if (!_initialized) return;
  if (_state == BLE_DISC_SCANNING) {
    _state = BLE_DISC_COMPLETE;
  }
}

bool BLEDiscovery::startScan(uint32_t durationMs) {
  if (!_initialized) return false;
  _state = BLE_DISC_SCANNING;
  _lastScanTime = millis();
  return true;
}

void BLEDiscovery::stopScan() {
  if (_state == BLE_DISC_SCANNING) _state = BLE_DISC_IDLE;
}

bool BLEDiscovery::isScanning() const { return _state == BLE_DISC_SCANNING; }
uint8_t BLEDiscovery::getPeerCount() const { return _peerCount; }

const BLEPeer* BLEDiscovery::getPeer(uint8_t index) const {
  if (index >= BLE_MAX_DISCOVERED) return nullptr;
  return _peers[index].active ? &_peers[index] : nullptr;
}

const BLEPeer* BLEDiscovery::findPeerByAddress(const String &address) const {
  for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
    if (_peers[i].active && address.equals(_peers[i].address)) return &_peers[i];
  }
  return nullptr;
}

const BLEPeer* BLEDiscovery::findNearestTamaPetchi() const {
  const BLEPeer *best = nullptr;
  int8_t bestRssi = -128;
  for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
    if (_peers[i].active && _peers[i].isTamaPetchi && _peers[i].rssi > bestRssi) {
      best = &_peers[i];
      bestRssi = _peers[i].rssi;
    }
  }
  return best;
}

void BLEDiscovery::setRssiThreshold(int8_t rssi) { _rssiThreshold = rssi; }
int8_t BLEDiscovery::getRssiThreshold() const { return _rssiThreshold; }
BLEDiscoveryState BLEDiscovery::getState() const { return _state; }

void BLEDiscovery::clearResults() {
  memset(_peers, 0, sizeof(_peers));
  _peerCount = 0;
}

void BLEDiscovery::addOrUpdatePeer(const char *address, const char *name, int8_t rssi, bool isTama) {
  for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
    if (_peers[i].active && strcmp(_peers[i].address, address) == 0) {
      _peers[i].rssi = rssi;
      _peers[i].lastSeen = millis();
      _peers[i].isTamaPetchi = isTama;
      return;
    }
  }
  if (_peerCount < BLE_MAX_DISCOVERED) {
    for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
      if (!_peers[i].active) {
        strncpy(_peers[i].address, address, sizeof(_peers[i].address) - 1);
        strncpy(_peers[i].name, name, sizeof(_peers[i].name) - 1);
        _peers[i].rssi = rssi;
        _peers[i].lastSeen = millis();
        _peers[i].isTamaPetchi = isTama;
        _peers[i].active = true;
        _peerCount++;
        return;
      }
    }
  }
}

// ============================================================
// NFC Manager Stub Implementation
// ============================================================

NFCManager::NFCManager()
    : _state(NFC_OFF)
    , _lastPollTime(0)
    , _initialized(false)
{}

NFCManager& NFCManager::getInstance() {
  static NFCManager instance;
  return instance;
}

bool NFCManager::begin() {
  _state = NFC_IDLE;
  _initialized = true;
  return true;
}

void NFCManager::end() {
  _state = NFC_OFF;
  _initialized = false;
}

void NFCManager::update() {
  if (!_initialized) return;
  _state = NFC_IDLE;
}

NFCState NFCManager::getState() const { return _state; }
bool NFCManager::isReady() const { return _initialized; }

bool NFCManager::readTag(NFCTagData &tag) {
  memset(&tag, 0, sizeof(tag));
  return false;
}

bool NFCManager::writeTag(const NFCTagData &tag) {
  (void)tag;
  return true;
}

bool NFCManager::writeNDEF(const uint8_t *data, uint16_t len) {
  (void)data; (void)len;
  return true;
}

bool NFCManager::readNDEF(uint8_t *data, uint16_t &len) {
  (void)data; len = 0;
  return false;
}

bool NFCManager::startTradeOffer(const NFCTradePayload &payload) {
  (void)payload;
  _state = NFC_TRADE_PENDING;
  return true;
}

bool NFCManager::readTradeOffer(NFCTradePayload &payload) {
  memset(&payload, 0, sizeof(payload));
  return false;
}

void NFCManager::cancelTrade() { _state = NFC_IDLE; }

bool NFCManager::serializeTradePayload(const NFCTradePayload &payload, uint8_t *out, uint16_t &outLen) {
  if (outLen < sizeof(NFCTradePayload)) return false;
  memcpy(out, &payload, sizeof(NFCTradePayload));
  outLen = sizeof(NFCTradePayload);
  return true;
}

bool NFCManager::deserializeTradePayload(const uint8_t *data, uint16_t len, NFCTradePayload &payload) {
  if (len < sizeof(NFCTradePayload)) return false;
  memcpy(&payload, data, sizeof(NFCTradePayload));
  if (strncmp(payload.magic, "TAMA", 4) != 0) return false;
  uint8_t calcChecksum = 0;
  const uint8_t *p = (const uint8_t *)&payload;
  for (size_t i = 0; i < sizeof(NFCTradePayload) - 2; i++) {
    calcChecksum ^= p[i];
  }
  return calcChecksum == payload.checksum;
}

String NFCManager::getLastError() const { return _lastError; }
void NFCManager::clearError() { _lastError = ""; }

bool NFCManager::pn532WriteCommand(const uint8_t *cmd, uint8_t cmdLen) { (void)cmd; (void)cmdLen; return true; }
bool NFCManager::pn532ReadResponse(uint8_t *response, uint8_t &responseLen, uint16_t timeout) {
  (void)response; (void)timeout; responseLen = 0; return true;
}
bool NFCManager::pn532SAMConfig() { return true; }
bool NFCManager::pn532GetFirmwareVersion(uint8_t &version) { version = 0x01; return true; }

// ============================================================
// AppState_v2 Native Implementation
// Required because src/AppState_v2.cpp is not compiled in native builds.
// ============================================================

AppStateV2::AppStateV2()
    : wifiConnected(false)
    , bleConnected(false)
    , apMode(false)
    , uptimeSeconds(0)
    , freeHeap(327680)
    , batteryPercent(75)
    , displayReady(false)
    , touchReady(false)
    , brightness(255)
    , audioReady(false)
    , volume(80)
{}

AppStateV2& AppStateV2::getInstance() {
    static AppStateV2 instance;
    return instance;
}

void AppStateV2::update() {
    uptimeSeconds = millis() / 1000;
    freeHeap = 327680;
}

void AppStateV2::reset() {
    wifiConnected = false;
    bleConnected = false;
    apMode = false;
    uptimeSeconds = 0;
    displayReady = false;
    touchReady = false;
    audioReady = false;
}
