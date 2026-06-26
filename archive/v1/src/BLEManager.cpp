// ============================================================
// BLEManager.cpp — BLE GATT Server Implementation
// Phase 22.1: BLE GATT Server with NimBLE
// ============================================================

#include "BLEManager.h"
#include "config_v2.h"

// ESP32: Use NimBLE-Arduino
// Native: Stub implementation
#if defined(UNIT_TEST) || !defined(CHIP_ESP32_S3)
// ============================================================
// NATIVE STUB IMPLEMENTATION
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
  return true;
}

void BLEManager::end() {
  _state = BLE_OFF;
}

void BLEManager::update() {
  // Stub: no-op
}

BLEState BLEManager::getState() const { return _state; }
bool BLEManager::isConnected() const { return _state == BLE_CONNECTED; }
uint8_t BLEManager::getConnectedCount() const {
  return (_state == BLE_CONNECTED) ? 1 : 0;
}

void BLEManager::notifyPetState(const String &jsonState) {}
void BLEManager::notifyHistory(const String &jsonHistory) {}
void BLEManager::notifyStatus(const String &jsonStatus) {}
void BLEManager::notifyTrade(const String &jsonTradeData) {}

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

#else
// ============================================================
// ESP32 IMPLEMENTATION (NimBLE-Arduino)
// ============================================================

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLECharacteristic.h>

static NimBLEServer *pServer = nullptr;
static NimBLECharacteristic *pPetStateChar = nullptr;
static NimBLECharacteristic *pCommandChar = nullptr;
static NimBLECharacteristic *pHistoryChar = nullptr;
static NimBLECharacteristic *pTradeChar = nullptr;
static NimBLECharacteristic *pStatusChar = nullptr;

// Command queue populated by BLE write callback
static BLEManager *s_instance = nullptr;

class TamaServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    if (s_instance) {
      s_instance->_state = BLE_CONNECTED;
      s_instance->_lastActivity = millis();
      Serial.printf("[BLE] Connected: %s\n", connInfo.getAddress().toString().c_str());
    }
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
    if (s_instance) {
      s_instance->_state = BLE_IDLE;
      Serial.printf("[BLE] Disconnected (reason: %d)\n", reason);
      // Restart advertising after disconnect
      s_instance->startAdvertising();
    }
  }

  void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override {
    if (s_instance) {
      s_instance->_mtu = MTU;
      Serial.printf("[BLE] MTU changed: %d\n", MTU);
    }
  }
};

class TamaCharCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
    if (!s_instance) return;
    std::string value = pChar->getValue();
    if (value.empty()) return;

    // Parse command: first byte is type, rest is payload
    BLECommand cmd;
    cmd.type = (BLECommandType)(uint8_t)value[0];
    cmd.payloadLen = min((size_t)(value.length() - 1), sizeof(cmd.payload) - 1);
    if (cmd.payloadLen > 0) {
      memcpy(cmd.payload, value.data() + 1, cmd.payloadLen);
    }
    cmd.payload[cmd.payloadLen] = '\0';
    s_instance->enqueueCommand(cmd);

    Serial.printf("[BLE] Command received: type=%d, len=%d\n", cmd.type, cmd.payloadLen);
  }
};

BLEManager::BLEManager()
    : _state(BLE_OFF)
    , _mtu(23)
    , _lastActivity(0)
    , _cmdHead(0)
    , _cmdTail(0)
    , _cmdCount(0)
{
  s_instance = this;
}

BLEManager& BLEManager::getInstance() {
  static BLEManager instance;
  return instance;
}

