#include "../include/order_book.hpp"
#include <omp.h>
#include <iostream>

OrderBook::OrderBook() : next_order_id(1), next_trade_id(1)
{
}

int OrderBook::addOrder(Order order)
{
    std::lock_guard<std::mutex> lock(book_mutex);

    order.order_id = next_order_id++;

    if (order.type == OrderType::BUY)
    {
        buy_orders[order.price].push_back(order);
    }
    else
    {
        sell_orders[order.price].push_back(order);
    }

    return order.order_id;
}

std::vector<ExecutedTrade> OrderBook::matchOrders()
{
    std::lock_guard<std::mutex> lock(book_mutex);
    std::vector<ExecutedTrade> new_trades;

    // Get best bid and ask
    while (!buy_orders.empty() && !sell_orders.empty())
    {
        auto buy_it = buy_orders.begin();   // Highest buy price
        auto sell_it = sell_orders.begin(); // Lowest sell price

        double best_bid = buy_it->first;
        double best_ask = sell_it->first;

        // If bid >= ask, we can match orders
        if (best_bid < best_ask)
        {
            break; // No match possible
        }

        std::vector<Order> &buy_list = buy_it->second;
        std::vector<Order> &sell_list = sell_it->second;

        if (buy_list.empty() || sell_list.empty())
        {
            // Clean up empty price levels
            if (buy_list.empty())
                buy_orders.erase(buy_it);
            if (sell_list.empty())
                sell_orders.erase(sell_it);
            continue;
        }

// Parallel matching within price levels using OpenMP
// For simplicity, process sequentially but mark parallel-ready sections
#pragma omp parallel for schedule(dynamic) if (buy_list.size() > 10)
        for (int i = 0; i < buy_list.size(); i++)
        {
            Order &buy_order = buy_list[i];
            if (buy_order.isFilled())
                continue;

// Match with sell orders
#pragma omp critical
            {
                for (auto &sell_order : sell_list)
                {
                    if (sell_order.isFilled())
                        continue;
                    if (buy_order.isFilled())
                        break;

                    // Match orders
                    int match_quantity = std::min(
                        buy_order.getRemainingQuantity(),
                        sell_order.getRemainingQuantity());

                    if (match_quantity > 0)
                    {
                        // Execute trade at the limit price (use sell order's ask price)
                        ExecutedTrade trade;
                        trade.trade_id = next_trade_id++;
                        trade.buy_order_id = buy_order.order_id;
                        trade.sell_order_id = sell_order.order_id;
                        trade.buyer_id = buy_order.trader_id;
                        trade.seller_id = sell_order.trader_id;
                        trade.price = sell_order.price; // Price-time priority
                        trade.quantity = match_quantity;
                        trade.timestamp = buy_order.timestamp;

                        // Update orders
                        buy_order.filled_quantity += match_quantity;
                        sell_order.filled_quantity += match_quantity;

                        if (buy_order.isFilled())
                        {
                            buy_order.status = OrderStatus::FILLED;
                        }
                        else
                        {
                            buy_order.status = OrderStatus::PARTIALLY_FILLED;
                        }

                        if (sell_order.isFilled())
                        {
                            sell_order.status = OrderStatus::FILLED;
                        }
                        else
                        {
                            sell_order.status = OrderStatus::PARTIALLY_FILLED;
                        }

                        new_trades.push_back(trade);
                        executed_trades.push_back(trade);
                    }
                }
            }
        }

        // Remove filled orders
        buy_list.erase(
            std::remove_if(buy_list.begin(), buy_list.end(),
                           [](const Order &o)
                           { return o.isFilled(); }),
            buy_list.end());

        sell_list.erase(
            std::remove_if(sell_list.begin(), sell_list.end(),
                           [](const Order &o)
                           { return o.isFilled(); }),
            sell_list.end());

        // Clean up empty price levels
        if (buy_list.empty())
            buy_orders.erase(buy_it);
        if (sell_list.empty())
            sell_orders.erase(sell_it);
    }

    return new_trades;
}

void OrderBook::cleanupFilledOrders()
{
    std::lock_guard<std::mutex> lock(book_mutex);

    // Remove filled orders from buy side
    for (auto it = buy_orders.begin(); it != buy_orders.end();)
    {
        auto &orders = it->second;
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                           [](const Order &o)
                           { return o.isFilled(); }),
            orders.end());

        if (orders.empty())
        {
            it = buy_orders.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Remove filled orders from sell side
    for (auto it = sell_orders.begin(); it != sell_orders.end();)
    {
        auto &orders = it->second;
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                           [](const Order &o)
                           { return o.isFilled(); }),
            orders.end());

        if (orders.empty())
        {
            it = sell_orders.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

int OrderBook::getBuyOrderCount() const
{
    int count = 0;
    for (const auto &[price, orders] : buy_orders)
    {
        count += orders.size();
    }
    return count;
}

int OrderBook::getSellOrderCount() const
{
    int count = 0;
    for (const auto &[price, orders] : sell_orders)
    {
        count += orders.size();
    }
    return count;
}

double OrderBook::getBestBid() const
{
    if (buy_orders.empty())
        return 0.0;
    return buy_orders.begin()->first;
}

double OrderBook::getBestAsk() const
{
    if (sell_orders.empty())
        return 0.0;
    return sell_orders.begin()->first;
}

double OrderBook::getSpread() const
{
    double bid = getBestBid();
    double ask = getBestAsk();
    if (bid == 0.0 || ask == 0.0)
        return 0.0;
    return ask - bid;
}

std::vector<std::pair<double, int>> OrderBook::getBuyDepth(int levels) const
{
    std::vector<std::pair<double, int>> depth;
    int count = 0;

    for (const auto &[price, orders] : buy_orders)
    {
        if (count >= levels)
            break;

        int total_quantity = 0;
        for (const auto &order : orders)
        {
            total_quantity += order.getRemainingQuantity();
        }

        depth.push_back({price, total_quantity});
        count++;
    }

    return depth;
}

std::vector<std::pair<double, int>> OrderBook::getSellDepth(int levels) const
{
    std::vector<std::pair<double, int>> depth;
    int count = 0;

    for (const auto &[price, orders] : sell_orders)
    {
        if (count >= levels)
            break;

        int total_quantity = 0;
        for (const auto &order : orders)
        {
            total_quantity += order.getRemainingQuantity();
        }

        depth.push_back({price, total_quantity});
        count++;
    }

    return depth;
}
