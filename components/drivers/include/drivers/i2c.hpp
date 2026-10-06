#pragma once

// RAII wrapper over the ESP-IDF I2C master driver (driver/i2c_master.h).
//
// This uses the modern i2c_master_* API, not the legacy i2c_* driver that
// ESP-IDF v6 still ships for compatibility.
//
// LIFETIME RULE: an I2cDevice is only valid while its I2cMasterBus is alive.
// The bus must outlive every device created from it. Declare the bus first and
// the devices after it, in the same or a narrower scope.

#include <cstddef>
#include <cstdint>

#include "core/error.hpp"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

namespace drivers {

class I2cMasterBus;

/// One addressed device on an I2cMasterBus.
///
/// Moving is allowed; copying is not. The destructor detaches the device from
/// the bus.
class I2cDevice {
public:
    I2cDevice() = default;
    ~I2cDevice();

    I2cDevice(const I2cDevice&) = delete;
    I2cDevice& operator=(const I2cDevice&) = delete;
    I2cDevice(I2cDevice&& other) noexcept;
    I2cDevice& operator=(I2cDevice&& other) noexcept;

    /// Send `len` bytes.
    core::Status write(const uint8_t* data, size_t len, uint32_t timeout_ms = 1000) noexcept;

    /// Receive `len` bytes.
    [[nodiscard]] core::Result<size_t> read(uint8_t* buffer, size_t len, uint32_t timeout_ms = 1000) noexcept;

    /// Write, then read, in a single transaction (the usual register access).
    core::Status write_read(const uint8_t* write_data, size_t write_len,
                            uint8_t* read_buffer, size_t read_len,
                            uint32_t timeout_ms = 1000) noexcept;

    /// Read one byte from register `reg`.
    [[nodiscard]] core::Result<uint8_t> read_register(uint8_t reg, uint32_t timeout_ms = 1000) noexcept;

    /// Write one byte to register `reg`.
    core::Status write_register(uint8_t reg, uint8_t value, uint32_t timeout_ms = 1000) noexcept;

    /// Read a block starting at register `reg`.
    [[nodiscard]] core::Result<size_t> read_registers(uint8_t reg, uint8_t* buffer, size_t len,
                                                      uint32_t timeout_ms = 1000) noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] uint16_t address() const noexcept { return address_; }
    [[nodiscard]] i2c_master_dev_handle_t handle() const noexcept { return handle_; }

private:
    friend class I2cMasterBus;

    void detach() noexcept;

    i2c_master_dev_handle_t handle_{nullptr};
    uint16_t address_{0};
};

/// An I2C master bus.
class I2cMasterBus {
public:
    struct Config {
        /// -1 lets the driver pick a free port.
        i2c_port_num_t port = I2C_NUM_0;
        gpio_num_t sda = GPIO_NUM_NC;
        gpio_num_t scl = GPIO_NUM_NC;
        /// Glitches shorter than this many module clocks are filtered out.
        uint8_t glitch_ignore_cnt = 7;
        /// Internal pull-ups are weak; fit external resistors for anything
        /// above ~100 kHz.
        bool enable_internal_pullup = true;
    };

    I2cMasterBus() = default;
    ~I2cMasterBus();

    I2cMasterBus(const I2cMasterBus&) = delete;
    I2cMasterBus& operator=(const I2cMasterBus&) = delete;
    I2cMasterBus(I2cMasterBus&& other) noexcept;
    I2cMasterBus& operator=(I2cMasterBus&& other) noexcept;

    /// Create the bus and take ownership of the SDA/SCL pins.
    [[nodiscard]] static core::Result<I2cMasterBus> create(const Config& cfg) noexcept;

    /// Attach a device at `address`, clocked at `scl_speed_hz`.
    [[nodiscard]] core::Result<I2cDevice> add_device(uint16_t address, uint32_t scl_speed_hz) noexcept;

    /// True if a device answers at `address`. Useful for bus scanning.
    [[nodiscard]] core::Result<bool> probe(uint16_t address, uint32_t timeout_ms = 100) noexcept;

    /// Reset the bus after a stuck transaction.
    core::Status reset() noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] i2c_master_bus_handle_t handle() const noexcept { return handle_; }

    /// Give up ownership without deleting. Used by the move constructor.
    [[nodiscard]] i2c_master_bus_handle_t release() noexcept;

private:
    i2c_master_bus_handle_t handle_{nullptr};
};

} // namespace drivers
