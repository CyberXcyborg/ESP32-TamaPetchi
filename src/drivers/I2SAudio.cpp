// ============================================================
// I2SAudio.cpp — I2S Audio Output Driver for MAX98357A
// Phase 21.1: I2S Audio Driver
// ============================================================

#include "drivers/I2SAudio.h"
#include <driver/i2s.h>
#include <freertos/queue.h>

// ============================================================
// Module-level state
// ============================================================
static QueueHandle_t s_audioQueue = nullptr;
static int16_t* s_dmaBuffers[I2S_DMA_BUFFER_COUNT] = {nullptr};
static volatile bool s_dmaBusy = false;
static uint8_t s_currentVolume = I2S_DEFAULT_VOLUME;

// ============================================================
// Singleton
// ============================================================
I2SAudio& I2SAudio::getInstance() {
    static I2SAudio instance;
    return instance;
}

// ============================================================
// Constructor / Destructor
// ============================================================
I2SAudio::I2SAudio()
    : _initialized(false), _playing(false),
      _volume(I2S_DEFAULT_VOLUME), _sampleRate(I2S_SAMPLE_RATE) {
}

I2SAudio::~I2SAudio() {
    end();
}

// ============================================================
// Public API
// ============================================================
bool I2SAudio::begin() {
    if (_initialized) return true;

    // Validate sample rate
    if (_sampleRate != 22050 && _sampleRate != 44100) {
        _sampleRate = I2S_SAMPLE_RATE; // fallback to default
    }

    // Configure I2S
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = _sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // MAX98357A: left channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUFFER_COUNT,
        .dma_buf_len = I2S_DMA_BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // Install I2S driver
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sConfig, I2S_QUEUE_SIZE, &s_audioQueue);
    if (err != ESP_OK) {
        DEBUG_PRINTF("[I2S] Driver install failed: %d\n", err);
        return false;
    }

    // Configure I2S pins for MAX98357A
    i2s_pin_config_t pinConfig = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_LRC_PIN,
        .data_out_num = I2S_DIN_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    err = i2s_set_pin(I2S_NUM_0, &pinConfig);
    if (err != ESP_OK) {
        DEBUG_PRINTF("[I2S] Pin config failed: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }

    // Initialize DMA buffers
    for (int i = 0; i < I2S_DMA_BUFFER_COUNT; i++) {
        s_dmaBuffers[i] = (int16_t*)ps_malloc(I2S_DMA_BUFFER_SIZE * sizeof(int16_t));
        if (!s_dmaBuffers[i]) {
            DEBUG_PRINTF("[I2S] DMA buffer %d alloc failed\n", i);
            for (int j = 0; j < i; j++) free(s_dmaBuffers[j]);
            i2s_driver_uninstall(I2S_NUM_0);
            return false;
        }
        memset(s_dmaBuffers[i], 0, I2S_DMA_BUFFER_SIZE * sizeof(int16_t));
    }

    // Set default volume
    setVolume(_volume);

    _initialized = true;
    _playing = false;
    DEBUG_PRINTLN("[I2S] Initialized");
    return true;
}

void I2SAudio::end() {
    if (!_initialized) return;

    stop();

    // Free DMA buffers
    for (int i = 0; i < I2S_DMA_BUFFER_COUNT; i++) {
        if (s_dmaBuffers[i]) {
            free(s_dmaBuffers[i]);
            s_dmaBuffers[i] = nullptr;
        }
    }

    // Uninstall driver
    i2s_driver_uninstall(I2S_NUM_0);
    s_audioQueue = nullptr;

    _initialized = false;
    DEBUG_PRINTLN("[I2S] Shutdown");
}

int I2SAudio::play(const int16_t* data, size_t samples) {
    if (!_initialized) return I2S_ERR_NOT_INITIALIZED;
    if (!data || samples == 0) return I2S_ERR_INVALID_PARAM;

    // Apply volume scaling
    uint8_t vol = _volume;
    size_t totalBytes = samples * sizeof(int16_t);

    // Write to I2S DMA
    size_t bytesWritten = 0;

    if (vol >= 100) {
        // Full volume: write directly
        esp_err_t err = i2s_write(I2S_NUM_0, data, totalBytes, &bytesWritten, portMAX_DELAY);
        if (err != ESP_OK) return I2S_ERR_DMA_FAILURE;
    } else if (vol == 0) {
        // Muted: write silence
        memset(s_dmaBuffers[0], 0, totalBytes < I2S_DMA_BUFFER_SIZE * sizeof(int16_t) ? totalBytes : I2S_DMA_BUFFER_SIZE * sizeof(int16_t));
        esp_err_t err = i2s_write(I2S_NUM_0, s_dmaBuffers[0], totalBytes, &bytesWritten, portMAX_DELAY);
        if (err != ESP_OK) return I2S_ERR_DMA_FAILURE;
    } else {
        // Volume scaling: copy and scale samples
        size_t copySamples = samples < I2S_DMA_BUFFER_SIZE ? samples : I2S_DMA_BUFFER_SIZE;
        for (size_t i = 0; i < copySamples; i++) {
            s_dmaBuffers[0][i] = (int16_t)((int32_t)data[i] * vol / 100);
        }
        esp_err_t err = i2s_write(I2S_NUM_0, s_dmaBuffers[0], copySamples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
        if (err != ESP_OK) return I2S_ERR_DMA_FAILURE;

        // Write remaining samples if buffer was larger than DMA buffer
        if (samples > I2S_DMA_BUFFER_SIZE) {
            const int16_t* src = data + I2S_DMA_BUFFER_SIZE;
            size_t remaining = samples - I2S_DMA_BUFFER_SIZE;
            while (remaining > 0) {
                copySamples = remaining < I2S_DMA_BUFFER_SIZE ? remaining : I2S_DMA_BUFFER_SIZE;
                for (size_t i = 0; i < copySamples; i++) {
                    s_dmaBuffers[0][i] = (int16_t)((int32_t)src[i] * vol / 100);
                }
                err = i2s_write(I2S_NUM_0, s_dmaBuffers[0], copySamples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
                if (err != ESP_OK) return I2S_ERR_DMA_FAILURE;
                src += copySamples;
                remaining -= copySamples;
            }
        }
    }

    // Wait for DMA to complete (blocking for simplicity in v1)
    i2s_wait_tx_done(I2S_NUM_0, portMAX_DELAY);

    _playing = false;
    return I2S_OK;
}

void I2SAudio::stop() {
    if (!_initialized) return;
    i2s_stop(I2S_NUM_0);
    _playing = false;
    // Zero DMA buffers
    for (int i = 0; i < I2S_DMA_BUFFER_COUNT; i++) {
        if (s_dmaBuffers[i]) {
            memset(s_dmaBuffers[i], 0, I2S_DMA_BUFFER_SIZE * sizeof(int16_t));
        }
    }
}

bool I2SAudio::isPlaying() const {
    return _initialized && _playing;
}

void I2SAudio::setVolume(uint8_t vol) {
    _volume = (vol > 100) ? 100 : vol;
    s_currentVolume = _volume;
}

uint8_t I2SAudio::getVolume() const {
    return _volume;
}

uint32_t I2SAudio::getSampleRate() const {
    return _sampleRate;
}

void I2SAudio::setSampleRate(uint32_t rate) {
    if (rate == 22050 || rate == 44100) {
        _sampleRate = rate;
    }
}

bool I2SAudio::isInitialized() const {
    return _initialized;
}
