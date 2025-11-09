@echo off
ECHO Starting simulation in new window...

:: This command launches the executable in a new command prompt window.
:: We check for the "Release" version first, which is the optimized one.
IF EXIST .\build\bin\Release\tradingSim.exe (
    start "Trading Simulator" .\build\bin\Release\tradingSim.exe
) ELSE (
    ECHO Could not find Release build. Trying Debug...
    start "Trading Simulator" .\build\bin\Debug\tradingSim.exe
)
