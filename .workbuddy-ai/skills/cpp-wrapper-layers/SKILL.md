---
name: cpp-wrapper-layers
description: "Load this for ANY work on the C++ wrapper layers of this ESP-IDF project: adding or modifying a wrapper, adding a component, moving code between layers, or answering questions about how core/rtos/drivers fit together. Covers the four-layer architecture, the RAII wrapper contract, the exact step-by-step procedure for adding a new wrapper, the build workflow, and every pitfall hit so far. Trigger words: ESP32, ESP-IDF, core, rtos, drivers, wrapper, RAII, driver, component, idf.py, build. For naming, formatting and style rules, load the `coding-standards` skill instead."
agent_created: true
---

# The C++ wrapper layers

This is an ESP-IDF firmware template written in **C++**. Application code never
calls an ESP-IDF C API directly: everything goes through the RAII wrapper layers
under `components/`.

**Naming, formatting and style are owned by the `coding-standards` skill.**
Load that too when you are writing new code. This skill covers *structure*:
which layer a thing belongs in, how a wrapper is shaped, and how to add one.

---

## 1. The four layers

```
L4  app      main/                 depends on: core, rtos, drivers
L3  drivers  components/drivers/   depends on: core (+ rtos when a driver locks)
L2  rtos     components/rtos/      depends on: core
L1  core     components/core/      depends on: nothing internal
    ---------------------------------------------------------------------------
    ESP-IDF / FreeRTOS
```

| Layer | Component | Contents |
|---|---|---|
| L1 | `core` | `Error`/`Result`/`Status` + `CORE_TRY_*`, `CORE_LOG*`, `Timer`, `detail/ticks.hpp` |
| L2 | `rtos` | `Task`, `Mutex` + `LockGuard`, `Queue<T>` |
| L3 | `drivers` | `Gpio`, `Uart`, `I2cMasterBus`/`I2cDevice`, `SpiBus`/`SpiDevice`, `NvsHandle` |
| L4 | `main` | `app_main` and the application |

Rules that are not negotiable:

- A component `REQUIRES` only components **below** it.
- `core` never includes `rtos` or `drivers`. If it seems to need to, the thing
  belongs in `core` — move it there.
- Cross-layer types are **fully qualified** (`core::Result<T>`), so the dependency
  direction is visible at every call site.
- `main/` keeps the name `main`: ESP-IDF requires the component defining
  `app_main` to be called `main`. It is the app layer despite the folder name.
- Each layer has an umbrella header: `core/core.hpp`, `rtos/rtos.hpp`,
  `drivers/drivers.hpp`.

### The component name is not `freertos`

The FreeRTOS layer is called **`rtos`** because ESP-IDF already owns the component
name `freertos`. `REQUIRES freertos` pulls in ESP-IDF's C component; `REQUIRES rtos`
pulls in ours. Both appear in `components/rtos/CMakeLists.txt`.

---

## 2. Hard constraints (verified, not assumed)

| Fact | Value | Consequence |
|---|---|---|
| C++ standard | `gnu++26` | `<expected>`, `<span>`, `<concepts>` are available |
| `CONFIG_COMPILER_CXX_EXCEPTIONS` | **not set** | no `throw`, no `try`/`catch` |
| `CONFIG_COMPILER_CXX_RTTI` | **not set** | no `dynamic_cast`, no `typeid` |
| Chip | chosen per project with `set-target` | dual-core parts only: `Task::Config::core` is 0, 1, or `tskNO_AFFINITY` |

Re-verify before relying on them:

```bash
grep -E "CONFIG_COMPILER_CXX" sdkconfig
```

### Consequences to design around

- **Errors are values.** A fallible function returns `core::Result<T>` or
  `core::Status`. Never a bare `bool`, never an out-parameter, never `abort()`.
- **No `std::function`, no `std::string`, no `std::vector` in hot paths.** They
  heap-allocate, and with exceptions off their error paths call `std::terminate`.
  Use `std::string_view`, `std::array`, `std::span`, fixed buffers.
- **`std::expected::value()` throws on error** — which aborts here. Use
  `operator*` after a check, `value_or()`, or `CORE_TRY_*`. Never `.value()` on a
  result you have not already verified.

---

## 3. File layout

