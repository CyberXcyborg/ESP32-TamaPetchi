// ============================================================
// BLETradeGame.h — BLE Pet Trading Game for TamaPetchi v2.0
// Phase 22.5: BLE trading game integration
//
// Enables two TamaPetchi devices to trade pets via BLE.
// Uses BLE for discovery and negotiation, NFC for the
// actual pet data transfer (tap-to-trade).
// ============================================================

#ifndef BLE_TRADE_GAME_H
#define BLE_TRADE_GAME_H

#include <Arduino.h>
#include "BLEManager.h"
#include "BLEProtocol.h"
#include "BLEDiscovery.h"
#include "NFCManager.h"
#include "Pet_v2.h"
#include "AppState_v2.h"

// Trade session states
enum BLETradeState {
  BLE_TRADE_IDLE = 0,
  BLE_TRADE_SCANNING,
  BLE_TRADE_PEER_FOUND,
  BLE_TRADE_OFFER_SENT,
  BLE_TRADE_OFFER_RECEIVED,
  BLE_TRADE_NEGOTIATING,
  BLE_TRADE_NFC_PENDING,      // Waiting for NFC tap
  BLE_TRADE_NFC_EXCHANGING,   // NFC data transfer in progress
  BLE_TRADE_COMPLETE,
  BLE_TRADE_CANCELLED,
  BLE_TRADE_ERROR
};

// Trade session info
struct BLETradeSession {
  BLETradeState state;
  char peerAddress[18];
  char peerName[32];
  char localPetName[17];
  char remotePetName[17];
  uint8_t tradePin[5];        // 4-digit PIN + null
  bool isInitiator;           // True if we initiated the trade
  uint32_t startTime;
  uint32_t lastActivity;
  BLEPeer discoveredPeers[BLE_MAX_DISCOVERED];
  uint8_t discoveredCount;
  uint8_t selectedPeerIdx;
};

class BLETradeGame {
public:
  static BLETradeGame& getInstance();

  // Lifecycle
  bool begin();
  void end();
  void update();  // Call in loop

  // Trade flow
  bool startTrade();
  bool cancelTrade();
  bool acceptTrade();
  bool rejectTrade();

  // Peer selection
  uint8_t getDiscoveredPeerCount() const;
  const BLEPeer* getDiscoveredPeer(uint8_t index) const;
  bool selectPeer(uint8_t index);

  // Offer management
  bool sendTradeOffer();
  bool receiveTradeOffer(const char *peerAddress, const char *petName, const char *pin);

  // NFC integration
  bool writeTradeDataToNFC();
  bool readTradeDataFromNFC();

  // State
  BLETradeState getState() const;
  const BLETradeSession& getSession() const;
  String getStateString() const;

  // Serialization for NFC transfer
  bool buildTradePayload(NFCTradePayload &payload);
  bool processReceivedPayload(const NFCTradePayload &payload);

  // Error
  String getLastError() const;
  void clearError();

private:
  BLETradeGame();
  BLETradeGame(const BLETradeGame&) = delete;
  BLETradeGame& operator=(const BLETradeGame&) = delete;

  BLETradeSession _session;
  String _lastError;
  bool _initialized;

  void setState(BLETradeState newState);
  void processIncomingCommand(const BLECommand &cmd);
  void completeTrade();
  uint8_t calculateChecksum(const NFCTradePayload &payload);
};

// Global accessor
#define bleTradeGame BLETradeGame::getInstance()

#endif // BLE_TRADE_GAME_H
