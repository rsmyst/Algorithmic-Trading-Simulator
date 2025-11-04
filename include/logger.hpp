#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include "order.hpp"
#include "trader.hpp"

class DataLogger
{
private:
    std::string log_directory;
    std::mutex log_mutex;
    bool mpi_enabled;
    int mpi_rank;
    int mpi_size;

    // File streams
    std::ofstream trade_log;
    std::ofstream price_log;
    std::ofstream trader_stats_log;
    std::ofstream order_book_log;

    // Buffered data for batch writing
    std::vector<std::string> trade_buffer;
    std::vector<std::string> price_buffer;

public:
    DataLogger(const std::string &directory = "logs");
    ~DataLogger();

    // Initialize logger with MPI support
    void initialize(bool use_mpi = false, int rank = 0, int size = 1);

    // Log trade data (CSV format)
    void logTrade(const ExecutedTrade &trade);

    // Log price history (CSV format)
    void logPrice(double timestamp, double price, double volume, int buy_orders, int sell_orders);

    // Log trader statistics (CSV format)
    void logTraderStats(double timestamp, const std::vector<std::unique_ptr<Trader>> &traders, double market_price);

    // Log order book depth (CSV format)
    void logOrderBook(double timestamp,
                      const std::vector<std::pair<double, int>> &buy_depth,
                      const std::vector<std::pair<double, int>> &sell_depth);

    // Flush buffers to disk (parallel write with OpenMP)
    void flush();

    // Aggregate logs across MPI processes
    void aggregateMPI();

    // Export to JSON format
    void exportToJSON(const std::string &filename);

private:
    // Helper to create directory
    void createDirectory(const std::string &dir);

    // Parallel file write
    void parallelWrite(const std::string &filename, const std::vector<std::string> &data);
};
