# اصول کدنویسی

> آخرین به‌روزرسانی: ۱۴۰۵/۰۷/۱۴ (2026-10-06)
>
> این سند **قانون** است، نه پیشنهاد. هر خط کدی که در این مخزن نوشته می‌شود باید
> با آن سازگار باشد. اگر جایی مجبور شدی از این قواعد عبور کنی، اول این سند را
> اصلاح کن و دلیلش را در `docs/ROADMAP.md` (جدول تصمیم‌ها) ثبت کن.
>
> نسخه‌ی عملیاتی و خلاصه‌ی این سند برای ایجنت، در اسکیل پروژه است:
> `.workbuddy-ai/skills/coding-standards/SKILL.md`

---

## ۰) ده قاعده‌ی طلایی

اگر فقط همین ده مورد را به خاطر بسپاری، ۹۰٪ راه را رفته‌ای:

1. **کد برنامه هرگز مستقیم API سی فریم‌ورک را صدا نمی‌زند.** همه چیز از رَپِرها می‌گذرد.
2. **هر تابعی که می‌تواند شکست بخورد `core::Result<T>` یا `core::Status` برمی‌گرداند** —
   نه `bool` لخت، نه پارامتر خروجی، نه `abort()`.
3. **هیچ استثنایی وجود ندارد.** `throw` / `try` / `catch` ممنوع.
4. **هیچ RTTI وجود ندارد.** `dynamic_cast` / `typeid` ممنوع.
5. **مالکیت با RAII است.** هر چیزی که گرفته می‌شود، در مخرب آزاد می‌شود.
6. **وابستگی فقط رو به پایین است.** لایه‌ی پایین هرگز لایه‌ی بالا را include نمی‌کند.
7. **نوع‌های بین‌لایه‌ای همیشه کامل صدا زده می‌شوند:** `core::Result<T>`، نه `Result<T>`.
8. **هر عدد جادویی یک نام می‌گیرد.**
9. **هر واحد در نام می‌آید:** `timeout_ms`، `period_us`، `stack_bytes`.
10. **کامنت «چرا» را می‌گوید، نه «چه».** کد خودش می‌گوید چه می‌کند.

---

## ۱) معماری چندلایه و جهت وابستگی

کد در چهار لایه سازمان می‌یابد. **جهت وابستگی همیشه رو به پایین است** و هیچ‌وقت
برنمی‌گردد.

```
┌──────────────────────────────────────────────────────────────┐
│  L4 — app        main/          منطق برنامه، app_main        │
│                  depends on: core, rtos, drivers             │
├──────────────────────────────────────────────────────────────┤
│  L3 — drivers    components/drivers/   Gpio, Uart, I2c,      │
│                  Spi, Nvs               + Timer در core      │
│                  depends on: core  (+ rtos در صورت نیاز)     │
├──────────────────────────────────────────────────────────────┤
│  L2 — rtos       components/rtos/      Task, Mutex, Queue<T> │
│                  depends on: core                            │
├──────────────────────────────────────────────────────────────┤
│  L1 — core       components/core/      Error/Result/Status,  │
│                  Log, Timer, ticks      هیچ وابستگی داخلی    │
├──────────────────────────────────────────────────────────────┤
│  ESP-IDF / FreeRTOS — کد سی که فقط رَپِرها می‌بینند          │
└──────────────────────────────────────────────────────────────┘
```

> **چرا پوشه‌ی لایه‌ی ۴ `main/` است و `app/` نیست:** ESP-IDF کامپوننتی که
> `app_main` را تعریف می‌کند باید `main` نام داشته باشد. تغییر نام آن یعنی
> دست‌کاری `COMPONENT_DIRS` که ارزشش را ندارد. پس نام پوشه `main` می‌ماند ولی
> نقشش «لایه‌ی برنامه» است.

### قواعد لایه‌بندی

- یک کامپوننت **فقط** می‌تواند کامپوننت‌های پایین‌تر از خودش را `REQUIRES` کند.
- **ممنوع:** `core` چیزی از `rtos` یا `drivers` را include کند. اگر لازم شد،
  یعنی آن چیز جای اشتباهی است و باید به `core` منتقل شود.
