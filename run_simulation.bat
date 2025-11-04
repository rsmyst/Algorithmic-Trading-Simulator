@echo off
REM Algorithmic Trading Simulation - Build and Run Script
REM This script sets up the environment, compiles, and runs the simulation

echo ================================================
echo Algorithmic Trading Simulation
echo Using OpenMP for Parallel Processing
echo ================================================
echo.

REM Set build configuration
set BUILD_TYPE=Release
set BUILD_DIR=build

REM Check if build directory exists
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir %BUILD_DIR%
)

REM Navigate to build directory
cd %BUILD_DIR%

REM Configure with CMake
echo.
echo ================================================
echo Configuring project with CMake...
echo ================================================
cmake ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo.
echo ================================================
echo Building project...
echo ================================================
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
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
echo ================================================
echo.

REM Parse command line arguments or use defaults
set NUM_TRADERS=12
set DURATION=60
set INITIAL_PRICE=100.0
set INITIAL_CASH=10000.0

REM Check for command line arguments
if not "%1"=="" set NUM_TRADERS=%1
if not "%2"=="" set DURATION=%2
if not "%3"=="" set INITIAL_PRICE=%3
if not "%4"=="" set INITIAL_CASH=%4

echo Running simulation with parameters:
echo   Traders: %NUM_TRADERS%
echo   Duration: %DURATION% seconds
echo   Initial Price: $%INITIAL_PRICE%
echo   Initial Cash: $%INITIAL_CASH%
echo.
echo Press Ctrl+C to stop early, or 'q' in the application
echo ================================================
echo.

REM Run the simulation
.\build\bin\Release\ftxui_demo.exe -t %NUM_TRADERS% -d %DURATION% -p %INITIAL_PRICE% -c %INITIAL_CASH%

echo.
echo ================================================
echo Simulation finished!
echo ================================================
pause
