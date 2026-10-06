#pragma once

// drivers — Layer 3: peripheral access.
//
// One RAII wrapper per ESP-IDF peripheral driver. Depends on core, and on
// rtos the moment a driver needs locking. Never depends on the application.
//
// Prefer including only the peripheral you need; this umbrella exists for
// convenience in small files.

#include "drivers/gpio.hpp"
#include "drivers/i2c.hpp"
#include "drivers/nvs.hpp"
#include "drivers/spi.hpp"
#include "drivers/uart.hpp"
