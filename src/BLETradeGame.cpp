// ============================================================
// BLETradeGame.cpp — BLE Pet Trading Game Implementation
// Phase 22.5: BLE trading game integration
//
// Full trading flow:
// 1. Device A scans for peers via BLEDiscovery
// 2. Device A selects Device B and sends trade offer
// 3. Device B receives offer and accepts/rejects
// 4. If accepted, both devices prompt for NFC tap
// 5. Device A writes pet data to NFC tag
// 6. Device B reads pet data from NFC tag
// 7. Trade completes, both devices update pet state
// ============================================================

#include "BLETradeGame.h"
#include "config_v2.h"

#if !defined(UNIT_TEST) && defined(CHIP_ESP32_S3)
// ESP32: Include actual pet system
#include "Pet.h"
extern Pet g_pet;
#endif

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
  if (_initialized) return true;
  memset(&_session, 0, sizeof(_session));
  _session.state = BLE_TRADE_IDLE;
  _initialized = true;
  Serial.println("[BLETrade] Trading game initialized");
  return true;
}

void BLETradeGame::end() {
  _session.state = BLE_TRADE_IDLE;
  _initialized = false;
}

void BLETradeGame::update() {
  if (!_initialized) return;

  uint32_t now = millis();

  // Process incoming BLE commands
  BLEManager &ble = BLEManager::getInstance();
  while (ble.hasCommand()) {
    BLECommand cmd = ble.getCommand();
    processIncomingCommand(cmd);
  }

  // State-specific updates
  switch (_session.state) {
    case BLE_TRADE_SCANNING: {
      // Check if scan is complete
      BLEDiscovery &disc = BLEDiscovery::getInstance();
      if (!disc.isScanning()) {
        _session.discoveredCount = disc.getPeerCount();
        if (_session.discoveredCount > 0) {
          setState(BLE_TRADE_PEER_FOUND);
          Serial.printf("[BLETrade] Found %d peers\n", _session.discoveredCount);
        } else {
          // No peers found, restart scan
          disc.startScan();
        }
      }
      break;
    }

    case BLE_TRADE_NFC_PENDING: {
      // Check for NFC tag
      NFCManager &nfc = NFCManager::getInstance();
      if (_session.isInitiator) {
        // Initiator writes trade data
        NFCTradePayload payload;
        if (buildTradePayload(payload)) {
          if (nfc.startTradeOffer(payload)) {
            setState(BLE_TRADE_NFC_EXCHANGING);
          }
        }
      } else {
        // Receiver reads trade data
        NFCTradePayload payload;
        if (nfc.readTradeOffer(payload)) {
          if (processReceivedPayload(payload)) {
            completeTrade();
          } else {
            _lastError = "Invalid trade data received";
            setState(BLE_TRADE_ERROR);
          }
        }
      }
      break;
    }

    case BLE_TRADE_NFC_EXCHANGING: {
      // Wait for NFC operation to complete
      NFCManager &nfc = NFCManager::getInstance();
      if (nfc.getState() == NFC_TRADE_COMPLETE) {
        completeTrade();
      } else if (nfc.getState() == NFC_ERROR) {
        _lastError = "NFC transfer failed: " + nfc.getLastError();
        setState(BLE_TRADE_ERROR);
      }
      break;
    }

    default:
      break;
  }

  // Timeout check (5 minute trade timeout)
  if (_session.state != BLE_TRADE_IDLE &&
      _session.state != BLE_TRADE_COMPLETE &&
      _session.state != BLE_TRADE_CANCELLED &&
      _session.state != BLE_TRADE_ERROR) {
    if (now - _session.startTime > 300000UL) {
      _lastError = "Trade timed out";
      setState(BLE_TRADE_ERROR);
    }
  }
}

