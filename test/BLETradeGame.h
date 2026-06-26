// Minimal stub of BLETradeGame.h for native unit tests
// The full implementation is provided in test/BLETradeGame_Native.cpp
// The real header is archived in archive/v1/src/BLETradeGame.h
#ifndef BLE_TRADE_GAME_H
#define BLE_TRADE_GAME_H

#include <Arduino.h>
#include "BLEManager.h"
#include "BLEDiscovery.h"
#include "BLEProtocol.h"
#include "NFCManager.h"
#include "Pet_v2.h"
using namespace PetV2;

enum BLETradeState {
  BLE_TRADE_IDLE = 0,
  BLE_TRADE_SCANNING,
  BLE_TRADE_PEER_FOUND,
  BLE_TRADE_OFFER_SENT,
  BLE_TRADE_OFFER_RECEIVED,
  BLE_TRADE_NEGOTIATING,
  BLE_TRADE_NFC_PENDING,
  BLE_TRADE_NFC_EXCHANGING,
  BLE_TRADE_COMPLETE,
  BLE_TRADE_CANCELLED,
  BLE_TRADE_ERROR
};

struct BLETradeSession {
  BLETradeState state;
  char peerAddress[18];
  char peerName[32];
  char localPetName[17];
  char remotePetName[17];
  uint8_t tradePin[5];
  bool isInitiator;
  uint32_t startTime;
  uint32_t lastActivity;
  BLEPeer discoveredPeers[BLE_MAX_DISCOVERED];
  uint8_t discoveredCount;
  uint8_t selectedPeerIdx;
};

class BLETradeGame {
public:
  static BLETradeGame& getInstance();
  bool begin();
  void end();
  void update();
  bool startTrade();
  bool cancelTrade();
  bool acceptTrade();
  bool rejectTrade();
  uint8_t getDiscoveredPeerCount() const;
  const BLEPeer* getDiscoveredPeer(uint8_t index) const;
  bool selectPeer(uint8_t index);
  bool sendTradeOffer();
  bool receiveTradeOffer(const char *peerAddress, const char *petName, const char *pin);
  bool writeTradeDataToNFC();
  bool readTradeDataFromNFC();
  BLETradeState getState() const;
  const BLETradeSession& getSession() const;
  String getStateString() const;
  bool buildTradePayload(NFCTradePayload &payload);
  bool processReceivedPayload(const NFCTradePayload &payload);
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

#define bleTradeGame BLETradeGame::getInstance()

#endif // BLE_TRADE_GAME_H
