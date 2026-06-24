// ============================================================
// NFCManager.h — NFC Pet Trading via PN532 (I2C)
// Phase 22.3: NFC tag read/write for pet trading
//
// Provides NFC peer-to-peer pet trading using PN532 module.
// Two TamaPetchi devices tap together to exchange pet data.
// Uses I2C interface (PN532 address 0x24).
//
// ESP32: Uses PN532 library with I2C (Wire)
// Native: Stub implementation for unit testing
// ============================================================

#ifndef NFC_MANAGER_H
#define NFC_MANAGER_H

#include <Arduino.h>

// PN532 I2C address
#define PN532_I2C_ADDRESS   0x24

// NFC configuration
#define NFC_POLL_INTERVAL_MS    1000   // Poll for tags every 1s
#define NFC_TRADE_TIMEOUT_MS    30000  // 30s trade timeout
#define NFC_MAX_TAG_SIZE        128    // Max NDEF message size
#define NFC_TRADE_NDEF_TYPE     "application/tamapetchi.trade"

// NFC state
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

// NFC tag data
struct NFCTagData {
  uint8_t uid[7];
  uint8_t uidLen;
  uint8_t data[NFC_MAX_TAG_SIZE];
  uint16_t dataLen;
  bool valid;
};

// NFC trade payload (serialized pet data for tap-to-trade)
struct NFCTradePayload {
  char magic[4];        // "TAMA"
  uint8_t version;      // Protocol version
  char petName[17];     // Pet name (max 16 + null)
  uint8_t petType;      // PetType enum
  uint8_t petStage;     // PetStage enum
  uint16_t petAge;      // Age in minutes
  uint8_t hunger;
  uint8_t happiness;
  uint8_t health;
  uint8_t energy;
  uint8_t checksum;     // Simple XOR checksum
};

// NFC Manager class
class NFCManager {
public:
  static NFCManager& getInstance();

  // Lifecycle
  bool begin();
  void end();
  void update();  // Call in loop

  // State
  NFCState getState() const;
  bool isReady() const;

  // Tag operations
  bool readTag(NFCTagData &tag);
  bool writeTag(const NFCTagData &tag);

  // NDEF operations
  bool writeNDEF(const uint8_t *data, uint16_t len);
  bool readNDEF(uint8_t *data, uint16_t &len);

  // Pet trading
  bool startTradeOffer(const NFCTradePayload &payload);
  bool readTradeOffer(NFCTradePayload &payload);
  void cancelTrade();

  // Trade payload serialization
  static bool serializeTradePayload(const NFCTradePayload &payload, uint8_t *out, uint16_t &outLen);
  static bool deserializeTradePayload(const uint8_t *data, uint16_t len, NFCTradePayload &payload);

  // Error handling
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

  // PN532 communication helpers
  bool pn532WriteCommand(const uint8_t *cmd, uint8_t cmdLen);
  bool pn532ReadResponse(uint8_t *response, uint8_t &responseLen, uint16_t timeout = 1000);
  bool pn532SAMConfig();
  bool pn532GetFirmwareVersion(uint8_t &version);
};

// Global accessor
#define nfcManager NFCManager::getInstance()

#endif // NFC_MANAGER_H
