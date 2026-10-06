#pragma once

// Internal: millisecond <-> FreeRTOS tick conversion.
//
// Lives in detail/ because it is plumbing shared by the wrappers, not part of
// the public surface. It is still installed as a public header so the inline
// functions can be used from other headers.

#include <cstdint>

#include "freertos/FreeRTOS.h"

namespace core {

/// Pass as a timeout to mean "wait indefinitely".
inline constexpr uint32_t kWaitForever = UINT32_MAX;

namespace detail {

/// Convert a millisecond timeout to ticks, honouring kWaitForever.
[[nodiscard]] inline constexpr TickType_t to_ticks(uint32_t timeout_ms) noexcept
{
    return (timeout_ms == kWaitForever) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
}

} // namespace detail
} // namespace core
