// Minimal stub of BLEDiscovery.h for native unit tests
// The full implementation is provided in test/BLE_NFC_Native.cpp
// The real header is archived in archive/v1/src/BLEDiscovery.h
#ifndef BLE_DISCOVERY_H
#define BLE_DISCOVERY_H

#include <Arduino.h>

#define BLE_SCAN_DURATION_MS    5000
#define BLE_SCAN_INTERVAL_MS    30000
#define BLE_MAX_DISCOVERED      10
#define BLE_RSSI_THRESHOLD      -80

struct BLEPeer {
  char address[18];
  char name[32];
  int8_t rssi;
  uint32_t lastSeen;
  bool isTamaPetchi;
  bool active;
};

enum BLEDiscoveryState {
  BLE_DISC_OFF,
  BLE_DISC_IDLE,
  BLE_DISC_SCANNING,
  BLE_DISC_COMPLETE
};

class BLEDiscovery {
public:
  static BLEDiscovery& getInstance();
  bool begin();
  void end();
  void update();
  bool startScan(uint32_t durationMs = BLE_SCAN_DURATION_MS);
  void stopScan();
  bool isScanning() const;
  uint8_t getPeerCount() const;
  const BLEPeer* getPeer(uint8_t index) const;
  const BLEPeer* findPeerByAddress(const String &address) const;
  const BLEPeer* findNearestTamaPetchi() const;
  void setRssiThreshold(int8_t rssi);
  int8_t getRssiThreshold() const;
  BLEDiscoveryState getState() const;
  void clearResults();
  void addOrUpdatePeer(const char *address, const char *name, int8_t rssi, bool isTama);
private:
  BLEDiscovery();
  BLEDiscovery(const BLEDiscovery&) = delete;
  BLEDiscovery& operator=(const BLEDiscovery&) = delete;
  BLEDiscoveryState _state;
  BLEPeer _peers[BLE_MAX_DISCOVERED];
  uint8_t _peerCount;
  int8_t _rssiThreshold;
  uint32_t _lastScanTime;
  bool _initialized;
};

#define bleDiscovery BLEDiscovery::getInstance()

#endif // BLE_DISCOVERY_H