```
components/core/
├── CMakeLists.txt
├── include/core/
│   ├── core.hpp            umbrella
│   ├── error.hpp           Error, Result<T>, Status, CORE_TRY_*
│   ├── log.hpp             CORE_LOGx, log_hex, log_hexdump, set_log_level
│   ├── timer.hpp           Timer (esp_timer)
│   └── detail/ticks.hpp    kWaitForever, detail::to_ticks()
└── src/{error,timer}.cpp

components/rtos/
├── CMakeLists.txt
├── include/rtos/
│   ├── rtos.hpp            umbrella
│   ├── task.hpp            Task
│   ├── mutex.hpp           Mutex, LockGuard
│   └── queue.hpp           Queue<T>   (header-only template)
└── src/{task,mutex}.cpp

components/drivers/
├── CMakeLists.txt
├── include/drivers/
│   ├── drivers.hpp         umbrella
│   ├── gpio.hpp            Gpio
│   ├── uart.hpp            Uart
│   ├── i2c.hpp             I2cMasterBus, I2cDevice
│   ├── spi.hpp             SpiBus, SpiDevice
│   └── nvs.hpp             nvs_init(), NvsHandle
└── src/{gpio,uart,i2c,spi,nvs}.cpp

main/
├── CMakeLists.txt          REQUIRES core rtos drivers
└── main.cpp
```

One source file per non-template module. Header-only templates (like `Queue<T>`)
have no `.cpp`.

---

## 4. The RAII contract every wrapper follows

1. **Non-copyable, movable.** `= delete` the copy operations; implement the move
   constructor and move assignment; leave the move source inert.
2. **The destructor releases everything** the object acquired — pins, bus handles,
   driver installs, tasks, timers, NVS handles. A wrapper that leaks is a bug.
3. **Move-assignment releases the current resource first**, then steals. It must be
   safe to move-assign over a live object.
4. **The destructor is `noexcept`** and never logs meaningfully or blocks.
5. **`release()`** hands back the raw handle and makes the object inert, for the
   rare case where ownership transfers elsewhere.
6. **Construction of a resource-owning object is a static factory returning
   `core::Result<T>`**, not a constructor. Constructors cannot report failure here.

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
    [[nodiscard]] bool valid() const noexcept;
