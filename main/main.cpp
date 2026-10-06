// Layer 4: the application.
//
// This is a skeleton that exercises the wrapper layers so the build proves the
// whole chain works. Replace the body with the real application (see
// docs/ARCHITECTURE.md).

#include "core/log.hpp"
#include "rtos/task.hpp"

#include "esp_system.h"

namespace {

constexpr const char* kTag = "app";

// Period of the placeholder heartbeat.
constexpr unsigned kHeartbeatMs = 1000;

} // namespace

// app_main must keep C linkage: ESP-IDF's startup code looks it up by name.
extern "C" void app_main(void)
{
    CORE_LOGI(kTag, "app_main started (ESP-IDF %s)", esp_get_idf_version());

    // Placeholder loop. Replaced by the real application later.
    while (true) {
        CORE_LOGI(kTag, "alive");
        rtos::Task::delay_ms(kHeartbeatMs);
    }
}
