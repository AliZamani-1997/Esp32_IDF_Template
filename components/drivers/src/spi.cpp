#include "drivers/spi.hpp"

namespace drivers {
namespace {

/// DMA transfers must start on a word boundary and be a multiple of 4 bytes.
/// Rather than silently corrupting data, reject anything that cannot be sent.
constexpr bool dma_compatible(const void* buffer, size_t len) noexcept
{
    if (buffer == nullptr) {
        return true;
    }
    const auto addr = reinterpret_cast<uintptr_t>(buffer);
    return (addr % 4 == 0) && (len % 4 == 0);
}

} // namespace

// ---------------------------------------------------------------------------
// SpiDevice
// ---------------------------------------------------------------------------

SpiDevice::~SpiDevice()
{
    detach();
}

SpiDevice::SpiDevice(SpiDevice&& other) noexcept
    : handle_(other.handle_)
{
    other.handle_ = nullptr;
}

SpiDevice& SpiDevice::operator=(SpiDevice&& other) noexcept
{
    if (this != &other) {
        detach();
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

void SpiDevice::detach() noexcept
{
    if (handle_ != nullptr) {
        (void)spi_bus_remove_device(handle_);
        handle_ = nullptr;
    }
}

core::Status SpiDevice::transmit(const uint8_t* tx, uint8_t* rx, size_t len) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (len == 0) {
        return {};
    }
    if (!dma_compatible(tx, len) || !dma_compatible(rx, len)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    return core::to_status(spi_device_polling_transmit(handle_, &t));
}

core::Status SpiDevice::write(const uint8_t* data, size_t len) noexcept
{
    return transmit(data, nullptr, len);
}

core::Status SpiDevice::read(uint8_t* buffer, size_t len) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (buffer == nullptr && len != 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    spi_transaction_t t = {};
    t.length = len * 8;
    t.rxlength = len * 8;
    t.rx_buffer = buffer;
    // No tx_buffer: the driver clocks out 0x00, which most devices accept as a
    // read frame. Override with transmit() when a chip needs 0xFF.
    return core::to_status(spi_device_polling_transmit(handle_, &t));
}

core::Status SpiDevice::write_read(const uint8_t* write_data, size_t write_len,
                                   uint8_t* read_buffer, size_t read_len) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }

    spi_transaction_t t = {};
    t.length = write_len * 8;
    t.rxlength = read_len * 8;
    t.tx_buffer = write_data;
    t.rx_buffer = read_buffer;

    return core::to_status(spi_device_polling_transmit(handle_, &t));
}

// ---------------------------------------------------------------------------
// SpiBus
// ---------------------------------------------------------------------------

SpiBus::~SpiBus()
{
    if (handle_valid_) {
        (void)spi_bus_free(host_);
        handle_valid_ = false;
    }
}

SpiBus::SpiBus(SpiBus&& other) noexcept
    : host_(other.host_)
    , handle_valid_(other.handle_valid_)
{
    other.handle_valid_ = false;
}

SpiBus& SpiBus::operator=(SpiBus&& other) noexcept
{
    if (this != &other) {
        if (handle_valid_) {
            (void)spi_bus_free(host_);
        }
        host_ = other.host_;
        handle_valid_ = other.handle_valid_;
        other.handle_valid_ = false;
    }
    return *this;
}

core::Result<SpiBus> SpiBus::create(const Config& cfg) noexcept
{
    if (cfg.mosi == GPIO_NUM_NC || cfg.sclk == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    spi_bus_config_t bus = {};
    bus.mosi_io_num = cfg.mosi;
    bus.miso_io_num = cfg.miso;
    bus.sclk_io_num = cfg.sclk;
    bus.quadwp_io_num = GPIO_NUM_NC;
    bus.quadhd_io_num = GPIO_NUM_NC;
    bus.max_transfer_sz = cfg.max_transfer_bytes;

    const spi_dma_chan_t dma = cfg.use_dma ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED;
    if (const esp_err_t rc = spi_bus_initialize(cfg.host, &bus, dma); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }

    SpiBus out;
    out.host_ = cfg.host;
    out.handle_valid_ = true;
    return out;
}

core::Result<SpiDevice> SpiBus::add_device(gpio_num_t cs_pin, int clock_hz, int mode) noexcept
{
    if (!handle_valid_) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (mode < 0 || mode > 3) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    spi_device_interface_config_t dev = {};
    dev.clock_speed_hz = clock_hz;
    dev.mode = mode;
    dev.spics_io_num = cs_pin;
    dev.queue_size = 1;

    SpiDevice device;
    if (const esp_err_t rc = spi_bus_add_device(host_, &dev, &device.handle_); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return device;
}

} // namespace drivers
