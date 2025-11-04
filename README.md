# Algorithmic Trading Simulation

A C++ TUI (Terminal User Interface) program that simulates an algorithmic trading environment with multiple trader agents using different strategies. The simulation leverages **OpenMP** for parallel processing to enable concurrent trader decision-making.

## Features

✨ **Multi-Agent Trading System**

- Multiple trader agents with different strategies:
  - **Momentum Trading**: Buys when prices are trending up, sells when trending down
  - **Mean Reversion**: Buys when prices are below average, sells when above average
  - **Random Trading**: Makes random trading decisions

🚀 **Parallel Processing**

- Uses OpenMP to parallelize trader decision-making
- Efficient concurrent execution of trading strategies

📊 **Real-Time Visualization**

- Beautiful TUI built with FTXUI library
- Live price charts with htop-style graphs
- Real-time trader statistics and rankings
- Market statistics and analytics

💰 **Market Simulation**

- Dynamic price updates based on supply and demand
- Market responds to trader actions
- Realistic price volatility

📈 **Performance Metrics**

- Net worth tracking for each trader
- Profit/Loss calculations
- Trade execution counts
- Final rankings and detailed statistics

## Requirements

- C++17 compatible compiler
- CMake 3.11 or higher
- OpenMP support (usually included with compiler)
- Windows/Linux/macOS

## Building the Project

### Using PowerShell (Windows)

```powershell
# Build and run with default parameters
.\run_simulation.ps1

# Build and run with custom parameters
.\run_simulation.ps1 -Traders 20 -Duration 120 -Price 150.0 -Cash 15000.0
```

### Using Batch Script (Windows)

```cmd
# Build and run with default parameters
run_simulation.bat

# Build and run with custom parameters (traders, duration, price, cash)
run_simulation.bat 20 120 150.0 15000.0
```

### Manual Build

```powershell
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --config Release

# Run
cd ..
.\build\bin\Release\ftxui_demo.exe
```

## Usage

### Command Line Options

```
Usage: ftxui_demo [options]

Options:
  -t, --traders <num>     Number of trader agents (default: 12)
  -d, --duration <sec>    Simulation duration in seconds (default: 60)
  -p, --price <value>     Initial asset price (default: 100.0)
  -c, --cash <value>      Initial cash per trader (default: 10000.0)
  -h, --help              Show help message

Examples:
  ftxui_demo -t 20 -d 120 -p 150.0
  ftxui_demo --traders 30 --duration 180
```

### During Simulation

- **Press 'q'**: Stop the simulation and view final results
- The simulation automatically stops after the specified duration
- Live updates show:
  - Current market price with historical graph
  - Top 5 traders by net worth
  - Market statistics (total trades, volume, volatility)
  - Time remaining

### After Simulation

The program displays detailed final results including:

- Simulation duration and statistics
- Complete trader rankings
- Individual trader performance:
  - Net worth
  - Profit/Loss
  - Number of trades executed
  - Current holdings and cash

## Project Structure

```
Alg Trad/
├── include/
│   ├── trader.hpp          # Trader agent definitions
│   ├── market.hpp          # Market simulation
│   └── simulation.hpp      # Main simulation controller
├── src/
│   ├── main.cpp           # TUI and main program
│   ├── trader.cpp         # Trader implementation
│   ├── market.cpp         # Market implementation
│   └── simulation.cpp     # Simulation implementation
├── CMakeLists.txt         # CMake configuration
├── run_simulation.ps1     # PowerShell build/run script
├── run_simulation.bat     # Batch build/run script
└── README.md
```

## How It Works

1. **Initialization**: The simulation creates N trader agents with different strategies and initializes the market with a starting price.

2. **Simulation Loop**: Every 100ms:

   - All traders make decisions based on current price and their strategy (parallelized with OpenMP)
   - Market updates prices based on buy/sell pressure
   - UI refreshes to show current state

3. **Trading Strategies**:

   - **Momentum**: Analyzes recent vs older price averages, buys on uptrends
   - **Mean Reversion**: Compares current price to historical mean, exploits deviations
   - **Random**: Makes random buy/sell decisions for baseline comparison

4. **Market Dynamics**: Price changes based on:
   - Net buy/sell pressure from traders
   - Random market noise for volatility
   - Price bounds to prevent unrealistic values

## Performance Notes

- OpenMP parallelizes trader decision-making across available CPU cores
- UI refresh rate: 50ms (20 FPS)
- Simulation step rate: 100ms (10 steps/second)
- Graph displays last 60 data points

## License

This project uses the FTXUI library (MIT License) for the terminal interface.

## Future Enhancements (Not Implemented)

The original specification included:

- MPI for distributed computing across multiple nodes
- More complex trading strategies
- Extended data logging to files
- Multi-node batch scripts

These features were intentionally kept simple for this implementation.
