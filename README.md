# Algorithmic Trading Simulator

A high-performance C++ TUI (Terminal User Interface) program that simulates an algorithmic trading environment with multiple trader agents using different strategies. Features limit order book, advanced technical indicators, parallel processing with OpenMP, and comprehensive data logging.

## Features

### 🏦 **Limit Order Book**

- Full order book implementation with buy/sell orders at specific prices
- Price-time priority matching algorithm
- Real-time bid/ask spread calculation
- Order book depth visualization (top 5 levels)
- Parallel order matching using OpenMP

### 📊 **Advanced Trading Strategies**

Nine different trading strategies with technical indicators:

- **Momentum**: Buys when price is trending up, sells when trending down
- **Mean Reversion**: Buys when price is below average, sells when above
- **Random**: Random trading for baseline comparison
- **Risk Averse**: Conservative with smaller positions (5 shares)
- **High Risk**: Aggressive with larger positions (20 shares)
- **RSI-Based**: Trades on Relative Strength Index (oversold < 30, overbought > 70)
- **MACD-Based**: Uses Moving Average Convergence Divergence
- **Bollinger Bands**: Trades at upper/lower band touches
- **Multi-Indicator**: Combines RSI, MACD, and Bollinger Bands (requires 2+ signals)

### 🔬 **Technical Indicators**

All with parallel OpenMP computation:

- **SMA** (Simple Moving Average) with parallel sum reduction
- **EMA** (Exponential Moving Average)
- **RSI** (Relative Strength Index) with parallel gain/loss calculation
- **MACD** with parallel fast/slow EMA computation
- **Bollinger Bands** with parallel standard deviation calculation

### 🚀 **Parallel Processing**

- **OpenMP** parallelization for:
  - Trader decision-making
  - Order matching within price levels
  - Technical indicator calculations
  - Data logging
- **MPI** framework (optional) for distributed order book across nodes

### � **Real-Time Visualization**

- Beautiful TUI built with FTXUI library
- Dynamic price graph with initial price baseline
- Color-coded price movements (green for up, red for down)
- Real-time trader rankings
- Order book depth display (side-by-side with statistics)
- Market statistics and analytics

### � **Data Logging**

Comprehensive parallel I/O logging to CSV files:

- `trades.csv`: All executed trades with timestamps, prices, quantities
- `prices.csv`: Price history with volume and order counts
- `trader_stats.csv`: Net worth, profit/loss, trades, RSI, MACD per trader
- `order_book.csv`: Order book depth snapshots
- `simulation_summary.json`: Summary statistics with metadata

## Requirements

- **C++17** compatible compiler
- **CMake** 3.11 or higher
- **OpenMP** support (included with most compilers)
- **MPI** (optional - MS-MPI for Windows, OpenMPI for Linux)
- Windows/Linux/macOS

## Building the Project

### Quick Start (Windows)

```cmd
# Build
build.bat

# Run with defaults (12 traders, 60 seconds)
run.bat

# Run with custom parameters (traders, duration, initial_price, initial_cash)
run.bat 20 120 150.0 15000.0
```

### Manual Build

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build Release version
cmake --build . --config Release

# Run
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

- **Press 'q'**: Stop simulation and view final results
- Auto-stops after specified duration
- Live UI displays:
  - Current market price with percentage change indicator
  - Historical price graph with dynamic baseline (initial price)
  - Order book depth (buy/sell sides)
  - Top 5 traders by net worth
  - Market statistics (trades, volume, volatility, pending orders)
  - Time remaining

### After Simulation

Final results include:

- Complete simulation statistics
- Trader rankings sorted by ID
- Individual performance metrics (net worth, profit/loss, trades)
- Performance summary (max profit/loss with strategies)
- Data exported to `logs/` directory

## Technical Architecture

### Parallelization Strategy

**OpenMP (Intra-node):**

1. Order creation: `#pragma omp parallel for` in simulation step
2. Order matching: Dynamic scheduling within price levels
3. Technical indicators: `#pragma omp parallel sections`
4. Data logging: Parallel trader stats writing

**MPI (Multi-node - Framework):**

