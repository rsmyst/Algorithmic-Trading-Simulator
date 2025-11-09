#pragma once
#include <string>
#include <vector>
#include <random>

enum class Strategy
{   
    HUMAN,
    MOMENTUM,       // Buy when price is rising, sell when falling
    MEAN_REVERSION, // Buy when price is low, sell when high
    RANDOM,         // Random trading
    RISK_AVERSE,    // Conservative trading, small positions
    HIGH_RISK,      // Aggressive trading, large positions
    RSI_BASED,      // Trade based on RSI indicator
    MACD_BASED,     // Trade based on MACD indicator
    BOLLINGER,      // Trade based on Bollinger Bands
    MULTI_INDICATOR // Combination of multiple indicators
};

// Technical Indicators class for parallel computation
class TechnicalIndicators
{
public:
    // Calculate Simple Moving Average
    static double calculateSMA(const std::vector<double> &prices, int period);

    // Calculate Exponential Moving Average
    static double calculateEMA(const std::vector<double> &prices, int period);

    // Calculate RSI (Relative Strength Index)
    static double calculateRSI(const std::vector<double> &prices, int period = 14);

    // Calculate MACD (Moving Average Convergence Divergence)
    // Returns {MACD line, Signal line, Histogram}
    static std::tuple<double, double, double> calculateMACD(
        const std::vector<double> &prices,
        int fast_period = 12,
        int slow_period = 26,
        int signal_period = 9);

    // Calculate Bollinger Bands
    // Returns {Upper band, Middle band, Lower band}
    static std::tuple<double, double, double> calculateBollingerBands(
        const std::vector<double> &prices,
        int period = 20,
        double std_dev = 2.0);

    // Parallel computation of multiple indicators
    static void calculateAllIndicators(
        const std::vector<double> &prices,
        double &rsi,
        std::tuple<double, double, double> &macd,
        std::tuple<double, double, double> &bollinger);
};

struct Trade
{
    int trader_id;
    double price;
    int quantity;
    bool is_buy;
    double timestamp;
};

struct TraderOrder
{
    int trader_id;
    bool is_buy;
    double price;
    int quantity;
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
    Trader(int trader_id, Strategy strat, double initial_cash, unsigned int seed);

    // Make trading decision based on current price and strategy
    Trade makeDecision(double current_price, double timestamp);

    // Create limit order based on strategy
    TraderOrder createOrder(double current_price, double timestamp);

    // Execute a trade
    void executeTrade(const Trade &trade);

    // Execute order (for order book)
    void executeOrder(bool is_buy, double price, int quantity);

    // Give initial holdings
    void setInitialHoldings(int initial_holdings) { holdings = initial_holdings; }

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

    // Get recent technical indicators (cached)
    double getLastRSI() const { return last_rsi; }
    double getLastMACD() const { return last_macd; }

private:
    // Technical indicator calculations (with caching)
    void updateIndicators();
    double last_rsi;
    double last_macd;
    double last_bollinger_upper;
    double last_bollinger_lower;
};
