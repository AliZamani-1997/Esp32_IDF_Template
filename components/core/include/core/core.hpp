#pragma once

// core — Layer 1: the foundation.
//
// Error handling, logging, and time. Nothing here touches a peripheral, and
// nothing here depends on another component of this project: core is the
// bottom of the stack, so every other layer may include it.
//
// Prefer including only the module you need; this umbrella exists for
// convenience in small files.

#include "core/error.hpp"
#include "core/log.hpp"
#include "core/timer.hpp"
