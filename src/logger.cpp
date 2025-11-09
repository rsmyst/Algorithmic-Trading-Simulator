#include "../include/logger.hpp"
#include <omp.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <memory>

#ifdef _WIN32
#include <direct.h>
#define CREATE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define CREATE_DIR(path) mkdir(path, 0755)
#endif

DataLogger::DataLogger(const std::string &directory)
    : log_directory(directory), mpi_enabled(false), mpi_rank(0), mpi_size(1)
{
    createDirectory(log_directory);
}

DataLogger::~DataLogger()
{
    flush();

    if (trade_log.is_open())
        trade_log.close();
    if (price_log.is_open())
        price_log.close();
    if (trader_stats_log.is_open())
        trader_stats_log.close();
    if (order_book_log.is_open())
        order_book_log.close();
}

void DataLogger::initialize(bool use_mpi, int rank, int size, int sim_index)
{
    mpi_enabled = use_mpi;
    mpi_rank = rank;
    mpi_size = size;

    // Create log files with rank suffix if MPI is enabled
    std::string rank_suffix = mpi_enabled ? "_rank" + std::to_string(mpi_rank) : "";
    std::string sim_suffix = (sim_index >= 0) ? "_sim" + std::to_string(sim_index) : "";

    trade_log.open(log_directory + "/trades" + rank_suffix + ".csv");
    price_log.open(log_directory + "/prices" + rank_suffix + ".csv");
    trader_stats_log.open(log_directory + "/trader_stats" + rank_suffix + ".csv");
    order_book_log.open(log_directory + "/order_book" + rank_suffix + ".csv");

    // Write CSV headers
    if (trade_log.is_open())
    {
        trade_log << "TradeID,Timestamp,BuyOrderID,SellOrderID,BuyerID,SellerID,Price,Quantity\n";
    }

    if (price_log.is_open())
    {
        price_log << "Timestamp,Price,Volume,BuyOrders,SellOrders\n";
    }

    if (trader_stats_log.is_open())
    {
        trader_stats_log << "Timestamp,TraderID,Strategy,Cash,Holdings,NetWorth,TotalProfit,TradesExecuted,RSI,MACD\n";
    }

    if (order_book_log.is_open())
    {
        order_book_log << "Timestamp,Side,Price,Quantity\n";
    }
}

void DataLogger::createDirectory(const std::string &dir)
{
    try
    {
        std::filesystem::create_directories(dir);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
    }
}

void DataLogger::logTrade(const ExecutedTrade &trade)
{
    std::lock_guard<std::mutex> lock(log_mutex);

    std::ostringstream ss;
    ss << trade.trade_id << ","
       << std::fixed << std::setprecision(2) << trade.timestamp << ","
       << trade.buy_order_id << ","
       << trade.sell_order_id << ","
       << trade.buyer_id << ","
       << trade.seller_id << ","
       << std::fixed << std::setprecision(2) << trade.price << ","
       << trade.quantity << "\n";

    trade_buffer.push_back(ss.str());

    // Flush if buffer is large
    if (trade_buffer.size() >= 100)
    {
        for (const auto &line : trade_buffer)
        {
            trade_log << line;
        }
        trade_buffer.clear();
    }
}

void DataLogger::logPrice(double timestamp, double price, double volume, int buy_orders, int sell_orders)
{
    std::lock_guard<std::mutex> lock(log_mutex);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << timestamp << ","
       << std::fixed << std::setprecision(2) << price << ","
       << std::fixed << std::setprecision(2) << volume << ","
       << buy_orders << ","
       << sell_orders << "\n";

    price_buffer.push_back(ss.str());

    // Flush if buffer is large
    if (price_buffer.size() >= 50)
    {
        for (const auto &line : price_buffer)
        {
            price_log << line;
        }
        price_buffer.clear();
    }
}

