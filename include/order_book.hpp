#pragma once
#include "order.hpp"
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>

class OrderBook
{
private:
    // Price-level order books (price -> list of orders)
    std::map<double, std::vector<Order>, std::greater<double>> buy_orders; // Descending (highest first)
    std::map<double, std::vector<Order>> sell_orders;                      // Ascending (lowest first)

    std::vector<ExecutedTrade> executed_trades;
    std::mutex book_mutex;

    int next_order_id;
    int next_trade_id;

public:
    OrderBook();

    // Add order to book
    int addOrder(Order order);

    // Match orders and execute trades (parallelized with OpenMP)
    std::vector<ExecutedTrade> matchOrders();

    // Get order book statistics
    int getBuyOrderCount() const;
    int getSellOrderCount() const;
    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;

    // Get all executed trades
    const std::vector<ExecutedTrade> &getExecutedTrades() const { return executed_trades; }

    // Clear old filled orders
    void cleanupFilledOrders();

    // Get order book depth (for display)
    std::vector<std::pair<double, int>> getBuyDepth(int levels = 5) const;
    std::vector<std::pair<double, int>> getSellDepth(int levels = 5) const;
};
