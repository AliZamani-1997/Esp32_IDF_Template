#pragma once

// rtos — Layer 2: the FreeRTOS abstraction.
//
// Tasks, mutexes and queues. Depends on core only. Nothing here knows about a
// specific peripheral, so drivers may use these primitives freely.
//
// The component is called `rtos` rather than `freertos` because ESP-IDF
// already owns the component name `freertos`; this is the C++ layer over it.
//
// Prefer including only the module you need; this umbrella exists for
// convenience in small files.

#include "rtos/mutex.hpp"
#include "rtos/queue.hpp"
#include "rtos/task.hpp"
