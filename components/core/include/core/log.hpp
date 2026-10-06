#pragma once

// Logging façade.
//
// The ESP_LOGx macros are already the right primitive: they filter at compile
// time via CONFIG_LOG_MAXIMUM_LEVEL and at run time via esp_log_level_set().
// These wrappers exist so the project has ONE place to change if logging ever
// needs to go somewhere else (a ring buffer, a custom sink, a different
// timestamp format) — call sites keep using CORE_LOG* and never change.
//
// `tag` must be a string literal or another compile-time constant, exactly as
// with ESP_LOGx. Prefer a `constexpr const char* kTag = "module";` per file.

#include <cstddef>
#include <cstdint>

#include "esp_log.h"

#define CORE_LOGE(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define CORE_LOGW(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define CORE_LOGI(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define CORE_LOGD(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)
#define CORE_LOGV(tag, fmt, ...) ESP_LOGV(tag, fmt, ##__VA_ARGS__)

namespace core {

/// Hex dump a buffer to the log at the given level.
inline void log_hex(const char* tag, const void* data, size_t len, esp_log_level_t level = ESP_LOG_INFO) noexcept
{
    if (level == ESP_LOG_ERROR) {
        ESP_LOG_BUFFER_HEX_LEVEL(tag, data, len, ESP_LOG_ERROR);
    } else if (level == ESP_LOG_WARN) {
        ESP_LOG_BUFFER_HEX_LEVEL(tag, data, len, ESP_LOG_WARN);
    } else if (level == ESP_LOG_DEBUG) {
        ESP_LOG_BUFFER_HEX_LEVEL(tag, data, len, ESP_LOG_DEBUG);
    } else if (level == ESP_LOG_VERBOSE) {
        ESP_LOG_BUFFER_HEX_LEVEL(tag, data, len, ESP_LOG_VERBOSE);
    } else {
        ESP_LOG_BUFFER_HEX_LEVEL(tag, data, len, ESP_LOG_INFO);
    }
}

/// Hex + ASCII dump, the readable variant for protocol debugging.
inline void log_hexdump(const char* tag, const void* data, size_t len, esp_log_level_t level = ESP_LOG_INFO) noexcept
{
    if (level == ESP_LOG_ERROR) {
        ESP_LOG_BUFFER_HEXDUMP(tag, data, len, ESP_LOG_ERROR);
    } else if (level == ESP_LOG_WARN) {
        ESP_LOG_BUFFER_HEXDUMP(tag, data, len, ESP_LOG_WARN);
    } else if (level == ESP_LOG_DEBUG) {
        ESP_LOG_BUFFER_HEXDUMP(tag, data, len, ESP_LOG_DEBUG);
    } else if (level == ESP_LOG_VERBOSE) {
        ESP_LOG_BUFFER_HEXDUMP(tag, data, len, ESP_LOG_VERBOSE);
    } else {
        ESP_LOG_BUFFER_HEXDUMP(tag, data, len, ESP_LOG_INFO);
    }
}

/// Change the log level of one tag at run time.
inline void set_log_level(const char* tag, esp_log_level_t level) noexcept
{
    esp_log_level_set(tag, level);
}

} // namespace core
