# core — Layer 1: the foundation.
#
# Error/Result/Status, logging, and time. Nothing here touches a peripheral,
# and nothing here depends on another component of this project: core is the
# bottom of the stack. Every other layer may include core.
#
# Conventions: .workbuddy-ai/skills/cpp-wrapper-layers

idf_component_register(
    SRCS
        "src/error.cpp"
        "src/timer.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_common
        log
        esp_timer
        freertos
)
