// ============================================================
// NFCManager.cpp — NFC Pet Trading Implementation
// Phase 22.3: PN532 I2C communication and pet trading
// ============================================================

#include "NFCManager.h"
#include "config_v2.h"
#include <Wire.h>

// ============================================================
// PN532 Constants
// ============================================================
#define PN532_PREAMBLE          0x00
#define PN532_STARTCODE1        0x00
#define PN532_STARTCODE2        0xFF
#define PN532_POSTAMBLE         0x00
#define PN532_COMMAND_GETFIRMWAREVERSION  0x02
#define PN532_COMMAND_SAMCONFIGURATION    0x14
#define PN532_COMMAND_INLISTPASSIVETARGET 0x4A
#define PN532_COMMAND_TGGETDATA           0x60
#define PN532_COMMAND_TGSETDATA           0x8E
#define PN532_COMMAND_INDATAEXCHANGE      0x40
#define PN532_TG_INIT_AS_TARGET           0x8C

#if defined(UNIT_TEST) || !defined(CHIP_ESP32_S3)

// ============================================================
// NATIVE STUB IMPLEMENTATION
// ============================================================

NFCManager::NFCManager()
    : _state(NFC_OFF)
    , _lastPollTime(0)
    , _initialized(false)
{}

NFCManager& NFCManager::getInstance() {
  static NFCManager instance;
  return instance;
}

bool NFCManager::begin() {
  _state = NFC_IDLE;
  _initialized = true;
  return true;
}

void NFCManager::end() {
  _state = NFC_OFF;
  _initialized = false;
}

void NFCManager::update() {
  if (!_initialized) return;
  // Stub: simulate no tag present
  _state = NFC_IDLE;
}

NFCState NFCManager::getState() const { return _state; }
bool NFCManager::isReady() const { return _initialized; }

bool NFCManager::readTag(NFCTagData &tag) {
  memset(&tag, 0, sizeof(tag));
  return false;  // No tag in stub
}

bool NFCManager::writeTag(const NFCTagData &tag) {
  return true;  // Stub always succeeds
}

bool NFCManager::writeNDEF(const uint8_t *data, uint16_t len) {
  return true;
}

bool NFCManager::readNDEF(uint8_t *data, uint16_t &len) {
  len = 0;
  return false;
}

bool NFCManager::startTradeOffer(const NFCTradePayload &payload) {
  _state = NFC_TRADE_PENDING;
  return true;
}

bool NFCManager::readTradeOffer(NFCTradePayload &payload) {
  memset(&payload, 0, sizeof(payload));
  return false;
}

void NFCManager::cancelTrade() {
  _state = NFC_IDLE;
}

bool NFCManager::serializeTradePayload(const NFCTradePayload &payload, uint8_t *out, uint16_t &outLen) {
  if (outLen < sizeof(NFCTradePayload)) return false;
  memcpy(out, &payload, sizeof(NFCTradePayload));
  outLen = sizeof(NFCTradePayload);
  return true;
}

bool NFCManager::deserializeTradePayload(const uint8_t *data, uint16_t len, NFCTradePayload &payload) {
  if (len < sizeof(NFCTradePayload)) return false;
  memcpy(&payload, data, sizeof(NFCTradePayload));
  // Validate magic
  if (strncmp(payload.magic, "TAMA", 4) != 0) return false;
  return true;
}

String NFCManager::getLastError() const { return _lastError; }
void NFCManager::clearError() { _lastError = ""; }

bool NFCManager::pn532WriteCommand(const uint8_t *cmd, uint8_t cmdLen) { return true; }
bool NFCManager::pn532ReadResponse(uint8_t *response, uint8_t &responseLen, uint16_t timeout) {
  responseLen = 0;
  return true;
}
bool NFCManager::pn532SAMConfig() { return true; }
bool NFCManager::pn532GetFirmwareVersion(uint8_t &version) { version = 0x01; return true; }

#else
// ============================================================
// ESP32 IMPLEMENTATION (PN532 via I2C - Adafruit PN532)
// Uses Adafruit_PN532 library v1.3.4 with unified I2C API
// ============================================================

#include <Adafruit_PN532.h>
#include <Wire.h>

// IRQ and RESET pins for PN532 I2C mode (-1 if not connected)
#define PN532_IRQ_PIN   -1
#define PN532_RESET_PIN -1

static Adafruit_PN532 nfc(PN532_IRQ_PIN, PN532_RESET_PIN, &Wire);

// NDEF helper: build a MIME media NDEF record into buffer
// Returns total bytes written
static uint16_t buildNdefMimeRecord(uint8_t *out, uint16_t maxLen,
                                     const char *mimeType,
                                     const uint8_t *payload, uint16_t payloadLen) {
  // NDEF record header: MB=1, ME=1, SR=1, TNF=0x02 (MIME media)
  // Byte 0: Type length
  // Bytes 1-4: Payload length (SR=1 means 1 byte, but we use 4 for simplicity)
  // Byte 5+: Type (MIME string)
  // Byte after type: Payload
  uint8_t typeLen = strlen(mimeType);
  uint16_t totalLen = 3 + 1 + typeLen + payloadLen;  // flags + typeLen + type + payload
  if (totalLen > maxLen) return 0;

  uint16_t idx = 0;
  out[idx++] = 0xD2;  // MB=1, ME=1, SR=1, TNF=0x02
  out[idx++] = typeLen;
  out[idx++] = (uint8_t)payloadLen;  // SR=1: 1-byte payload length
  memcpy(out + idx, mimeType, typeLen);
  idx += typeLen;
  memcpy(out + idx, payload, payloadLen);
  idx += payloadLen;
  return idx;
}

