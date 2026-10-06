@echo off
REM ============================================================
REM  idf.py wrapper -- activates the ESP-IDF environment first,
REM  then forwards every argument to idf.py.
REM
REM  WHY: `idf.py` is only on PATH once the environment is activated. This
REM  wrapper makes that step transparent, so VS Code tasks and plain
REM  terminals can call it without knowing where ESP-IDF is installed.
REM
REM  It works with a standard ESP-IDF install, with the ESP-IDF Installation
REM  Manager (EIM), and with a per-machine override -- see idf-env.cmd.
REM
REM  Usage (from anywhere in the repo):
REM      tools\idf.cmd build
REM      tools\idf.cmd -p COM5 flash monitor
REM      tools\idf.cmd menuconfig
REM      tools\idf.cmd size
REM
REM  VS Code tasks call this file (see .vscode\tasks.json).
REM ============================================================

REM Keep idf-env.cmd quiet so only idf.py output is shown.
set "IDF_ENV_QUIET=1"
call "%~dp0idf-env.cmd"

REM If activation failed, stop before idf.py reports a confusing error.
if errorlevel 1 exit /b %ERRORLEVEL%

idf.py %*
exit /b %ERRORLEVEL%
