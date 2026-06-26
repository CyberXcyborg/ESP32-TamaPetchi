// ============================================================
// BLEManager.h — BLE GATT Server for TamaPetchi v2.0
// Phase 22.1: BLE GATT Server with NimBLE
//
// Provides a BLE peripheral with custom GATT service for
// companion app communication. Characteristics expose pet
// state, commands, history, and trading data.
//
// ESP32: Uses NimBLE-Arduino (lightweight BLE stack)
// Native: Stub implementation for unit testing
// ============================================================

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

// BLE UUIDs (128-bit custom service)
// Base UUID: E5C9-0000-4AE0-8F9B-2E8C7E01A100
#define BLE_SERVICE_UUID        "e5c90000-4ae0-8f9b-2e8c-7e01a1000000"
#define BLE_CHAR_PET_STATE_UUID "e5c90001-4ae0-8f9b-2e8c-7e01a1000000"  // R/N
#define BLE_CHAR_COMMAND_UUID   "e5c90002-4ae0-8f9b-2e8c-7e01a1000000"  // W
#define BLE_CHAR_HISTORY_UUID   "e5c90003-4ae0-8f9b-2e8c-7e01a1000000"  // R/N
#define BLE_CHAR_TRADE_UUID     "e5c90004-4ae0-8f9b-2e8c-7e01a1000000"  // R/W/N
#define BLE_CHAR_STATUS_UUID    "e5c90005-4ae0-8f9b-2e8c-7e01a1000000"  // R/N

// BLE configuration
#define BLE_DEVICE_NAME         "TamaPetchi"
#define BLE_ADVERTISING_INTERVAL 160  // 100ms (units of 0.625ms)
#define BLE_MTU_SIZE            256
#define BLE_MAX_CONNECTED       1

// BLE connection state
enum BLEState {
  BLE_OFF,
  BLE_IDLE,
  BLE_ADVERTISING,
  BLE_CONNECTED,
  BLE_DISCONNECTING
};

// BLE command types (written by companion app)
enum BLECommandType {
  BLE_CMD_FEED = 1,
  BLE_CMD_PLAY,
  BLE_CMD_CLEAN,
  BLE_CMD_SLEEP,
  BLE_CMD_WAKE,
  BLE_CMD_HEAL,
  BLE_CMD_GET_STATE,
  BLE_CMD_GET_HISTORY,
  BLE_CMD_TRADE_OFFER,
  BLE_CMD_TRADE_ACCEPT,
  BLE_CMD_TRADE_REJECT,
  BLE_CMD_SET_NAME,
  BLE_CMD_SET_DIFFICULTY,
  BLE_CMD_TOGGLE_SOUND,
  BLE_CMD_TOGGLE_MUSIC,
  BLE_CMD_FACTORY_RESET
};

// BLE command structure (parsed from write)
struct BLECommand {
  BLECommandType type;
  char payload[128];
  uint8_t payloadLen;
};

// BLE Manager class
class BLEManager {
public:
  static BLEManager& getInstance();

  // Lifecycle
  bool begin();
  void end();
  void update();  // Call in loop

  // State
  BLEState getState() const;
  bool isConnected() const;
  uint8_t getConnectedCount() const;

  // Data updates (called when pet state changes)
  void notifyPetState(const String &jsonState);
  void notifyHistory(const String &jsonHistory);
  void notifyStatus(const String &jsonStatus);
  void notifyTrade(const String &jsonTradeData);

  // Command handling
  bool hasCommand() const;
  BLECommand getCommand();
  void clearCommand();

  // Advertising control
  void startAdvertising();
  void stopAdvertising();

  // MTU negotiation
  uint16_t getMTU() const;

  // Device info
  String getDeviceAddress() const;

  // Command queue access (public for callback access)
  void enqueueCommand(const BLECommand &cmd);

  // Internal state (public for callback access)
  BLEState _state;
  uint16_t _mtu;
  uint32_t _lastActivity;

private:
  BLEManager();
  BLEManager(const BLEManager&) = delete;
  BLEManager& operator=(const BLEManager&) = delete;

  // Command queue (ring buffer)
  static const uint8_t CMD_QUEUE_SIZE = 8;
  BLECommand _cmdQueue[CMD_QUEUE_SIZE];
  uint8_t _cmdHead;
  uint8_t _cmdTail;
  uint8_t _cmdCount;
};

// Global accessor
#define bleManager BLEManager::getInstance()

#endif // BLE_MANAGER_H
