#pragma once
#include <string>

enum class OrderType
{
    BUY,
    SELL
};

enum class OrderStatus
{
    PENDING,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED
};

struct Order
{
    int order_id;
    int trader_id;
    OrderType type;
    double price;
    int quantity;
    int filled_quantity;
    OrderStatus status;
    double timestamp;

    Order() : order_id(0), trader_id(0), type(OrderType::BUY),
              price(0.0), quantity(0), filled_quantity(0),
              status(OrderStatus::PENDING), timestamp(0.0) {}

    Order(int id, int t_id, OrderType t, double p, int q, double ts)
        : order_id(id), trader_id(t_id), type(t), price(p),
          quantity(q), filled_quantity(0), status(OrderStatus::PENDING),
          timestamp(ts) {}

    int getRemainingQuantity() const
    {
        return quantity - filled_quantity;
    }

    bool isFilled() const
    {
        return filled_quantity >= quantity;
    }
};

struct ExecutedTrade
{
    int trade_id;
    int buy_order_id;
    int sell_order_id;
    int buyer_id;
    int seller_id;
    double price;
    int quantity;
    double timestamp;
};