void DataLogger::logTraderStats(double timestamp, const std::vector<std::unique_ptr<Trader>> &traders, double market_price)
{
    std::lock_guard<std::mutex> lock(log_mutex);

    if (!trader_stats_log.is_open())
        return;

    // Parallel logging of trader stats
    std::vector<std::string> stats_lines(traders.size());

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < traders.size(); i++)
    {
        const auto &trader = traders[i];
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << timestamp << ","
           << trader->getId() << ","
           << trader->getStrategyName() << ","
           << std::fixed << std::setprecision(2) << trader->getCash() << ","
           << trader->getHoldings() << ","
           << std::fixed << std::setprecision(2) << trader->getNetWorth(market_price) << ","
           << std::fixed << std::setprecision(2) << trader->getTotalProfit() << ","
           << trader->getTradesExecuted() << ","
           << std::fixed << std::setprecision(2) << trader->getLastRSI() << ","
           << std::fixed << std::setprecision(2) << trader->getLastMACD() << "\n";

        stats_lines[i] = ss.str();
    }

    // Write all lines
    for (const auto &line : stats_lines)
    {
        trader_stats_log << line;
    }
}

void DataLogger::logOrderBook(double timestamp,
                              const std::vector<std::pair<double, int>> &buy_depth,
                              const std::vector<std::pair<double, int>> &sell_depth)
{
    std::lock_guard<std::mutex> lock(log_mutex);

    if (!order_book_log.is_open())
        return;

    // Log buy side
    for (const auto &[price, quantity] : buy_depth)
    {
        order_book_log << std::fixed << std::setprecision(2) << timestamp << ","
                       << "BUY,"
                       << std::fixed << std::setprecision(2) << price << ","
                       << quantity << "\n";
    }

    // Log sell side
    for (const auto &[price, quantity] : sell_depth)
    {
        order_book_log << std::fixed << std::setprecision(2) << timestamp << ","
                       << "SELL,"
                       << std::fixed << std::setprecision(2) << price << ","
                       << quantity << "\n";
    }
}

void DataLogger::flush()
{
    std::lock_guard<std::mutex> lock(log_mutex);

    // Flush trade buffer
    if (trade_log.is_open())
    {
        for (const auto &line : trade_buffer)
        {
            trade_log << line;
        }
        trade_log.flush();
        trade_buffer.clear();
    }

    // Flush price buffer
    if (price_log.is_open())
    {
        for (const auto &line : price_buffer)
        {
            price_log << line;
        }
        price_log.flush();
        price_buffer.clear();
    }

    // Flush other logs
    if (trader_stats_log.is_open())
        trader_stats_log.flush();
    if (order_book_log.is_open())
        order_book_log.flush();
}

void DataLogger::aggregateMPI()
{
    if (!mpi_enabled || mpi_size <= 1)
        return;

    // Flush all buffers first
    flush();

    // MPI aggregation would be implemented here
    // For now, this is a placeholder that would use MPI_Gather or MPI_Reduce
    // to combine logs from all processes into a master log file

    std::cout << "MPI Aggregation not fully implemented (rank " << mpi_rank << ")" << std::endl;
}

void DataLogger::exportToJSON(const std::string &filename)
{
    flush();

    // Simple JSON export (basic implementation)
    std::ofstream json_file(log_directory + "/" + filename);

    if (!json_file.is_open())
    {
        std::cerr << "Failed to open JSON file: " << filename << std::endl;
        return;
    }

    json_file << "{\n";
    json_file << "  \"simulation_log\": {\n";
    json_file << "    \"mpi_rank\": " << mpi_rank << ",\n";
    json_file << "    \"mpi_size\": " << mpi_size << ",\n";
    json_file << "    \"log_directory\": \"" << log_directory << "\",\n";
    json_file << "    \"timestamp\": \"" << std::chrono::system_clock::now().time_since_epoch().count() << "\"\n";
    json_file << "  }\n";
    json_file << "}\n";

    json_file.close();

    std::cout << "Exported logs to JSON: " << filename << std::endl;
}

void DataLogger::parallelWrite(const std::string &filename, const std::vector<std::string> &data)
{
    std::ofstream file(filename, std::ios::app);

    if (!file.is_open())
    {
        std::cerr << "Failed to open file for parallel write: " << filename << std::endl;
        return;
    }

// Parallel write using OpenMP critical sections
#pragma omp parallel for ordered
    for (int i = 0; i < data.size(); i++)
    {
#pragma omp ordered
        {
            file << data[i];
        }
    }

    file.close();
}
