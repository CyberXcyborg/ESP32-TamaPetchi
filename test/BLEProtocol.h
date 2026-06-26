// Minimal stub of BLEProtocol.h for native unit tests
// The full implementation is provided in test/BLE_NFC_Native.cpp
// The real header is archived in archive/v1/src/BLEProtocol.h
#ifndef BLE_PROTOCOL_H
#define BLE_PROTOCOL_H

#include <Arduino.h>
#include "BLEManager.h"

#define BLE_PROTOCOL_VERSION "2.0.0"

enum BLEResponseType {
  BLE_RESP_STATE = 1,
  BLE_RESP_HISTORY,
  BLE_RESP_STATUS,
  BLE_RESP_ACK,
  BLE_RESP_ERROR,
  BLE_RESP_TRADE_UPDATE,
  BLE_RESP_NOTIFICATION
};

enum BLEErrorCode {
  BLE_ERR_NONE = 0,
  BLE_ERR_INVALID_CMD = 1,
  BLE_ERR_INVALID_PAYLOAD = 2,
  BLE_ERR_PET_DEAD = 3,
  BLE_ERR_TRADE_IN_PROGRESS = 4,
  BLE_ERR_NO_TRADE_PENDING = 5,
  BLE_ERR_BUSY = 6,
  BLE_ERR_UNKNOWN = 99
};

class BLEProtocol {
public:
  static BLEProtocol& getInstance();
  void begin();
  String processCommand(const BLECommand &cmd);
  String serializePetState();
  String serializeStatus();
  String serializeHistory();
  String serializeTradeData();
  String createAckResponse(BLECommandType cmdType, const String &message);
  String createErrorResponse(BLEErrorCode code, const String &message);
  String createNotification(const String &event, const String &data);
  bool parseTradeOffer(const char *payload, uint8_t len,
                       String &peerDeviceId, String &petName, String &tradePin);
  bool parseSetName(const char *payload, uint8_t len, String &newName);
private:
  BLEProtocol() {}
  BLEProtocol(const BLEProtocol&) = delete;
  BLEProtocol& operator=(const BLEProtocol&) = delete;
  String handleFeed();
  String handlePlay();
  String handleClean();
  String handleSleep();
  String handleWake();
  String handleHeal();
  String handleGetState();
  String handleGetHistory();
  String handleTradeOffer(const char *payload, uint8_t len);
  String handleTradeAccept();
  String handleTradeReject();
  String handleSetName(const char *payload, uint8_t len);
  String handleSetDifficulty(const char *payload, uint8_t len);
  String handleToggleSound();
  String handleToggleMusic();
  String handleFactoryReset();
};

#define bleProtocol BLEProtocol::getInstance()

#endif // BLE_PROTOCOL_H
