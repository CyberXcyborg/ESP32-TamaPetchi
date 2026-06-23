// ============================================================
// I2SAudio.h — I2S Audio Output Driver for MAX98357A
// Phase 21.1: I2S Audio Driver
// ============================================================
// Supports 16-bit PCM, 22050Hz and 44100Hz sample rates.
// DMA-based playback (non-blocking) with double-buffer scheme.
// Uses FreeRTOS queue for audio buffer management.
// ============================================================

#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <Arduino.h>
#include "config_v2.h"

// Audio buffer configuration
#define I2S_DMA_BUFFER_SIZE     1024    // Samples per DMA buffer
#define I2S_DMA_BUFFER_COUNT    2       // Double-buffering
#define I2S_QUEUE_SIZE          4       // Max queued buffers
#define I2S_DEFAULT_VOLUME      50      // 0-100

// Error codes
enum I2SAudioError {
    I2S_OK = 0,
    I2S_ERR_NOT_INITIALIZED = -1,
    I2S_ERR_DMA_FAILURE = -2,
    I2S_ERR_INVALID_PARAM = -3,
    I2S_ERR_BUFFER_FULL = -4
};

class I2SAudio {
public:
    static I2SAudio& getInstance();

    // Initialize I2S peripheral with default pins and config
    bool begin();

    // Shutdown I2S peripheral
    void end();

    // Play PCM audio buffer (non-blocking)
    // data: int16_t PCM samples, samples: count of samples
    // Returns I2S_OK on success, negative on error
    int play(const int16_t* data, size_t samples);

    // Stop playback immediately
    void stop();

    // Check if audio is currently playing
    bool isPlaying() const;

    // Set volume (0-100, clamped)
    void setVolume(uint8_t vol);

    // Get current volume
    uint8_t getVolume() const;

    // Get current sample rate
    uint32_t getSampleRate() const;

    // Set sample rate (must be called before begin() or after end())
    void setSampleRate(uint32_t rate);

    // Check if driver is initialized
    bool isInitialized() const;

private:
    I2SAudio();
    ~I2SAudio();
    I2SAudio(const I2SAudio&) = delete;
    I2SAudio& operator=(const I2SAudio&) = delete;

    bool _initialized;
    bool _playing;
    uint8_t _volume;
    uint32_t _sampleRate;
};

#endif // I2S_AUDIO_H
