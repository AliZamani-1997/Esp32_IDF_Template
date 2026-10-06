@echo off
REM ============================================================
REM  Activate the ESP-IDF environment in a CMD shell.
REM
REM  Usage (from the repo root, in a CMD terminal):
REM      tools\idf-env.cmd
REM  then:
REM      idf.py build
REM
REM  Or just use tools\idf.cmd, which calls this and then runs idf.py.
REM
REM  HOW IT FINDS ESP-IDF
REM  --------------------
REM  No machine-specific path is stored in this repository. The script tries,
REM  in order:
REM
REM    1. Already active    IDF_PATH is set and idf.py resolves -> nothing to do.
REM
REM    2. Local override    tools\idf-env.local.cmd exists -> it is called.
REM                         That file is NOT tracked by git; it is the escape
REM                         hatch for layouts the steps below do not handle.
REM
REM    3. EIM install       The ESP-IDF Installation Manager generates an
REM                         activation profile next to its tools folder.
REM                         Running that profile with -e prints the environment
REM                         as KEY=VALUE lines, which are applied here. This is
REM                         why no path has to be hard-coded: the paths come
REM                         from the installer that created them.
REM
REM    4. Classic install   <framework>\export.bat, the standard ESP-IDF script.
REM
REM  If none of these match, the script prints what to do.
REM ============================================================

REM idf.py refuses to run when MSYSTEM is set (it thinks it is under MSYS).
set "MSYSTEM="

REM ---------------------------------------------------------------
REM 1) Already active?
REM ---------------------------------------------------------------
if defined IDF_PATH (
    where idf.py >nul 2>&1 && goto :activated
)

REM ---------------------------------------------------------------
REM 2) Per-machine override, not tracked by git.
REM ---------------------------------------------------------------
if exist "%~dp0idf-env.local.cmd" (
    if not defined IDF_ENV_QUIET echo [idf-env] using local override: %~dp0idf-env.local.cmd
    call "%~dp0idf-env.local.cmd"
    goto :check
)

REM ---------------------------------------------------------------
REM 3) EIM install -- ask EIM's own activation profile for the env.
REM ---------------------------------------------------------------
call :try_eim
if defined IDF_PATH goto :check

REM ---------------------------------------------------------------
REM 4) Classic install -- the standard export.bat.
REM ---------------------------------------------------------------
call :try_export_bat
if defined IDF_PATH goto :check

goto :not_found


REM ===============================================================
REM  Subroutines
REM ===============================================================

:try_eim
set "_eim_profile="

REM The activation profile lives in the EIM tools root. IDF_TOOLS_PATH points
REM at it once set; the usual roots are probed too so this works on a fresh
REM shell where nothing is set yet.
for %%R in ("%IDF_TOOLS_PATH%" "%USERPROFILE%\.espressif" "%USERPROFILE%\esp" "%USERPROFILE%\esp\tools" "C:\Espressif\tools") do (
    if not defined _eim_profile if not "%%~R"=="" (
        for %%P in ("%%~R\Microsoft.*.PowerShell_profile.ps1") do set "_eim_profile=%%~fP"
    )
)

if not defined _eim_profile exit /b 1

if not defined IDF_ENV_QUIET echo [idf-env] EIM install found: %_eim_profile%

REM The profile accepts -e ("export only"): it prints KEY=VALUE lines and
REM returns before changing anything. Process-scope Bypass is required because
REM script execution is disabled on many Windows machines, and it affects only
REM the child process started here.
REM
REM PATH from that output holds the ESP-IDF tool folders ONLY -- the profile
REM itself prepends them to the existing PATH at activation time, so it must be
REM prepended here too. Replacing PATH would drop System32 and break idf.py.
REM %PATH% is expanded once, when this for statement is parsed, so it still
REM refers to the pre-loop value. SYSTEM_PATH is the installer's record of the
REM original PATH and is skipped -- it is what we are preserving anyway.
for /f "usebackq tokens=1,* delims==" %%A in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%_eim_profile%" -e 2^>nul`) do (
    if /i "%%A"=="PATH" (
        set "PATH=%%B;%PATH%"
    ) else if /i not "%%A"=="SYSTEM_PATH" (
        set "%%A=%%B"
    )
)

exit /b 0


:try_export_bat
REM Classic installs ship export.bat at the framework root. The framework is
REM identified by tools\cmake\project.cmake, which every ESP-IDF has.
set "_idf_candidate="

if defined IDF_PATH if exist "%IDF_PATH%\export.bat" set "_idf_candidate=%IDF_PATH%"

if not defined _idf_candidate (
    for %%D in ("%USERPROFILE%\esp\esp-idf" "%USERPROFILE%\esp-idf" "%USERPROFILE%\esp\v6.1\esp-idf" "C:\Espressif\frameworks\esp-idf" "C:\esp-idf") do (
        if not defined _idf_candidate if exist "%%~D\tools\cmake\project.cmake" set "_idf_candidate=%%~D"
    )
)

if not defined _idf_candidate exit /b 1

if not defined IDF_ENV_QUIET echo [idf-env] ESP-IDF framework found: %_idf_candidate%

set "IDF_PATH=%_idf_candidate%"
call "%_idf_candidate%\export.bat"
exit /b 0


:check
if not defined IDF_PATH goto :not_found
if not exist "%IDF_PATH%\tools\cmake\project.cmake" goto :not_found

:activated
if defined IDF_ENV_QUIET exit /b 0
echo.
echo [idf-env] ESP-IDF environment activated.
echo [idf-env] IDF_PATH = %IDF_PATH%
if defined IDF_TOOLS_PATH echo [idf-env] IDF_TOOLS_PATH = %IDF_TOOLS_PATH%
echo [idf-env] Try:  idf.py --version
exit /b 0


:not_found
echo.
echo [idf-env] ERROR: could not locate an ESP-IDF installation.
echo.
echo   Whichever of these applies to you:
echo.
echo   a) Standard install or git clone
echo      Nothing to do -- this script finds it through export.bat.
echo.
echo   b) ESP-IDF Installation Manager (EIM)
echo      Nothing to do either -- this script reads the activation profile
echo      that EIM generates.
echo.
echo   c) A layout neither of the above covers
echo      Copy tools\idf-env.local.cmd.example to tools\idf-env.local.cmd
echo      and set IDF_PATH (and IDF_TOOLS_PATH) there. That file is not
echo      tracked by git, so it can differ per machine.
echo.
exit /b 1
