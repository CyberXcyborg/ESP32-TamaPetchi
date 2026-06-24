// ============================================================
// WavPlayer.cpp — WAV File Decoder & Playback
// Phase 21.2: WAV Decoder
// ============================================================

#include "drivers/WavPlayer.h"
#include "drivers/I2SAudio.h"
#include <LittleFS.h>

// ============================================================
// Singleton
// ============================================================
WavPlayer& WavPlayer::getInstance() {
    static WavPlayer instance;
    return instance;
}

WavPlayer::WavPlayer()
    : _initialized(false), _playing(false),
      _lastError(WAV_OK), _readBuffer(nullptr), _volume(80) {
}

WavPlayer::~WavPlayer() {
    end();
}

// ============================================================
// Public API
// ============================================================
bool WavPlayer::begin() {
    if (_initialized) return true;

    // Initialize I2S audio
    if (!I2SAudio::getInstance().begin()) {
        _lastError = WAV_ERR_NOT_INITIALIZED;
        return false;
    }

    // Allocate read buffer
    _readBuffer = (uint8_t*)ps_malloc(WAV_READ_BUFFER_SIZE);
    if (!_readBuffer) {
        _readBuffer = (uint8_t*)malloc(WAV_READ_BUFFER_SIZE);
    }
    if (!_readBuffer) {
        _lastError = WAV_ERR_NOT_INITIALIZED;
        return false;
    }

    _initialized = true;
    DEBUG_PRINTLN("[WAV] Initialized");
    return true;
}

void WavPlayer::end() {
    if (!_initialized) return;
    stop();
    if (_readBuffer) {
        free(_readBuffer);
        _readBuffer = nullptr;
    }
    I2SAudio::getInstance().end();
    _initialized = false;
    DEBUG_PRINTLN("[WAV] Shutdown");
}

int WavPlayer::play(const char* path) {
    if (!_initialized) return WAV_ERR_NOT_INITIALIZED;
    if (!path) return WAV_ERR_NO_FILE;

    // Open file
    File file = LittleFS.open(path, "r");
    if (!file) {
        DEBUG_PRINTF("[WAV] Cannot open: %s\n", path);
        _lastError = WAV_ERR_NO_FILE;
        return WAV_ERR_NO_FILE;
    }

    // Parse header
    WavHeader header;
    if (!parseHeader(file, header)) {
        file.close();
        return _lastError;
    }

    // Validate header
    if (!validateHeader(header)) {
        file.close();
        return _lastError;
    }

    DEBUG_PRINTF("[WAV] Playing: %s (%dHz, %dch, %dbit, %d samples)\n",
        path, header.sampleRate, header.numChannels, header.bitsPerSample,
        header.dataSize / (header.numChannels * header.bitsPerSample / 8));

    // Stream and play audio
    _playing = true;
    int result = streamAudio(file, header);
    _playing = false;

    file.close();
    _lastError = (result == WAV_OK) ? WAV_OK : WAV_ERR_READ_FAILED;
    return result;
}

void WavPlayer::stop() {
    _playing = false;
    I2SAudio::getInstance().stop();
}

bool WavPlayer::isPlaying() const {
    return _playing;
}

WavError WavPlayer::getLastError() const {
    return _lastError;
}

int WavPlayer::validate(const char* path) {
    if (!path) return WAV_ERR_NO_FILE;

    File file = LittleFS.open(path, "r");
    if (!file) return WAV_ERR_NO_FILE;

    WavHeader header;
    if (!parseHeader(file, header)) {
        file.close();
        return _lastError;
    }

    if (!validateHeader(header)) {
        file.close();
        return _lastError;
    }

    file.close();
    return WAV_OK;
}

bool WavPlayer::getInfo(const char* path, WavHeader& header) {
    if (!path) return false;

    File file = LittleFS.open(path, "r");
    if (!file) return false;

    bool ok = parseHeader(file, header);
    file.close();
    return ok;
}

// ============================================================
// Private Methods
// ============================================================
bool WavPlayer::parseHeader(File& file, WavHeader& header) {
    // Read standard 44-byte header
    size_t bytesRead = file.read((uint8_t*)&header, WAV_HEADER_SIZE);
    if (bytesRead < WAV_HEADER_SIZE) {
        DEBUG_PRINTLN("[WAV] Header too short");
        _lastError = WAV_ERR_INVALID_HEADER;
        return false;
    }

    // Validate RIFF header
    if (memcmp(header.riffId, "RIFF", 4) != 0) {
        DEBUG_PRINTLN("[WAV] Missing RIFF tag");
        _lastError = WAV_ERR_INVALID_HEADER;
        return false;
    }

    if (memcmp(header.waveId, "WAVE", 4) != 0) {
        DEBUG_PRINTLN("[WAV] Missing WAVE tag");
        _lastError = WAV_ERR_INVALID_HEADER;
        return false;
    }

    if (memcmp(header.fmtId, "fmt ", 4) != 0) {
        DEBUG_PRINTLN("[WAV] Missing fmt tag");
        _lastError = WAV_ERR_INVALID_HEADER;
        return false;
    }

    // Handle non-standard headers (skip extra fmt bytes)
    if (header.fmtSize > 16) {
        uint16_t extraBytes = header.fmtSize - 16;
        file.seek(WAV_HEADER_SIZE - 20 + 16 + extraBytes); // Skip to data chunk
    }

    // Find data chunk (handle LIST chunks etc.)
    char chunkId[4];
    uint32_t chunkSize = 0;
    bool foundData = false;

    // Check if data chunk is at expected position
    if (memcmp(header.dataId, "data", 4) == 0) {
        foundData = true;
    } else {
        // Search for data chunk
        while (file.available()) {
            file.read((uint8_t*)chunkId, 4);
            file.read((uint8_t*)&chunkSize, 4);
            if (memcmp(chunkId, "data", 4) == 0) {
                memcpy(header.dataId, "data", 4);
                header.dataSize = chunkSize;
                foundData = true;
                break;
            }
            // Skip this chunk
            file.seek(file.position() + chunkSize);
        }
    }

    if (!foundData) {
        DEBUG_PRINTLN("[WAV] No data chunk found");
        _lastError = WAV_ERR_INVALID_HEADER;
        return false;
    }

    return true;
}

