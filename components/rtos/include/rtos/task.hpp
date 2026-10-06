#pragma once

// RAII wrapper over FreeRTOS tasks (freertos/task.h).

#include <cstdint>

#include "core/error.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace rtos {

/// A FreeRTOS task owned by this object.
///
/// The task is created suspended-free and starts running immediately. The
/// destructor deletes it, unless the task is deleting itself — see stop().
class Task {
public:
    /// The task body. Must never return on a task created by Task::start();
    /// call Task::stop() or vTaskDelete(nullptr) to end it.
    using Entry = void (*)(void*);

    /// Everything xTaskCreatePinnedToCore needs, with sane defaults.
    struct Config {
        const char* name = "task";
        uint32_t stack_bytes = 4096;        ///< Stack size in BYTES (ESP-IDF convention).
        UBaseType_t priority = 5;           ///< 0 = idle, higher = more urgent.
        BaseType_t core = tskNO_AFFINITY;   ///< tskNO_AFFINITY, 0 or 1.
        void* arg = nullptr;                ///< Passed to Entry.
    };

    Task() = default;
    ~Task();

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& other) noexcept;
    Task& operator=(Task&& other) noexcept;

    /// Create and start a task.
    [[nodiscard]] static core::Result<Task> start(const Config& cfg, Entry entry) noexcept;

    /// Delete the task.
    ///
    /// Refuses when called from the task itself: deleting the running task
    /// would skip this object's destructor and leak whatever it owns. A task
    /// that wants to end itself should call vTaskDelete(nullptr) directly and
    /// accept that its own Task object must be owned elsewhere.
    core::Status stop() noexcept;

    /// Give up ownership without deleting. Used by the move constructor.
    [[nodiscard]] TaskHandle_t release() noexcept;

    [[nodiscard]] bool running() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] TaskHandle_t handle() const noexcept { return handle_; }

    // --- Task-side helpers (no object required) ----------------------------

    /// Sleep for `ms`, yielding the CPU to lower-priority work.
    static void delay_ms(uint32_t ms) noexcept;

    /// Sleep for `ticks`.
    static void delay_ticks(TickType_t ticks) noexcept;

    /// Yield to any task of equal or higher priority that is ready to run.
    static void yield() noexcept;

    /// Free heap in bytes, or 0 if the heap is exhausted.
    [[nodiscard]] static uint32_t free_heap_bytes() noexcept;

    /// Stack high-water mark of the calling task, in bytes.
    [[nodiscard]] static uint32_t stack_high_water_bytes() noexcept;

    /// Handle of the calling task.
    [[nodiscard]] static TaskHandle_t current_handle() noexcept;

private:
    TaskHandle_t handle_{nullptr};
};

} // namespace rtos
