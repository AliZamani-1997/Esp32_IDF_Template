---
name: coding-standards
description: "Load this BEFORE writing, editing or reviewing ANY code in this ESP-IDF C++ project — C++, CMake, or tooling. It is the enforceable summary of docs/CODING_STANDARDS.md: naming rules, the four-layer dependency direction, the error-as-value model, formatting, and the banned-construct list. Trigger words: ESP32, ESP-IDF, C++, naming, style, convention, coding standard, review, refactor, new component, new wrapper, namespace, layer."
agent_created: true
---

# Coding standards (enforceable summary)

The full rationale lives in **`docs/CODING_STANDARDS.md`**. This file is the
operational version: read it, apply it, and do not deviate without first editing
the doc and logging the decision in `docs/ROADMAP.md`.

> If you only remember one thing: **application code never calls an ESP-IDF C API
> directly.** Everything goes through the RAII wrapper layers.

---

## 1. The ten golden rules

1. No raw ESP-IDF / FreeRTOS C API calls in application code — use the wrappers.
2. Every fallible function returns `core::Result<T>` or `core::Status`. Never a
   bare `bool`, never an error out-parameter, never `abort()`.
3. No exceptions. `throw` / `try` / `catch` do not exist here.
4. No RTTI. `dynamic_cast` / `typeid` do not exist here.
5. Ownership is RAII. Whatever you acquire, the destructor releases.
6. Dependencies point **downward only**. A lower layer never includes a higher one.
7. Cross-layer types are **fully qualified**: `core::Result<T>`, not `Result<T>`.
8. Every magic number gets a name.
9. Every unit goes in the name: `timeout_ms`, `period_us`, `stack_bytes`.
10. Comments explain **why**, not **what**.

---

## 2. Layers and dependency direction

```
L4  app      main/                 depends on: core, rtos, drivers
L3  drivers  components/drivers/   Gpio, Uart, I2cMasterBus/I2cDevice,
                                   SpiBus/SpiDevice, NvsHandle
                                   depends on: core   (+ rtos when a driver locks)
L2  rtos     components/rtos/      Task, Mutex, LockGuard, Queue<T>
                                   depends on: core
L1  core     components/core/      Error/Result/Status, CORE_LOG*, Timer, ticks
                                   depends on: nothing internal
    ---------------------------------------------------------------------------
    ESP-IDF / FreeRTOS — C code that only the wrappers see
```

- A component may `REQUIRES` only components **below** it.
- `core` must never include `rtos` or `drivers`. If it seems necessary, the thing
  belongs in `core` — move it.
- Each layer has an umbrella header: `core/core.hpp`, `rtos/rtos.hpp`,
  `drivers/drivers.hpp`.
- `main/` is the app layer. It keeps the name `main` because ESP-IDF requires the
  component defining `app_main` to be called `main`.

### Adding a layer

Same pattern: new folder under `components/`, matching namespace, umbrella header,
`REQUIRES` pointing only downward. Update `docs/ARCHITECTURE.md` and this skill.

---

## 3. Naming

| Thing | Rule | Example |
|---|---|---|
| Namespace | = component name, `snake_case` | `core`, `rtos`, `drivers` |
| Internal helpers | `<comp>::detail` | `core::detail::to_ticks` |
| Class / struct / enum | `PascalCase` | `Gpio`, `I2cMasterBus`, `LockGuard` |
| Function / method | `snake_case` | `set_level`, `read_register` |
| Data member | `snake_case` + trailing `_` | `handle_`, `pin_` |
| Constant | `k` prefix + `PascalCase` | `kWaitForever`, `kHeartbeatMs` |
| Enum value | `snake_case` | `Direction::input`, `Pull::up_down` |
| Macro | `SCREAMING_SNAKE`, component prefix | `CORE_TRY_VOID`, `CORE_LOGI` |
| File | `snake_case.hpp` / `.cpp` | `i2c.hpp`, `error.cpp` |
| Log tag | `constexpr const char* kTag` per file | `kTag = "i2c"` |

### Acronyms are ordinary words

`I2c`, `Spi`, `Uart`, `Nvs`, `Gpio` — **never** `I2C`, `SPI`, `UART`, `NVS`, `GPIO`.
(All-caps is reserved for constants and macros.)

### Descriptive names

