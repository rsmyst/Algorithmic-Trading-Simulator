# Algorithmic Trading Simulation - PowerShell Build and Run Script
# This script sets up the environment, compiles, and runs the simulation

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Algorithmic Trading Simulation" -ForegroundColor Cyan
Write-Host "Using OpenMP for Parallel Processing" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Set build configuration
$BUILD_TYPE = "Release"
$BUILD_DIR = "build"

# Check if build directory exists
if (-not (Test-Path $BUILD_DIR)) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
}

# Navigate to build directory
Push-Location $BUILD_DIR

# Configure with CMake
Write-Host ""
Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Configuring project with CMake..." -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
cmake ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Pop-Location
    Read-Host "Press Enter to exit"
    exit 1
}

# Build the project
Write-Host ""
Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Building project..." -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
cmake --build . --config $BUILD_TYPE
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Pop-Location
    Read-Host "Press Enter to exit"
    exit 1
}

# Return to root directory
Pop-Location

Write-Host ""
Write-Host "================================================" -ForegroundColor Green
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host ""

# Parse command line arguments or use defaults
param(
    [int]$Traders = 12,
    [int]$Duration = 60,
    [double]$Price = 100.0,
    [double]$Cash = 10000.0
)

Write-Host "Running simulation with parameters:" -ForegroundColor Yellow
Write-Host "  Traders: $Traders" -ForegroundColor White
Write-Host "  Duration: $Duration seconds" -ForegroundColor White
Write-Host "  Initial Price: `$$Price" -ForegroundColor White
Write-Host "  Initial Cash: `$$Cash" -ForegroundColor White
Write-Host ""
Write-Host "Press Ctrl+C to stop early, or 'q' in the application" -ForegroundColor Yellow
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Run the simulation
& ".\build\bin\Release\ftxui_demo.exe" -t $Traders -d $Duration -p $Price -c $Cash

Write-Host ""
Write-Host "================================================" -ForegroundColor Green
Write-Host "Simulation finished!" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Read-Host "Press Enter to exit"
