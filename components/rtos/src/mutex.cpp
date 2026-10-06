#include "rtos/mutex.hpp"

namespace rtos {

Mutex::~Mutex()
{
    if (handle_ != nullptr) {
        vSemaphoreDelete(handle_);
        handle_ = nullptr;
    }
}

core::Status Mutex::init(Kind kind) noexcept
{
    if (handle_ != nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }

    handle_ = (kind == Kind::recursive) ? xSemaphoreCreateRecursiveMutex()
                                        : xSemaphoreCreateMutex();
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_NO_MEM});
    }
    return {};
}

core::Status Mutex::lock(uint32_t timeout_ms) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(xSemaphoreTake(handle_, core::detail::to_ticks(timeout_ms)) == pdTRUE
                           ? ESP_OK
                           : ESP_ERR_TIMEOUT);
}

core::Status Mutex::lock_from_isr() noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    BaseType_t higher_priority_task_woken = pdFALSE;
    const BaseType_t taken = xSemaphoreTakeFromISR(handle_, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
    return core::to_status(taken == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT);
}

core::Status Mutex::unlock() noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(xSemaphoreGive(handle_) == pdTRUE ? ESP_OK : ESP_ERR_INVALID_STATE);
}

LockGuard::LockGuard(Mutex& mutex, uint32_t timeout_ms) noexcept
    : mutex_(mutex)
{
    locked_ = mutex_.lock(timeout_ms).has_value();
}

LockGuard::~LockGuard()
{
    if (locked_) {
        (void)mutex_.unlock();
    }
}

} // namespace rtos
