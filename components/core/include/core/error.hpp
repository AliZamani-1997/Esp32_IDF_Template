#pragma once

// Error / Result / Status — the error channel for the whole project.
//
// WHY NOT EXCEPTIONS
// ------------------
// sdkconfig leaves CONFIG_COMPILER_CXX_EXCEPTIONS unset (the ESP-IDF default),
// so throw / try / catch do not exist. Every fallible operation returns a
// value instead. std::expected gives that a familiar, readable shape and
// costs nothing on the happy path.
//
// The rule for the whole codebase: a function that can fail returns
// core::Result<T> or core::Status. It never returns a bare bool, never sets
// an out-parameter to signal failure, and never aborts.

#include <cstdint>
#include <expected>
#include <string_view>
#include <type_traits>
#include <utility>

#include "esp_err.h"

namespace core {

/// A thin wrapper around `esp_err_t`.
///
/// The numeric code is preserved exactly, so it can be handed straight back to
/// any ESP-IDF C API, while C++ code gets a real type to pass around and
/// compare.
class Error {
public:
    constexpr Error() noexcept = default;
    constexpr explicit Error(esp_err_t code) noexcept : code_(code) {}

    [[nodiscard]] constexpr esp_err_t code() const noexcept { return code_; }
    [[nodiscard]] constexpr bool ok() const noexcept { return code_ == ESP_OK; }

    /// Human-readable name, e.g. "ESP_ERR_TIMEOUT". Never null.
    [[nodiscard]] const char* name() const noexcept;

    /// Human-readable description. Never empty.
    [[nodiscard]] std::string_view message() const noexcept;

    friend constexpr bool operator==(const Error& a, const Error& b) noexcept
    {
        return a.code_ == b.code_;
    }
    friend constexpr bool operator!=(const Error& a, const Error& b) noexcept
    {
        return !(a == b);
    }

private:
    esp_err_t code_{ESP_OK};
};

/// A value, or the reason there is none.
template <class T>
using Result = std::expected<T, Error>;

/// Success, or the reason it is not.
using Status = std::expected<void, Error>;

// ---------------------------------------------------------------------------
// Bridges from the C world.
// ---------------------------------------------------------------------------

/// Turn a bare `esp_err_t` into a Status.
[[nodiscard]] inline Status to_status(esp_err_t rc) noexcept
{
    if (rc == ESP_OK) {
        return {};
    }
    return std::unexpected(Error{rc});
}

/// Turn a bare `esp_err_t` plus a value into a Result.
template <class T>
[[nodiscard]] inline Result<std::decay_t<T>> to_result(esp_err_t rc, T&& value) noexcept
{
    if (rc == ESP_OK) {
        return std::forward<T>(value);
    }
    return std::unexpected(Error{rc});
}

} // namespace core

// ---------------------------------------------------------------------------
// Error propagation.
//
// Both helpers are single statements, so they are safe to use inside an
// if/else or a loop body without wrapping braces.
// ---------------------------------------------------------------------------

/// Run a Status-returning expression; return early on failure.
#define CORE_TRY_VOID(expr)                                                    \
    do {                                                                       \
        ::core::Status _core_status = (expr);                                  \
        if (!_core_status) {                                                   \
            return std::unexpected(std::move(_core_status).error());           \
        }                                                                      \
    } while (0)

/// Evaluate a core::Result<T>-returning expression into `lhs`; return early on failure.
///
/// `lhs` must be an already-declared, non-const lvalue — the macro *assigns*
/// to it, it does not declare it. Write
///     bool current = false;
///     CORE_TRY_ASSIGN(current, level());
/// NOT
///     CORE_TRY_ASSIGN(const bool current, level());
/// The latter expands to `(const bool current) = ...` and fails to compile,
/// and even a non-const declaration would be scoped to the macro's do-block
/// and invisible afterwards.
#define CORE_TRY_ASSIGN(lhs, expr)                                             \
    do {                                                                       \
        auto _core_result = (expr);                                            \
        if (!_core_result) {                                                   \
            return std::unexpected(std::move(_core_result).error());           \
        }                                                                      \
        (lhs) = std::move(*_core_result);                                      \
    } while (0)
