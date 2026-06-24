// ============================================================
// BLETradeGame_Native.cpp — Native stubs for BLE Trade Game
// Provides test implementations for native unit tests
// ============================================================

#include "BLETradeGame.h"
#include "BLEDiscovery.h"
#include "NFCManager.h"
#include "Pet_v2.h"
#include <cstring>
#include <cstdio>

// ============================================================
// PetEngine Native Stub
// Required because BLETradeGame calls PetEngine().getData()
// but Pet_v2.cpp is not compiled in native builds (it pulls in
// too many ESP32-specific dependencies via config_v2.h).
// ============================================================
PetEngine::PetEngine() {
    // Field-by-field initialization — PetData has String members, memset is UB
    _data.name = "StubPet";
    _data.hunger = 50;
    _data.happiness = 60;
    _data.energy = 70;
    _data.cleanliness = 80;
    _data.health = 90;
    _data.age_minutes = 0;
    _data.birth_timestamp = 0;
    _data.stage = PET_STAGE_BABY;
    _data.state = PET_STATE_IDLE;
    _data.generation = 0;
}

BLETradeGame::BLETradeGame()
    : _initialized(false)
{
  memset(&_session, 0, sizeof(_session));
  _session.state = BLE_TRADE_IDLE;
}

BLETradeGame& BLETradeGame::getInstance() {
  static BLETradeGame instance;
  return instance;
}

bool BLETradeGame::begin() {
  memset(&_session, 0, sizeof(_session));
  _session.state = BLE_TRADE_IDLE;
  _initialized = true;
  return true;
}

void BLETradeGame::end() {
  _session.state = BLE_TRADE_IDLE;
  _initialized = false;
}

void BLETradeGame::update() {
  if (!_initialized) return;

  // In native stub, scanning completes immediately
  if (_session.state == BLE_TRADE_SCANNING) {
    _session.discoveredCount = 0;
    setState(BLE_TRADE_PEER_FOUND);
  }

  // NFC operations complete immediately in stub
  if (_session.state == BLE_TRADE_NFC_EXCHANGING) {
    completeTrade();
  }
}

bool BLETradeGame::startTrade() {
  if (!_initialized) return false;
  if (_session.state != BLE_TRADE_IDLE) {
    _lastError = "Trade already in progress";
    return false;
  }

  memset(&_session, 0, sizeof(_session));
  _session.isInitiator = true;
  _session.startTime = millis();
  _session.lastActivity = _session.startTime;

  setState(BLE_TRADE_SCANNING);
  return true;
}

bool BLETradeGame::cancelTrade() {
  if (_session.state == BLE_TRADE_IDLE) return true;

  NFCManager &nfc = NFCManager::getInstance();
  nfc.cancelTrade();

  setState(BLE_TRADE_CANCELLED);
  return true;
}

bool BLETradeGame::acceptTrade() {
  if (_session.state != BLE_TRADE_OFFER_RECEIVED) {
    _lastError = "No trade offer to accept";
    return false;
  }

  _session.lastActivity = millis();
  setState(BLE_TRADE_NFC_PENDING);
  return true;
}

bool BLETradeGame::rejectTrade() {
  if (_session.state != BLE_TRADE_OFFER_RECEIVED) {
    _lastError = "No trade offer to reject";
    return false;
  }

  setState(BLE_TRADE_IDLE);
  return true;
}

uint8_t BLETradeGame::getDiscoveredPeerCount() const {
  return _session.discoveredCount;
}

const BLEPeer* BLETradeGame::getDiscoveredPeer(uint8_t index) const {
  if (index >= _session.discoveredCount) return nullptr;
  return &_session.discoveredPeers[index];
}

bool BLETradeGame::selectPeer(uint8_t index) {
  if (index >= _session.discoveredCount) {
    _lastError = "Invalid peer index";
    return false;
  }
  _session.selectedPeerIdx = index;
  const BLEPeer *peer = &_session.discoveredPeers[index];
  strncpy(_session.peerAddress, peer->address, sizeof(_session.peerAddress) - 1);
  strncpy(_session.peerName, peer->name, sizeof(_session.peerName) - 1);
  return true;
}

bool BLETradeGame::sendTradeOffer() {
  if (_session.state != BLE_TRADE_PEER_FOUND &&
      _session.state != BLE_TRADE_SCANNING) {
    _lastError = "No peer selected";
    return false;
  }

  PetEngine engine;
  const PetData &pet = engine.getData();
  strncpy(_session.localPetName, pet.name.c_str(), sizeof(_session.localPetName) - 1);

  uint32_t pin = (millis() % 9000) + 1000;
  snprintf((char*)_session.tradePin, sizeof(_session.tradePin), "%04lu", pin);

  setState(BLE_TRADE_OFFER_SENT);
  return true;
}

bool BLETradeGame::receiveTradeOffer(const char *peerAddress, const char *petName, const char *pin) {
  if (_session.state != BLE_TRADE_IDLE &&
      _session.state != BLE_TRADE_SCANNING) {
    _lastError = "Cannot receive offer during active trade";
    return false;
  }

  strncpy(_session.peerAddress, peerAddress, sizeof(_session.peerAddress) - 1);
  strncpy(_session.remotePetName, petName, sizeof(_session.remotePetName) - 1);
  strncpy((char*)_session.tradePin, pin, sizeof(_session.tradePin) - 1);
  _session.isInitiator = false;
  _session.startTime = millis();
  _session.lastActivity = _session.startTime;

  setState(BLE_TRADE_OFFER_RECEIVED);
  return true;
}