- **ممنوع:** دو کامپوننت هم‌سطح به هم وابسته شوند (`drivers` → `rtos` فقط اگر
  واقعاً لازم شد، و `rtos` هرگز به `drivers` وابسته نشود).
- هر کامپوننت یک هدر چتر دارد: `core/core.hpp`، `rtos/rtos.hpp`، `drivers/drivers.hpp`.
- در کد، نوع‌های بین‌لایه‌ای **کامل** نوشته می‌شوند تا جهت وابستگی در هر خط دیده شود:

  ```cpp
  namespace drivers {

  class Uart {
      // درست: خواننده می‌بیند که Result از لایه‌ی core می‌آید.
      [[nodiscard]] static core::Result<Uart> open(uart_port_t port, const Config& cfg) noexcept;

      // غلط: Result از کجا آمد؟ وابستگی پنهان شد.
      [[nodiscard]] static Result<Uart> open(uart_port_t port, const Config& cfg) noexcept;
  };

  } // namespace drivers
  ```

### افزودن یک لایه‌ی جدید

اگر روزی لایه‌ی پنجمی لازم شد (مثلاً `services` برای منطق دامنه)، همان الگو را
تکرار کن: پوشه‌ی جدید زیر `components/`، فضای‌نام هم‌نام، هدر چتر، و
`REQUIRES` فقط به سمت پایین. قبلش این سند و `docs/ARCHITECTURE.md` را به‌روز کن.

---

## ۲) نام‌گذاری

| مورد | قاعده | نمونه |
|---|---|---|
| فضای‌نام | هم‌نام کامپوننت، `snake_case` | `core`, `rtos`, `drivers` |
| کمک‌کننده‌های داخلی | `<comp>::detail` | `core::detail::to_ticks` |
| کلاس / ساختار / enum | `PascalCase` | `Gpio`, `I2cMasterBus`, `LockGuard` |
| تابع و متد | `snake_case` | `set_level`, `read_register` |
| عضو داده | `snake_case` + زیرخط انتهایی | `handle_`, `pin_`, `dir_` |
| ثابت | پیشوند `k` + `PascalCase` | `kWaitForever`, `kHeartbeatMs` |
| مقدار enum | `snake_case` | `Direction::input`, `Pull::up_down` |
| ماکرو | `SCREAMING_SNAKE` با پیشوند کامپوننت | `CORE_TRY_VOID`, `CORE_LOGI` |
| فایل | `snake_case.hpp` / `snake_case.cpp` | `i2c.hpp`, `error.cpp` |
| تگ لاگ | `constexpr const char* kTag` در هر فایل | `kTag = "i2c"` |
| هدر چتر | هم‌نام کامپوننت | `core/core.hpp` |

### حروف اختصاری

حروف اختصاری **به‌صورت کلمه‌ی معمولی** نوشته می‌شوند، نه با حروف بزرگ:

| درست | غلط |
|---|---|
| `I2c`, `I2cMasterBus` | `I2C`, `I2CMasterBus` |
| `Spi`, `SpiBus` | `SPI`, `SPIBus` |
| `Uart` | `UART` |
| `Nvs`, `NvsHandle` | `NVS`, `NVSHandle` |
| `Gpio` | `GPIO` |

دلیل: `UART` و `Uart` وقتی کنار هم می‌آیند (`UARTHandler`؟ `UartHandler`؟) قابل
پیش‌بینی نیستند. با یک قاعده‌ی واحد، نام‌ها همیشه یک شکل دارند.

استثنا: ثابت‌ها و ماکروها همه‌حروف‌بزرگ‌اند (`ESP_ERR_TIMEOUT`, `CORE_LOGI`).

### نام‌های توصیفی

- **واحد همیشه در نام می‌آید.** این پروژه فریم‌ور است؛ `timeout` بدون واحد یعنی باگ.

  ```cpp
  uint32_t timeout_ms;      // درست
  uint64_t period_us;       // درست
  uint32_t stack_bytes;     // درست
  uint32_t timeout;         // غلط — میلی‌ثانیه؟ تیک؟ میکروثانیه؟
  ```

