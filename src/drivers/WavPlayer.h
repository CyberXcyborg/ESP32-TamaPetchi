// ============================================================
// WavPlayer.h — WAV File Decoder & Playback
// Phase 21.2: WAV Decoder
// ============================================================
// Parses WAV headers, streams from LittleFS in 4KB chunks.
// Supports PCM format, ≤44100Hz, mono/stereo, 8/16-bit.
// Handles stereo→mono mixdown, 8-bit→16-bit sign extension.
// ============================================================

#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include <Arduino.h>
#include "config_v2.h"
#include <LittleFS.h>

// WAV constants
#define WAV_READ_BUFFER_SIZE    4096    // 4KB read buffer
#define WAV_HEADER_SIZE         44      // Standard WAV header size

// WAV format validation
#define WAV_MAX_SAMPLE_RATE     44100
#define WAV_MAX_BIT_DEPTH       16

// Error codes
enum WavError {
    WAV_OK = 0,
    WAV_ERR_NO_FILE = -1,
    WAV_ERR_INVALID_HEADER = -2,
    WAV_ERR_UNSUPPORTED_FORMAT = -3,
    WAV_ERR_READ_FAILED = -4,
    WAV_ERR_NOT_INITIALIZED = -5
};

// WAV header structure (packed)
struct __attribute__((packed)) WavHeader {
    // RIFF chunk
    char riffId[4];         // "RIFF"
    uint32_t fileSize;      // File size - 8
    char waveId[4];         // "WAVE"
    // fmt chunk
    char fmtId[4];          // "fmt "
    uint32_t fmtSize;       // 16 for PCM
    uint16_t audioFormat;   // 1 = PCM
    uint16_t numChannels;   // 1 = mono, 2 = stereo
    uint32_t sampleRate;    // e.g., 22050, 44100
    uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;    // numChannels * bitsPerSample/8
    uint16_t bitsPerSample; // 8 or 16
    // data chunk
    char dataId[4];         // "data"
    uint32_t dataSize;      // Size of audio data
};

class WavPlayer {
public:
    static WavPlayer& getInstance();

    // Initialize WAV player (initializes I2S if needed)
    bool begin();

    // Shutdown
    void end();

    // Play a WAV file from LittleFS (blocking)
    // path: file path in LittleFS (e.g., "/sounds/feed.wav")
    int play(const char* path);

    // Stop playback
    void stop();

    // Check if playing
    bool isPlaying() const;

    // Set/get volume (0-100)
    void setVolume(uint8_t vol) { _volume = (vol > 100) ? 100 : vol; }
    uint8_t getVolume() const { return _volume; }

    // Get last error
    WavError getLastError() const;

    // Validate a WAV file without playing (returns error code)
    int validate(const char* path);

    // Get info about a WAV file
    bool getInfo(const char* path, WavHeader& header);

private:
    WavPlayer();
    ~WavPlayer();
    WavPlayer(const WavPlayer&) = delete;
    WavPlayer& operator=(const WavPlayer&) = delete;

    // Parse WAV header from file
    bool parseHeader(File& file, WavHeader& header);

    // Validate parsed header
    bool validateHeader(const WavHeader& header);

    // Stream and play audio data from file
    int streamAudio(File& file, const WavHeader& header);

    bool _initialized;
    bool _playing;
    WavError _lastError;
    uint8_t* _readBuffer;
    uint8_t _volume;
};

#endif // WAV_PLAYER_H
