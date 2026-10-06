# راه‌اندازی محیط ESP-IDF

این سند توضیح می‌دهد چطور `tools/idf.cmd` محیط ESP-IDF را پیدا می‌کند، و اگر
پیدا نکرد چطور بهش بگویی کجاست.

> **نکته‌ی اصلی:** هیچ مسیر مخصوص یک ماشین در این ریپو ذخیره نشده. هر مسیری که
> لازم شود، در زمان اجرا کشف می‌شود یا در یک فایل محلی و خارج از گیت می‌نشیند.

---

## ۱) چطور کار می‌کند

همه‌ی دستورها از `tools/idf.cmd` می‌گذرند. این فایل اول `tools/idf-env.cmd` را
صدا می‌زند (که محیط را فعال می‌کند) و بعد آرگومان‌ها را به `idf.py` می‌فرستد.

`idf-env.cmd` چهار استراتژی را به ترتیب امتحان می‌کند:

| # | استراتژی | چه زمانی جواب می‌دهد |
|---|---|---|
| ۱ | **محیط از قبل فعال است** | `IDF_PATH` ست است و `idf.py` پیدا می‌شود |
| ۲ | **فایل محلی override** | `tools/idf-env.local.cmd` وجود دارد (در گیت نیست) |
| ۳ | **نصب با EIM** | پروفایل فعال‌سازی EIM پیدا شود |
| ۴ | **نصب کلاسیک** | `<framework>\export.bat` وجود داشته باشد |

اگر هیچ‌کدام جواب نداد، پیام روشنی چاپ می‌شود که چه کار کنی.

### استراتژی ۳ چطور بدون مسیر هاردکد کار می‌کند

EIM (ESP-IDF Installation Manager) یک پروفایل فعال‌سازی کنار پوشه‌ی ابزارهایش
می‌سازد، به شکل `Microsoft.<version>.PowerShell_profile.ps1`. این پروفایل
پارامتر `-e` دارد که **فقط محیط را چاپ می‌کند** (`KEY=VALUE`) و قبل از تغییر
هیچ چیزی برمی‌گردد. `idf-env.cmd` همان را اجرا می‌کند و خطوط را اعمال می‌کند.

یعنی مسیرها از خودِ نصب‌کننده می‌آیند، نه از این ریپو. روی هر ماشینی که EIM
نصب باشد کار می‌کند، بدون اینکه چیزی در گیت ذخیره شود.

> برای دیدن خروجی خام، خودت اجرا کن:
> ```
> powershell -NoProfile -ExecutionPolicy Bypass -File "<مسیر-پروفایل>" -e
> ```

---

## ۲) نصب ESP-IDF

### روش A — نصب‌کننده‌ی رسمی یا EIM

اگر با نصب‌کننده‌ی رسمی Espressif یا با **EIM** نصب کرده‌ای، هیچ کاری لازم نیست.
`tools/idf.cmd build` را بزن؛ خودش پیدایش می‌کند.

### روش B — کلون دستی

```bash
git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh          # ویندوز: install.bat
```

بعد در ترمینال:

```bash
. ./export.sh          # ویندوز CMD: export.bat
```

اگر محیط فعال باشد، `tools/idf.cmd` هم مستقیم کار می‌کند (استراتژی ۱).

### روش C — هر جای دیگری

اگر نصب در جای غیرمعمولی است یا فعال‌سازی‌اش متفاوت است، از فایل محلی استفاده کن:

```bash
cp tools/idf-env.local.cmd.example tools/idf-env.local.cmd
# حالا مسیرها را داخلش ویرایش کن
```

این فایل در `.gitignore` است، پس هر ماشین می‌تواند نسخه‌ی خودش را داشته باشد.
`idf-env.cmd` به‌محض وجود داشتن، قبل از هر تشخیص خودکاری آن را صدا می‌زند.

نمونه‌ی داخل `idf-env.local.cmd.example` یک نصب واقعی EIM روی ویندوز است — ببین
شکل متغیرها چطور است.

---

## ۳) متغیرهای محیطی

ESP-IDF برای کار به این‌ها نیاز دارد:

| متغیر | کار |
|---|---|
| `IDF_PATH` | ریشه‌ی فریم‌ورک (پوشه‌ای که `tools/cmake/project.cmake` دارد) |
| `IDF_TOOLS_PATH` | جایی که ابزارها (کامپایلر، CMake، Ninja، ...) نصب‌اند |
| `IDF_PYTHON_ENV_PATH` | محیط مجازی پایتون ESP-IDF |
| `ESP_IDF_VERSION` | نسخه، مثلاً `5.5` |
| `IDF_VERSION` | نسخه‌ی کامل، مثلاً `5.5.1` |

به‌علاوه پوشه‌ی `bin` همه‌ی ابزارها باید روی `PATH` باشد.

> `ESP_IDF_VERSION` و `IDF_VERSION` را فراموش نکن. بدون آن‌ها
> `idf_component_manager` با خطای `expected string or bytes-like object, got
> NoneType` می‌افتد.

---

## ۴) تسک‌های VS Code

در `.vscode/tasks.json` — با `Ctrl+Shift+P` → `Tasks: Run Task`:

