// Minimal stub of BLEManager.h for native unit tests
// The full implementation is provided in test/BLE_NFC_Native.cpp
// The real header is archived in archive/v1/src/BLEManager.h
#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

enum BLEState {
  BLE_OFF,
  BLE_IDLE,
  BLE_ADVERTISING,
  BLE_CONNECTED,
  BLE_DISCONNECTING
};

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

struct BLECommand {
  BLECommandType type;
  char payload[128];
  uint8_t payloadLen;
};

class BLEManager {
public:
  static BLEManager& getInstance();
  bool begin();
  void end();
  void update();
  BLEState getState() const;
  bool isConnected() const;
  uint8_t getConnectedCount() const;
  void notifyPetState(const String &jsonState);
  void notifyHistory(const String &jsonHistory);
  void notifyStatus(const String &jsonStatus);
  void notifyTrade(const String &jsonTradeData);
  bool hasCommand() const;
  BLECommand getCommand();
  void clearCommand();
  void startAdvertising();
  void stopAdvertising();
  uint16_t getMTU() const;
  String getDeviceAddress() const;
  void enqueueCommand(const BLECommand &cmd);
  BLEState _state;
  uint16_t _mtu;
  uint32_t _lastActivity;
private:
  BLEManager();
  BLEManager(const BLEManager&) = delete;
  BLEManager& operator=(const BLEManager&) = delete;
  static const uint8_t CMD_QUEUE_SIZE = 8;
  BLECommand _cmdQueue[CMD_QUEUE_SIZE];
  uint8_t _cmdHead;
  uint8_t _cmdTail;
  uint8_t _cmdCount;
};

#define bleManager BLEManager::getInstance()

#endif // BLE_MANAGER_H
