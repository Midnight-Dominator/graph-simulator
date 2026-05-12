@echo off
REM ================================================================
REM build.bat — Graph Sandbox C++ SDL2 (MSYS2 / MinGW-w64)
REM Chay trong Developer Command Prompt hoac MSYS2 bash
REM ================================================================

SET OUT=graph_sandbox.exe
SET SRC=main.cpp

REM ── Tim g++ ──────────────────────────────────────────────────────
where g++ >nul 2>&1
IF ERRORLEVEL 1 (
    echo [ERROR] Khong tim thay g++. Cai MSYS2 va them vao PATH.
    pause & exit /b 1
)

REM ── Tim pkg-config ───────────────────────────────────────────────
where pkg-config >nul 2>&1
IF ERRORLEVEL 1 (
    echo [ERROR] Khong tim thay pkg-config.
    echo Chay: pacman -S mingw-w64-x86_64-pkg-config
    pause & exit /b 1
)

REM ── Lay flags SDL2 ───────────────────────────────────────────────
FOR /F "tokens=*" %%i IN ('pkg-config --cflags sdl2 SDL2_ttf') DO SET SDL_CF=%%i
FOR /F "tokens=*" %%i IN ('pkg-config --libs   sdl2 SDL2_ttf') DO SET SDL_LF=%%i

REM ── Font ─────────────────────────────────────────────────────────
IF NOT EXIST assets mkdir assets
IF NOT EXIST assets\font.ttf (
    IF EXIST C:\Windows\Fonts\consola.ttf (
        copy C:\Windows\Fonts\consola.ttf assets\font.ttf >nul
        echo [OK] Font: Consolas
    ) ELSE IF EXIST C:\Windows\Fonts\arial.ttf (
        copy C:\Windows\Fonts\arial.ttf assets\font.ttf >nul
        echo [OK] Font: Arial
    ) ELSE (
        echo [WARN] Khong tim thay font — copy .ttf vao assets\font.ttf
    )
)

REM ── Build ─────────────────────────────────────────────────────────
echo [*] Building %SRC% ...
g++ -std=c++23 -O2 -Wall %SDL_CF% %SRC% -o %OUT% %SDL_LF%

IF ERRORLEVEL 1 (
    echo [ERROR] Build that bai!
    pause & exit /b 1
)

echo.
echo =========================================
echo   BUILD OK:  %OUT%
echo =========================================
echo   Chay: %OUT%
echo =========================================
pause
