Algorithmic Trading Simulator

A high-performance C++ simulator that models an algorithmic trading environment. This project features a limit order book, multiple agent-based strategies using advanced technical indicators, and a dual-parallel architecture. It supports multi-threading with OpenMP for intra-simulation speedup and distributed computing with MPI for large-scale ensemble analysis.

A key feature is its hybrid-mode operation: it can be run as a real-time interactive Terminal User Interface (TUI) allowing a human to trade against the AI, or as a headless "ensemble" simulation for statistical backtesting.

Features

Simulation Modes

Interactive TUI Mode: A real-time, graphical dashboard built with FTXUI. It visualizes the price graph, order book depth, and market statistics as the simulation runs.

Human-in-the-Loop (HIL): In TUI mode, Trader 0 is reserved for the human player. The UI provides dedicated input boxes and buttons for placing buy/sell orders in real-time to compete directly against the AI agents.

Ensemble (Headless) Mode: A powerful statistical analysis mode. Using the -E flag, the simulator can run hundreds or thousands of simulations headlessly. It leverages MPI to distribute these runs across multiple processes, then aggregates the results to provide a statistical summary of strategy performance.

Core Engine

Limit Order Book: Full implementation of a central limit order book (CLOB) with buy/sell orders at specific price levels.

Price-Time Priority Matching: A high-fidelity matching engine that correctly implements the standard exchange algorithm (best price wins, first-in-first-out for ties).

Advanced Trading Strategies: A diverse ecosystem of autonomous agents. Trader 0 is reserved as the Human Player, while AI agents are assigned one of nine strategies:

Momentum

Mean Reversion

Random (for baseline comparison)

Risk Averse (smaller positions)

High Risk (larger positions)

RSI-Based

MACD-Based

Bollinger Bands

Multi-Indicator (confirmatory signals)

Parallelism & Performance

OpenMP (Intra-Simulation): Multi-threaded parallelism used within a single simulation to accelerate computationally heavy tasks, including trader decision-making and technical indicator calculations.

MPI (Inter-Simulation): Distributed computing used to execute the "Ensemble Mode." N simulation runs are automatically distributed across P available processes, and final results are aggregated via MPI_Reduce.

Technical Indicators: Parallel computation of SMA, EMA, RSI, MACD, and Bollinger Bands using OpenMP.

Data Logging: Comprehensive, thread-safe I/O for logging all simulation activity to CSV files (trades, prices, trader_stats, order_book) and a final JSON summary.

Requirements

C++17 compatible compiler

CMake (3.11 or higher)

OpenMP support (included with most modern C++ compilers)

An MPI implementation (e.g., OpenMPI on Linux/macOS, MS-MPI on Windows)

Building the Project

Quick Start (Windows)

# Build
build.bat

# Run Interactive TUI Mode
run.bat

# Run with custom parameters (traders, duration, initial_price)
run.bat 20 120 150.0


Manual Build (Linux/macOS)

# Create build directory
mkdir build
cd build

# Configure with CMake (this will find OpenMP and MPI)
cmake ..

# Build in Release mode using all available cores
cmake --build . --config Release -j

# Run the TUI mode
./bin/tradingSim


Usage

Command Line Options

Usage: tradingSim [options]

Options:
  -t, --traders <num>     Number of trader agents (default: 12)
  -d, --duration <sec>    Simulation duration in seconds (default: 60)
  -p, --price <value>     Initial asset price (default: 170.0)
  -c, --cash <value>      Initial cash per trader (default: 10000.0)
  -s, --speed <scale>     Time scale multiplier for TUI mode (default: 1.0)
  -h, --help              Show this help message

Ensemble Mode Options:
  -E, --ensemble <N>      Run N simulations headlessly (disables TUI)
  --seed <S>              Base seed for ensemble runs (default: 12345)


Running the Simulator

1. Interactive TUI Mode

To run the standard simulation with the interactive dashboard, execute the program directly:

./bin/tradingSim -t 20 -d 120 -p 150.0


Controls:

Press 'q': Stop simulation and view final results.