private:
    Handle handle_{nullptr};
};
```

### Lifetime rules that are easy to get wrong

- **`I2cDevice` / `SpiDevice` must not outlive their bus.** Declare the bus first,
  devices after it, in the same or a narrower scope. The device holds only a
  handle; the bus owns the hardware.
- **`Task::stop()` refuses when called from the task itself** (returns
  `ESP_ERR_INVALID_STATE`). Deleting the running task would skip its own destructor
  and leak what it owns. A self-terminating task calls `vTaskDelete(nullptr)` and
  must not own its `Task` object.
- **`NvsHandle` does not commit on destruction.** Writes are lost unless you call
  `commit()`. That is deliberate — it keeps durability explicit at the call site.

---

## 5. Adding a new wrapper — the procedure

**Step 1. Decide the layer before writing anything.**

| The thing wraps… | Layer |
|---|---|
| error handling, logging, time, a pure utility | `core` |
| a FreeRTOS primitive (task, mutex, queue, semaphore, event group, timer) | `rtos` |
| an ESP-IDF peripheral or system service driver (ADC, PWM, RMT, I2S, TWAI, SD, Wi-Fi, OTA…) | `drivers` |

If it is unclear, ask: *does it need a peripheral or an RTOS primitive?* No to
both → `core`. RTOS primitive → `rtos`. Peripheral → `drivers`.

**Step 2. Read the real ESP-IDF header first. Do not guess the API.**
ESP-IDF v6 renames and moves things (I2C and SPI especially). Check before writing:

```bash
ls /c/Espressif/v6.1/esp-idf/components/esp_driver_<peripheral>/include/driver/
grep -nE "^esp_err_t <prefix>_" <header>
awk '/^typedef struct \{/,/} <type>_t;/' <header>
```

**Step 3. Write `include/<comp>/<module>.hpp`** — public API only, in
`namespace <comp>`, following §4 and the `coding-standards` skill. Document units
(ms vs µs vs ticks vs bytes) and lifetime. Put shared plumbing in `detail/`.

**Step 4. Write `src/<module>.cpp`** — include only the matching header plus what
it needs. Keep the C API entirely inside the `.cpp`; no ESP-IDF handle type may
leak into the public signature.

**Step 5. Register the source** in `components/<comp>/CMakeLists.txt` under `SRCS`,
and add any new IDF component to `REQUIRES`. Forgetting `REQUIRES` gives
`fatal error: driver/foo.h: No such file or directory` at build time.

**Step 6. Add it to the component's umbrella header** (`<comp>/<comp>.hpp`).

**Step 7. Build and fix:**

```bash
tools/idf.cmd build
```

**Step 8. Update `docs/ARCHITECTURE.md` (the module map in §5) and this skill's §3
and §8.** Update the `coding-standards` skill only if a *rule* changed.

---

## 6. C++ over C — the conversions that do not exist

This is the single most frequent class of build error in this layer, because
ESP-IDF is a **C** codebase and C++ is stricter.

| What you wrote | Why it fails | What to write |
|---|---|---|
| `gpio_num_t p = UART_PIN_NO_CHANGE;` | the macro expands to `(-1)`, an `int`; C++ has **no** implicit int → enum conversion | `GPIO_NUM_NC` (same value −1, right enum type) |
| `if (handle_ == nullptr)` where `handle_` is `nvs_handle_t` | `nvs_handle_t` is `uint32_t`, **not** a pointer | `if (handle_ == 0)` |
| `if (handle_ != nullptr)` where the handle is a real pointer (`i2c_master_bus_handle_t`, `spi_device_handle_t`, `TaskHandle_t`, `esp_timer_handle_t`, `QueueHandle_t`, `SemaphoreHandle_t`) | — | **correct**; pointers do compare to `nullptr` |
| `-1` passed to a `gpio_num_t` parameter | same as row 1 | `GPIO_NUM_NC` |

The rule: **check the typedef before comparing a handle to `nullptr`.** Most
ESP-IDF handles are opaque pointers, but NVS handles are integers. When a macro is
involved, expand it in your head — if the result is a bare number, it needs the
enum constant instead.

### Conditionally-compiled API surface

Some enum values only exist when a Kconfig option is on. `ESP_TIMER_ISR` is declared
only under `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`. Guard such uses and
report `ESP_ERR_NOT_SUPPORTED` rather than silently downgrading:

```cpp
#include "sdkconfig.h"

#if !CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD
    if (dispatch == Dispatch::isr) {
        return std::unexpected(core::Error{ESP_ERR_NOT_SUPPORTED});
    }
#endif
```

Do not assume a header's enum is complete — read it, and read the `#if` around it.

---

## 7. Required workflows

### Building — always through the wrapper

`idf.py` may not be on PATH — that depends on how ESP-IDF was installed. Always
go through the wrapper, which activates the environment first and auto-detects
where ESP-IDF lives (see `SETUP_ESPIDF.md`). All commands run from the repo root:

```bash
tools/idf.cmd build          # or: set-target <chip> | flash | monitor | menuconfig | size
```

VS Code tasks (`.vscode/tasks.json`) route through the same wrapper. Never tell
the user to run a bare `idf.py`.

A full reconfigure after changing the component list takes ~4 minutes here
(CMake configure ≈180 s + generate ≈48 s), then the compile is quick. Redirect to
a log file rather than piping through `head`, or the pipeline can be cut short:

```bash
mkdir -p .scratch
cmd //c 'tools\idf.cmd build' > .scratch/build.log 2>&1; echo "EXIT=$?"
grep -nE "error:|warning:|Project build complete" .scratch/build.log
```

### graphify — mandatory in this project

Always use graphify to orient before changing code, and to answer architecture
questions. If `graphify-out/graph.json` exists, treat a codebase question as a
graphify query rather than re-reading files:

```bash
graphify query "<question>"        # existing graph
graphify . --update                # after adding or moving files
```

Load the `graphify` skill for the full pipeline. Do not answer "how does X connect
to Y" by guessing — ask the graph. `graphify-out/` is gitignored.

Two practical notes for this machine:

- Call `extract(..., parallel=False)`. The parallel path spawns workers that fail
  on Windows when the driver script comes from stdin — you get a
  `BrokenProcessPool` warning plus noisy `OSError: Invalid argument: '<stdin>'`
  tracebacks before it falls back to sequential anyway. Passing `parallel=False`
  skips the failure and the noise.