bool BLETradeGame::startTrade() {
  if (!_initialized) return false;
  if (_session.state != BLE_TRADE_IDLE) {
    _lastError = "Trade already in progress";
    return false;
  }

  // Reset session
  memset(&_session, 0, sizeof(_session));
  _session.isInitiator = true;
  _session.startTime = millis();
  _session.lastActivity = _session.startTime;

  // Start BLE discovery
  BLEDiscovery &disc = BLEDiscovery::getInstance();
  if (!disc.begin()) {
    _lastError = "Failed to start BLE discovery";
    return false;
  }

  if (!disc.startScan()) {
    _lastError = "Failed to start scan";
    return false;
  }

  setState(BLE_TRADE_SCANNING);
  Serial.println("[BLETrade] Trade started - scanning for peers");
  return true;
}

bool BLETradeGame::cancelTrade() {
  if (_session.state == BLE_TRADE_IDLE) return true;

  // Stop BLE discovery
  BLEDiscovery &disc = BLEDiscovery::getInstance();
  disc.stopScan();

  // Cancel NFC if active
  NFCManager &nfc = NFCManager::getInstance();
  nfc.cancelTrade();

  // Notify peer if connected
  BLEManager &ble = BLEManager::getInstance();
  if (ble.isConnected()) {
    BLEProtocol &proto = BLEProtocol::getInstance();
    String resp = proto.createAckResponse(BLE_CMD_TRADE_REJECT, "Trade cancelled");
    ble.notifyTrade(resp);
  }

  setState(BLE_TRADE_CANCELLED);
  Serial.println("[BLETrade] Trade cancelled");
  return true;
}

bool BLETradeGame::acceptTrade() {
  if (_session.state != BLE_TRADE_OFFER_RECEIVED) {
    _lastError = "No trade offer to accept";
    return false;
  }

  _session.lastActivity = millis();

  // Send acceptance
  BLEManager &ble = BLEManager::getInstance();
  BLEProtocol &proto = BLEProtocol::getInstance();
  String resp = proto.createAckResponse(BLE_CMD_TRADE_ACCEPT, "Trade accepted");
  ble.notifyTrade(resp);

  setState(BLE_TRADE_NFC_PENDING);
  Serial.println("[BLETrade] Trade accepted - waiting for NFC tap");
  return true;
}

bool BLETradeGame::rejectTrade() {
  if (_session.state != BLE_TRADE_OFFER_RECEIVED) {
    _lastError = "No trade offer to reject";
    return false;
  }

  // Send rejection
  BLEManager &ble = BLEManager::getInstance();
  BLEProtocol &proto = BLEProtocol::getInstance();
  String resp = proto.createAckResponse(BLE_CMD_TRADE_REJECT, "Trade rejected");
  ble.notifyTrade(resp);

  setState(BLE_TRADE_IDLE);
  Serial.println("[BLETrade] Trade rejected");
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

  Serial.printf("[BLETrade] Selected peer: %s (%s)\n", peer->name, peer->address);
  return true;
}

bool BLETradeGame::sendTradeOffer() {
  if (_session.state != BLE_TRADE_PEER_FOUND &&
      _session.state != BLE_TRADE_SCANNING) {
    _lastError = "No peer selected";
    return false;
  }

  if (strlen(_session.peerAddress) == 0) {
    _lastError = "No peer address";
    return false;
  }

  // Build trade offer JSON
  BLEProtocol &proto = BLEProtocol::getInstance();
  BLEManager &ble = BLEManager::getInstance();

  // Get local pet name
  const PetData &pet = PetEngine().getData();
  strncpy(_session.localPetName, pet.name.c_str(), sizeof(_session.localPetName) - 1);

  // Generate trade PIN
  uint32_t pin = (millis() % 9000) + 1000;
  snprintf((char*)_session.tradePin, sizeof(_session.tradePin), "%04lu", pin);

  // Send offer via BLE
  StaticJsonDocument<256> doc;
  doc["peerId"] = ble.getDeviceAddress();
  doc["petName"] = _session.localPetName;
  doc["pin"] = (char*)_session.tradePin;

  String payload;
  serializeJson(doc, payload);

  BLECommand cmd;
  cmd.type = BLE_CMD_TRADE_OFFER;
  cmd.payloadLen = min((size_t)payload.length(), sizeof(cmd.payload) - 1);
  memcpy(cmd.payload, payload.c_str(), cmd.payloadLen);
  cmd.payload[cmd.payloadLen] = '\0';

  ble.enqueueCommand(cmd);

  // Also send via trade characteristic
  String resp = proto.createAckResponse(BLE_CMD_TRADE_OFFER, "Trade offer sent");
  ble.notifyTrade(resp);

  setState(BLE_TRADE_OFFER_SENT);
  Serial.printf("[BLETrade] Trade offer sent to %s (PIN: %s)\n",
    _session.peerAddress, _session.tradePin);
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
  Serial.printf("[BLETrade] Trade offer from %s (pet: %s)\n", peerAddress, petName);
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

  // Magic
  strncpy(payload.magic, "TAMA", 4);
  payload.version = 1;

  // Pet data from engine
  const PetData &pet = PetEngine().getData();
  strncpy(payload.petName, pet.name.c_str(), sizeof(payload.petName) - 1);
  payload.petType = 0;  // BLOB (default)
  payload.petStage = pet.stage;
  payload.petAge = pet.age_minutes;
  payload.hunger = pet.hunger;
  payload.happiness = pet.happiness;
  payload.health = pet.health;
  payload.energy = pet.energy;

  // Calculate checksum
  payload.checksum = calculateChecksum(payload);

  return true;
}