| تسک | کار |
|---|---|
| ESP-IDF: set target | انتخاب چیپ (از فهرست) |
| ESP-IDF: build | بیلد (`Ctrl+Shift+B`) |
| ESP-IDF: flash | ریختن روی برد |
| ESP-IDF: monitor | دیدن لاگ سریال |
| ESP-IDF: flash & monitor | هر دو با هم |
| ESP-IDF: menuconfig | تنظیمات پروژه |
| ESP-IDF: size | حجم باینری |
| ESP-IDF: fullclean | پاک‌سازی کامل |
| ESP-IDF: erase flash | پاک کردن کل فلش |
| ESP-IDF: doctor | بررسی سلامت محیط |

همه‌ی این‌ها از `tools\idf.cmd` استفاده می‌کنند، پس **بدون هیچ کار اضافه‌ای** کار
می‌کنند — حتی اگر افزونه‌ی ESP-IDF نصب نباشد.

---

## ۵) افزونه‌ی ESP-IDF در VS Code

نصب نیست؟ یک بار:

```bash
code --install-extension espressif.esp-idf-extension
```

بعد `Ctrl+Shift+P` → `ESP-IDF: Configure ESP-IDF Extension` → حالت **EXPRESS**.

**چرا ارزش دارد؟** محیط را خودش می‌چیند، `compile_commands.json` برای
IntelliSense می‌سازد، دیباگ را پیکربندی می‌کند و build/flash/monitor را با یک
کلیک انجام می‌دهد. اگر نصبش کنی، می‌توانی `launch.json` و
`c_cpp_properties.json` را هم با `ESP-IDF: Add .vscode configuration folder`
دوباره بسازی تا مسیرهای ماشین خودت درست شوند.

---

## ۶) چیپ هدف

یک بار برای هر پروژه:

```bash
tools/idf.cmd set-target esp32
```

مقادیر مجاز: `esp32` · `esp32s2` · `esp32s3` · `esp32c2` · `esp32c3` ·
`esp32c6` · `esp32h2` · `esp32p4`

> `CONFIG_IDF_TARGET` عمداً در `sdkconfig.defaults` نیست. با `set-target`
> انتخاب می‌شود و در `sdkconfig` تولیدشده می‌نشیند.

---

## ۷) بیلد، فلش و مانیتور

```bash
tools/idf.cmd build
tools/idf.cmd -p COM5 flash monitor
```

- خروج از مانیتور: `Ctrl+]`
- پیدا کردن پورت روی ویندوز:

```powershell
Get-PnpDevice -Class Ports | Select-Object FriendlyName,Status
```

اگر پورت دیده نشد، درایور USB-UART لازم است: **CP210x** (Silicon Labs) یا
**CH34x** (WCH).

`tools/serial_monitor.py` هم یک مانیتور سبک است (نیاز به `pyserial`):

```bash
python tools/serial_monitor.py COM5 115200
```

---

## ۸) ساختار پوشه‌ی ابزارها

```
tools/
├── idf.cmd                      wrapper سراسری برای idf.py
├── idf-env.cmd                  تشخیص و فعال‌سازی محیط (قابل حمل)
├── idf-env.local.cmd.example    نمونه‌ی فایل محلی برای نصب غیرمعمول
└── serial_monitor.py            مانیتور سریال سبک
```

`tools/idf-env.local.cmd` (بدون `.example`) در گیت نیست.

---

## ۹) عیب‌یابی

| مشکل | راه‌حل |
|---|---|
| `idf.py: command not found` | از `tools/idf.cmd <دستور>` استفاده کن |
| `could not locate an ESP-IDF installation` | `tools/idf-env.local.cmd` را از روی `.example` بساز |
| تسک‌های VS Code کار نمی‌کنند | `Ctrl+Shift+P` → `Developer: Reload Window` |
| `running scripts is disabled` | `idf-env.cmd` خودش با `-ExecutionPolicy Bypass` دورش می‌زند؛ اگر باز هم شد: `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` |
| `got 'NoneType'` از idf_component_manager | `ESP_IDF_VERSION` ست نیست — بخش ۳ |
| `MSys/Mingw is no longer supported` | `MSYSTEM` ست است؛ `idf-env.cmd` خودش پاکش می‌کند |
| `export.bat` می‌گوید "for Windows CMD.EXE shell only" | از Git Bash اجرا شده؛ از CMD استفاده کن |
| `enabled ccache in build but ccache program not found` | محیط فعال نشده |
| `xtensa-esp32-elf-gcc ... not found in the PATH` | همان مشکل بالا |
| `export.bat` خطای `python_env ... not found` می‌دهد | نصب با EIM است و `export.bat` با آن سازگار نیست — استراتژی ۳ خودش حلش می‌کند |
| IntelliSense پر از خط قرمز | یک بار بیلد کن تا `compile_commands.json` ساخته شود، بعد `C/C++: Reset IntelliSense Database` |
| پورت COM پیدا نمی‌شود | درایور USB-UART را نصب کن |
| `Failed to connect to ESP32` | دکمه‌ی BOOT را نگه دار و RST را بزن |
| `sdkconfig` عوض شد ولی اثر نکرد | `tools/idf.cmd fullclean` و بعد `build` |