- After a restructure, do a **full** rebuild rather than `--update`. The corpus
  size here (≈40 files, ≈21k words) makes it cheap: detect + AST is seconds, and
  the docs are a single semantic subagent chunk. Verify the node count grew before
  accepting the result — `to_json()` refuses to shrink `graph.json` (#479).

---

## 8. Pitfalls already hit in this project

| Symptom | Cause | Fix |
|---|---|---|
| `expected string or bytes-like object, got 'NoneType'` | `ESP_IDF_VERSION` unset | use `tools/idf.cmd`, never bare `idf.py` |
| `MSys/Mingw is no longer supported` | `MSYSTEM` inherited from Git Bash | `tools/idf-env.cmd` clears it |
| `running scripts is disabled` | execution policy `Restricted` | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` |
| `xtensa-esp32-elf-gcc not found in the PATH` | environment not activated | use `tools/idf.cmd` |
| `fatal error: driver/xxx.h: No such file` | missing `REQUIRES` | add the IDF component in that layer's `CMakeLists.txt` |
| `undefined reference` to a wrapper method | source not listed in `SRCS` | add it |
| `error: exception handling disabled` | used `throw`/`try` | return `core::Result<T>` / `core::Status` |
| `error: cannot use typeid with -fno-rtti` | used RTTI | use a tag/enum, not `dynamic_cast` |
| `error: 'ESP_TIMER_ISR' was not declared in this scope; did you mean 'ESP_TIMER_TASK'?` | the enum value is behind `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD` | `#include "sdkconfig.h"` and `#if`-guard it; return `ESP_ERR_NOT_SUPPORTED` when off |
| `error: invalid conversion from 'int' to 'gpio_num_t' [-fpermissive]` | `UART_PIN_NO_CHANGE` / `-1` is an `int`, not the enum | use `GPIO_NUM_NC` |
| `error: invalid operands of types 'nvs_handle_t' {aka 'long unsigned int'} and 'std::nullptr_t' to binary 'operator=='` | `nvs_handle_t` is a `uint32_t` | compare with `0`, not `nullptr` |
| `error: expected primary-expression before 'const'` at a `CORE_TRY_ASSIGN` line | the macro assigns to an existing lvalue; you passed a declaration (or a `const`) | declare the variable first, then `CORE_TRY_ASSIGN(var, expr)` |
| build silently does nothing after adding a file | stale CMake cache | `tools/idf.cmd reconfigure` |

### Restructuring pitfall

Moving modules between components breaks two things that a naive text substitution
gets wrong:

- **Cross-component includes.** `drivers/uart.hpp` includes `core/detail/ticks.hpp`,
  not `drivers/detail/ticks.hpp`. Map each module to its owning component; never
  blanket-replace the prefix.
- **Unqualified type names.** Inside `namespace drivers`, `Result<T>` no longer
  resolves — it is `core::Result<T>`. Add the qualification with a negative
  lookbehind for `::` (`(?<!::)\bResult\b`) so you do not produce `core::core::Result`.
  Also mind digits in filenames: a `[a-z/]+` include regex silently misses `i2c.hpp`.
- After a split, re-align wrapped continuation lines: adding `core::` shifts the
  opening paren, and every continuation line indented to the old column is now off.

---

## 9. Scope of the layers today

Building green — `tools/idf.cmd build` links `build/<project>.elf`, zero
warnings:

- **core** — `Error`/`Result`/`Status` + `CORE_TRY_*`, `CORE_LOG*`, `Timer`, ticks
- **rtos** — `Task`, `Mutex` + `LockGuard`, `Queue<T>`
- **drivers** — `Gpio`, `Uart`, `I2cMasterBus`/`I2cDevice`, `SpiBus`/`SpiDevice`,
  `NvsHandle`, `nvs_init()`

Nothing has run on hardware yet — the build is the only verification so far.
Treat runtime behaviour as unproven until it is flashed.

Not wrapped yet — add them the same way, into the layer §5 Step 1 selects: ADC,
DAC, PWM (LEDC/MCPWM), RMT, I2S, TWAI/CAN, SD/MMC, Wi-Fi, BLE, HTTP client/server,
OTA, deep sleep, watchdog, semaphores, event groups, stream/message buffers,
GPTimer.
