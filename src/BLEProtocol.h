// ============================================================
// BLEProtocol.h — Companion App Protocol for BLE
// Phase 22.2: JSON-based command/response protocol
//
// Defines the BLE companion app protocol:
// - Command parsing (app → device)
// - Response formatting (device → app)
// - Pet state serialization for BLE
// - Trade data exchange format
// ============================================================

#ifndef BLE_PROTOCOL_H
#define BLE_PROTOCOL_H

#include <Arduino.h>
#include "BLEManager.h"

// Protocol version
#define BLE_PROTOCOL_VERSION "2.0.0"

// Response types
enum BLEResponseType {
  BLE_RESP_STATE = 1,     // Full pet state
  BLE_RESP_HISTORY,       // Activity history
  BLE_RESP_STATUS,        // System status (battery, uptime, etc.)
  BLE_RESP_ACK,           // Command acknowledged
  BLE_RESP_ERROR,         // Command error
  BLE_RESP_TRADE_UPDATE,  // Trade state update
  BLE_RESP_NOTIFICATION   // Async notification
};

// Error codes
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

// BLE Protocol handler class
class BLEProtocol {
public:
  static BLEProtocol& getInstance();

  // Initialize protocol handler
  void begin();

  // Process incoming BLE command, return JSON response
  String processCommand(const BLECommand &cmd);

  // Serialize pet state for BLE (compact JSON)
  String serializePetState();

  // Serialize system status
  String serializeStatus();

  // Serialize activity history
  String serializeHistory();

  // Serialize trade data
  String serializeTradeData();

  // Create ACK response
  String createAckResponse(BLECommandType cmdType, const String &message);

  // Create error response
  String createErrorResponse(BLEErrorCode code, const String &message);

  // Create notification
  String createNotification(const String &event, const String &data);

  // Parse trade offer from BLE write payload
  bool parseTradeOffer(const char *payload, uint8_t len,
                       String &peerDeviceId, String &petName, String &tradePin);

  // Parse set-name command payload
  bool parseSetName(const char *payload, uint8_t len, String &newName);

private:
  BLEProtocol() {}
  BLEProtocol(const BLEProtocol&) = delete;
  BLEProtocol& operator=(const BLEProtocol&) = delete;

  // Command handlers
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

// Global accessor
#define bleProtocol BLEProtocol::getInstance()

#endif // BLE_PROTOCOL_H
