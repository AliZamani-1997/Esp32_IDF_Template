#pragma once

// RAII wrapper over the ESP-IDF NVS key-value store (nvs_flash.h).
//
// An NvsHandle is one open namespace. Values are written to flash by nvs_set_*
// and made durable by commit(); the destructor closes the handle but does NOT
// commit, so an uncommitted write is discarded. That is deliberate: it keeps
// the "did this reach flash?" question explicit at the call site.

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "core/error.hpp"

#include "nvs.h"
#include "nvs_flash.h"

namespace drivers {

/// Initialise the default NVS partition.
///
/// Handles the two states that a fresh device legitimately hits: a partition
/// with no free pages (erased and retried once) and a version mismatch (erased
/// and retried once). Anything else is returned as an error.
[[nodiscard]] core::Status nvs_init() noexcept;

/// Erase the whole default NVS partition.
[[nodiscard]] core::Status nvs_erase_partition() noexcept;

/// One open NVS namespace.
class NvsHandle {
public:
    enum class Mode { read_only, read_write };

    NvsHandle() = default;
    ~NvsHandle();

    NvsHandle(const NvsHandle&) = delete;
    NvsHandle& operator=(const NvsHandle&) = delete;
    NvsHandle(NvsHandle&& other) noexcept;
    NvsHandle& operator=(NvsHandle&& other) noexcept;

    [[nodiscard]] static core::Result<NvsHandle> open(std::string_view name_space,
                                                      Mode mode = Mode::read_write) noexcept;

    /// Read a fixed-width integer. T must be one of the NVS integer types.
    template <class T>
    [[nodiscard]] core::Result<T> get(std::string_view key) const noexcept
    {
        static_assert(std::is_integral_v<T>, "NvsHandle::get<T> requires an integral T");
        if (handle_ == 0) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }

        T value{};
        esp_err_t rc = ESP_FAIL;
        if constexpr (std::is_same_v<T, int8_t>) {
            rc = nvs_get_i8(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            rc = nvs_get_u8(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            rc = nvs_get_i16(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            rc = nvs_get_u16(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            rc = nvs_get_i32(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            rc = nvs_get_u32(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            rc = nvs_get_i64(handle_, key.data(), &value);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            rc = nvs_get_u64(handle_, key.data(), &value);
        } else {
            static_assert(sizeof(T) == 0, "unsupported NVS integer type");
        }

        if (rc != ESP_OK) {
            return std::unexpected(core::Error{rc});
        }
        return value;
    }

    /// Write a fixed-width integer. Not durable until commit().
    template <class T>
    [[nodiscard]] core::Status set(std::string_view key, T value) noexcept
    {
        static_assert(std::is_integral_v<T>, "NvsHandle::set<T> requires an integral T");
        if (handle_ == 0) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }

        esp_err_t rc = ESP_FAIL;
        if constexpr (std::is_same_v<T, int8_t>) {
            rc = nvs_set_i8(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            rc = nvs_set_u8(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            rc = nvs_set_i16(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            rc = nvs_set_u16(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            rc = nvs_set_i32(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            rc = nvs_set_u32(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            rc = nvs_set_i64(handle_, key.data(), value);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            rc = nvs_set_u64(handle_, key.data(), value);
        } else {
            static_assert(sizeof(T) == 0, "unsupported NVS integer type");
        }

        return core::to_status(rc);
    }

    /// Length in bytes of the stored string, including the terminator.
    [[nodiscard]] core::Result<size_t> get_str_size(std::string_view key) const noexcept;

    /// Read a string into `buffer`. The result is the number of bytes written,
    /// excluding the terminator.
    [[nodiscard]] core::Result<size_t> get_str(std::string_view key, char* buffer,
                                               size_t buffer_size) const noexcept;

    /// Store a string. Not durable until commit().
    [[nodiscard]] core::Status set_str(std::string_view key, std::string_view value) noexcept;

    /// Length in bytes of the stored blob.
    [[nodiscard]] core::Result<size_t> get_blob_size(std::string_view key) const noexcept;

    /// Read a blob into `buffer`.
    [[nodiscard]] core::Result<size_t> get_blob(std::string_view key, void* buffer,
                                                size_t buffer_size) const noexcept;

    /// Store a blob. Not durable until commit().
    [[nodiscard]] core::Status set_blob(std::string_view key, const void* data, size_t len) noexcept;

    /// Remove one key.
    [[nodiscard]] core::Status erase(std::string_view key) noexcept;

    /// Remove every key in this namespace.
    [[nodiscard]] core::Status erase_all() noexcept;

    /// Flush pending writes to flash. Writes are lost without this.
    [[nodiscard]] core::Status commit() noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_ != 0; }
    [[nodiscard]] nvs_handle_t handle() const noexcept { return handle_; }

private:
    void close() noexcept;

    // nvs_handle_t is a uint32_t, not a pointer, so 0 (not nullptr) is the
    // "no open namespace" sentinel.
    nvs_handle_t handle_{0};
};

} // namespace drivers