Human Player (Trader 0): Use the Price and Qty input boxes and the BUY / SELL buttons to place your own trades in real-time. Your performance is tracked in the "Human Control" panel.

2. Ensemble (Headless) Mode

To run a statistical analysis, use mpiexec. This example runs 100 simulations across 4 processes:

mpiexec -n 4 ./bin/tradingSim -E 100 --seed 42 -d 30


This will not show a TUI. It will print status messages to the console and end with an "Ensemble Summary" of the aggregated results from all 100 runs.

After Simulation

Final results are printed to the console, including:

Complete simulation statistics (total trades, volume, etc.)

Final trader rankings sorted by ID, showing individual profit/loss.

Data exported to the logs/ directory for analysis.

Technical Architecture

Parallelization Strategy

OpenMP (Intra-Simulation):
Used for multi-threading within a single simulation. The primary use is in the step() function's order generation phase (#pragma omp parallel for), allowing all AI agents to "think" concurrently. It is also used in the TechnicalIndicators class to speed up calculations for RSI, MACD, etc.

MPI (Inter-Simulation):
Used for inter-simulation parallelism in Ensemble Mode. mpiexec launches P processes. Each process (or "rank") calculates its share of the N total simulations, runs them headlessly by calling runHeadless(), and then reports its aggregated results (e.g., local_total_trades) back to Rank 0 via MPI_Reduce.

Project Structure

Algorithmic-Trading-Simulator/
├── include/
│   ├── trader.hpp          # Trader agents & technical indicators
│   ├── market.hpp          # Market simulation & dynamics
│   ├── order_book.hpp      # Limit order book & matching engine
│   ├── logger.hpp          # Data logging system
│   └── simulation.hpp      # Main simulation controller
├── src/
│   ├── main.cpp            # Main entry, TUI, and MPI logic
│   ├── trader.cpp          # Trader & indicator implementation
│   ├── market.cpp          # Market implementation
│   ├── order_book.cpp      # Order book implementation
│   ├── logger.cpp          # Logging implementation
│   └── simulation.cpp      # Simulation `step()` implementation
├── logs/                   # Generated during simulation
├── CMakeLists.txt          # CMake configuration
├── build.bat               # Windows build script
├── run.bat                 # Windows run script
└── README.md


How It Works

Initialization:

main.cpp initializes MPI and parses command-line args.

If in TUI mode, TradingSimulation is created.

The constructor creates N traders. Trader 0 is assigned Strategy::HUMAN.

All AI traders (1 to N) are assigned a strategy (Momentum, RSI, etc.) and a unique random seed.

Half the traders receive initial holdings (50 shares) to provide market liquidity.

Simulation Loop (TUI Mode):

main.cpp runs a render loop that refreshes the screen ~20 FPS.

Every 100ms, it calls simulation.step().

Inside step(), all AI traders (1 to N) generate orders in parallel (OpenMP). Trader 0 is skipped.

Human orders (if any were submitted) are injected via addHumanOrder().

All orders are added to the OrderBook.

order_book.matchOrders() executes trades based on price-time priority.

Trader cash/holdings are updated.

market.updatePrice() adjusts the price based on order pressure.

The TUI refreshes with the new state.

Simulation Loop (Ensemble Mode):

main.cpp divides the N simulations among P processes.

Each process loops, creating a new TradingSimulation object for each run (with a unique seed).

It calls simulation.runHeadless(), which runs the step() function in a tight loop at maximum speed.

The final SimulationStats are returned and aggregated.

Future Enhancements

GPU Acceleration: Porting technical indicator calculations to CUDA.

Machine Learning: Implementing adaptive strategies using ML frameworks.

Network Latency: Simulating realistic network delay for HFT strategies.

Historical Backtesting: Modifying the Market module to read real historical trade data instead of generating random noise.

References

Order Book: Classic limit order book with price-time priority.

Technical Indicators: Standard definitions (e.g., J. W. Wilder's New Concepts in Technical Trading Systems).

OpenMP: OpenMP API Specification.

MPI: MPI-3.1 Standard.

FTXUI: A functional terminal user interface library (MIT License).

Version: 3.0.0

Date: November 9, 2025