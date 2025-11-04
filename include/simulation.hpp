#pragma once
#include "trader.hpp"
#include "market.hpp"
#include "order_book.hpp"
#include "logger.hpp"
#include <vector>
#include <memory>
#include <mutex>

struct SimulationStats
{
    double total_volume;
    int total_trades;
    double avg_price;
    double price_volatility;
    double simulation_time;
    int pending_buy_orders;
    int pending_sell_orders;
    double best_bid;
    double best_ask;
    double spread;
};

class TradingSimulation
{
private:
    std::vector<std::unique_ptr<Trader>> traders;
    Market market;
    OrderBook order_book;
    DataLogger logger;
    std::vector<Trade> trade_log;
    std::mutex trade_mutex;

    double current_time;
    double time_step;

    bool mpi_enabled;
    int mpi_rank;
    int mpi_size;

public:
    TradingSimulation(int num_traders, double initial_price, double initial_cash);

    // Initialize with MPI support
    void initializeMPI(bool use_mpi = false, int rank = 0, int size = 1);

    // Set time scale (multiplier for simulation speed)
    void setTimeScale(double scale);

    // Run one simulation step (with order book matching)
    void step();

    // Get market data
    const Market &getMarket() const { return market; }

    // Get order book
    const OrderBook &getOrderBook() const { return order_book; }

    // Get traders
    const std::vector<std::unique_ptr<Trader>> &getTraders() const { return traders; }

    // Get current time
    double getCurrentTime() const { return current_time; }

    // Get simulation statistics
    SimulationStats getStats() const;

    // Get trade log
    const std::vector<Trade> &getTradeLog() const { return trade_log; }

    // Get logger
    DataLogger &getLogger() { return logger; }
};