// Parse NDEF record from buffer, extract MIME payload
// Returns payload length, 0 if no valid MIME record found
static uint16_t parseNdefMimeRecord(const uint8_t *data, uint16_t dataLen,
                                     const char *expectedType,
                                     uint8_t *outPayload, uint16_t maxPayloadLen) {
  if (dataLen < 5) return 0;
  uint16_t idx = 0;

  // Check for NDEF record start
  if (data[idx] != 0xD2 && data[idx] != 0x92 && data[idx] != 0x52) return 0;
  idx++;

  uint8_t typeLen = data[idx++];
  uint8_t payloadLen = data[idx++];

  if (idx + typeLen + payloadLen > dataLen) return 0;

  // Check type matches
  if (typeLen != (uint8_t)strlen(expectedType)) return 0;
  if (memcmp(data + idx, expectedType, typeLen) != 0) return 0;

  idx += typeLen;
  uint16_t copyLen = (payloadLen < maxPayloadLen) ? payloadLen : maxPayloadLen;
  memcpy(outPayload, data + idx, copyLen);
  return copyLen;
}

NFCManager::NFCManager()
    : _state(NFC_OFF)
    , _lastPollTime(0)
    , _initialized(false)
{}

NFCManager& NFCManager::getInstance() {
  static NFCManager instance;
  return instance;
}

bool NFCManager::begin() {
  if (_initialized) return true;

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    _lastError = "PN532 not found";
    _state = NFC_ERROR;
    Serial.println("[NFC] PN532 not found - check wiring");
    return false;
  }

  // Configure PN532
  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();

  _initialized = true;
  _state = NFC_IDLE;
  Serial.printf("[NFC] PN532 firmware: 0x%08X\n", versiondata);
  return true;
}

void NFCManager::end() {
  _initialized = false;
  _state = NFC_OFF;
}

void NFCManager::update() {
  if (!_initialized) return;
  if (_state == NFC_TRADE_PENDING || _state == NFC_TAG_DETECTED) return;

  uint32_t now = millis();
  if (now - _lastPollTime < NFC_POLL_INTERVAL_MS) return;
  _lastPollTime = now;

  // Poll for ISO14443A targets (NFC tags / other devices)
  uint8_t uid[7];
  uint8_t uidLen = 0;
  bool found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100);
  if (found && uidLen > 0) {
    _state = NFC_TAG_DETECTED;
    Serial.printf("[NFC] Tag detected (uid len=%d)\n", uidLen);
  }
}

NFCState NFCManager::getState() const { return _state; }
bool NFCManager::isReady() const { return _initialized; }

bool NFCManager::readTag(NFCTagData &tag) {
  memset(&tag, 0, sizeof(tag));

  uint8_t uid[7];
  uint8_t uidLen = 0;
  bool found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 500);
  if (!found || uidLen == 0) {
    _lastError = "No tag found";
    _state = NFC_IDLE;
    return false;
  }

  tag.uidLen = (uidLen > 7) ? 7 : uidLen;
  memcpy(tag.uid, uid, tag.uidLen);

  // Attempt to read data from tag (Mifare Ultralight / NTAG)
  // Page 4 is the first user data page on Ultralight/NTAG2xx
  uint8_t pageBuf[4];
  uint8_t totalDataLen = 0;

  // Try reading pages 4..15 to collect potential NDEF data
  for (uint8_t page = 4; page < 16 && totalDataLen < NFC_MAX_TAG_SIZE - 4; page++) {
    uint8_t err = nfc.ntag2xx_ReadPage(page, pageBuf);
    if (err != 1) break;  // Read failed
    memcpy(tag.data + totalDataLen, pageBuf, 4);
    totalDataLen += 4;
  }
  tag.dataLen = totalDataLen;
  tag.valid = true;
  _state = NFC_IDLE;
  return true;
}