- **`get_` و `set_` بی‌دلیل نه.** اگر تابع فقط می‌خواند، اسمش خودِ چیز است.
  اگر می‌نویسد، `set_` معنا دارد.

  ```cpp
  bool valid() const;          // درست
  gpio_num_t pin() const;      // درست
  bool get_valid() const;      // غلط
  void set_level(bool high);   // درست — واقعاً می‌نویسد
  ```

- **تابع بولین مثل یک پرسش خوانده شود:** `valid()`, `running()`, `open()`.
- **نام منفی نه:** `enabled` بنویس، نه `not_disabled`.
- **پارامترها بدون پیشوند:** `void set_level(bool high)`، نه `bHigh`.
- **متغیر سراسری ممنوع.** اگر واقعاً لازم شد (نباید بشود): پیشوند `g_`.
- **متغیر ایستا در فایل:** داخل `namespace { ... }` بی‌نام، بدون `static` تکراری.

### نام‌گذاری فایل‌ها

- هر فایل هدر یک **موضوع** را پوشش می‌دهد، نه یک لایه‌ی فنی:
  `gpio.hpp` (شامل `Gpio`, `Direction`, `Pull`, `Trigger`) درست است؛
  `enums.hpp` غلط است.
- فایل `.cpp` هم‌نام هدرش است: `gpio.hpp` ↔ `gpio.cpp`.
- قالب‌های فقط-هدر پسوند `.hpp` می‌گیرند و `.cpp` ندارند (`queue.hpp`).
- هدرهای داخلی زیر `detail/` می‌روند و بخشی از API عمومی نیستند.

---

## ۳) قالب‌بندی

### فاصله و عرض

- تورفتگی **۴ فاصله**. تب ممنوع.
- عرض نرم خط: **۱۰۰ کاراکتر**. اگر از این بیشتر شد، بشکن.
- فاصله‌ی انتهایی خطوط حذف می‌شود. فایل با یک خط خالی در انتها تمام می‌شود.
- داخل `namespace` تورفتگی اضافه نمی‌دهیم.

### آکولادها

آکولاد **تابع** در خط جدید (Allman). آکولاد **کنترل جریان** در همان خط (K&R).

```cpp
// تابع: آکولاد در خط بعد
core::Status Uart::write(const void* data, size_t len) noexcept
{
    // کنترل جریان: آکولاد در همان خط
    if (data == nullptr || len == 0) {
        return std::unexpected(core::Error{ESP_ERR_INVALID_ARG});
    }

    while (true) {
        // ...
    }
}
```

همیشه آکولاد می‌گذاریم، حتی برای بدنه‌ی یک‌خطی.

### شکستن خطوط طولانی

خط ادامه‌دار **به آخرین پرانتز بازِ تراز** می‌شود:

```cpp
[[nodiscard]] static core::Result<Gpio> claim(gpio_num_t pin,
                                              Direction dir,
                                              Pull pull = Pull::none) noexcept;
```

شرط‌های طولانی `if` استثنا هستند و با تورفتگی ۴ فاصله ادامه می‌یابند:

```cpp
if (const esp_err_t rc = i2c_master_receive(handle_, buffer, len, static_cast<int>(timeout_ms));
    rc != ESP_OK) {
    return std::unexpected(core::Error{rc});
}
```

### ماکروهای چندخطی

خطوط ماکرو با `\` ادامه می‌یابند و **بک‌اسلش در ستون ۸۰** تراز می‌شود:

```cpp
#define CORE_TRY_VOID(expr)                                                    \
    do {                                                                       \
        ::core::Status _core_status = (expr);                                  \
        if (!_core_status) {                                                   \
            return std::unexpected(std::move(_core_status).error());           \
        }                                                                      \
    } while (0)
```

### نشانگر و ارجاع

ستاره و امپرسند **چسبیده به نوع**:

```cpp
const char* name;       // درست
const char *name;       // غلط
void* arg;              // درست
Config& cfg;            // درست
```

### `const` و `constexpr`

`const` غربی است (قبل از نوع):

```cpp
const char* kTag = "app";
constexpr uint32_t kWaitForever = UINT32_MAX;
```

### ترتیب include

سه گروه، هر گروه با یک خط خالی، هر گروه مرتب‌شده:

```cpp
#include "drivers/uart.hpp"          // ۱. هدر خودِ همین فایل

