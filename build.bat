@echo off
REM =============================================================================
REM DreamForge — 一键编译脚本
REM 自动查找 Visual Studio 自带的 CMake，无需配置环境变量
REM =============================================================================

setlocal enabledelayedexpansion

echo === DreamForge Build Script ===
echo.

REM ---------------------------------------------------------------------------
REM 1. 查找 CMake（按优先级：VS2022 → PATH → 手动提示）
REM ---------------------------------------------------------------------------
set CMAKE=

REM VS2022 (v18) Community
for /d %%v in ("C:\Program Files\Microsoft Visual Studio\18\Community") do (
    if exist "%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE=%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
)

REM VS2022 (v18) Professional
if "%CMAKE%"=="" (
    for /d %%v in ("C:\Program Files\Microsoft Visual Studio\18\Professional") do (
        if exist "%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
            set "CMAKE=%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        )
    )
)

REM VS2022 BuildTools
if "%CMAKE%"=="" (
    for /d %%v in ("C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools") do (
        if exist "%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
            set "CMAKE=%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        )
    )
)

REM Fallback: check PATH
if "%CMAKE%"=="" (
    where cmake >nul 2>&1
    if !errorlevel!==0 set "CMAKE=cmake"
)

if "%CMAKE%"=="" (
    echo [ERROR] Cannot find CMake.
    echo Please install Visual Studio 2022 Community from:
    echo   https://visualstudio.microsoft.com/downloads/
    echo Make sure to check "Desktop development with C++" during install.
    pause
    exit /b 1
)

echo [OK] CMake: %CMAKE%
echo.

REM ---------------------------------------------------------------------------
REM 2. 配置（首次需要，后续可跳过）
REM ---------------------------------------------------------------------------
if not exist "build\CMakeCache.txt" (
    echo [Step 1/2] Configuring project...
    "%CMAKE%" -B build -G "Visual Studio 17 2022"
    if !errorlevel! neq 0 (
        echo [ERROR] CMake configure failed.
        pause
        exit /b 1
    )
    echo [OK] Configure done.
) else (
    echo [Step 1/2] Build directory exists, skipping configure.
)
echo.

REM ---------------------------------------------------------------------------
REM 3. 编译
REM ---------------------------------------------------------------------------
echo [Step 2/2] Building...
"%CMAKE%" --build build --config Debug
if !errorlevel! neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Build succeeded!
echo   Output: build\bin\Debug\sample_game.exe
echo ============================================
echo.
echo Run:   .\build\bin\Debug\sample_game.exe
echo.

endlocal
pause
