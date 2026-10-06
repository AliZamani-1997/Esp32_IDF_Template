#include "drivers/i2c.hpp"

namespace drivers {

// ---------------------------------------------------------------------------
// I2cDevice
// ---------------------------------------------------------------------------

I2cDevice::~I2cDevice()
{
    detach();
}

I2cDevice::I2cDevice(I2cDevice&& other) noexcept
    : handle_(other.handle_)
    , address_(other.address_)
{
    other.handle_ = nullptr;
}

I2cDevice& I2cDevice::operator=(I2cDevice&& other) noexcept
{
    if (this != &other) {
        detach();
        handle_ = other.handle_;
        address_ = other.address_;
        other.handle_ = nullptr;
    }
    return *this;
}

void I2cDevice::detach() noexcept
{
    if (handle_ != nullptr) {
        (void)i2c_master_bus_rm_device(handle_);
        handle_ = nullptr;
    }
}

core::Status I2cDevice::write(const uint8_t* data, size_t len, uint32_t timeout_ms) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (data == nullptr && len != 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    return core::to_status(i2c_master_transmit(handle_, data, len, static_cast<int>(timeout_ms)));
}

core::Result<size_t> I2cDevice::read(uint8_t* buffer, size_t len, uint32_t timeout_ms) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (buffer == nullptr && len != 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    if (const esp_err_t rc = i2c_master_receive(handle_, buffer, len, static_cast<int>(timeout_ms));
        rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return len;
}

core::Status I2cDevice::write_read(const uint8_t* write_data, size_t write_len,
                                   uint8_t* read_buffer, size_t read_len,
                                   uint32_t timeout_ms) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(i2c_master_transmit_receive(handle_, write_data, write_len, read_buffer,
                                                       read_len, static_cast<int>(timeout_ms)));
}

core::Result<uint8_t> I2cDevice::read_register(uint8_t reg, uint32_t timeout_ms) noexcept
{
    uint8_t value = 0;
    CORE_TRY_VOID(write_read(&reg, 1, &value, 1, timeout_ms));
    return value;
}

core::Status I2cDevice::write_register(uint8_t reg, uint8_t value, uint32_t timeout_ms) noexcept
{
    const uint8_t frame[2] = {reg, value};
    return write(frame, sizeof(frame), timeout_ms);
}

core::Result<size_t> I2cDevice::read_registers(uint8_t reg, uint8_t* buffer, size_t len,
                                               uint32_t timeout_ms) noexcept
{
    if (buffer == nullptr && len != 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    CORE_TRY_VOID(write_read(&reg, 1, buffer, len, timeout_ms));
    return len;
}

// ---------------------------------------------------------------------------
// I2cMasterBus
// ---------------------------------------------------------------------------

I2cMasterBus::~I2cMasterBus()
{
    if (handle_ != nullptr) {
        (void)i2c_del_master_bus(handle_);
        handle_ = nullptr;
    }
}

I2cMasterBus::I2cMasterBus(I2cMasterBus&& other) noexcept
    : handle_(other.release())
{
}

I2cMasterBus& I2cMasterBus::operator=(I2cMasterBus&& other) noexcept
{
    if (this != &other) {
        if (handle_ != nullptr) {
            (void)i2c_del_master_bus(handle_);
        }
        handle_ = other.release();
    }
    return *this;
}

i2c_master_bus_handle_t I2cMasterBus::release() noexcept
{
    i2c_master_bus_handle_t handle = handle_;
    handle_ = nullptr;
    return handle;
}

core::Result<I2cMasterBus> I2cMasterBus::create(const Config& cfg) noexcept
{
    if (cfg.sda == GPIO_NUM_NC || cfg.scl == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    i2c_master_bus_config_t bus = {};
    bus.i2c_port = cfg.port;
    bus.sda_io_num = cfg.sda;
    bus.scl_io_num = cfg.scl;
    bus.clk_source = I2C_CLK_SRC_DEFAULT;
    bus.glitch_ignore_cnt = cfg.glitch_ignore_cnt;
    bus.flags.enable_internal_pullup = cfg.enable_internal_pullup ? 1U : 0U;

    I2cMasterBus out;
    if (const esp_err_t rc = i2c_new_master_bus(&bus, &out.handle_); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return out;
}

core::Result<I2cDevice> I2cMasterBus::add_device(uint16_t address, uint32_t scl_speed_hz) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }

    i2c_device_config_t dev = {};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev.device_address = address;
    dev.scl_speed_hz = scl_speed_hz;

    I2cDevice device;
    if (const esp_err_t rc = i2c_master_bus_add_device(handle_, &dev, &device.handle_); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    device.address_ = address;
    return device;
}

core::Result<bool> I2cMasterBus::probe(uint16_t address, uint32_t timeout_ms) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    const esp_err_t rc = i2c_master_probe(handle_, address, static_cast<int>(timeout_ms));
    if (rc == ESP_OK) {
        return true;
    }
    // A missing device is a normal outcome of a scan, not a failure.
    if (rc == ESP_ERR_NOT_FOUND || rc == ESP_ERR_TIMEOUT) {
        return false;
    }
    return std::unexpected(core::Error{rc});
}

core::Status I2cMasterBus::reset() noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(i2c_master_bus_reset(handle_));
}

} // namespace drivers