#include "core/error.hpp"            // ۲. هدرهای پروژه
#include "core/detail/ticks.hpp"

#include "driver/gpio.h"             // ۳. ESP-IDF
#include "esp_err.h"

#include <cstddef>                   // ۴. استاندارد سی++
#include <string_view>
```

---

## ۴) ساختار فایل و هدر

- همه‌ی هدرها `#pragma once`. گارد ماکرویی نه.
- هدر عمومی در `include/<comp>/`، هدر داخلی در `include/<comp>/detail/`.
- **هدر تا حد امکان خودبسنده باشد.** اگر `gpio.hpp` را تنها include کنی باید
  کامپایل شود. یعنی هر چیزی که استفاده می‌کند را خودش include کند.
- هدر نباید چیز اضافه include کند. include یعنی وابستگی.
- هیچ هدر پروژه‌ای هدر ESP-IDF را در امضای عمومی خود آشکار نمی‌کند، مگر
  ناچیز باشد (`gpio_num_t`, `uart_port_t`). نوع‌های هندلی (`i2c_master_bus_handle_t`)
  در `private:` می‌مانند یا با `release()` بیرون داده می‌شوند.
- امضای عمومی حداقل باشد: فقط چیزی که مصرف‌کننده لازم دارد.
- یک نوع = یک مسئولیت. کلاسی که هم باس را می‌سازد و هم دستگاه را، باید بشکند
  (همان‌طور که `I2cMasterBus` و `I2cDevice` شدند).

### قالب یک ماژول جدید

```
components/<comp>/
├── CMakeLists.txt
├── include/<comp>/
│   ├── <comp>.hpp          هدر چتر
│   ├── <module>.hpp        API عمومی
│   └── detail/<helper>.hpp داخلی
└── src/<module>.cpp        پیاده‌سازی
```

```cmake
idf_component_register(
    SRCS "src/<module>.cpp"
    INCLUDE_DIRS "include"
    REQUIRES <لایه‌های پایین‌تر>
)
```

فراموش کردن `REQUIRES` → خطای `fatal error: xxx.h: No such file or directory`.
فراموش کردن یک فایل در `SRCS` → خطای `undefined reference`.

---

## ۵) خطا — «خطا به‌عنوان مقدار»

### قاعده

هر تابعی که می‌تواند شکست بخورد **یک مقدار** برمی‌گرداند:

| نوع | معنا |
|---|---|
| `core::Error` | پوشش نازک روی `esp_err_t`؛ کد عددی دست‌نخورده |
| `core::Result<T>` | یا مقدار `T`، یا دلیل نبودنش |
| `core::Status` | موفقیت یا دلیل شکست (معادل `Result<void>`) |
| `core::to_status(rc)` | تبدیل `esp_err_t` خام به `Status` |
| `core::to_result(rc, value)` | همان، ولی با مقدار |

```cpp
[[nodiscard]] core::Result<Uart> Uart::open(uart_port_t port, const Config& cfg) noexcept
{
    // ...
    if (const esp_err_t rc = uart_driver_install(port, cfg.rx_buffer_size, cfg.tx_buffer_size,
                                                 0, nullptr, 0);
        rc != ESP_OK) {
        return std::unexpected(core::Error{rc});
    }
    return uart;
}
```

### انتشار خطا

```cpp
core::Status blink(gpio_num_t pin)
{
    drivers::Gpio led;                                  // ۱. اول اعلان
    CORE_TRY_ASSIGN(led, drivers::Gpio::output(pin, false));  // ۲. بعد انتساب
    CORE_TRY_VOID(led.set_level(true));
    rtos::Task::delay_ms(100);
    CORE_TRY_VOID(led.set_level(false));
    return {};
}
```

> ⚠️ `CORE_TRY_ASSIGN(var, expr)` **انتساب** می‌دهد، اعلان نمی‌کند. متغیر باید
> از قبل اعلان شده و غیر-`const` باشد. `CORE_TRY_ASSIGN(const bool x, ...)` کامپایل
> نمی‌شود و `CORE_TRY_ASSIGN(Type x, ...)` هم متغیر را در بلوک داخلی ماکرو حبس
> می‌کند و بعد از ماکرو دیده نمی‌شود.

