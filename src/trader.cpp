#include "../include/trader.hpp"
#include <cmath>
#include <algorithm>

Trader::Trader(int trader_id, Strategy strat, double initial_cash)
    : id(trader_id), strategy(strat), cash(initial_cash),
      holdings(0), total_profit(0), trades_executed(0),
      rng(std::random_device{}())
{
}

Trade Trader::makeDecision(double current_price, double timestamp)
{
    Trade trade;
    trade.trader_id = id;
    trade.price = current_price;
    trade.timestamp = timestamp;
    trade.quantity = 0;
    trade.is_buy = true;

    // Store price in history
    price_history.push_back(current_price);
    if (price_history.size() > 20)
    {
        price_history.erase(price_history.begin());
    }

    // Need at least some history for strategies
    if (price_history.size() < 5)
    {
        return trade;
    }

    bool should_buy = false;
    bool should_sell = false;

    switch (strategy)
    {
    case Strategy::MOMENTUM:
    {
        // Calculate recent trend
        double recent_avg = 0;
        double older_avg = 0;
        int half = price_history.size() / 2;

        for (int i = half; i < price_history.size(); i++)
        {
            recent_avg += price_history[i];
        }
        recent_avg /= (price_history.size() - half);

        for (int i = 0; i < half; i++)
        {
            older_avg += price_history[i];
        }
        older_avg /= half;

        // Buy if price is trending up
        if (recent_avg > older_avg * 1.02)
        {
            should_buy = true;
        }
        // Sell if price is trending down
        else if (recent_avg < older_avg * 0.98)
        {
            should_sell = true;
        }
        break;
    }

    case Strategy::MEAN_REVERSION:
    {
        // Calculate mean and deviation
        double mean = 0;
        for (double p : price_history)
        {
            mean += p;
        }
        mean /= price_history.size();

        // Buy if price is significantly below mean
        if (current_price < mean * 0.95)
        {
            should_buy = true;
        }
        // Sell if price is significantly above mean
        else if (current_price > mean * 1.05)
        {
            should_sell = true;
        }
        break;
    }

    case Strategy::RANDOM:
    {
        std::uniform_int_distribution<> dist(0, 10);
        int decision = dist(rng);
        should_buy = (decision == 1);
        should_sell = (decision == 2);
        break;
    }

    case Strategy::RISK_AVERSE:
    {
        // Conservative strategy - only trade when confident
        double mean = 0;
        for (double p : price_history)
        {
            mean += p;
        }
        mean /= price_history.size();

        // Only buy if price is significantly below mean (good deal)
        if (current_price < mean * 0.90)
        {
            should_buy = true;
        }
        // Only sell if price is significantly above mean (good profit)
        else if (current_price > mean * 1.10)
        {
            should_sell = true;
        }
        break;
    }

    case Strategy::HIGH_RISK:
    {
        // Aggressive strategy - trade frequently with larger positions
        double recent_avg = 0;
        int recent_count = std::min(3, static_cast<int>(price_history.size()));
        for (int i = price_history.size() - recent_count; i < price_history.size(); i++)
        {
            recent_avg += price_history[i];
        }
        recent_avg /= recent_count;

        // Buy on slight uptrend
        if (current_price > recent_avg * 1.01)
        {
            should_buy = true;
        }
        // Sell on slight downtrend
        else if (current_price < recent_avg * 0.99)
        {
            should_sell = true;
        }
        break;
    }
    }

    // Execute decision
    int trade_size = 10;

    // Adjust trade size based on strategy
    if (strategy == Strategy::RISK_AVERSE)
    {
        trade_size = 5; // Smaller positions for conservative traders
    }
    else if (strategy == Strategy::HIGH_RISK)
    {
        trade_size = 20; // Larger positions for aggressive traders
    }

    if (should_buy && cash >= current_price * trade_size)
    {
        trade.is_buy = true;
        trade.quantity = std::min(trade_size, static_cast<int>(cash / current_price));
    }
    else if (should_sell && holdings >= trade_size)
    {
        trade.is_buy = false;
        trade.quantity = std::min(trade_size, holdings);
    }

    return trade;
}

void Trader::executeTrade(const Trade &trade)
{
    if (trade.quantity == 0)
        return;

    if (trade.is_buy)
    {
        double cost = trade.price * trade.quantity;
        if (cash >= cost)
        {
            cash -= cost;
            holdings += trade.quantity;
            trades_executed++;
        }
    }
    else
    {
        if (holdings >= trade.quantity)
        {
            cash += trade.price * trade.quantity;
            holdings -= trade.quantity;
            trades_executed++;
        }
    }
}

std::string Trader::getStrategyName() const
{
    switch (strategy)
    {
    case Strategy::MOMENTUM:
        return "Momentum";
    case Strategy::MEAN_REVERSION:
        return "Mean Reversion";
    case Strategy::RANDOM:
        return "Random";
    case Strategy::RISK_AVERSE:
        return "Risk Averse";
    case Strategy::HIGH_RISK:
        return "High Risk";
    default:
        return "Unknown";
    }
}
