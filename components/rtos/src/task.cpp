#include "rtos/task.hpp"

#include "esp_system.h"

namespace rtos {

Task::~Task()
{
    if (handle_ != nullptr) {
        // Only safe because the destructor cannot run on the task itself:
        // stop() refuses in that case, so a self-owned Task never reaches here
        // while running.
        vTaskDelete(handle_);
        handle_ = nullptr;
    }
}

Task::Task(Task&& other) noexcept
    : handle_(other.release())
{
}

Task& Task::operator=(Task&& other) noexcept
{
    if (this != &other) {
        if (handle_ != nullptr) {
            vTaskDelete(handle_);
        }
        handle_ = other.release();
    }
    return *this;
}

TaskHandle_t Task::release() noexcept
{
    TaskHandle_t handle = handle_;
    handle_ = nullptr;
    return handle;
}

core::Result<Task> Task::start(const Config& cfg, Entry entry) noexcept
{
    if (entry == nullptr) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    Task task;
    const BaseType_t rc = xTaskCreatePinnedToCore(entry,
                                                  cfg.name,
                                                  cfg.stack_bytes,
                                                  cfg.arg,
                                                  cfg.priority,
                                                  &task.handle_,
                                                  cfg.core);
    if (rc != pdPASS) {
        return std::unexpected(core::Error{ESP_ERR_NO_MEM});
    }
    return task;
}

core::Status Task::stop() noexcept
{
    if (handle_ == nullptr) {
        return {};
    }
    if (handle_ == xTaskGetCurrentTaskHandle()) {
        // Deleting the running task would skip this object's destructor, and
        // with it everything the object owns. Callers that need this should
        // use vTaskDelete(nullptr) and not own a Task object.
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    vTaskDelete(handle_);
    handle_ = nullptr;
    return {};
}

void Task::delay_ms(uint32_t ms) noexcept
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void Task::delay_ticks(TickType_t ticks) noexcept
{
    vTaskDelay(ticks);
}

void Task::yield() noexcept
{
    taskYIELD();
}

uint32_t Task::free_heap_bytes() noexcept
{
    return static_cast<uint32_t>(esp_get_free_heap_size());
}

uint32_t Task::stack_high_water_bytes() noexcept
{
    // StackType_t is uint8_t on ESP32, so the high-water mark is already bytes.
    return static_cast<uint32_t>(uxTaskGetStackHighWaterMark(nullptr));
}

TaskHandle_t Task::current_handle() noexcept
{
    return xTaskGetCurrentTaskHandle();
}

} // namespace rtos