### ممنوعیت‌ها

```cpp
// غلط — bool لخت برای عملیات شکست‌پذیر
bool init_uart();

// غلط — خطا از طریق پارامتر خروجی
esp_err_t read(uint8_t* buf, size_t len, bool* ok);

// غلط — abort کردن برنامه
if (rc != ESP_OK) {
    abort();
}

// غلط — value() روی نتیجه‌ی بررسی‌نشده. با استثنای خاموش = std::terminate
auto uart = Uart::open(UART_NUM_1, cfg).value();

// درست
auto uart = Uart::open(UART_NUM_1, cfg);
if (!uart) {
    CORE_LOGE(kTag, "uart open failed: %s", uart.error().name());
    return;
}
```

### در مرز برنامه

لایه‌ی برنامه (`app_main` و تسک‌های سطح بالا) جایی است که خطا **گزارش** می‌شود:

```cpp
if (auto rc = init_subsystems(); !rc) {
    CORE_LOGE(kTag, "startup failed: %s (%s)", rc.error().name(),
              rc.error().message().data());
}
```

`Error::name()` نام نمادین می‌دهد (`"ESP_ERR_TIMEOUT"`)، `Error::message()`
توضیح انسانی.

---

## ۶) لاگ

همیشه از ماکروهای `CORE_LOG*` با تگ فایل استفاده کن. هرگز `printf`، هرگز
`ESP_LOGx` خام، هرگز `std::cout`.

```cpp
namespace {

constexpr const char* kTag = "i2c";

} // namespace

CORE_LOGI(kTag, "bus %d up at %u Hz", static_cast<int>(port), cfg.scl_speed_hz);
```

| ماکرو | کجا |
|---|---|
| `CORE_LOGE` | خطایی که عملکرد را متوقف می‌کند |
| `CORE_LOGW` | وضعیت غیرمنتظره ولی ادامه‌پذیر |
| `CORE_LOGI` | رویدادهای مهم: راه‌اندازی، اتصال، تغییر حالت |
| `CORE_LOGD` | جزئیات اشکال‌زدایی (در انتشار خاموش) |
| `CORE_LOGV` | پرحجم‌ترین سطح |

قواعد:

- در حلقه‌های پرتکرار و در ISR **لاگ نکن**. لاگ در ISR هم ممنوع است هم خطرناک.
- سطح پیش‌فرض `INFO` است. سطح را با `core::set_log_level()` می‌توان در زمان اجرا عوض کرد.
- پیام باید بگوید **چه چیزی** شکست خورد و **با چه مقداری**، نه فقط «error».
- برای داده‌ی باینری از `core::log_hex()` و `core::log_hexdump()` استفاده کن.

---

## ۷) حافظه، مالکیت و RAII

### قواعد رَپِر

1. **غیرقابل کپی، قابل جابه‌جایی.**

   ```cpp
   class Thing {
   public:
       Thing() = default;
       ~Thing();

       Thing(const Thing&) = delete;
       Thing& operator=(const Thing&) = delete;
       Thing(Thing&& other) noexcept;
       Thing& operator=(Thing&& other) noexcept;
   };
   ```

2. **مخرب همه چیز را آزاد می‌کند** — پین، هندل باس، نصب درایور، تسک، تایمر،
   هندل NVS. رَپِری که نشت کند باگ است.
3. **move assignment اول منبع فعلی را آزاد می‌کند**، بعد می‌دزدد. باید بتوان روی
   یک شیء زنده move-assign کرد.
4. **مخرب `noexcept` است** و لاگ سنگین نمی‌کند و بلاک نمی‌شود.
5. **`release()`** هندل خام را پس می‌دهد و شیء را بی‌اثر می‌کند.
6. **ساخت یک منبع، تابع کارخانه‌ای `static` است که `Result<T>` برمی‌گرداند** —
   نه سازنده. چون سازنده در این پروژه نمی‌تواند شکست را گزارش کند.

### ممنوعیت‌ها

