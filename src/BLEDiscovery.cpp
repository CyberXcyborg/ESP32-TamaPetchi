// ============================================================
// BLEDiscovery.cpp — BLE Peer Discovery Implementation
// Phase 22.4: Device scanning and peer discovery
// ============================================================

#include "BLEDiscovery.h"
#include "config_v2.h"

#if defined(UNIT_TEST) || !defined(CHIP_ESP32_S3)

// ============================================================
// NATIVE STUB IMPLEMENTATION
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
    // Stub: scan completes immediately
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
  if (_state == BLE_DISC_SCANNING) {
    _state = BLE_DISC_IDLE;
  }
}

bool BLEDiscovery::isScanning() const { return _state == BLE_DISC_SCANNING; }

uint8_t BLEDiscovery::getPeerCount() const { return _peerCount; }

const BLEPeer* BLEDiscovery::getPeer(uint8_t index) const {
  if (index >= BLE_MAX_DISCOVERED) return nullptr;
  return _peers[index].active ? &_peers[index] : nullptr;
}

const BLEPeer* BLEDiscovery::findPeerByAddress(const String &address) const {
  for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
    if (_peers[i].active && address.equals(_peers[i].address)) {
      return &_peers[i];
    }
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
  // Check if already known
  for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
    if (_peers[i].active && strcmp(_peers[i].address, address) == 0) {
      _peers[i].rssi = rssi;
      _peers[i].lastSeen = millis();
      _peers[i].isTamaPetchi = isTama;
      return;
    }
  }
  // Add new
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

#else
// ============================================================
// ESP32 IMPLEMENTATION (NimBLE Scan)
// ============================================================

#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include "BLEManager.h"  // For BLE_SERVICE_UUID

static BLEDiscovery *s_discInstance = nullptr;
static NimBLEScan *pScan = nullptr;

class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
    if (!s_discInstance) return;

    String addr = advertisedDevice->getAddress().toString().c_str();
    String name = advertisedDevice->getName().c_str();
    int8_t rssi = advertisedDevice->getRSSI();

    // Check if it's a TamaPetchi device
    bool isTamaPetchi = false;
    if (advertisedDevice->haveServiceUUID()) {
      isTamaPetchi = advertisedDevice->getServiceUUID().toString() == BLE_SERVICE_UUID;
    }
    // Also check by name
    if (name.indexOf("TamaPetchi") >= 0) {
      isTamaPetchi = true;
    }

    s_discInstance->addOrUpdatePeer(
      addr.c_str(),
      name.length() > 0 ? name.c_str() : "Unknown",
      rssi,
      isTamaPetchi
    );
  }
};

BLEDiscovery::BLEDiscovery()
    : _state(BLE_DISC_OFF)
    , _peerCount(0)
    , _rssiThreshold(BLE_RSSI_THRESHOLD)
    , _lastScanTime(0)
    , _initialized(false)
{
  memset(_peers, 0, sizeof(_peers));
  s_discInstance = this;
}

BLEDiscovery& BLEDiscovery::getInstance() {
  static BLEDiscovery instance;
  return instance;
}

bool BLEDiscovery::begin() {
  if (_initialized) return true;

  pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);

  _initialized = true;
  _state = BLE_DISC_IDLE;
  Serial.println("[BLEDiscovery] Initialized");
  return true;
}

void BLEDiscovery::end() {
  if (_state == BLE_DISC_SCANNING) stopScan();
  _state = BLE_DISC_OFF;
  _initialized = false;
}

void BLEDiscovery::update() {
  if (!_initialized) return;

  if (_state == BLE_DISC_SCANNING) {
    // Check if scan duration elapsed
    if (millis() - _lastScanTime >= BLE_SCAN_DURATION_MS) {
      stopScan();
      _state = BLE_DISC_COMPLETE;
      Serial.printf("[BLEDiscovery] Scan complete: %d peers\n", _peerCount);
    }
  }
}

bool BLEDiscovery::startScan(uint32_t durationMs) {
  if (!_initialized) return false;
  if (_state == BLE_DISC_SCANNING) return true;

  clearResults();
  _state = BLE_DISC_SCANNING;
  _lastScanTime = millis();

  pScan->start(durationMs / 1000, false);
  return true;
}

void BLEDiscovery::stopScan() {
  if (_state != BLE_DISC_SCANNING) return;
  pScan->stop();
  _state = BLE_DISC_IDLE;
}

bool BLEDiscovery::isScanning() const { return _state == BLE_DISC_SCANNING; }

uint8_t BLEDiscovery::getPeerCount() const { return _peerCount; }

const BLEPeer* BLEDiscovery::getPeer(uint8_t index) const {
  if (index >= BLE_MAX_DISCOVERED) return nullptr;
  return _peers[index].active ? &_peers[index] : nullptr;
}

const BLEPeer* BLEDiscovery::findPeerByAddress(const String &address) const {
  for (uint8_t i = 0; i < BLE_MAX_DISCOVERED; i++) {
    if (_peers[i].active && address.equals(_peers[i].address)) {
      return &_peers[i];
    }
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
  if (rssi < _rssiThreshold) return;

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

#endif // UNIT_TEST / ESP32
