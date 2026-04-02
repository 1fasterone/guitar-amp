@echo off
setlocal

:: ------------------------------------------------------------
:: Guitar Amp - Windows Build Script
:: Requires: CMake, Visual Studio 2022, Git
:: First run will download JUCE (~200 MB) automatically.
:: ------------------------------------------------------------

set BUILD_DIR=build

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [1/2] Configuring...
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed.
    echo Make sure CMake is installed: https://cmake.org/download/
    pause
    exit /b 1
)

echo.
echo [2/2] Building (Release)...
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed. Check output above.
    pause
    exit /b 1
)

echo.
echo Build complete.
echo Executable: %BUILD_DIR%\GuitarAmp_artefacts\Release\GuitarAmp.exe
pause