```cpp
new / delete                 // ممنوع — از رَپِرها یا بافر ثابت استفاده کن
malloc / free                // ممنوع در کد برنامه
std::string                  // در مسیرهای داغ ممنوع
std::vector                  // در مسیرهای داغ ممنوع
std::function                // ممنوع — تخصیص پویا و مسیر خطای std::terminate
```

جایگزین‌ها:

| به‌جای | استفاده کن از |
|---|---|
| `std::string` | `std::string_view` برای خواندن، `std::array<char, N>` برای مالکیت |
| `std::vector` | `std::array<T, N>` یا `std::span<T>` |
| `std::function` | اشارهگر تابع ساده + `void* arg` (همان الگوی `Task::Entry`) |

### مالکیت طول عمر

- **`I2cDevice` / `SpiDevice` نباید از باسشان زنده‌تر بمانند.** باس را اول
  اعلان کن، دستگاه‌ها را بعد، در همان دامنه یا دامنه‌ی تنگ‌تر.
- **`NvsHandle` در مخرب `commit()` نمی‌کند.** نوشتن‌ها تا `commit()` صدا نشود از
  دست می‌روند. این عمدی است: «آیا به فلش رسید؟» باید در محل صدا زدن صریح باشد.
- **`Task::stop()` وقتی از خودِ تسک صدا زده شود رد می‌کند.** حذف تسک در حال اجرا
  مخرب خودش را رد می‌کند و هر چه در اختیار دارد نشت می‌کند.

---

## ۸) همزمانی

- برای قفل از `rtos::Mutex` + `LockGuard` استفاده کن، نه سمافور خام.

  ```cpp
  {
      rtos::LockGuard lock(mutex_);
      // ناحیه‌ی بحرانی
  }   // آزادسازی خودکار، حتی اگر return وسط باشد
  ```

- **در ISR هرگز بلاک نکن.** فقط نسخه‌های `*_from_isr` را صدا بزن.
- برای تبادل داده بین تسک‌ها **پیام‌رسانی (Queue) را به وضعیت مشترک ترجیح بده**.
  وضعیت مشترک یعنی قفل، و قفل یعنی بن‌بست بالقوه.
- هر تسک باید صریحاً بگوید: اولویت، هسته، دوره یا رویداد محرک، و اینکه مالک چه
  داده‌ای است. این‌ها در `docs/ARCHITECTURE.md` بخش ۶ ثبت می‌شوند.
- ناحیه‌ی بحرانی را تا حد امکان کوتاه نگه دار. داخل قفل لاگ نکن و I/O نکن.
- **ترتیب گرفتن چند قفل را مستند کن.** اگر دو قفل داری، همه‌جا با یک ترتیب بگیر.

---

## ۹) کامنت و مستندسازی

- کامنت **چرا** را می‌گوید. کد خودش می‌گوید **چه**.

  ```cpp
  // غلط — کد را تکرار می‌کند
  // مقدار را روی high تنظیم می‌کند
  return to_status(gpio_set_level(pin_, high ? 1u : 0u));

  // درست — تصمیم و دلیلش را می‌گوید
  // gpio_config() ممکن است رجیستر خروجی را پاک کند؛ برای اطمینان دوباره می‌نویسیم.
  CORE_TRY_VOID(set_level(initial_level));
  ```

- هر API عمومی یک کامنت مستندسازی دارد (`///`) که **واحدها، مالکیت و طول عمر**
  را بگوید:

  ```cpp
  /// Read up to `len` bytes, waiting at most `timeout_ms`.
  /// Pass kWaitForever to block indefinitely.
  [[nodiscard]] core::Result<size_t> read(void* buffer, size_t len,
                                          uint32_t timeout_ms = core::kWaitForever) noexcept;
  ```

- کد کامنت‌شده **نگه ندار**. گیت تاریخچه را نگه می‌دارد.
- `TODO` فقط با توضیح: `// TODO(i2c): handle bus arbitration loss.`
- بلوک‌های منطقی طولانی با یک کامنت کوتاه جدا می‌شوند، ولی تابع‌های بلند را
  **بشکن**، کامنت نگذار.
- نام‌های نادرست را کامنت نکن — **نام را درست کن**.

---

## ۱۰) گیت و کامیت