- **Units always in the name.** `timeout_ms`, not `timeout`. This is firmware;
  an unqualified duration is a bug waiting to happen.
- **No gratuitous `get_`/`set_`.** A pure reader is named after the thing
  (`valid()`, `pin()`, `level()`); `set_` only when it actually writes.
- **Booleans read as questions:** `valid()`, `running()`.
- **No negative names.** `enabled`, not `not_disabled`.
- **No mutable globals.** If truly unavoidable (it should not be): `g_` prefix.
- **File-scope helpers** go in an anonymous `namespace { ... }`, without a
  redundant `static`.

---

## 4. Formatting

- 4 spaces, never tabs. Soft line limit **100 columns**.
- Braces: **Allman for functions**, **K&R for control flow**.
  ```cpp
  core::Status Uart::write(const void* data, size_t len) noexcept
  {
      if (data == nullptr || len == 0) {
          return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
      }
  }
  ```
- Always brace, even single-statement bodies.
- **Wrapped lines align to the last unmatched open paren.**
  ```cpp
  [[nodiscard]] static core::Result<Gpio> claim(gpio_num_t pin,
                                                Direction dir,
                                                Pull pull = Pull::none) noexcept;
  ```
- Long `if` conditions are the exception: continuation indented +4.
- **Macro continuation backslashes align at column 80.**
- Pointer/ref binds to the type: `const char* name`, `Config& cfg`.
- West `const`: `const char* kTag`.
- No trailing whitespace; file ends with a newline; no indentation inside `namespace`.
- Include order: own header → project headers → ESP-IDF → std, each group
  alphabetised and separated by a blank line.

---

## 5. Headers and components

- `#pragma once` everywhere.
- Public header in `include/<comp>/`, internal in `include/<comp>/detail/`.
- **A header must be self-sufficient** — including it alone must compile.
- A header includes only what it needs; an include is a dependency.
- Public signatures must not expose ESP-IDF handle types. Those stay `private:`
  or leave via `release()`.
- One type, one responsibility. A class that both creates a bus and owns a device
  must be split (`I2cMasterBus` / `I2cDevice`).

```cmake
idf_component_register(
    SRCS "src/<module>.cpp"
    INCLUDE_DIRS "include"
    REQUIRES <lower layers>
)
```

Forgetting `REQUIRES` → `fatal error: xxx.h: No such file or directory`.
Forgetting a file in `SRCS` → `undefined reference`.

---

## 6. Errors — value, not control flow

```cpp
[[nodiscard]] core::Result<Uart> Uart::open(uart_port_t port, const Config& cfg) noexcept
{
    if (const esp_err_t rc = uart_driver_install(port, cfg.rx_buffer_size,
                                                 cfg.tx_buffer_size, 0, nullptr, 0);
        rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return uart;
}
```

Propagating:

```cpp
core::Status blink(gpio_num_t pin)
{
    drivers::Gpio led;                                        // declare FIRST
    CORE_TRY_ASSIGN(led, drivers::Gpio::output(pin, false));  // then assign
    CORE_TRY_VOID(led.set_level(true));
    rtos::Task::delay_ms(100);
    CORE_TRY_VOID(led.set_level(false));
    return {};
}
```

- `CORE_TRY_ASSIGN(lhs, expr)` **assigns**; it does not declare. `lhs` must already
  exist and be non-`const`. `CORE_TRY_ASSIGN(const bool x, ...)` fails to compile;
  `CORE_TRY_ASSIGN(Type x, ...)` scopes `x` to the macro's `do`-block and hides it.
- **Never `.value()` on an unchecked result.** With exceptions off it calls
  `std::terminate`.
- Report at the boundary (`app_main`, top-level tasks) with `Error::name()` and
  `Error::message()`.

---

## 7. Logging

`CORE_LOGE/W/I/D/V` with the file's `kTag`. Never `printf`, never bare `ESP_LOGx`,
never `std::cout`. Never log in a hot loop or in an ISR.

```cpp
namespace { constexpr const char* kTag = "i2c"; }

CORE_LOGI(kTag, "bus %d up at %u Hz", static_cast<int>(port), cfg.scl_speed_hz);
```

Message must say **what** failed and **with what value**, not just "error".

---

## 8. Memory, ownership, RAII

