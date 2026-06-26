// Minimal stub of NFCManager.h for native unit tests
// The full implementation is provided in test/BLE_NFC_Native.cpp
// The real header is archived in archive/v1/src/NFCManager.h
#ifndef NFC_MANAGER_H
#define NFC_MANAGER_H

#include <Arduino.h>

#define NFC_MAX_TAG_SIZE 128

enum NFCState {
  NFC_OFF,
  NFC_IDLE,
  NFC_POLLING,
  NFC_TAG_DETECTED,
  NFC_READING,
  NFC_WRITING,
  NFC_TRADE_PENDING,
  NFC_TRADE_COMPLETE,
  NFC_ERROR
};

struct NFCTagData {
  uint8_t uid[7];
  uint8_t uidLen;
  uint8_t data[NFC_MAX_TAG_SIZE];
  uint16_t dataLen;
  bool valid;
};

struct NFCTradePayload {
  char magic[4];
  uint8_t version;
  char petName[17];
  uint8_t petType;
  uint8_t petStage;
  uint16_t petAge;
  uint8_t hunger;
  uint8_t happiness;
  uint8_t health;
  uint8_t energy;
  uint8_t checksum;
};

class NFCManager {
public:
  static NFCManager& getInstance();
  bool begin();
  void end();
  void update();
  NFCState getState() const;
  bool isReady() const;
  bool readTag(NFCTagData &tag);
  bool writeTag(const NFCTagData &tag);
  bool writeNDEF(const uint8_t *data, uint16_t len);
  bool readNDEF(uint8_t *data, uint16_t &len);
  bool startTradeOffer(const NFCTradePayload &payload);
  bool readTradeOffer(NFCTradePayload &payload);
  void cancelTrade();
  static bool serializeTradePayload(const NFCTradePayload &payload, uint8_t *out, uint16_t &outLen);
  static bool deserializeTradePayload(const uint8_t *data, uint16_t len, NFCTradePayload &payload);
  String getLastError() const;
  void clearError();
private:
  NFCManager();
  NFCManager(const NFCManager&) = delete;
  NFCManager& operator=(const NFCManager&) = delete;
  NFCState _state;
  uint32_t _lastPollTime;
  String _lastError;
  bool _initialized;
  bool pn532WriteCommand(const uint8_t *cmd, uint8_t cmdLen);
  bool pn532ReadResponse(uint8_t *response, uint8_t &responseLen, uint16_t timeout = 1000);
  bool pn532SAMConfig();
  bool pn532GetFirmwareVersion(uint8_t &version);
};

#endif // NFC_MANAGER_H