قالب پیام: `<type>(<scope>): <توضیح کوتاه>` با پیام بدنه‌ی اختیاری.

| type | کجا |
|---|---|
| `feat` | قابلیت جدید |
| `fix` | رفع باگ |
| `refactor` | تغییر ساختار بدون تغییر رفتار |
| `docs` | فقط مستندات |
| `build` | CMake، وابستگی‌ها، ابزار ساخت |
| `chore` | کارهای متفرقه |
| `test` | تست |

`scope` نام کامپوننت یا حوزه است: `core`, `rtos`, `drivers`, `app`, `docs`, `tools`, `vscode`.

```
feat(drivers): add Gpio wrapper with RAII pin ownership
fix(core): guard ESP_TIMER_ISR behind its Kconfig option
refactor: split the peripheral wrapper component into separate layers
docs(standards): add CODING_STANDARDS.md
```

قواعد:

- هر کامیت **یک تغییر منطقی**. کامیت غول‌پیکر ممنوع.
- فایل‌های تولیدشده کامیت نمی‌شوند: `build/`, `managed_components/`,
  `sdkconfig`, `graphify-out/`, `eim_config.toml`,
  `tools/idf-env.local.cmd`.
- قبل از کامیت، بیلد باید سبز باشد: `tools/idf.cmd build`.
- هرگز `--no-verify` نزن.

---

## ۱۱) چک‌لیست بازبینی

قبل از اینکه بگویی «تمام شد»، این‌ها را تیک بزن:

- [ ] `tools/idf.cmd build` سبز است، بدون خطا و بدون هشدار جدید.
- [ ] هیچ `throw` / `try` / `catch` / `dynamic_cast` / `typeid` در کد نیست.
- [ ] هیچ `printf`، `ESP_LOGx` خام یا `std::cout` نیست.
- [ ] هیچ `new` / `delete` / `malloc` در کد برنامه نیست.
- [ ] کد برنامه هیچ API سی فریم‌ورک را مستقیم صدا نمی‌زند.
- [ ] هر تابع شکست‌پذیر `core::Result<T>` یا `core::Status` برمی‌گرداند.
- [ ] هیچ `.value()` روی نتیجه‌ی بررسی‌نشده نیست.
- [ ] `CORE_TRY_ASSIGN` فقط روی متغیر از قبل اعلان‌شده استفاده شده.
- [ ] include‌ها به سمت بالا نیست (لایه‌بندی نقض نشده).
- [ ] هر عدد جادویی نام دارد.
- [ ] هر مقدار زمان‌دار واحدش در نامش است.
- [ ] API عمومی کامنت مستندسازی دارد.
- [ ] هدرها خودبسنده‌اند و زیاد include نمی‌کنند.
- [ ] رَپِر جدید غیرقابل کپی، قابل جابه‌جایی، و مخربش آزادکننده است.
- [ ] نام‌ها با جدول بخش ۲ سازگارند.
- [ ] `docs/ARCHITECTURE.md` در صورت تغییر معماری به‌روز شده.

---

## ۱۲) فهرست ممنوعه‌ها

یک نگاه سریع. اگر یکی از این‌ها را دیدی، برگرد:

```
throw / try / catch                      استثنا وجود ندارد
dynamic_cast / typeid                    RTTI وجود ندارد
printf / ESP_LOGx / std::cout            از CORE_LOG* استفاده کن
new / delete / malloc / free             از RAII و بافر ثابت استفاده کن
std::string / std::vector / std::function  در مسیرهای داغ
.value() بدون بررسی                     به std::terminate می‌رسد
abort() / exit()                         خطا را به بالا برگردان
bool برای عملیات شکست‌پذیر               Result<T> برگردان
پارامتر خروجی برای خطا                   Result<T> برگردان
API سی خام در کد برنامه                  رَپِر بنویس
وابستگی رو به بالا                       لایه‌بندی را نقض می‌کند
متغیر سراسری قابل تغییر                  وابستگی پنهان می‌سازد
عدد جادویی                               نام‌دار کن
timeout بدون واحد                        timeout_ms بنویس
کد کامنت‌شده                             گیت تاریخچه را نگه می‌دارد
```
