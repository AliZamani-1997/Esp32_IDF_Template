#pragma once

// RAII wrapper over the ESP-IDF GPIO driver (driver/gpio.h).

#include "core/error.hpp"

#include "driver/gpio.h"

namespace drivers {

/// Which way a pin faces.
enum class Direction { input, output };

/// Internal pull resistor setting.
enum class Pull { none, up, down, up_down };

/// Interrupt trigger for an input pin.
enum class Trigger { none, rising, falling, any_edge, low_level, high_level };

/// An owned GPIO pin.
///
/// Non-copyable, movable. The destructor resets the pin to the ESP-IDF default
/// state, so pins never leak between subsystems when an object goes away.
///
/// Move-assigning over a live Gpio releases the old pin first.
class Gpio {
public:
    Gpio() = default;
    ~Gpio();

    Gpio(const Gpio&) = delete;
    Gpio& operator=(const Gpio&) = delete;
    Gpio(Gpio&& other) noexcept;
    Gpio& operator=(Gpio&& other) noexcept;

    /// Claim `pin` with the given direction and pull setting.
    [[nodiscard]] static core::Result<Gpio> claim(gpio_num_t pin,
                                                  Direction dir,
                                                  Pull pull = Pull::none) noexcept;

    /// Claim `pin` as an output and drive it to `initial_level`.
    ///
    /// The level is written before the output driver is enabled, so the line
    /// never briefly shows the wrong state.
    [[nodiscard]] static core::Result<Gpio> output(gpio_num_t pin,
                                                   bool initial_level = false) noexcept;

    /// Claim `pin` as an input.
    [[nodiscard]] static core::Result<Gpio> input(gpio_num_t pin,
                                                  Pull pull = Pull::none) noexcept;

    [[nodiscard]] bool valid() const noexcept { return pin_ != GPIO_NUM_NC; }
    [[nodiscard]] gpio_num_t pin() const noexcept { return pin_; }

    /// Drive an output pin.
    core::Status set_level(bool high) noexcept;

    /// Toggle an output pin. Returns the new level.
    [[nodiscard]] core::Result<bool> toggle() noexcept;

    /// Read the current level of any pin.
    [[nodiscard]] core::Result<bool> level() const noexcept;

    /// Attach or detach an interrupt trigger.
    core::Status set_trigger(Trigger trigger) noexcept;

    /// Give up ownership without resetting the pin. Used by the move ctor.
    [[nodiscard]] gpio_num_t release() noexcept;

private:
    gpio_num_t pin_{GPIO_NUM_NC};
    Direction dir_{Direction::input};
};

} // namespace drivers
