@echo off
REM Build Script for Algorithmic Trading Simulator
REM This script compiles the project

echo ================================================
echo Algorithmic Trading Simulator - Build Script
echo ================================================
echo.

REM Set build configuration
set BUILD_TYPE=Release
set BUILD_DIR=build

REM Check if build directory exists, create if not
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir %BUILD_DIR%
)

REM Navigate to build directory
cd %BUILD_DIR%

REM Configure with CMake
echo.
echo Configuring project with CMake...
echo ================================================
cmake ..
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building project...
echo ================================================
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

REM Return to root directory
cd ..

echo.
echo ================================================
echo Build successful!
echo Executable: build\bin\Release\tradingSim.exe
echo ================================================
echo.
pause
