# Algorithmic Trading Simulator

A C++ TUI (Terminal User Interface) program that simulates an algorithmic trading environment with multiple trader agents using different strategies. The simulation leverages **OpenMP** for parallel processing to enable concurrent trader decision-making.

## Features

✨ **Multi-Agent Trading System**

- Multiple trader agents with five different strategies:
  - **Momentum Trading**: Buys when prices are trending up, sells when trending down
  - **Mean Reversion**: Buys when prices are below average, sells when above average
  - **Random Trading**: Makes random trading decisions for baseline comparison
  - **Risk Averse**: Conservative strategy with smaller positions, only trades on strong signals
  - **High Risk**: Aggressive strategy with larger positions, trades frequently on minor trends

🚀 **Parallel Processing**

- Uses OpenMP to parallelize trader decision-making
- Efficient concurrent execution of trading strategies

📊 **Real-Time Visualization**

- Beautiful TUI built with FTXUI library
- Live price charts with htop-style graphs (10 lines tall)
- Color-coded price bars (green for rising, red for falling, cyan for stable)
- Real-time price change percentage indicator with up/down arrows
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
- Detailed final statistics sorted by trader ID
- Maximum profit and maximum loss tracking across all traders

## Requirements

- C++17 compatible compiler
- CMake 3.11 or higher
- OpenMP support (usually included with compiler)
- Windows/Linux/macOS

## Building the Project

### Quick Start (Windows)

**Build the project:**

```cmd
build.bat
```

**Run the simulation:**

```cmd
# Run with default parameters
run.bat

# Run with custom parameters (traders, duration, price, cash)
run.bat 20 120 150.0 15000.0
```

### Manual Build

```cmd
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --config Release

# Run
cd ..
.\build\bin\Release\tradingSim.exe
```

## Usage

### Command Line Options

```
Usage: tradingSim [options]

Options:
  -t, --traders <num>     Number of trader agents (default: 12)
  -d, --duration <sec>    Simulation duration in seconds (default: 60)
  -p, --price <value>     Initial asset price (default: 100.0)
  -c, --cash <value>      Initial cash per trader (default: 10000.0)
  -h, --help              Show help message

Examples:
  tradingSim -t 20 -d 120 -p 150.0
  tradingSim --traders 30 --duration 180
```

### During Simulation

- **Press 'q'**: Stop the simulation and view final results
- The simulation automatically stops after the specified duration
- Live updates show:
  - Current market price with percentage change (▲ green for up, ▼ red for down)
  - Tall historical price graph (10 lines) with color-coded bars
  - Top 5 traders by net worth
  - Market statistics (total trades, volume, volatility)
  - Time remaining

### After Simulation

The program displays detailed final results including:

- Simulation duration and statistics
- Complete trader rankings **sorted by trader ID** (ascending order)
- Individual trader performance:
  - Net worth
  - Profit/Loss
  - Number of trades executed
  - Current holdings and cash
- **Performance Summary**:
  - Maximum profit earned by any trader (with strategy name)
  - Maximum loss incurred by any trader (with strategy name)

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
├── build.bat              # Build script (compiles project)
├── run.bat                # Run script (executes simulation)
└── README.md
```

## How It Works

1. **Initialization**: The simulation creates N trader agents with different strategies and initializes the market with a starting price.

2. **Simulation Loop**: Every 100ms:

   - All traders make decisions based on current price and their strategy (parallelized with OpenMP)
   - Market updates prices based on buy/sell pressure
   - UI refreshes to show current state

3. **Trading Strategies**:

   - **Momentum**: Analyzes recent vs older price averages, buys on uptrends (standard 10-share positions)
   - **Mean Reversion**: Compares current price to historical mean, exploits deviations (standard 10-share positions)
   - **Random**: Makes random buy/sell decisions for baseline comparison (standard 10-share positions)
   - **Risk Averse**: Conservative approach, only trades on strong 10% deviations, uses smaller 5-share positions
   - **High Risk**: Aggressive approach, trades on 1% price movements, uses larger 20-share positions

4. **Market Dynamics**: Price changes based on:

   - Net buy/sell pressure from traders
   - Random market noise for volatility
   - Price bounds to prevent unrealistic values

## Performance Notes

- OpenMP parallelizes trader decision-making across available CPU cores
- UI refresh rate: 50ms (20 FPS)
- Simulation step rate: 100ms (10 steps/second)
- Graph displays last 60 data points with 10-line height
- Color-coded bars: Green (price rising), Red (price falling), Cyan (stable)

## License

This project uses the FTXUI library (MIT License) for the terminal interface.

## Future Enhancements (Not Implemented)

The original specification included:

- MPI for distributed computing across multiple nodes
- More complex trading strategies
- Extended data logging to files
- Multi-node batch scripts

These features were intentionally kept simple for this implementation.
