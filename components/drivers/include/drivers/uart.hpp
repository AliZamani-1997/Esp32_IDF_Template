#pragma once

// RAII wrapper over the ESP-IDF UART driver (driver/uart.h).
//
// Owns the driver installation and the TX/RX buffers. Closing the Uart releases
// both, so ports never stay claimed after the object dies.

#include <cstddef>
#include <cstdint>

#include "core/detail/ticks.hpp"
#include "core/error.hpp"

#include "driver/gpio.h"
#include "driver/uart.h"

namespace drivers {

/// A UART port.
class Uart {
public:
    /// Everything needed to bring a port up.
    struct Config {
        int baud_rate = 115200;
        uart_word_length_t data_bits = UART_DATA_8_BITS;
        uart_parity_t parity = UART_PARITY_DISABLE;
        uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
        uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uint8_t rx_flow_ctrl_thresh = 0;

        /// Driver buffers, in bytes. 0 disables that direction's buffer.
        int rx_buffer_size = 1024;
        int tx_buffer_size = 0;

        /// Pin routing. GPIO_NUM_NC (== UART_PIN_NO_CHANGE) keeps the
        /// current/default pin. The macro itself cannot be used here because it
        /// expands to a plain int (-1), which does not implicitly convert to
        /// the gpio_num_t enum in C++.
        gpio_num_t tx_pin = GPIO_NUM_NC;
        gpio_num_t rx_pin = GPIO_NUM_NC;
        gpio_num_t rts_pin = GPIO_NUM_NC;
        gpio_num_t cts_pin = GPIO_NUM_NC;
    };

    Uart() = default;
    ~Uart();

    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;
    Uart(Uart&& other) noexcept;
    Uart& operator=(Uart&& other) noexcept;

    /// Configure pins, apply settings, then install the driver.
    [[nodiscard]] static core::Result<Uart> open(uart_port_t port, const Config& cfg) noexcept;

    /// Write `len` bytes. Blocks until the driver buffer accepts them.
    core::Status write(const void* data, size_t len) noexcept;

    /// Write a NUL-terminated string.
    core::Status write_str(const char* text) noexcept;

    /// Read up to `len` bytes, waiting at most `timeout_ms`.
    [[nodiscard]] core::Result<size_t> read(void* buffer, size_t len,
                                            uint32_t timeout_ms = core::kWaitForever) noexcept;

    /// Discard whatever is in the RX buffer.
    core::Status flush_input() noexcept;

    /// Bytes currently waiting in the RX buffer.
    [[nodiscard]] core::Result<size_t> available() const noexcept;

    /// Change the baud rate at run time.
    core::Status set_baud_rate(int baud_rate) noexcept;

    [[nodiscard]] bool valid() const noexcept { return open_; }
    [[nodiscard]] uart_port_t port() const noexcept { return port_; }

private:
    void close() noexcept;

    uart_port_t port_{UART_NUM_0};
    bool open_{false};
};

} // namespace drivers
