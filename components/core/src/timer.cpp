#include "core/timer.hpp"

#include "sdkconfig.h"

#include "esp_rom_sys.h"

namespace core {

Timer::~Timer()
{
    if (handle_ != nullptr) {
        (void)esp_timer_stop(handle_);
        (void)esp_timer_delete(handle_);
        handle_ = nullptr;
    }
}

Timer::Timer(Timer&& other) noexcept
    : handle_(other.release())
{
}

Timer& Timer::operator=(Timer&& other) noexcept
{
    if (this != &other) {
        if (handle_ != nullptr) {
            (void)esp_timer_stop(handle_);
            (void)esp_timer_delete(handle_);
        }
        handle_ = other.release();
    }
    return *this;
}

esp_timer_handle_t Timer::release() noexcept
{
    esp_timer_handle_t handle = handle_;
    handle_ = nullptr;
    return handle;
}

Result<Timer> Timer::create(const char* name, Callback callback, void* arg, Dispatch dispatch) noexcept
{
    if (callback == nullptr) {
        return std::unexpected(Error{ESP_ERR_INVALID_ARG});
    }

#if !CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD
    // ESP_TIMER_ISR only exists when this Kconfig option is enabled. Reject the
    // request explicitly instead of silently downgrading to task dispatch.
    if (dispatch == Dispatch::isr) {
        return std::unexpected(Error{ESP_ERR_NOT_SUPPORTED});
    }
#endif

    esp_timer_create_args_t args = {};
    args.callback = callback;
    args.arg = arg;
#if CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD
    args.dispatch_method = (dispatch == Dispatch::isr) ? ESP_TIMER_ISR : ESP_TIMER_TASK;
#else
    args.dispatch_method = ESP_TIMER_TASK;
#endif
    args.name = name;

    Timer timer;
    if (const esp_err_t rc = esp_timer_create(&args, &timer.handle_); rc != ESP_OK) {
        return std::unexpected(Error{rc});
    }
    return timer;
}

Status Timer::start_once(uint64_t period_us) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(Error{ESP_ERR_INVALID_STATE});
    }
    return to_status(esp_timer_start_once(handle_, period_us));
}

Status Timer::start_periodic(uint64_t period_us) noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(Error{ESP_ERR_INVALID_STATE});
    }
    return to_status(esp_timer_start_periodic(handle_, period_us));
}

Status Timer::stop() noexcept
{
    if (handle_ == nullptr) {
        return std::unexpected(Error{ESP_ERR_INVALID_STATE});
    }
    return to_status(esp_timer_stop(handle_));
}

uint64_t Timer::now_us() noexcept
{
    return static_cast<uint64_t>(esp_timer_get_time());
}

uint64_t Timer::now_ms() noexcept
{
    return static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
}

void Timer::busy_wait_us(uint32_t us) noexcept
{
    esp_rom_delay_us(us);
}

} // namespace core
