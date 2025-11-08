@echo off
REM Run Script for Algorithmic Trading Simulator
REM This script runs the compiled executable

echo ================================================
echo Algorithmic Trading Simulator
echo ================================================
echo.

REM Check if executable exists
if not exist "build\bin\Release\tradingSim.exe" (
    echo ERROR: tradingSim.exe not found!
    echo Please run build.bat first to compile the project.
    echo.
    pause
    exit /b 1
)

REM Parse command line arguments or use defaults
set NUM_TRADERS=10
set DURATION=120
set INITIAL_PRICE=111.46
set INITIAL_CASH=5000.0
set INITIAL_SPEED=1.0

REM Check for command line arguments
if not "%1"=="" set NUM_TRADERS=%1
if not "%2"=="" set DURATION=%2
if not "%3"=="" set INITIAL_PRICE=%3
if not "%4"=="" set INITIAL_CASH=%4
if not "%5"=="" set INITIAL_SPEED=%5

echo Running simulation with parameters:
echo   Traders: %NUM_TRADERS%
echo   Duration: %DURATION% seconds
echo   Initial Price: $%INITIAL_PRICE%
echo   Initial Cash: $%INITIAL_CASH%
echo   Time Scale: %INITIAL_SPEED%x speed
echo.
echo Press Ctrl+C to stop early, or 'q' in the application
echo ================================================
echo.

REM Run the simulation
.\build\bin\Release\tradingSim.exe -t %NUM_TRADERS% -d %DURATION% -p %INITIAL_PRICE% -c %INITIAL_CASH% -s %INITIAL_SPEED%

echo.
echo ================================================
echo Simulation finished!
echo ================================================
