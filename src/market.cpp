#include "../include/market.hpp"
#include <cmath>
#include <algorithm>

Market::Market(double initial_price)
    : current_price(initial_price), previous_price(initial_price), base_price(initial_price),
      rng(std::random_device{}()), noise_dist(-0.5, 0.5),
      buy_pressure(0), sell_pressure(0)
{
    price_history.push_back(initial_price);
}

void Market::updatePrice(int buy_orders, int sell_orders)
{
    previous_price = current_price;

    buy_pressure += buy_orders;
    sell_pressure += sell_orders;

    double pressure_diff = (buy_pressure - sell_pressure) * 0.1;
    double noise = noise_dist(rng);
    double price_change = pressure_diff + noise;
    current_price += price_change;
    current_price = std::max(base_price * 0.2, std::min(current_price, base_price * 3.0));

    price_history.push_back(current_price);

    if (price_history.size() > 1000)
    {
        price_history.erase(price_history.begin());
    }

    buy_pressure = static_cast<int>(buy_pressure * 0.8);
    sell_pressure = static_cast<int>(sell_pressure * 0.8);
}

std::vector<double> Market::getRecentHistory(int points) const
{
    if (price_history.size() <= points)
    {
        return price_history;
    }

    return std::vector<double>(
        price_history.end() - points,
        price_history.end());
}
