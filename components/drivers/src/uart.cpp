#include "drivers/uart.hpp"

namespace drivers {

Uart::~Uart()
{
    close();
}

Uart::Uart(Uart&& other) noexcept
    : port_(other.port_)
    , open_(other.open_)
{
    other.open_ = false;
}

Uart& Uart::operator=(Uart&& other) noexcept
{
    if (this != &other) {
        close();
        port_ = other.port_;
        open_ = other.open_;
        other.open_ = false;
    }
    return *this;
}

void Uart::close() noexcept
{
    if (open_) {
        (void)uart_driver_delete(port_);
        open_ = false;
    }
}

core::Result<Uart> Uart::open(uart_port_t port, const Config& cfg) noexcept
{
    // 1. Route the pins first, so the driver installs onto the right pads.
    if (const esp_err_t rc = uart_set_pin(port, cfg.tx_pin, cfg.rx_pin, cfg.rts_pin, cfg.cts_pin);
        rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }

    // 2. Apply the line settings.
    uart_config_t line = {};
    line.baud_rate = cfg.baud_rate;
    line.data_bits = cfg.data_bits;
    line.parity = cfg.parity;
    line.stop_bits = cfg.stop_bits;
    line.flow_ctrl = cfg.flow_ctrl;
    line.rx_flow_ctrl_thresh = cfg.rx_flow_ctrl_thresh;

    if (const esp_err_t rc = uart_param_config(port, &line); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }

    // 3. Install the driver last, once the port is fully described.
    if (const esp_err_t rc = uart_driver_install(port, cfg.rx_buffer_size, cfg.tx_buffer_size, 0,
                                                 nullptr, 0);
        rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }

    Uart u;
    u.port_ = port;
    u.open_ = true;
    return u;
}

core::Status Uart::write(const void* data, size_t len) noexcept
{
    if (!open_) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (len == 0) {
        return {};
    }
    const int written = uart_write_bytes(port_, data, len);
    if (written < 0) {
        return std::unexpected(core::Error{ESP_FAIL});
    }
    return {};
}

core::Status Uart::write_str(const char* text) noexcept
{
    if (text == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    size_t len = 0;
    while (text[len] != '\0') {
        ++len;
    }
    return write(text, len);
}

core::Result<size_t> Uart::read(void* buffer, size_t len, uint32_t timeout_ms) noexcept
{
    if (!open_) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (buffer == nullptr || len == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    const TickType_t ticks = core::detail::to_ticks(timeout_ms);
    const int got = uart_read_bytes(port_, buffer, len, ticks);
    if (got < 0) {
        return std::unexpected(core::Error{ESP_FAIL});
    }
    return static_cast<size_t>(got);
}

core::Status Uart::flush_input() noexcept
{
    if (!open_) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(uart_flush_input(port_));
}

core::Result<size_t> Uart::available() const noexcept
{
    if (!open_) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    size_t pending = 0;
    if (const esp_err_t rc = uart_get_buffered_data_len(port_, &pending); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return pending;
}

core::Status Uart::set_baud_rate(int baud_rate) noexcept
{
    if (!open_) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(uart_set_baudrate(port_, baud_rate));
}

} // namespace drivers
