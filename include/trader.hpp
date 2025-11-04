#pragma once
#include <string>
#include <vector>
#include <random>

enum class Strategy
{
    MOMENTUM,       // Buy when price is rising, sell when falling
    MEAN_REVERSION, // Buy when price is low, sell when high
    RANDOM          // Random trading
};

struct Trade
{
    int trader_id;
    double price;
    int quantity;
    bool is_buy;
    double timestamp;
};

class Trader
{
private:
    int id;
    Strategy strategy;
    double cash;
    int holdings;
    double total_profit;
    int trades_executed;
    std::vector<double> price_history;
    std::mt19937 rng;

public:
    Trader(int trader_id, Strategy strat, double initial_cash);

    // Make trading decision based on current price and strategy
    Trade makeDecision(double current_price, double timestamp);

    // Execute a trade
    void executeTrade(const Trade &trade);

    // Getters
    int getId() const { return id; }
    Strategy getStrategy() const { return strategy; }
    double getCash() const { return cash; }
    int getHoldings() const { return holdings; }
    double getTotalProfit() const { return total_profit; }
    int getTradesExecuted() const { return trades_executed; }
    double getNetWorth(double current_price) const
    {
        return cash + holdings * current_price;
    }

    std::string getStrategyName() const;
};