bool BLEManager::begin() {
  if (_state != BLE_OFF) return true;

  // Initialize NimBLE
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // +9dBm max power
  NimBLEDevice::setMTU(BLE_MTU_SIZE);

  // Create BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new TamaServerCallbacks());

  // Create BLE Service
  NimBLEService *pService = pServer->createService(BLE_SERVICE_UUID);

  // Pet State Characteristic (Read + Notify)
  pPetStateChar = pService->createCharacteristic(
    BLE_CHAR_PET_STATE_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY
  );
  pPetStateChar->setValue("{}");

  // Command Characteristic (Write)
  pCommandChar = pService->createCharacteristic(
    BLE_CHAR_COMMAND_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  pCommandChar->setCallbacks(new TamaCharCallbacks());

  // History Characteristic (Read + Notify)
  pHistoryChar = pService->createCharacteristic(
    BLE_CHAR_HISTORY_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY
  );
  pHistoryChar->setValue("[]");

  // Trade Characteristic (Read + Write + Notify)
  pTradeChar = pService->createCharacteristic(
    BLE_CHAR_TRADE_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::NOTIFY
  );
  pTradeChar->setValue("{}");

  // Status Characteristic (Read + Notify)
  pStatusChar = pService->createCharacteristic(
    BLE_CHAR_STATUS_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY
  );
  pStatusChar->setValue("{}");

  // Start service (deprecated in NimBLE v2.x — auto-started with server)
  // pService->start();

  _state = BLE_IDLE;
  Serial.println("[BLE] GATT server initialized");
  return true;
}

void BLEManager::end() {
  if (_state == BLE_OFF) return;
  stopAdvertising();
  NimBLEDevice::deinit(true);
  pServer = nullptr;
  pPetStateChar = nullptr;
  pCommandChar = nullptr;
  pHistoryChar = nullptr;
  pTradeChar = nullptr;
  pStatusChar = nullptr;
  _state = BLE_OFF;
  Serial.println("[BLE] Deinitialized");
}

void BLEManager::update() {
  // Check for timeout (disconnect if no activity for 60s and not connected)
  // This is a placeholder — actual timeout handled by BLE layer
}

BLEState BLEManager::getState() const { return _state; }
bool BLEManager::isConnected() const { return _state == BLE_CONNECTED; }
uint8_t BLEManager::getConnectedCount() const {
  return (_state == BLE_CONNECTED) ? 1 : 0;
}

void BLEManager::notifyPetState(const String &jsonState) {
  if (_state != BLE_CONNECTED || !pPetStateChar) return;
  pPetStateChar->setValue(jsonState.c_str());
  pPetStateChar->notify();
}

void BLEManager::notifyHistory(const String &jsonHistory) {
  if (_state != BLE_CONNECTED || !pHistoryChar) return;
  pHistoryChar->setValue(jsonHistory.c_str());
  pHistoryChar->notify();
}

void BLEManager::notifyStatus(const String &jsonStatus) {
  if (_state != BLE_CONNECTED || !pStatusChar) return;
  pStatusChar->setValue(jsonStatus.c_str());
  pStatusChar->notify();
}

void BLEManager::notifyTrade(const String &jsonTradeData) {
  if (_state != BLE_CONNECTED || !pTradeChar) return;
  pTradeChar->setValue(jsonTradeData.c_str());
  pTradeChar->notify();
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

void BLEManager::startAdvertising() {
  if (_state == BLE_OFF) return;
  if (!pServer) return;

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setMinInterval(BLE_ADVERTISING_INTERVAL);
  pAdvertising->setMaxInterval(BLE_ADVERTISING_INTERVAL * 2);
  NimBLEDevice::startAdvertising();

  _state = BLE_ADVERTISING;
  Serial.println("[BLE] Advertising started");
}

void BLEManager::stopAdvertising() {
  if (_state == BLE_ADVERTISING) {
    NimBLEDevice::stopAdvertising();
    _state = BLE_IDLE;
    Serial.println("[BLE] Advertising stopped");
  }
}

uint16_t BLEManager::getMTU() const { return _mtu; }

String BLEManager::getDeviceAddress() const {
  if (!pServer) return "00:00:00:00:00:00";
  return String(NimBLEDevice::getAddress().toString().c_str());
}

void BLEManager::enqueueCommand(const BLECommand &cmd) {
  if (_cmdCount >= CMD_QUEUE_SIZE) return;
  _cmdQueue[_cmdHead] = cmd;
  _cmdHead = (_cmdHead + 1) % CMD_QUEUE_SIZE;
  _cmdCount++;
}

#endif // UNIT_TEST / ESP32
