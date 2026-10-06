#pragma once

// RAII wrapper over the esp_timer high-resolution timer (esp_timer.h).
//
// esp_timer callbacks run in the esp_timer task by default, NOT in an ISR, so
// they may block briefly. Keep them short anyway: a slow callback delays every
// other timer in the system.

#include <cstdint>

#include "core/error.hpp"

#include "esp_timer.h"

namespace core {

/// A one-shot or periodic timer.
class Timer {
public:
    /// The timer body. Runs in the esp_timer task; must not block for long.
    using Callback = void (*)(void*);

    /// Where the callback runs.
    enum class Dispatch {
        task,  ///< esp_timer's own task. Can use FreeRTOS APIs.
        isr,   ///< Interrupt context. Must be ISR-safe and very short.
    };

    Timer() = default;
    ~Timer();

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&& other) noexcept;
    Timer& operator=(Timer&& other) noexcept;

    /// Create the timer. It is not started yet.
    [[nodiscard]] static Result<Timer> create(const char* name,
                                              Callback callback,
                                              void* arg = nullptr,
                                              Dispatch dispatch = Dispatch::task) noexcept;

    /// Fire once after `period_us` microseconds.
    Status start_once(uint64_t period_us) noexcept;

    /// Fire every `period_us` microseconds until stopped.
    Status start_periodic(uint64_t period_us) noexcept;

    /// Stop a running timer. Safe to call when already stopped.
    Status stop() noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] esp_timer_handle_t handle() const noexcept { return handle_; }

    /// Give up ownership without deleting. Used by the move constructor.
    [[nodiscard]] esp_timer_handle_t release() noexcept;

    // --- Clock helpers ------------------------------------------------------

    /// Microseconds since boot. Monotonic, never jumps backwards.
    [[nodiscard]] static uint64_t now_us() noexcept;

    /// Milliseconds since boot.
    [[nodiscard]] static uint64_t now_ms() noexcept;

    /// Busy-wait for `us` microseconds. Blocks the CPU; use sparingly.
    static void busy_wait_us(uint32_t us) noexcept;

private:
    esp_timer_handle_t handle_{nullptr};
};

} // namespace core
