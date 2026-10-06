# drivers — Layer 3: peripheral access.
#
# One RAII wrapper per ESP-IDF peripheral driver. Depends on core, and on
# rtos the moment a driver needs locking. Never depends on the application.
#
# Conventions: .workbuddy-ai/skills/cpp-wrapper-layers

idf_component_register(
    SRCS
        "src/gpio.cpp"
        "src/i2c.cpp"
        "src/nvs.cpp"
        "src/spi.cpp"
        "src/uart.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        core
        esp_driver_gpio
        esp_driver_uart
        esp_driver_i2c
        esp_driver_spi
        nvs_flash
)
