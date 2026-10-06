#include "drivers/nvs.hpp"

namespace drivers {
namespace {

/// NVS keys are NUL-terminated, so a string_view must be. Guard against a
/// view that points into a larger buffer and would silently read past its end.
bool key_is_terminated(std::string_view key) noexcept
{
    return !key.empty() && key.data()[key.size()] == '\0';
}

} // namespace

core::Status nvs_init() noexcept
{
    esp_err_t rc = nvs_flash_init();

    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Both mean "the partition is unusable as-is". Erase and retry once;
        // if it fails again the flash itself is the problem.
        if (const esp_err_t erase_rc = nvs_flash_erase(); erase_rc != ESP_OK) {
            return std::unexpected(core::Error{erase_rc});
        }
        rc = nvs_flash_init();
    }

    return core::to_status(rc);
}

core::Status nvs_erase_partition() noexcept
{
    return core::to_status(nvs_flash_erase());
}

NvsHandle::~NvsHandle()
{
    close();
}

NvsHandle::NvsHandle(NvsHandle&& other) noexcept
    : handle_(other.handle_)
{
    other.handle_ = 0;
}

NvsHandle& NvsHandle::operator=(NvsHandle&& other) noexcept
{
    if (this != &other) {
        close();
        handle_ = other.handle_;
        other.handle_ = 0;
    }
    return *this;
}

void NvsHandle::close() noexcept
{
    if (handle_ != 0) {
        nvs_close(handle_);
        handle_ = 0;
    }
}

core::Result<NvsHandle> NvsHandle::open(std::string_view name_space, Mode mode) noexcept
{
    if (!key_is_terminated(name_space)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    NvsHandle handle;
    const nvs_open_mode_t open_mode = (mode == Mode::read_only) ? NVS_READONLY : NVS_READWRITE;
    if (const esp_err_t rc = nvs_open(name_space.data(), open_mode, &handle.handle_); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return handle;
}

core::Result<size_t> NvsHandle::get_str_size(std::string_view key) const noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (!key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    size_t len = 0;
    if (const esp_err_t rc = nvs_get_str(handle_, key.data(), nullptr, &len); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return len;
}

core::Result<size_t> NvsHandle::get_str(std::string_view key, char* buffer, size_t buffer_size) const noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (buffer == nullptr || buffer_size == 0 || !key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    size_t len = buffer_size;
    if (const esp_err_t rc = nvs_get_str(handle_, key.data(), buffer, &len); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    // nvs_get_str reports the length including the terminator.
    return (len > 0) ? (len - 1) : 0;
}

core::Status NvsHandle::set_str(std::string_view key, std::string_view value) noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (!key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    return core::to_status(nvs_set_str(handle_, key.data(), value.data()));
}

core::Result<size_t> NvsHandle::get_blob_size(std::string_view key) const noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (!key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    size_t len = 0;
    if (const esp_err_t rc = nvs_get_blob(handle_, key.data(), nullptr, &len); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return len;
}

core::Result<size_t> NvsHandle::get_blob(std::string_view key, void* buffer, size_t buffer_size) const noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (buffer == nullptr || buffer_size == 0 || !key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    size_t len = buffer_size;
    if (const esp_err_t rc = nvs_get_blob(handle_, key.data(), buffer, &len); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return len;
}

core::Status NvsHandle::set_blob(std::string_view key, const void* data, size_t len) noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (!key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    return core::to_status(nvs_set_blob(handle_, key.data(), data, len));
}

core::Status NvsHandle::erase(std::string_view key) noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    if (!key_is_terminated(key)) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }
    return core::to_status(nvs_erase_key(handle_, key.data()));
}

core::Status NvsHandle::erase_all() noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(nvs_erase_all(handle_));
}

core::Status NvsHandle::commit() noexcept
{
    if (handle_ == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(nvs_commit(handle_));
}

} // namespace drivers
