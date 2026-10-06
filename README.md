# قالب پروژه‌ی ESP-IDF (C++)

یک نقطه‌ی شروع آماده برای هر پروژه‌ی ESP-IDF: معماری چهارلایه در C++، مدل
«خطا به‌عنوان مقدار» بدون استثنا، و ابزارهایی که **هیچ مسیر مخصوص یک ماشین** در
خودشان ندارند.

این پوشه خودش یک پروژه‌ی کارآمد است، ولی هدفش این است که کپی شود و پروژه‌ی
جدید از رویش ساخته شود.

---

## شروع یک پروژه‌ی جدید

**۱) پوشه را با نام دلخواه کپی کن.** نام پروژه خودکار از نام پوشه گرفته
می‌شود، پس همین یک قدم همه‌جا اعمال می‌شود:

```bash
# کلون کردن قالب با نام پروژه‌ی جدید
git clone <آدرس-قالب> MyNewProject
cd MyNewProject

# تاریخچه‌ی قالب را دور بریز و تاریخچه‌ی خودت را شروع کن
rm -rf .git
git init
```

اگر قالب را به‌صورت پوشه‌ی محلی داری:

```bash
robocopy . MyNewProject /E /XD .git build managed_components graphify-out .scratch MyNewProject
```

**۲) چیپ هدف را انتخاب کن** (یک بار):

```bash
tools/idf.cmd set-target esp32        # یا esp32s3، esp32c3، ...
```

**۳) بیلد کن:**

```bash
tools/idf.cmd build
```

خروجی `build/<نام-پوشه>.elf` است. اگر پوشه را `MyNewProject` نام بگذاری،
خروجی `build/MyNewProject.elf` می‌شود.

**۴) کد واقعی را بنویس.** نقطه‌ی ورود `main/main.cpp` است. یک حلقه‌ی ساده‌ی
heartbeat دارد که فقط ثابت می‌کند زنجیره‌ی ساخت کار می‌کند — جایش را با
برنامه‌ی واقعی عوض کن.

> **قبل از نوشتن کد، [`docs/CODING_STANDARDS.md`](docs/CODING_STANDARDS.md) را بخوان.**
> الزامی است، نه پیشنهاد.

---

## چرا نام پروژه از نام پوشه می‌آید؟

در `CMakeLists.txt` بالای پروژه، نام از نام پوشه ساخته می‌شود:

```cmake
get_filename_component(_project_dir_name "${CMAKE_CURRENT_LIST_DIR}" NAME)
string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _project_name "${_project_dir_name}")
project(${_project_name})
```

یعنی هیچ‌جای قالب نام محصول هاردکد نشده. پوشه را عوض کنی، نام پروژه، هدف‌های
CMake و نام فایل خروجی خودشان عوض می‌شوند. کاراکترهای غیرمجاز (فاصله، خط تیره،
نقطه) به `_` تبدیل می‌شوند.

---

## چه چیزی داخلش هست

```
.
├── CMakeLists.txt              پروژه‌ی سطح‌بالا — نام از نام پوشه
├── sdkconfig.defaults          پیش‌فرض‌های sdkconfig (در گیت)
├── main/                       لایه‌ی ۴ — برنامه (app_main)
├── components/
│   ├── core/                   لایه‌ی ۱ — خطا، لاگ، زمان
│   ├── rtos/                   لایه‌ی ۲ — رَپِر FreeRTOS
│   └── drivers/                لایه‌ی ۳ — رَپِر پریفرال‌ها
├── docs/                       مستندات طراحی
├── tools/                      ابزارهای فعال‌سازی محیط
├── .vscode/                    تنظیمات VS Code
└── SETUP_ESPIDF.md             راه‌اندازی محیط
```

### معماری در یک نگاه

چهار لایه، و **وابستگی فقط رو به پایین**:

```
L4  app       main/                 app_main، منطق برنامه
L3  drivers   components/drivers/   Gpio · Uart · I2c · Spi · Nvs
L2  rtos      components/rtos/      Task · Mutex · LockGuard · Queue<T>
L1  core      components/core/      Error/Result/Status · لاگ · Timer
```

**قاعده‌ی طلایی:** کد برنامه هرگز مستقیم `esp_*` یا `gpio_*` یا `xTask*` صدا
نمی‌زند. همه چیز از رَپِرها می‌گذرد — این تنها راهی است که بعداً بتوان
درایورها را عوض کرد.

جزئیات کامل: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)

---

## بیلد، فلش، مانیتور

همه‌ی دستورها از `tools/idf.cmd` می‌گذرند. این wrapper خودش محیط ESP-IDF را
پیدا و فعال می‌کند، پس `idf.py` لازم نیست روی `PATH` باشد:

```bash
tools/idf.cmd build
tools/idf.cmd -p COM5 flash monitor
tools/idf.cmd menuconfig
tools/idf.cmd size
```

در VS Code هم همین‌ها به‌صورت تسک آماده‌اند (`Ctrl+Shift+P` → `Tasks: Run Task`)
و `Ctrl+Shift+B` مستقیم بیلد می‌کند.

جزئیات راه‌اندازی محیط: [`SETUP_ESPIDF.md`](SETUP_ESPIDF.md)

---

## مستندات

| سند | محتوا |
|---|---|
| [`docs/CODING_STANDARDS.md`](docs/CODING_STANDARDS.md) | **اصول کدنویسی — الزامی** |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | معماری چهارلایه، نقشه‌ی ماژول‌ها، مدل خطا |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | فازها و وضعیت پروژه |
| [`SETUP_ESPIDF.md`](SETUP_ESPIDF.md) | راه‌اندازی محیط ESP-IDF |
| [`docs/README.md`](docs/README.md) | فهرست و قواعد مستندسازی |

قواعد کد و معماری یک نسخه‌ی عملیاتی هم دارند که در `.workbuddy-ai/skills/`
نگهداری می‌شود و **در گیت است**.

---

## پیش‌نیازها

- **ESP-IDF** — نسخه‌ی ۵ یا بالاتر. نصب استاندارد یا با ESP-IDF Installation
  Manager (EIM)؛ هر دو پشتیبانی می‌شوند.
- **پایتون ۳** و **CMake/Ninja** — همراه خود ESP-IDF نصب می‌شوند.
- **VS Code** (اختیاری) + افزونه‌ی `espressif.esp-idf-extension`.

اگر نصب ESP-IDF در جای غیرمعمولی است، `tools/idf-env.local.cmd.example` را
ببین.

---

## چه چیزی عمداً داخلش نیست

این قالب تمیز نگه داشته شده. این‌ها را خودت اضافه می‌کنی:

- **نام محصول** — از نام پوشه می‌آید.
- **مسیرهای ابزار** — خودکار پیدا می‌شوند (`tools/idf-env.cmd`).
- **چیپ هدف** — با `set-target` انتخاب می‌شود، نه هاردکد.
- **کانفیگ برد JTAG** — در `ESP-IDF: Configure ESP-IDF Extension` انتخاب می‌شود.
- **حافظه‌ی اندازه‌گیری‌شده و اعداد کارایی** — مخصوص هر بیلد است؛ در
  `docs/ARCHITECTURE.md` جای‌شان `TBD` است.
