#pragma once
#include <vector>
#include <random>
#include <deque>

class Market
{
private:
    double current_price;
    double base_price;
    std::vector<double> price_history;
    std::mt19937 rng;
    std::uniform_real_distribution<> noise_dist;

    int buy_pressure;
    int sell_pressure;

public:
    Market(double initial_price);

    // Update price based on market dynamics and trading activity
    void updatePrice(int buy_orders, int sell_orders);

    // Get current price
    double getCurrentPrice() const { return current_price; }

    // Get price history
    const std::vector<double> &getPriceHistory() const { return price_history; }

    // Get recent price history (last N points)
    std::vector<double> getRecentHistory(int points) const;

    // Reset market pressures
    void resetPressures()
    {
        buy_pressure = 0;
        sell_pressure = 0;
    }
};
