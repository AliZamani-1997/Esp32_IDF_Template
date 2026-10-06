#pragma once

// Typed FIFO over the FreeRTOS queue primitives (freertos/queue.h).
//
// xQueueCreate stores items BY VALUE into its own buffer, so T must be
// copy-constructible (or trivially copyable). Keep payloads small — a few
// dozen bytes. For anything larger, queue a handle, a pointer to static
// storage, or a std::array.

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/detail/ticks.hpp"
#include "core/error.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace rtos {

/// A fixed-capacity FIFO of T.
///
/// Create with init() during start-up, then use send()/receive(). The object is
/// non-copyable and non-movable; own it as a member or a static.
template <class T>
class Queue {
    static_assert(std::is_copy_constructible_v<T> || std::is_trivially_copyable_v<T>,
                  "Queue<T> requires T to be copy-constructible or trivially copyable");

public:
    Queue() = default;

    ~Queue()
    {
        if (handle_ != nullptr) {
            vQueueDelete(handle_);
            handle_ = nullptr;
        }
    }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    /// Allocate the queue. Call once.
    [[nodiscard]] core::Status init(size_t capacity) noexcept
    {
        if (handle_ != nullptr) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }
        if (capacity == 0) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
        }
        handle_ = xQueueCreate(capacity, sizeof(T));
        if (handle_ == nullptr) {
            return std::unexpected(core::Error{ESP_ERR_NO_MEM});
        }
        return {};
    }

    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }

    /// Append an item. Fails with ESP_ERR_TIMEOUT if the queue stays full.
    [[nodiscard]] core::Status send(const T& item, uint32_t timeout_ms = core::kWaitForever) noexcept
    {
        if (handle_ == nullptr) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }
        return core::to_status(xQueueSend(handle_, &item, core::detail::to_ticks(timeout_ms)) == pdTRUE
                               ? ESP_OK
                               : ESP_ERR_TIMEOUT);
    }

    /// Append an item from an ISR. Never blocks.
    [[nodiscard]] core::Status send_from_isr(const T& item, BaseType_t* higher_priority_task_woken = nullptr) noexcept
    {
        if (handle_ == nullptr) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }
        BaseType_t woken = pdFALSE;
        BaseType_t* const out = (higher_priority_task_woken != nullptr) ? higher_priority_task_woken : &woken;
        const BaseType_t queued = xQueueSendFromISR(handle_, &item, out);
        if (higher_priority_task_woken == nullptr && woken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
        return core::to_status(queued == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT);
    }

    /// Remove the oldest item. Fails with ESP_ERR_TIMEOUT if none arrives.
    [[nodiscard]] core::Result<T> receive(uint32_t timeout_ms = core::kWaitForever) noexcept
    {
        if (handle_ == nullptr) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }
        T item{};
        if (xQueueReceive(handle_, &item, core::detail::to_ticks(timeout_ms)) != pdTRUE) {
            return std::unexpected(core::Error{ESP_ERR_TIMEOUT});
        }
        return item;
    }

    /// Remove the oldest item from an ISR. Never blocks.
    [[nodiscard]] core::Result<T> receive_from_isr() noexcept
    {
        if (handle_ == nullptr) {
            return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
        }
        T item{};
        BaseType_t woken = pdFALSE;
        const BaseType_t got = xQueueReceiveFromISR(handle_, &item, &woken);
        if (woken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
        if (got != pdTRUE) {
            return std::unexpected(core::Error{ESP_ERR_TIMEOUT});
        }
        return item;
    }

    /// Number of items currently waiting.
    [[nodiscard]] size_t size() const noexcept
    {
        return (handle_ == nullptr) ? 0 : static_cast<size_t>(uxQueueMessagesWaiting(handle_));
    }

    /// Free slots remaining.
    [[nodiscard]] size_t spaces() const noexcept
    {
        return (handle_ == nullptr) ? 0 : static_cast<size_t>(uxQueueSpacesAvailable(handle_));
    }

    /// Drop everything.
    void reset() noexcept
    {
        if (handle_ != nullptr) {
            (void)xQueueReset(handle_);
        }
    }

    [[nodiscard]] QueueHandle_t handle() const noexcept { return handle_; }

private:
    QueueHandle_t handle_{nullptr};
};

} // namespace rtos
