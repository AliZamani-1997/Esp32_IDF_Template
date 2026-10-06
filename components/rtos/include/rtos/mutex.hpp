#pragma once

// RAII wrapper over FreeRTOS mutexes (freertos/semphr.h).

#include <cstdint>

#include "core/detail/ticks.hpp"
#include "core/error.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace rtos {

/// A FreeRTOS mutex.
///
/// `normal` is the right default: it is cheaper, and it catches accidental
/// re-entrancy instead of hiding it. Use `recursive` only where a lock really
/// must be taken twice by the same task.
class Mutex {
public:
    enum class Kind { normal, recursive };

    Mutex() = default;
    ~Mutex();

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    /// Create the underlying semaphore. Call once, during start-up.
    core::Status init(Kind kind = Kind::normal) noexcept;

    /// Take the lock, waiting up to `timeout_ms`.
    core::Status lock(uint32_t timeout_ms = core::kWaitForever) noexcept;

    /// Take the lock from an ISR. Never blocks; fails immediately if taken.
    core::Status lock_from_isr() noexcept;

    /// Release the lock. Safe to call when not held (it is a no-op only if the
    /// semaphore exists; an unlocked release still returns an error).
    core::Status unlock() noexcept;

    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] SemaphoreHandle_t handle() const noexcept { return handle_; }

private:
    SemaphoreHandle_t handle_{nullptr};
};

/// RAII lock. Releases on scope exit, in the destructor.
///
/// Always check locked() when the timeout is finite — a failed acquisition is
/// not an error the guard can report.
///
///     Mutex m;
///     ...
///     {
///         LockGuard guard(m);
///         if (!guard.locked()) { /* timed out */ }
///         // critical section
///     }
class LockGuard {
public:
    explicit LockGuard(Mutex& mutex, uint32_t timeout_ms = core::kWaitForever) noexcept;
    ~LockGuard();

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
    LockGuard(LockGuard&&) = delete;
    LockGuard& operator=(LockGuard&&) = delete;

    [[nodiscard]] bool locked() const noexcept { return locked_; }

private:
    Mutex& mutex_;
    bool locked_{false};
};

} // namespace rtos
