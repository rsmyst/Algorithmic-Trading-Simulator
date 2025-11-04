#pragma once
#include "trader.hpp"
#include "market.hpp"
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
};

class TradingSimulation
{
private:
    std::vector<std::unique_ptr<Trader>> traders;
    Market market;
    std::vector<Trade> trade_log;
    std::mutex trade_mutex;

    double current_time;
    double time_step;

public:
    TradingSimulation(int num_traders, double initial_price, double initial_cash);

    // Run one simulation step
    void step();

    // Get market data
    const Market &getMarket() const { return market; }

    // Get traders
    const std::vector<std::unique_ptr<Trader>> &getTraders() const { return traders; }

    // Get current time
    double getCurrentTime() const { return current_time; }

    // Get simulation statistics
    SimulationStats getStats() const;

    // Get trade log
    const std::vector<Trade> &getTradeLog() const { return trade_log; }
};