bool BLETradeGame::writeTradeDataToNFC() {
  if (_session.state != BLE_TRADE_NFC_PENDING) {
    _lastError = "Not in NFC pending state";
    return false;
  }

  NFCTradePayload payload;
  if (!buildTradePayload(payload)) {
    _lastError = "Failed to build trade payload";
    return false;
  }

  NFCManager &nfc = NFCManager::getInstance();
  if (!nfc.startTradeOffer(payload)) {
    _lastError = "Failed to start NFC trade";
    return false;
  }

  setState(BLE_TRADE_NFC_EXCHANGING);
  return true;
}

bool BLETradeGame::readTradeDataFromNFC() {
  NFCManager &nfc = NFCManager::getInstance();
  NFCTradePayload payload;

  if (!nfc.readTradeOffer(payload)) {
    _lastError = "Failed to read NFC trade data";
    return false;
  }

  if (!processReceivedPayload(payload)) {
    _lastError = "Invalid trade payload";
    return false;
  }

  completeTrade();
  return true;
}

BLETradeState BLETradeGame::getState() const {
  return _session.state;
}

const BLETradeSession& BLETradeGame::getSession() const {
  return _session;
}

String BLETradeGame::getStateString() const {
  switch (_session.state) {
    case BLE_TRADE_IDLE:           return "Idle";
    case BLE_TRADE_SCANNING:       return "Scanning";
    case BLE_TRADE_PEER_FOUND:     return "Peer Found";
    case BLE_TRADE_OFFER_SENT:     return "Offer Sent";
    case BLE_TRADE_OFFER_RECEIVED: return "Offer Received";
    case BLE_TRADE_NEGOTIATING:    return "Negotiating";
    case BLE_TRADE_NFC_PENDING:    return "NFC Pending";
    case BLE_TRADE_NFC_EXCHANGING: return "NFC Exchanging";
    case BLE_TRADE_COMPLETE:       return "Complete";
    case BLE_TRADE_CANCELLED:      return "Cancelled";
    case BLE_TRADE_ERROR:          return "Error";
    default:                       return "Unknown";
  }
}

bool BLETradeGame::buildTradePayload(NFCTradePayload &payload) {
  memset(&payload, 0, sizeof(payload));

  strncpy(payload.magic, "TAMA", 4);
  payload.version = 1;

  // Use a local PetEngine to avoid dangling reference from temporary
  PetEngine engine;
  const PetData &pet = engine.getData();
  strncpy(payload.petName, pet.name.c_str(), sizeof(payload.petName) - 1);
  payload.petType = 0;
  payload.petStage = pet.stage;
  payload.petAge = pet.age_minutes;
  payload.hunger = pet.hunger;
  payload.happiness = pet.happiness;
  payload.health = pet.health;
  payload.energy = pet.energy;

  payload.checksum = calculateChecksum(payload);
  return true;
}

bool BLETradeGame::processReceivedPayload(const NFCTradePayload &payload) {
  if (strncmp(payload.magic, "TAMA", 4) != 0) return false;

  uint8_t calcChecksum = calculateChecksum(payload);
  if (calcChecksum != payload.checksum) {
    _lastError = "Checksum mismatch";
    return false;
  }

  strncpy(_session.remotePetName, payload.petName, sizeof(_session.remotePetName) - 1);
  return true;
}

String BLETradeGame::getLastError() const {
  return _lastError;
}

void BLETradeGame::clearError() {
  _lastError = "";
}

// --- Private methods ---

void BLETradeGame::setState(BLETradeState newState) {
  if (_session.state != newState) {
    _session.state = newState;
    _session.lastActivity = millis();
  }
}

void BLETradeGame::processIncomingCommand(const BLECommand &cmd) {
  switch (cmd.type) {
    case BLE_CMD_TRADE_OFFER: {
      BLEProtocol &proto = BLEProtocol::getInstance();
      String peerId, petName, pin;
      if (proto.parseTradeOffer(cmd.payload, cmd.payloadLen, peerId, petName, pin)) {
        receiveTradeOffer(peerId.c_str(), petName.c_str(), pin.c_str());
      }
      break;
    }
    case BLE_CMD_TRADE_ACCEPT: {
      if (_session.state == BLE_TRADE_OFFER_SENT) {
        setState(BLE_TRADE_NFC_PENDING);
      }
      break;
    }
    case BLE_CMD_TRADE_REJECT: {
      if (_session.state == BLE_TRADE_OFFER_SENT ||
          _session.state == BLE_TRADE_NEGOTIATING) {
        setState(BLE_TRADE_IDLE);
      }
      break;
    }
    default:
      break;
  }
}

void BLETradeGame::completeTrade() {
  setState(BLE_TRADE_COMPLETE);
}

uint8_t BLETradeGame::calculateChecksum(const NFCTradePayload &payload) {
  // Field-by-field checksum to avoid compiler padding issues
  uint8_t checksum = 0;
  const uint8_t *magic = (const uint8_t *)payload.magic;
  for (size_t i = 0; i < sizeof(payload.magic); i++) checksum ^= magic[i];
  checksum ^= payload.version;
  const uint8_t *name = (const uint8_t *)payload.petName;
  for (size_t i = 0; i < sizeof(payload.petName); i++) checksum ^= name[i];
  checksum ^= payload.petType;
  checksum ^= payload.petStage;
  checksum ^= (uint8_t)(payload.petAge & 0xFF);
  checksum ^= (uint8_t)((payload.petAge >> 8) & 0xFF);
  checksum ^= payload.hunger;
  checksum ^= payload.happiness;
  checksum ^= payload.health;
  checksum ^= payload.energy;
  // Note: checksum field itself is NOT included
  return checksum;
}
