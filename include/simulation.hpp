#pragma once
#include <vector>
#include <memory>
#include "../include/trader.hpp"
#include "../include/market.hpp"
#include "../include/order_book.hpp"
#include "../include/logger.hpp"

// Struct for final simulation statistics
struct SimulationStats {
    double simulation_time = 0.0;
    int total_trades = 0;
    double total_volume = 0.0;
    double avg_price = 0.0;
    double price_volatility = 0.0;
    int pending_buy_orders = 0;
    int pending_sell_orders = 0;
    double best_bid = 0.0;
    double best_ask = 0.0;
    double spread = 0.0;
};

class TradingSimulation {
public:
    TradingSimulation(int num_traders, double initial_price, double initial_cash, unsigned int seed = 12345);
    
    void setTimeScale(double scale);
    void initializeMPI(bool use_mpi, int rank, int size);
    
    void step();
    SimulationStats runHeadless(double duration_seconds);
    SimulationStats getStats() const;
    
    const Market& getMarket() const { return market; }
    const OrderBook& getOrderBook() const { return order_book; }
    const std::vector<std::unique_ptr<Trader>>& getTraders() const { return traders; }
    DataLogger& getLogger() { return logger; }

    // --- NEW: Function for interactive TUI ---
    void addHumanOrder(Order order);
    std::string getHumanNotification() const { return last_human_trade_notification; }


private:
    Market market;
    OrderBook order_book;
    std::vector<std::unique_ptr<Trader>> traders;
    DataLogger logger;
    
    double current_time;
    double time_step;
    unsigned int base_seed;
    
    bool mpi_enabled;
    int mpi_rank;
    int mpi_size;
    std::string last_human_trade_notification;
};