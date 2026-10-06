#pragma once

// RAII wrapper over the ESP-IDF SPI master driver (driver/spi_master.h).
//
// LIFETIME RULE: an SpiDevice is only valid while its SpiBus is alive. Declare
// the bus first, devices after it.
//
// Transactions are polling (spi_device_polling_transmit): the call blocks until
// the transfer finishes. That is the right default for register-style devices.
// Interrupt-driven queued transfers are not wrapped yet — add them when a
// driver actually needs the CPU back during a transfer.

#include <cstddef>
#include <cstdint>

#include "core/error.hpp"

#include "driver/gpio.h"
#include "driver/spi_master.h"

namespace drivers {

class SpiBus;

/// One chip-select line on an SpiBus.
class SpiDevice {
public:
    SpiDevice() = default;
    ~SpiDevice();

    SpiDevice(const SpiDevice&) = delete;
    SpiDevice& operator=(const SpiDevice&) = delete;
    SpiDevice(SpiDevice&& other) noexcept;
    SpiDevice& operator=(SpiDevice&& other) noexcept;

    /// Full-duplex transfer. Either buffer may be null to ignore that direction.
    core::Status transmit(const uint8_t* tx, uint8_t* rx, size_t len) noexcept;

    /// Send `len` bytes, ignore the response.
    core::Status write(const uint8_t* data, size_t len) noexcept;

    /// Receive `len` bytes, clocking out 0xFF (the usual "dummy" pattern).
    core::Status read(uint8_t* buffer, size_t len) noexcept;

    /// Write `write_len` bytes, then read `read_len` bytes, in one CS assertion.
    core::Status write_read(const uint8_t* write_data, size_t write_len,
                            uint8_t* read_buffer, size_t read_len) noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] spi_device_handle_t handle() const noexcept { return handle_; }

private:
    friend class SpiBus;

    void detach() noexcept;

    spi_device_handle_t handle_{nullptr};
};

/// An SPI master bus.
class SpiBus {
public:
    struct Config {
        spi_host_device_t host = SPI2_HOST;
        gpio_num_t mosi = GPIO_NUM_NC;
        gpio_num_t miso = GPIO_NUM_NC;
        gpio_num_t sclk = GPIO_NUM_NC;
        /// Largest single transfer, in bytes. Size it to the biggest burst the
        /// application will ever send; it is a hard limit, not a hint.
        int max_transfer_bytes = 4096;
        /// Use DMA. Needed for large transfers; adds a little latency to short
        /// ones.
        bool use_dma = true;
    };

    SpiBus() = default;
    ~SpiBus();

    SpiBus(const SpiBus&) = delete;
    SpiBus& operator=(const SpiBus&) = delete;
    SpiBus(SpiBus&& other) noexcept;
    SpiBus& operator=(SpiBus&& other) noexcept;

    [[nodiscard]] static core::Result<SpiBus> create(const Config& cfg) noexcept;

    /// Attach a device on `cs_pin`, clocked at `clock_hz`.
    ///
    /// `mode` is the standard SPI mode 0-3. Pass GPIO_NUM_NC for `cs_pin` only
    /// if the chip select is driven manually.
    [[nodiscard]] core::Result<SpiDevice> add_device(gpio_num_t cs_pin, int clock_hz, int mode = 0) noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_valid_; }
    [[nodiscard]] spi_host_device_t host() const noexcept { return host_; }

private:
    spi_host_device_t host_{SPI2_HOST};
    bool handle_valid_{false};
};

} // namespace drivers