- Order book distribution capability
- Rank-specific logging
- Aggregation framework ready for multi-node deployment

### Performance Characteristics

**Scalability:**

- OpenMP scales with CPU cores
- Order book: O(log n) insertion, parallel matching
- Technical indicators: Parallel computation reduces latency
- I/O: Buffered writes with batch processing

**Bottlenecks:**

- Order book matching requires synchronization (mutex locks)
- Technical indicator calculation scales with history size
- File I/O with high trade volumes

## Project Structure

```
Alg Trad/
├── include/
│   ├── trader.hpp          # Trader agents & technical indicators
│   ├── market.hpp          # Market simulation & dynamics
│   ├── order.hpp           # Order & trade structures
│   ├── order_book.hpp      # Limit order book
│   ├── logger.hpp          # Data logging system
│   └── simulation.hpp      # Main simulation controller
├── src/
│   ├── main.cpp            # TUI and main program
│   ├── trader.cpp          # Trader & indicator implementation
│   ├── market.cpp          # Market implementation
│   ├── order_book.cpp      # Order book implementation
│   ├── logger.cpp          # Logging implementation
│   └── simulation.cpp      # Simulation implementation
├── logs/                   # Generated during simulation
├── CMakeLists.txt          # CMake configuration
├── build.bat               # Windows build script
├── run.bat                 # Windows run script
└── README.md
```

## How It Works

1. **Initialization**

   - Creates N trader agents with diverse strategies
   - Initializes market with starting price
   - Half the traders receive initial holdings (50 shares) for market liquidity

2. **Simulation Loop** (every 100ms)

   - All traders analyze market and create orders (parallelized with OpenMP)
   - Orders added to limit order book
   - Order matching engine processes trades
   - Market updates prices based on supply/demand
   - UI refreshes with current state
   - Data logged periodically

3. **Order Matching**

   - Price-time priority algorithm
   - Buy orders at limit price ≥ sell price execute
   - Parallel matching within price levels
   - Automatic cleanup of filled orders

4. **Technical Analysis**

   - Indicators calculated in parallel for advanced strategies
   - Cached values to reduce redundant calculations
   - Real-time RSI, MACD displayed for relevant traders

5. **Price Dynamics**
   - Updates based on net buy/sell pressure
   - Random market noise for volatility
   - Bounded to prevent unrealistic values
   - Dynamic scaling on price graph

## Strategy Distribution

With default 12 traders, strategies cycle through:

- 1x Momentum, 1x Mean Reversion, 1x Random
- 1x Risk Averse, 1x High Risk
- 2x RSI-Based, 2x MACD-Based
- 1x Bollinger Bands, 1x Multi-Indicator
- (Pattern repeats for additional traders)

## Output Files

After simulation, check `logs/` directory:

```
logs/
  ├── trades.csv              # All executed trades
  ├── prices.csv              # Price history with volume
  ├── trader_stats.csv        # Per-trader performance & indicators
  ├── order_book.csv          # Order book depth snapshots
  └── simulation_summary.json # Summary with metadata
```

## Performance Notes

- OpenMP parallelizes across available CPU cores
- UI refresh rate: 50ms (20 FPS)
- Simulation step rate: 100ms (10 steps/second)
- Graph displays last 200 data points with 15-line height
- Initial price shown as dynamic horizontal baseline
- Order matching uses 0.5% price tolerance for liquidity

## Future Enhancements

1. **Full MPI Implementation**: Distribute order book across nodes
2. **GPU Acceleration**: CUDA for technical indicator calculations
3. **Real-time Analytics**: Live dashboard with streaming data
4. **Machine Learning**: ML-based adaptive strategies
5. **Network Latency**: Simulate realistic market conditions
6. **Historical Backtesting**: Test strategies on real market data

## References

- **Order Book**: Classic limit order book with price-time priority
- **Technical Indicators**: Standard definitions (Investopedia)
- **OpenMP**: OpenMP 4.5 specification
- **MPI**: MPI-3.1 standard
- **FTXUI**: Terminal UI library (MIT License)

## License

See LICENSE file for details.

---

**Version**: 2.0.0  
**Date**: November 4, 2025  
**Build Status**: ✅ Successful
