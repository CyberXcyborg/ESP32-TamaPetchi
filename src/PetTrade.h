#ifndef PET_TRADE_H
#define PET_TRADE_H

#include <Arduino.h>

// ============================================================
// Pet Trading via MQTT (Phase 11.4)
//
// Allows trading pets between two TamaPetchi devices via MQTT.
// Protocol: request → accept → transfer pet data → confirm
// Security: trade PIN required for acceptance.
// ============================================================

#define TRADE_PIN_LENGTH    4
#define TRADE_TIMEOUT_MS    30000   // 30 second trade timeout
#define TRADE_HISTORY_FILE  "/trade_history.json"
#define MAX_TRADE_HISTORY   10

// Trade state machine
enum TradeState {
  TRADE_IDLE,
  TRADE_REQUEST_SENT,
  TRADE_REQUEST_RECEIVED,
  TRADE_ACCEPTED,
  TRADE_DATA_SENT,
  TRADE_DATA_RECEIVED,
  TRADE_CONFIRMED,
  TRADE_CANCELLED
};

// Trade session info
struct TradeSession {
  TradeState state;
  char peerClientId[32];
  char tradePin[TRADE_PIN_LENGTH + 1];
  unsigned long stateChangeTime;
  bool isInitiator;
};

// Initialize pet trade system
void initPetTrade();

// Start a trade request to a peer
bool startTradeRequest(const String &peerClientId, const String &tradePin);

// Accept an incoming trade request
bool acceptTradeRequest();

// Reject/cancel current trade
void cancelTrade();

// Get current trade state
TradeState getTradeState();

// Get trade session info
const TradeSession& getTradeSession();

// Process incoming trade MQTT message
void handleTradeMessage(const String &topic, const String &payload);

// Get trade history as JSON
String getTradeHistoryJson();

// Register trade API routes
void registerTradeRoutes();

// MQTT topic helpers
String getTradeRequestTopic();
String getTradeAcceptTopic();
String getTradeRejectTopic();
String getTradeDataTopic();
String getTradeConfirmTopic();

#endif // PET_TRADE_H