bool NFCManager::writeTag(const NFCTagData &tag) {
  _state = NFC_WRITING;

  // Build NDEF MIME record with trade data
  uint8_t ndefBuf[NFC_MAX_TAG_SIZE];
  uint16_t ndefLen = buildNdefMimeRecord(ndefBuf, sizeof(ndefBuf),
                                          NFC_TRADE_NDEF_TYPE,
                                          tag.data, tag.dataLen);
  if (ndefLen == 0) {
    _lastError = "Failed to build NDEF record";
    _state = NFC_ERROR;
    return false;
  }

  // For Mifare Ultralight/NTAG, write NDEF data starting at page 4
  // Must write 4 bytes at a time (page-aligned)
  uint8_t pageData[4];
  bool writeOk = true;
  uint8_t firstPage = 4;  // User data starts at page 4 on Ultralight/NTAG

  for (uint16_t off = 0; off < ndefLen && writeOk; off += 4) {
    memset(pageData, 0, 4);
    uint16_t chunk = ((ndefLen - off) > 4) ? 4 : (ndefLen - off);
    memcpy(pageData, ndefBuf + off, chunk);
    uint8_t err = nfc.ntag2xx_WritePage(firstPage + (off / 4), pageData);
    if (err != 1) writeOk = false;
  }

  if (!writeOk) {
    _lastError = "Tag write failed";
    _state = NFC_ERROR;
    return false;
  }

  _state = NFC_IDLE;
  return true;
}

bool NFCManager::writeNDEF(const uint8_t *data, uint16_t len) {
  if (len > NFC_MAX_TAG_SIZE) len = NFC_MAX_TAG_SIZE;

  uint8_t ndefBuf[NFC_MAX_TAG_SIZE];
  uint16_t ndefLen = buildNdefMimeRecord(ndefBuf, sizeof(ndefBuf),
                                          NFC_TRADE_NDEF_TYPE, data, len);
  if (ndefLen == 0) return false;

  // Expect tag to be present from prior detection
  uint8_t uid[7];
  uint8_t uidLen = 0;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 500)) {
    return false;
  }

  // Write to NTAG/Ultralight pages
  uint8_t pageData[4];
  for (uint16_t off = 0; off < ndefLen; off += 4) {
    memset(pageData, 0, 4);
    uint16_t chunk = ((ndefLen - off) > 4) ? 4 : (ndefLen - off);
    memcpy(pageData, ndefBuf + off, chunk);
    uint8_t err = nfc.ntag2xx_WritePage(4 + (off / 4), pageData);
    if (err != 1) return false;
  }
  return true;
}

bool NFCManager::readNDEF(uint8_t *data, uint16_t &len) {
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nullptr, nullptr, 500)) {
    return false;
  }

  // Read raw tag data
  NFCTagData tag;
  if (!readTag(tag) || !tag.valid || tag.dataLen == 0) {
    return false;
  }

  // Parse NDEF MIME record
  uint16_t payloadLen = parseNdefMimeRecord(tag.data, tag.dataLen,
                                             NFC_TRADE_NDEF_TYPE,
                                             data, NFC_MAX_TAG_SIZE);
  if (payloadLen == 0) return false;
  len = payloadLen;
  return true;
}

bool NFCManager::startTradeOffer(const NFCTradePayload &payload) {
  Serial.println("[NFC] Trade offer ready - tap to share");
  _state = NFC_TRADE_PENDING;
  return true;
}

bool NFCManager::readTradeOffer(NFCTradePayload &payload) {
  memset(&payload, 0, sizeof(payload));

  uint8_t buffer[NFC_MAX_TAG_SIZE];
  uint16_t len = sizeof(buffer);

  if (!readNDEF(buffer, len)) {
    _lastError = "No trade data on tag";
    return false;
  }

  if (!deserializeTradePayload(buffer, len, payload)) {
    _lastError = "Invalid trade data";
    return false;
  }

  _state = NFC_TRADE_COMPLETE;
  Serial.printf("[NFC] Trade offer: %s (type=%d, stage=%d)\n",
    payload.petName, payload.petType, payload.petStage);
  return true;
}

void NFCManager::cancelTrade() {
  _state = NFC_IDLE;
}

bool NFCManager::serializeTradePayload(const NFCTradePayload &payload, uint8_t *out, uint16_t &outLen) {
  if (outLen < sizeof(NFCTradePayload)) return false;
  memcpy(out, &payload, sizeof(NFCTradePayload));
  outLen = sizeof(NFCTradePayload);
  return true;
}

bool NFCManager::deserializeTradePayload(const uint8_t *data, uint16_t len, NFCTradePayload &payload) {
  if (len < sizeof(NFCTradePayload)) return false;
  memcpy(&payload, data, sizeof(NFCTradePayload));
  if (strncmp(payload.magic, "TAMA", 4) != 0) return false;

  // Verify checksum
  uint8_t calcChecksum = 0;
  const uint8_t *p = (const uint8_t *)&payload;
  for (size_t i = 0; i < sizeof(NFCTradePayload) - 2; i++) {
    calcChecksum ^= p[i];
  }
  return calcChecksum == payload.checksum;
}

String NFCManager::getLastError() const { return _lastError; }
void NFCManager::clearError() { _lastError = ""; }

bool NFCManager::pn532WriteCommand(const uint8_t *cmd, uint8_t cmdLen) { return true; }
bool NFCManager::pn532ReadResponse(uint8_t *response, uint8_t &responseLen, uint16_t timeout) {
  responseLen = 0;
  return true;
}
bool NFCManager::pn532SAMConfig() { return true; }
bool NFCManager::pn532GetFirmwareVersion(uint8_t &version) { version = 0x01; return true; }

#endif // UNIT_TEST / ESP32