Every wrapper: **non-copyable, movable, destructor releases everything,
move-assignment releases the current resource first, `noexcept` destructor,
`release()` hands back the raw handle.**

Construction of a resource-owning object is a **static factory returning
`Result<T>`** — not a constructor. Constructors cannot report failure here.

```cpp
class Thing {
public:
    Thing() = default;
    ~Thing();
    Thing(const Thing&) = delete;
    Thing& operator=(const Thing&) = delete;
    Thing(Thing&& other) noexcept;
    Thing& operator=(Thing&& other) noexcept;
    [[nodiscard]] static core::Result<Thing> create(...) noexcept;
};
```

Banned: `new`, `delete`, `malloc`, `free`, and — in hot paths — `std::string`,
`std::vector`, `std::function`.

| Instead of | Use |
|---|---|
| `std::string` | `std::string_view` to read, `std::array<char, N>` to own |
| `std::vector` | `std::array<T, N>` or `std::span<T>` |
| `std::function` | plain function pointer + `void* arg` (as `Task::Entry` does) |

Lifetime traps: `I2cDevice`/`SpiDevice` must not outlive their bus.
`NvsHandle` does not commit on destruction. `Task::stop()` refuses when called
from the task itself.

---

## 9. Concurrency

- Lock with `rtos::Mutex` + `rtos::LockGuard`, never a raw semaphore.
- **Never block in an ISR** — only the `*_from_isr` variants.
- Prefer message passing (`rtos::Queue<T>`) over shared state.
- Keep critical sections short: no logging, no I/O inside a lock.
- Document the lock acquisition order if more than one lock is involved.
- Every task's priority, core, trigger and owned data is recorded in
  `docs/ARCHITECTURE.md` §6.

---

## 10. Comments

Explain **why**. Document units, ownership and lifetime on every public API with
`///`. No commented-out code — git keeps history. `TODO` must say what and why:
`// TODO(i2c): handle bus arbitration loss.` If a name is misleading, fix the
name instead of commenting about it.

---

## 11. Git

`<type>(<scope>): <summary>` — types `feat|fix|refactor|docs|build|chore|test`;
scopes `core|rtos|drivers|app|docs|tools|vscode`.

```
feat(drivers): add Gpio wrapper with RAII pin ownership
fix(core): guard ESP_TIMER_ISR behind its Kconfig option
refactor: split the peripheral wrapper component into separate layers
```

One logical change per commit. Never commit `build/`, `managed_components/`,
`sdkconfig`, `graphify-out/`, `eim_config.toml`, `tools/idf-env.local.cmd`.
Never `--no-verify`.

---

## 12. Verify before you claim done

```bash
tools/idf.cmd build          # must be green, no new warnings
```

Run it from the repository root (all paths in this skill are relative to it).
Then walk this checklist:

- [ ] build green, no new warnings
- [ ] no `throw`/`try`/`catch`/`dynamic_cast`/`typeid`
- [ ] no `printf` / bare `ESP_LOGx` / `std::cout`
- [ ] no `new` / `delete` / `malloc` in app code
- [ ] no direct ESP-IDF C API call in app code
- [ ] every fallible function returns `core::Result<T>` / `core::Status`
- [ ] no `.value()` on an unchecked result
- [ ] `CORE_TRY_ASSIGN` only on pre-declared, non-const lvalues
- [ ] includes never point upward (layering intact)
- [ ] every magic number named; every duration carries its unit
- [ ] public APIs documented; headers self-sufficient
- [ ] names match the table in §3
- [ ] `docs/ARCHITECTURE.md` updated if the architecture changed

---

## 13. Banned list — one glance

```
throw / try / catch                        no exceptions
dynamic_cast / typeid                      no RTTI
printf / ESP_LOGx / std::cout              use CORE_LOG*
new / delete / malloc / free               use RAII and fixed buffers
std::string / std::vector / std::function  not in hot paths
.value() on an unchecked result            reaches std::terminate
abort() / exit()                           return the error upward
bool for a fallible operation              return Result<T>
out-parameter for an error                 return Result<T>
raw C API call in app code                 write a wrapper
upward include                             breaks the layering
mutable global                             hidden coupling
magic number                               give it a name
timeout without a unit                     write timeout_ms
commented-out code                         git keeps history
```
