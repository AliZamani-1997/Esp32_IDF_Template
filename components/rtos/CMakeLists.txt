# rtos — Layer 2: the FreeRTOS abstraction.
#
# Task, Mutex + LockGuard, Queue<T>. Depends on core only: no peripheral
# knowledge lives here, so drivers may use these primitives freely.
#
# Named `rtos`, not `freertos`: ESP-IDF already owns the component name
# `freertos`. This component is the C++ layer *over* it.
#
# Conventions: .workbuddy-ai/skills/cpp-wrapper-layers

idf_component_register(
    SRCS
        "src/mutex.cpp"
        "src/task.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        core
        freertos
)
