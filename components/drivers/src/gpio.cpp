#include "drivers/gpio.hpp"

namespace drivers {
namespace {

constexpr gpio_pullup_t to_pullup(Pull p) noexcept
{
    switch (p) {
    case Pull::up:
    case Pull::up_down:
        return GPIO_PULLUP_ENABLE;
    case Pull::none:
    case Pull::down:
        break;
    }
    return GPIO_PULLUP_DISABLE;
}

constexpr gpio_pulldown_t to_pulldown(Pull p) noexcept
{
    switch (p) {
    case Pull::down:
    case Pull::up_down:
        return GPIO_PULLDOWN_ENABLE;
    case Pull::none:
    case Pull::up:
        break;
    }
    return GPIO_PULLDOWN_DISABLE;
}

constexpr gpio_int_type_t to_int_type(Trigger t) noexcept
{
    switch (t) {
    case Trigger::rising:
        return GPIO_INTR_POSEDGE;
    case Trigger::falling:
        return GPIO_INTR_NEGEDGE;
    case Trigger::any_edge:
        return GPIO_INTR_ANYEDGE;
    case Trigger::low_level:
        return GPIO_INTR_LOW_LEVEL;
    case Trigger::high_level:
        return GPIO_INTR_HIGH_LEVEL;
    case Trigger::none:
        break;
    }
    return GPIO_INTR_DISABLE;
}

constexpr gpio_mode_t to_mode(Direction d) noexcept
{
    return (d == Direction::output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
}

} // namespace

Gpio::~Gpio()
{
    (void)release();
}

Gpio::Gpio(Gpio&& other) noexcept
    : pin_(other.release())
    , dir_(other.dir_)
{
}

Gpio& Gpio::operator=(Gpio&& other) noexcept
{
    if (this != &other) {
        (void)release();
        pin_ = other.release();
        dir_ = other.dir_;
    }
    return *this;
}

gpio_num_t Gpio::release() noexcept
{
    const gpio_num_t pin = pin_;
    if (pin != GPIO_NUM_NC) {
        (void)gpio_reset_pin(pin);
        pin_ = GPIO_NUM_NC;
    }
    return pin;
}

core::Result<Gpio> Gpio::claim(gpio_num_t pin, Direction dir, Pull pull) noexcept
{
    if (pin == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << static_cast<unsigned>(pin);
    cfg.mode = to_mode(dir);
    cfg.pull_up_en = to_pullup(pull);
    cfg.pull_down_en = to_pulldown(pull);
    cfg.intr_type = GPIO_INTR_DISABLE;

    if (const esp_err_t rc = gpio_config(&cfg); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }

    Gpio g;
    g.pin_ = pin;
    g.dir_ = dir;
    return g;
}

core::Result<Gpio> Gpio::output(gpio_num_t pin, bool initial_level) noexcept
{
    if (pin == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    // Pre-load the output register before the driver is enabled so the pin
    // comes up in the intended state rather than glitching.
    if (const esp_err_t rc = gpio_set_level(pin, initial_level ? 1u : 0u); rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }

    auto g = claim(pin, Direction::output);
    if (!g) {
        return std::unexpected(g.error());
    }

    // gpio_config() may clear the output register; write it again to be sure.
    CORE_TRY_VOID(g->set_level(initial_level));
    return std::move(*g);
}

core::Result<Gpio> Gpio::input(gpio_num_t pin, Pull pull) noexcept
{
    return claim(pin, Direction::input, pull);
}

core::Status Gpio::set_level(bool high) noexcept
{
    if (pin_ == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(gpio_set_level(pin_, high ? 1u : 0u));
}

core::Result<bool> Gpio::toggle() noexcept
{
    // CORE_TRY_ASSIGN assigns into an existing lvalue, so the variable must be
    // declared first (a declaration inside the macro would be scoped to the
    // macro's internal do-block and invisible afterwards).
    bool current = false;
    CORE_TRY_ASSIGN(current, level());
    CORE_TRY_VOID(set_level(!current));
    return !current;
}

core::Result<bool> Gpio::level() const noexcept
{
    if (pin_ == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return gpio_get_level(pin_) != 0;
}

core::Status Gpio::set_trigger(Trigger trigger) noexcept
{
    if (pin_ == GPIO_NUM_NC) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_STATE});
    }
    return core::to_status(gpio_set_intr_type(pin_, to_int_type(trigger)));
}

} // namespace drivers
