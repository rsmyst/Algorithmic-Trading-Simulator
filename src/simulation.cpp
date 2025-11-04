#include "../include/simulation.hpp"
#include <omp.h>
#include <algorithm>
#include <numeric>

TradingSimulation::TradingSimulation(int num_traders, double initial_price, double initial_cash)
    : market(initial_price), current_time(0.0), time_step(0.1),
      mpi_enabled(false), mpi_rank(0), mpi_size(1)
{
    // Create traders with different strategies
    for (int i = 0; i < num_traders; i++)
    {
        Strategy strat;
        int strategy_type = i % 9; // Now we have 9 strategies
        if (strategy_type == 0)
            strat = Strategy::MOMENTUM;
        else if (strategy_type == 1)
            strat = Strategy::MEAN_REVERSION;
        else if (strategy_type == 2)
            strat = Strategy::RANDOM;
        else if (strategy_type == 3)
            strat = Strategy::RISK_AVERSE;
        else if (strategy_type == 4)
            strat = Strategy::HIGH_RISK;
        else if (strategy_type == 5)
            strat = Strategy::RSI_BASED;
        else if (strategy_type == 6)
            strat = Strategy::MACD_BASED;
        else if (strategy_type == 7)
            strat = Strategy::BOLLINGER;
        else
            strat = Strategy::MULTI_INDICATOR;

        traders.push_back(std::make_unique<Trader>(i, strat, initial_cash));
    }

    // Give half the traders initial holdings so they can sell
    // This creates market liquidity from the start
    int initial_holdings = 50; // Start with 50 shares
    for (int i = 0; i < num_traders / 2; i++)
    {
        traders[i]->setInitialHoldings(initial_holdings);
    }

    // Initialize logger
    logger.initialize(false, 0, 1);
}

void TradingSimulation::setTimeScale(double scale)
{
    if (scale <= 0.0)
        scale = 1.0;         // Ensure positive
    time_step = 0.1 / scale; // Base time step is 0.1, divided by scale for faster simulation
}

void TradingSimulation::initializeMPI(bool use_mpi, int rank, int size)
{
    mpi_enabled = use_mpi;
    mpi_rank = rank;
    mpi_size = size;

    logger.initialize(use_mpi, rank, size);
}

void TradingSimulation::step()
{
    current_time += time_step;

    std::vector<Order> current_orders;
    int total_buy_quantity = 0;
    int total_sell_quantity = 0;

    double current_price = market.getCurrentPrice();

// Phase 1: Parallel order creation with OpenMP
#pragma omp parallel
    {
        std::vector<Order> local_orders;
        int local_buy = 0;
        int local_sell = 0;

#pragma omp for nowait
        for (int i = 0; i < traders.size(); i++)
        {
            TraderOrder trader_order = traders[i]->createOrder(current_price, current_time);

            if (trader_order.quantity > 0)
            {
                Order order(
                    0, // order_id will be assigned by OrderBook
                    trader_order.trader_id,
                    trader_order.is_buy ? OrderType::BUY : OrderType::SELL,
                    trader_order.price,
                    trader_order.quantity,
                    trader_order.timestamp);

                local_orders.push_back(order);

                if (trader_order.is_buy)
                {
                    local_buy += trader_order.quantity;
                }
                else
                {
                    local_sell += trader_order.quantity;
                }
            }
        }

// Combine results from all threads
#pragma omp critical
        {
            current_orders.insert(current_orders.end(), local_orders.begin(), local_orders.end());
            total_buy_quantity += local_buy;
            total_sell_quantity += local_sell;
        }
    }

    // Phase 2: Add orders to order book
    for (auto &order : current_orders)
    {
        order_book.addOrder(order);
    }

    // Phase 3: Match orders (parallel matching inside OrderBook)
    std::vector<ExecutedTrade> executed_trades = order_book.matchOrders();

    // Phase 4: Execute trades and update traders
    for (const auto &trade : executed_trades)
    {
        // Update buyer
        traders[trade.buyer_id]->executeOrder(true, trade.price, trade.quantity);

        // Update seller
        traders[trade.seller_id]->executeOrder(false, trade.price, trade.quantity);

        // Log trade
        logger.logTrade(trade);
    }

    // Update market based on trading activity
    market.updatePrice(total_buy_quantity, total_sell_quantity);

    // Log price and order book data periodically
    if (static_cast<int>(current_time * 10) % 10 == 0)
    {
        double volume = 0.0;
        for (const auto &trade : executed_trades)
        {
            volume += trade.price * trade.quantity;
        }

        logger.logPrice(current_time, current_price, volume,
                        order_book.getBuyOrderCount(), order_book.getSellOrderCount());

        logger.logTraderStats(current_time, traders, current_price);

        auto buy_depth = order_book.getBuyDepth(5);
        auto sell_depth = order_book.getSellDepth(5);
        logger.logOrderBook(current_time, buy_depth, sell_depth);
    }

    // Cleanup filled orders periodically
    if (static_cast<int>(current_time) % 10 == 0)
    {
        order_book.cleanupFilledOrders();
    }
}

SimulationStats TradingSimulation::getStats() const
{
    SimulationStats stats;
    stats.simulation_time = current_time;

    const auto &executed_trades = order_book.getExecutedTrades();
    stats.total_trades = executed_trades.size();

    if (executed_trades.empty())
    {
        stats.total_volume = 0;
        stats.avg_price = market.getCurrentPrice();
        stats.price_volatility = 0;
    }
    else
    {
        // Calculate total volume
        stats.total_volume = 0;
        double sum_price = 0;
        for (const auto &trade : executed_trades)
        {
            stats.total_volume += trade.quantity * trade.price;
            sum_price += trade.price;
        }

        stats.avg_price = sum_price / executed_trades.size();

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
    }

    // Order book statistics
    stats.pending_buy_orders = order_book.getBuyOrderCount();
    stats.pending_sell_orders = order_book.getSellOrderCount();
    stats.best_bid = order_book.getBestBid();
    stats.best_ask = order_book.getBestAsk();
    stats.spread = order_book.getSpread();

    return stats;
}
