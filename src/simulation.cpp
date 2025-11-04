#include "../include/simulation.hpp"
#include <omp.h>
#include <algorithm>
#include <numeric>

TradingSimulation::TradingSimulation(int num_traders, double initial_price, double initial_cash)
    : market(initial_price), current_time(0.0), time_step(0.1)
{

    // Create traders with different strategies
    for (int i = 0; i < num_traders; i++)
    {
        Strategy strat;
        int strategy_type = i % 5;
        if (strategy_type == 0)
            strat = Strategy::MOMENTUM;
        else if (strategy_type == 1)
            strat = Strategy::MEAN_REVERSION;
        else if (strategy_type == 2)
            strat = Strategy::RANDOM;
        else if (strategy_type == 3)
            strat = Strategy::RISK_AVERSE;
        else
            strat = Strategy::HIGH_RISK;

        traders.push_back(std::make_unique<Trader>(i, strat, initial_cash));
    }
}

void TradingSimulation::step()
{
    current_time += time_step;

    std::vector<Trade> current_trades;
    int total_buy_quantity = 0;
    int total_sell_quantity = 0;

    double current_price = market.getCurrentPrice();

// Parallel trader decision making with OpenMP
#pragma omp parallel
    {
        std::vector<Trade> local_trades;
        int local_buy = 0;
        int local_sell = 0;

#pragma omp for nowait
        for (int i = 0; i < traders.size(); i++)
        {
            Trade trade = traders[i]->makeDecision(current_price, current_time);

            if (trade.quantity > 0)
            {
                local_trades.push_back(trade);
                if (trade.is_buy)
                {
                    local_buy += trade.quantity;
                }
                else
                {
                    local_sell += trade.quantity;
                }
            }
        }

// Combine results from all threads
#pragma omp critical
        {
            current_trades.insert(current_trades.end(), local_trades.begin(), local_trades.end());
            total_buy_quantity += local_buy;
            total_sell_quantity += local_sell;
        }
    }

    // Update market based on trading activity
    market.updatePrice(total_buy_quantity, total_sell_quantity);

    // Execute trades
    for (const auto &trade : current_trades)
    {
        traders[trade.trader_id]->executeTrade(trade);

        std::lock_guard<std::mutex> lock(trade_mutex);
        trade_log.push_back(trade);
    }
}

SimulationStats TradingSimulation::getStats() const
{
    SimulationStats stats;
    stats.simulation_time = current_time;
    stats.total_trades = trade_log.size();

    if (trade_log.empty())
    {
        stats.total_volume = 0;
        stats.avg_price = market.getCurrentPrice();
        stats.price_volatility = 0;
        return stats;
    }

    // Calculate total volume
    stats.total_volume = 0;
    double sum_price = 0;
    for (const auto &trade : trade_log)
    {
        stats.total_volume += trade.quantity * trade.price;
        sum_price += trade.price;
    }

    stats.avg_price = sum_price / trade_log.size();

    // Calculate price volatility
    const auto &history = market.getPriceHistory();
    if (history.size() > 1)
    {
        double mean = std::accumulate(history.begin(), history.end(), 0.0) / history.size();
        double sq_sum = 0;
        for (double price : history)
        {
            sq_sum += (price - mean) * (price - mean);
        }
        stats.price_volatility = std::sqrt(sq_sum / history.size());
    }
    else
    {
        stats.price_volatility = 0;
    }

    return stats;
}
