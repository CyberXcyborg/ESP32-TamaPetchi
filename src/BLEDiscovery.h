// ============================================================
// BLEDiscovery.h — BLE Peer Discovery for TamaPetchi
// Phase 22.4: Device scanning and peer discovery
//
// Allows a TamaPetchi device to scan for other TamaPetchi
// devices nearby, enabling proximity-based features like
// pet trading and social interactions.
//
// ESP32: Uses NimBLE scan API
// Native: Stub implementation
// ============================================================

#ifndef BLE_DISCOVERY_H
#define BLE_DISCOVERY_H

#include <Arduino.h>

// Discovery configuration
#define BLE_SCAN_DURATION_MS    5000   // 5 second scan window
#define BLE_SCAN_INTERVAL_MS    30000  // Scan every 30s
#define BLE_MAX_DISCOVERED      10     // Max peers to track
#define BLE_RSSI_THRESHOLD      -80    // Minimum RSSI (dBm)

// Discovered peer info
struct BLEPeer {
  char address[18];       // MAC address "AA:BB:CC:DD:EE:FF"
  char name[32];          // Device name
  int8_t rssi;            // Signal strength
  uint32_t lastSeen;      // Last seen timestamp (millis)
  bool isTamaPetchi;      // True if TamaPetchi service UUID found
  bool active;            // Slot in use
};

// Discovery state
enum BLEDiscoveryState {
  BLE_DISC_OFF,
  BLE_DISC_IDLE,
  BLE_DISC_SCANNING,
  BLE_DISC_COMPLETE
};

// BLE Discovery class
class BLEDiscovery {
public:
  static BLEDiscovery& getInstance();

  // Lifecycle
  bool begin();
  void end();
  void update();

  // Scanning
  bool startScan(uint32_t durationMs = BLE_SCAN_DURATION_MS);
  void stopScan();
  bool isScanning() const;

  // Results
  uint8_t getPeerCount() const;
  const BLEPeer* getPeer(uint8_t index) const;
  const BLEPeer* findPeerByAddress(const String &address) const;
  const BLEPeer* findNearestTamaPetchi() const;

  // Filter
  void setRssiThreshold(int8_t rssi);
  int8_t getRssiThreshold() const;

  // State
  BLEDiscoveryState getState() const;

  // Clear results
  void clearResults();

  // Peer management (public for callback access)
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

// Global accessor
#define bleDiscovery BLEDiscovery::getInstance()

#endif // BLE_DISCOVERY_H