bool BLETradeGame::processReceivedPayload(const NFCTradePayload &payload) {
  if (strncmp(payload.magic, "TAMA", 4) != 0) return false;

  // Verify checksum
  uint8_t calcChecksum = calculateChecksum(payload);
  if (calcChecksum != payload.checksum) {
    _lastError = "Checksum mismatch";
    return false;
  }

  // Store received pet data
  strncpy(_session.remotePetName, payload.petName, sizeof(_session.remotePetName) - 1);

  Serial.printf("[BLETrade] Received pet: %s (stage=%d, age=%d)\n",
    payload.petName, payload.petStage, payload.petAge);

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
    Serial.printf("[BLETrade] State: %d -> %d\n", _session.state, newState);
    _session.state = newState;
    _session.lastActivity = millis();
  }
}

void BLETradeGame::processIncomingCommand(const BLECommand &cmd) {
  BLEProtocol &proto = BLEProtocol::getInstance();

  switch (cmd.type) {
    case BLE_CMD_TRADE_OFFER: {
      String peerId, petName, pin;
      if (proto.parseTradeOffer(cmd.payload, cmd.payloadLen, peerId, petName, pin)) {
        receiveTradeOffer(peerId.c_str(), petName.c_str(), pin.c_str());
      }
      break;
    }

    case BLE_CMD_TRADE_ACCEPT: {
      if (_session.state == BLE_TRADE_OFFER_SENT) {
        setState(BLE_TRADE_NFC_PENDING);
        Serial.println("[BLETrade] Peer accepted - NFC tap required");
      }
      break;
    }

    case BLE_CMD_TRADE_REJECT: {
      if (_session.state == BLE_TRADE_OFFER_SENT ||
          _session.state == BLE_TRADE_NEGOTIATING) {
        setState(BLE_TRADE_IDLE);
        Serial.println("[BLETrade] Peer rejected trade");
      }
      break;
    }

    default:
      break;
  }
}

void BLETradeGame::completeTrade() {
  // Update BLE connection state
  AppStateV2 &state = g_state;
  state.bleConnected = BLEManager::getInstance().isConnected();

  setState(BLE_TRADE_COMPLETE);
  Serial.println("[BLETrade] Trade complete!");

  // Notify via BLE
  BLEManager &ble = BLEManager::getInstance();
  BLEProtocol &proto = BLEProtocol::getInstance();
  String resp = proto.createNotification("trade_complete", _session.remotePetName);
  ble.notifyTrade(resp);
}

uint8_t BLETradeGame::calculateChecksum(const NFCTradePayload &payload) {
  uint8_t checksum = 0;
  const uint8_t *p = (const uint8_t *)&payload;
  for (size_t i = 0; i < sizeof(NFCTradePayload) - 2; i++) {
    checksum ^= p[i];
  }
  return checksum;
}