bool WavPlayer::validateHeader(const WavHeader& header) {
    // Must be PCM
    if (header.audioFormat != 1) {
        DEBUG_PRINTF("[WAV] Non-PCM format: %d\n", header.audioFormat);
        _lastError = WAV_ERR_UNSUPPORTED_FORMAT;
        return false;
    }

    // Sample rate check
    if (header.sampleRate > WAV_MAX_SAMPLE_RATE) {
        DEBUG_PRINTF("[WAV] Sample rate too high: %d\n", header.sampleRate);
        _lastError = WAV_ERR_UNSUPPORTED_FORMAT;
        return false;
    }

    // Bit depth check
    if (header.bitsPerSample != 8 && header.bitsPerSample != 16) {
        DEBUG_PRINTF("[WAV] Unsupported bit depth: %d\n", header.bitsPerSample);
        _lastError = WAV_ERR_UNSUPPORTED_FORMAT;
        return false;
    }

    // Channel check
    if (header.numChannels < 1 || header.numChannels > 2) {
        DEBUG_PRINTF("[WAV] Unsupported channels: %d\n", header.numChannels);
        _lastError = WAV_ERR_UNSUPPORTED_FORMAT;
        return false;
    }

    return true;
}

int WavPlayer::streamAudio(File& file, const WavHeader& header) {
    I2SAudio& i2s = I2SAudio::getInstance();

    // Resample if needed (simple approach: skip samples for higher rates)
    uint32_t targetRate = i2s.getSampleRate();
    uint32_t sourceRate = header.sampleRate;
    bool needsResample = (sourceRate != targetRate);

    size_t bytesPerSample = header.bitsPerSample / 8;
    size_t frameSize = header.numChannels * bytesPerSample;
    size_t totalBytes = header.dataSize;
    size_t bytesProcessed = 0;

    // Temporary buffer for format conversion
    int16_t* pcmBuffer = (int16_t*)ps_malloc(I2S_DMA_BUFFER_SIZE * sizeof(int16_t));
    if (!pcmBuffer) {
        pcmBuffer = (int16_t*)malloc(I2S_DMA_BUFFER_SIZE * sizeof(int16_t));
    }
    if (!pcmBuffer) return WAV_ERR_READ_FAILED;

    while (bytesProcessed < totalBytes && _playing) {
        // Read chunk
        size_t toRead = (totalBytes - bytesProcessed) < WAV_READ_BUFFER_SIZE ?
                        (totalBytes - bytesProcessed) : WAV_READ_BUFFER_SIZE;
        size_t bytesRead = file.read(_readBuffer, toRead);
        if (bytesRead == 0) break;

        // Convert to 16-bit mono PCM
        size_t framesRead = bytesRead / frameSize;
        size_t pcmCount = 0;

        for (size_t i = 0; i < framesRead && pcmCount < I2S_DMA_BUFFER_SIZE; i++) {
            int16_t sample = 0;

            if (header.bitsPerSample == 16) {
                // 16-bit: read sample(s)
                int16_t* src = (int16_t*)(_readBuffer + i * frameSize);
                if (header.numChannels == 2) {
                    // Stereo→mono mixdown
                    int16_t left = src[0];
                    int16_t right = src[1];
                    sample = (int16_t)((((int32_t)left + (int32_t)right) / 2));
                } else {
                    sample = src[0];
                }
            } else {
                // 8-bit: unsigned, convert to 16-bit signed
                uint8_t* src = _readBuffer + i * frameSize;
                if (header.numChannels == 2) {
                    uint8_t left = src[0];
                    uint8_t right = src[1];
                    int16_t mono8 = (int16_t)((left + right) / 2);
                    sample = (int16_t)((mono8 - 128) * 256); // sign extend
                } else {
                    sample = (int16_t)((src[0] - 128) * 256);
                }
            }

            // Simple resampling: skip or duplicate samples
            if (needsResample) {
                if (sourceRate == 44100 && targetRate == 22050) {
                    // Downsample: take every other sample
                    if (i % 2 == 0) {
                        pcmBuffer[pcmCount++] = sample;
                    }
                } else if (sourceRate == 22050 && targetRate == 44100) {
                    // Upsample: duplicate each sample
                    if (pcmCount < I2S_DMA_BUFFER_SIZE - 1) {
                        pcmBuffer[pcmCount++] = sample;
                        pcmBuffer[pcmCount++] = sample;
                    } else {
                        pcmBuffer[pcmCount++] = sample;
                    }
                } else {
                    pcmBuffer[pcmCount++] = sample;
                }
            } else {
                pcmBuffer[pcmCount++] = sample;
            }
        }

        // Play the converted buffer
        if (pcmCount > 0) {
            int result = i2s.play(pcmBuffer, pcmCount);
            if (result != I2S_OK) {
                free(pcmBuffer);
                return WAV_ERR_READ_FAILED;
            }
        }

        bytesProcessed += bytesRead;
    }

    free(pcmBuffer);
    return WAV_OK;
}
