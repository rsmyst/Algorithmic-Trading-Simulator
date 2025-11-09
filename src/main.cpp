#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
// #include <ftxui/component/input.hpp> // --- NEW: For input boxes ---
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>
#include <type_traits>
#include <mpi.h>
#include <omp.h>

#include "../include/simulation.hpp"

using namespace ftxui;

// --- (Config struct, printHelp, parseArguments are IDENTICAL to Part 1) ---
struct Config
{
    int num_traders = 12;
    double initial_price = 170.0;
    double initial_cash = 10000.0;
    int duration_seconds = 60;
    double time_scale = 1.0;
    bool show_help = false;
    int ensemble_count = 0;
    unsigned int base_seed = 12345;
};
struct SimulationSummaryPacket
{
    int simulation_index = -1;
    SimulationStats stats{};
};

static_assert(std::is_trivially_copyable_v<SimulationSummaryPacket>, "SimulationSummaryPacket must be trivially copyable for MPI transfers.");
void printHelp()
{
    std::cout << "Algorithmic Trading Simulator\n\n";
    std::cout << "Usage: tradingSim [options]\n\n";
    std::cout << "Standard TUI Mode (default):\n";
    std::cout << "  -t, --traders <num>     Number of trader agents (default: 12)\n";
    std::cout << "  -d, --duration <sec>    Simulation duration in seconds (default: 60)\n";
    std::cout << "  -p, --price <value>     Initial asset price (default: 170.0)\n";
    std::cout << "  -c, --cash <value>      Initial cash per trader (default: 10000.0)\n";
    std::cout << "  -s, --speed <scale>     Time scale multiplier (default: 1.0)\n";
    std::cout << "  -h, --help              Show this help message\n\n";
    std::cout << "Ensemble (Headless) Mode:\n";
    std::cout << "  -E, --ensemble <N>      Run N simulations headlessly (disables TUI)\n";
    std::cout << "  --seed <S>              Base seed for ensemble runs (default: 12345)\n\n";
    std::cout << "Example (TUI):\n";
    std::cout << "  ./tradingSim -t 20 -d 120 -s 2.0\n";
    std::cout << "Example (Ensemble):\n";
    std::cout << "  mpiexec -n 4 ./tradingSim -E 100 --seed 42\n\n";
}
Config parseArguments(int argc, char *argv[])
{
    Config config;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            config.show_help = true;
            return config;
        }
        else if ((arg == "-t" || arg == "--traders") && i + 1 < argc)
        {
            config.num_traders = std::stoi(argv[++i]);
        }
        else if ((arg == "-d" || arg == "--duration") && i + 1 < argc)
        {
            config.duration_seconds = std::stoi(argv[++i]);
        }
        else if ((arg == "-p" || arg == "--price") && i + 1 < argc)
        {
            config.initial_price = std::stod(argv[++i]);
        }
        else if ((arg == "-c" || arg == "--cash") && i + 1 < argc)
        {
            config.initial_cash = std::stod(argv[++i]);
        }
        else if ((arg == "-s" || arg == "--speed") && i + 1 < argc)
        {
            config.time_scale = std::stod(argv[++i]);
            if (config.time_scale <= 0.0)
                config.time_scale = 1.0;
        }
        else if ((arg == "-E" || arg == "--ensemble") && i + 1 < argc)
        {
            config.ensemble_count = std::stoi(argv[++i]);
        }
        else if ((arg == "--seed") && i + 1 < argc)
        {
            config.base_seed = std::stoul(argv[++i]);
        }
    }
    return config;
}

// --- 4. Main Function ---
int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int mpi_rank = 0;
    int mpi_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    Config config = parseArguments(argc, argv);

    if (config.show_help)
    {
        if (mpi_rank == 0)
            printHelp();
        MPI_Finalize();
        return 0;
    }

    if (config.ensemble_count > 0)
    {
        // ==================================
        // === 1. ENSEMBLE (HEADLESS) MODE ===
        // ==================================
        int n = config.ensemble_count;
        int base_sims = n / mpi_size;
        int extra_sims = n % mpi_size;
        int sims_for_this_rank = base_sims + (mpi_rank < extra_sims ? 1 : 0);
        int start_index = (mpi_rank * base_sims) + std::min(mpi_rank, extra_sims);
        if (mpi_rank == 0)
        {
            std::cout << "=== Running Ensemble Mode ===\n"
                      << "Total Simulations: " << n << "\n"
                      << "MPI Processes: " << mpi_size << "\n"
                      << "----------------------------------\n";
        }
        // Local storage for simulation results
        std::vector<SimulationSummaryPacket> local_results;
        local_results.reserve(sims_for_this_rank);

        for (int i = 0; i < sims_for_this_rank; i++)
        {
            int global_sim_index = start_index + i;
            unsigned int sim_seed = config.base_seed + global_sim_index;
            std::cout << "[Rank " << mpi_rank << "] Starting sim " << global_sim_index << " (seed " << sim_seed << ")..." << std::endl;

            TradingSimulation sim(config.num_traders, config.initial_price, config.initial_cash, sim_seed);
            sim.setTimeScale(config.time_scale);
            sim.getLogger().initialize(true, mpi_rank, mpi_size, global_sim_index);
            SimulationStats stats = sim.runHeadless(config.duration_seconds);

            SimulationSummaryPacket packet;
            packet.simulation_index = global_sim_index;
            packet.stats = stats;
            local_results.push_back(packet);

            std::cout << "[Rank " << mpi_rank << "] Finished sim " << global_sim_index
                      << " (Trades: " << stats.total_trades
                      << ", Volume: $" << std::fixed << std::setprecision(2) << stats.total_volume << ")" << std::endl;
        }

        // Gather all results to rank 0
        std::vector<int> recv_counts(mpi_size);
        std::vector<int> displs(mpi_size);
        int local_count = static_cast<int>(local_results.size());

        MPI_Gather(&local_count, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

        std::vector<SimulationSummaryPacket> all_results;
        if (mpi_rank == 0)
        {
            int total_results = 0;
            for (int i = 0; i < mpi_size; i++)
            {
                displs[i] = total_results;
                total_results += recv_counts[i];
            }
            all_results.resize(total_results);
        }

        // Calculate byte counts and displacements for MPI_Gatherv
        constexpr int packet_size = sizeof(SimulationSummaryPacket);
        std::vector<int> byte_recv_counts(mpi_size);
        std::vector<int> byte_displs(mpi_size);

        if (mpi_rank == 0)
        {
            for (int i = 0; i < mpi_size; i++)
            {
                byte_recv_counts[i] = recv_counts[i] * packet_size;
                byte_displs[i] = displs[i] * packet_size;
            }
        }

        MPI_Gatherv(local_results.data(), local_count * packet_size, MPI_BYTE,
                    all_results.data(), byte_recv_counts.data(), byte_displs.data(), MPI_BYTE,
                    0, MPI_COMM_WORLD);

        if (mpi_rank == 0)
        {
            // Calculate comprehensive statistics
            long long total_trades = 0;
            double total_volume = 0.0;
            double sum_avg_price = 0.0;
            double sum_volatility = 0.0;

            int best_sim_idx = -1;
            int worst_sim_idx = -1;
            double best_volume = -std::numeric_limits<double>::infinity();
            double worst_volume = std::numeric_limits<double>::infinity();

            std::vector<int> trade_counts;
            std::vector<double> volumes;
            std::vector<double> avg_prices;
            std::vector<double> volatilities;

            for (const auto &packet : all_results)
            {
                total_trades += packet.stats.total_trades;
                total_volume += packet.stats.total_volume;
                sum_avg_price += packet.stats.avg_price;
                sum_volatility += packet.stats.price_volatility;

                trade_counts.push_back(packet.stats.total_trades);
                volumes.push_back(packet.stats.total_volume);
                avg_prices.push_back(packet.stats.avg_price);
                volatilities.push_back(packet.stats.price_volatility);

                if (packet.stats.total_volume > best_volume)
                {
                    best_volume = packet.stats.total_volume;
                    best_sim_idx = packet.simulation_index;
                }
                if (packet.stats.total_volume < worst_volume)
                {
                    worst_volume = packet.stats.total_volume;
                    worst_sim_idx = packet.simulation_index;
                }
            }

            int n_sims = static_cast<int>(all_results.size());
            double avg_trades_per_sim = static_cast<double>(total_trades) / n_sims;
            double avg_volume_per_sim = total_volume / n_sims;
            double avg_price = sum_avg_price / n_sims;
            double avg_volatility = sum_volatility / n_sims;

            // Calculate standard deviations
            auto calc_stddev = [n_sims](const std::vector<double> &values, double mean)
            {
                double sum_sq = 0.0;
                for (double v : values)
                {
                    double diff = v - mean;
                    sum_sq += diff * diff;
                }
                return std::sqrt(sum_sq / n_sims);
            };

            double stddev_volume = calc_stddev(volumes, avg_volume_per_sim);
            double stddev_trades = 0.0;
            {
                double sum_sq = 0.0;
                for (int tc : trade_counts)
                {
                    double diff = tc - avg_trades_per_sim;
                    sum_sq += diff * diff;
                }
                stddev_trades = std::sqrt(sum_sq / n_sims);
            }

            std::cout << "\n=======================================================\n";
            std::cout << "              ENSEMBLE SUMMARY STATISTICS             \n";
            std::cout << "=======================================================\n\n";

            std::cout << "Total Simulations: " << config.ensemble_count << "\n";
            std::cout << "MPI Processes Used: " << mpi_size << "\n\n";

            std::cout << "--- AGGREGATE METRICS ---\n";
            std::cout << "Grand Total Trades: " << total_trades << "\n";
            std::cout << "Grand Total Volume: $" << std::fixed << std::setprecision(2) << total_volume << "\n\n";

            std::cout << "--- AVERAGE PER SIMULATION ---\n";
            std::cout << "Avg Trades: " << std::fixed << std::setprecision(2) << avg_trades_per_sim
                      << " (±" << stddev_trades << ")\n";
            std::cout << "Avg Volume: $" << avg_volume_per_sim
                      << " (±$" << stddev_volume << ")\n";
            std::cout << "Avg Price: $" << avg_price << "\n";
            std::cout << "Avg Volatility: $" << avg_volatility << "\n\n";

            std::cout << "--- BEST SIMULATION ---\n";
            std::cout << "Sim Index: " << best_sim_idx << "\n";
            std::cout << "Volume: $" << best_volume << "\n";
            for (const auto &packet : all_results)
            {
                if (packet.simulation_index == best_sim_idx)
                {
                    std::cout << "Trades: " << packet.stats.total_trades << "\n";
                    std::cout << "Avg Price: $" << packet.stats.avg_price << "\n";
                    std::cout << "Volatility: $" << packet.stats.price_volatility << "\n";
                    break;
                }
            }

            std::cout << "\n--- WORST SIMULATION ---\n";
            std::cout << "Sim Index: " << worst_sim_idx << "\n";
            std::cout << "Volume: $" << worst_volume << "\n";
            for (const auto &packet : all_results)
            {
                if (packet.simulation_index == worst_sim_idx)
                {
                    std::cout << "Trades: " << packet.stats.total_trades << "\n";
                    std::cout << "Avg Price: $" << packet.stats.avg_price << "\n";
                    std::cout << "Volatility: $" << packet.stats.price_volatility << "\n";
                    break;
                }
            }

            std::cout << "\n=======================================================\n";
            std::cout << "Ensemble run complete. CSV logs saved to 'logs/' directory.\n";
            std::cout << "Each simulation has separate CSV files with naming pattern:\n";
            std::cout << "  trades_sim<N>_rank<R>.csv\n";
            std::cout << "  prices_sim<N>_rank<R>.csv\n";
            std::cout << "  trader_stats_sim<N>_rank<R>.csv\n";
            std::cout << "  order_book_sim<N>_rank<R>.csv\n";
            std::cout << "=======================================================\n";
        }
    }
    else
    {
        // ==================================
        // === 2. INTERACTIVE TUI MODE ===
        // ==================================

        if (mpi_rank == 0)
        {
            TradingSimulation simulation(config.num_traders, config.initial_price, config.initial_cash, config.base_seed);
            simulation.setTimeScale(config.time_scale);
            simulation.getLogger().initialize(false, 0, 1, -1);

            auto screen = ScreenInteractive::Fullscreen();

            bool running = true;
            auto start_time = std::chrono::steady_clock::now();
            auto last_update = std::chrono::steady_clock::now();

            // --- NEW: State for Human Trader ---
            std::string human_price_str = std::to_string(static_cast<int>(config.initial_price));
            std::string human_qty_str = "10";
            std::string human_trader_id = "0"; // We designate Trader 0 as the Human
            std::string last_action_msg = "Welcome, Trader 0!";

            // --- NEW: UI Components for Human ---
            Component price_input = Input(&human_price_str, "Price");
            Component qty_input = Input(&human_qty_str, "Qty");

            auto human_trade_action = [&](OrderType type)
            {
                try
                {
                    double price = std::stod(human_price_str);
                    int qty = std::stoi(human_qty_str);
                    int trader_id = std::stoi(human_trader_id);

                    if (qty <= 0)
                    {
                        last_action_msg = "Error: Qty must be > 0";
                        return;
                    }

                    // Get current time from simulation
                    double timestamp = simulation.getStats().simulation_time;

                    Order human_order(0, trader_id, type, price, qty, timestamp);
                    simulation.addHumanOrder(human_order); // Inject the order

                    last_action_msg = (type == OrderType::BUY ? "BUY" : "SELL");
                    last_action_msg += " order for " + human_qty_str + " @ $" + human_price_str + " sent!";
                }
                catch (const std::exception &e)
                {
                    last_action_msg = "Error: Invalid price or qty";
                }
            };

            Component buy_button = Button("  BUY  ", [&]()
                                          { human_trade_action(OrderType::BUY); }, ButtonOption::Animated(Color::Green));
            Component sell_button = Button("  SELL  ", [&]()
                                           { human_trade_action(OrderType::SELL); }, ButtonOption::Animated(Color::Red));

            // --- NEW: Add components to a container for focus ---
            auto main_container = Container::Vertical({price_input,
                                                       qty_input,
                                                       buy_button,
                                                       sell_button});

            // Handle 'q' to quit
            main_container |= CatchEvent([&](Event event)
                                         {
                if (event == Event::Character('q') || event == Event::Character('Q')) {
                    running = false;
                    screen.ExitLoopClosure()();
                    return true;
                }
                return false; });

            // Create renderer
            auto renderer = Renderer(main_container, [&]
                                     {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                
                if (elapsed >= config.duration_seconds && running) {
                    running = false;
                    screen.ExitLoopClosure()();
                }
                
                auto step_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
                int step_interval = static_cast<int>(100.0 / config.time_scale);
                if (step_elapsed >= step_interval && running) {
                    simulation.step();
                    last_update = now;
                }
                
                std::vector<Element> elements;
                
                // ... (Title, Info Bar, Price Graph are all the same as before) ...
                elements.push_back(text("Algorithmic Trading Simulation") | bold | color(Color::Cyan) | center);
                elements.push_back(separator());
                int remaining = config.duration_seconds - elapsed;
                std::stringstream ss;
                ss << "Time: " << elapsed << "s / " << config.duration_seconds << "s" << (remaining > 0 ? " (Remaining: " + std::to_string(remaining) + "s)" : " [COMPLETED]");
                elements.push_back(hbox({ text("Traders: " + std::to_string(config.num_traders)) | color(Color::Yellow), text(" | "), text(ss.str()) | color(running ? Color::Green : Color::Red), text(" | "), text("Press 'q' to quit") | dim }) | center);
                elements.push_back(separator());
                const auto& market = simulation.getMarket();
                double current_price = market.getCurrentPrice();
                double price_change = market.getPriceChangePercent();
                auto price_history = market.getRecentHistory(200); 
                Color price_change_color = price_change >= 0 ? Color::Green : Color::Red;
                std::string change_indicator = price_change >= 0 ? "▲" : "▼";
                std::stringstream price_ss;
                price_ss << std::fixed << std::setprecision(2) << (price_change >= 0 ? "+" : "") << price_change << "%";
                elements.push_back(hbox({ text("Current Price: ") | bold, text("$" + std::to_string(static_cast<int>(current_price))) | color(Color::GreenLight) | bold, text("  "), text(change_indicator + " " + price_ss.str()) | color(price_change_color) | bold, }));
                if (!price_history.empty()) {
                    // 1. Find the min/max price for auto-scaling
                    double max_price_seen = *std::max_element(price_history.begin(), price_history.end());
                    double min_price_seen = *std::min_element(price_history.begin(), price_history.end());
                    
                    double price_range = max_price_seen - min_price_seen;
                    if (price_range < 1.0) price_range = 1.0; // Avoid division by zero
                    
                    // Add 5% buffer to top and bottom
                    double max_price = max_price_seen + (price_range * 0.05);
                    double min_price = min_price_seen - (price_range * 0.05);
                    
                    double range = max_price - min_price;
                    if (range < 1.0) range = 1.0;
                    
                    const int graph_height = 15; // Height in text lines
                    std::vector<Element> columns;
                    
                    for (size_t i = 0; i < price_history.size(); i++) {
                        double price = price_history[i];
                        
                        // Normalize price to graph height (0 to 14)
                        double normalized = (price - min_price) / range;
                        int filled_lines = static_cast<int>(normalized * (graph_height - 1));
                        filled_lines = std::max(0, std::min(graph_height - 1, filled_lines));
                        
                        // 2. NEW COLOR LOGIC
                        Color bar_color = Color::GrayDark; // Default for no change
                        if (i > 0) {
                            if (price > price_history[i-1]) {
                                bar_color = Color::Green; // Price went UP
                            } else if (price < price_history[i-1]) {
                                bar_color = Color::Red; // Price went DOWN
                            }
                        } else if (price_history.size() > 0) {
                            // Color the first dot based on initial price (optional, but good)
                             bar_color = (price > config.initial_price) ? Color::Green : Color::Red;
                        }

                        // 3. NEW DRAWING LOOP (No Yellow Line)
                        std::vector<Element> vertical_blocks;
                        for (int line = 0; line < graph_height; line++) {
                            int line_from_bottom = graph_height - 1 - line;
                            
                            if (line_from_bottom == filled_lines) {
                                // Draw the price dot
                                vertical_blocks.push_back(text("●") | color(bar_color));
                            } else {
                                // Empty space
                                vertical_blocks.push_back(text(" "));
                            }
                        }
                        columns.push_back(vbox(std::move(vertical_blocks)));
                    }
                    elements.push_back(hbox(std::move(columns)) | border | flex);
                }
                // if (!price_history.empty()) {
                //     double initial_price = config.initial_price;
                //     double max_price_seen = *std::max_element(price_history.begin(), price_history.end());
                //     double min_price_seen = *std::min_element(price_history.begin(), price_history.end());
                //     max_price_seen = std::max(max_price_seen, initial_price); min_price_seen = std::min(min_price_seen, initial_price);
                //     double price_range = max_price_seen - min_price_seen; if (price_range < 1.0) price_range = 1.0;
                //     double buffer = price_range * 0.1; double max_price = max_price_seen + buffer; double min_price = min_price_seen - buffer;
                //     double range = max_price - min_price; if (range < 1.0) range = 1.0;
                //     const int graph_height = 15;
                //     double initial_price_normalized = (initial_price - min_price) / range;
                //     int baseline_pos = static_cast<int>(initial_price_normalized * (graph_height - 1));
                //     baseline_pos = std::max(0, std::min(graph_height - 1, baseline_pos));
                //     std::vector<Element> columns;
                //     for (size_t i = 0; i < price_history.size(); i++) {
                //         double price = price_history[i];
                //         double normalized = (price - min_price) / range;
                //         int filled_lines = static_cast<int>(normalized * (graph_height - 1));
                //         filled_lines = std::max(0, std::min(graph_height - 1, filled_lines));
                //         Color bar_color = (price > initial_price) ? Color::Green : Color::Red;
                //         std::vector<Element> vertical_blocks;
                //         for (int line = 0; line < graph_height; line++) {
                //             int line_from_bottom = graph_height - 1 - line;
                //             if (line_from_bottom == baseline_pos) {
                //                 if (line_from_bottom == filled_lines) vertical_blocks.push_back(text("◆") | color(bar_color) | bold);
                //                 else vertical_blocks.push_back(text("─") | color(Color::Yellow));
                //             } else if (line_from_bottom == filled_lines) {
                //                 vertical_blocks.push_back(text("●") | color(bar_color));
                //             } else vertical_blocks.push_back(text(" "));
                //         }
                //         columns.push_back(vbox(std::move(vertical_blocks)));
                //     }
                //     elements.push_back(hbox(std::move(columns)) | border | flex);
                // }
                // elements.push_back(separator());
                
                // --- (Order Book / Stats Row is the same) ---
                const auto& order_book = simulation.getOrderBook();
                auto buy_depth = order_book.getBuyDepth(5);
                auto sell_depth = order_book.getSellDepth(5);
                auto stats = simulation.getStats();
                elements.push_back(hbox({
                    vbox({ text("Order Book") | bold | color(Color::Yellow), hbox({ text("Bid: $" + std::to_string(static_cast<int>(order_book.getBestBid()))), text(" | "), text("Ask: $" + std::to_string(static_cast<int>(order_book.getBestAsk()))), }) | color(Color::Cyan), hbox({ text("Spread: $" + std::to_string(static_cast<int>(order_book.getSpread()))), }), separator(),
                        hbox({
                            vbox({ text("Buy Side") | bold | color(Color::Green) | center, separator(), vbox([&]() { std::vector<Element> buy_elements; for (const auto& [price, qty] : buy_depth) buy_elements.push_back(text("$" + std::to_string(static_cast<int>(price)) + " x" + std::to_string(qty)) | color(Color::GreenLight)); if (buy_elements.empty()) buy_elements.push_back(text("No orders") | dim | center); return buy_elements; }()) }) | flex | border,
                            vbox({ text("Sell Side") | bold | color(Color::Red) | center, separator(), vbox([&]() { std::vector<Element> sell_elements; for (const auto& [price, qty] : sell_depth) sell_elements.push_back(text("$" + std::to_string(static_cast<int>(price)) + " x" + std::to_string(qty)) | color(Color::RedLight)); if (sell_elements.empty()) sell_elements.push_back(text("No orders") | dim | center); return sell_elements; }()) }) | flex | border
                        })
                    }) | flex,
                    separator(),
                    vbox({ text("Market Statistics") | bold | color(Color::Yellow), text("Total Trades: " + std::to_string(stats.total_trades)), text("Total Volume: $" + std::to_string(static_cast<int>(stats.total_volume))), text("Avg Price: $" + std::to_string(static_cast<int>(stats.avg_price))), text("Volatility: $" + std::to_string(static_cast<int>(stats.price_volatility))), }) | flex
                }));
                elements.push_back(separator());

                // --- NEW: Human Trader Control Panel ---
                const auto& traders = simulation.getTraders();
                const Trader* human_trader = traders[std::stoi(human_trader_id)].get(); // Get Trader 0
                double human_net_worth = human_trader->getNetWorth(current_price);
                double human_profit = human_net_worth - config.initial_cash;

                std::string exec_notification = simulation.getHumanNotification();

                auto human_panel = vbox({
                    text("Human Control (Trader " + human_trader_id + ")") | bold | color(Color::BlueLight),
                    text("Net Worth: $" + std::to_string(static_cast<int>(human_net_worth))),
                    text("Profit: $" + std::to_string(static_cast<int>(human_profit))) | color(human_profit >= 0 ? Color::Green : Color::Red),
                    text("Cash: $" + std::to_string(static_cast<int>(human_trader->getCash()))),
                    text("Holdings: " + std::to_string(human_trader->getHoldings())),
                    separator(),
                    text("Place Order:"),
                    hbox({
                        text(" Price: "), price_input->Render(),
                        text(" Qty: "), qty_input->Render(),
                    }),
                    hbox({ buy_button->Render(), sell_button->Render() }) | center,
                    separator(),
                    text(last_action_msg) | center | dim,
                    text(exec_notification) | center | bold | color(Color::GreenLight)
                }) | border;

                // --- MODIFIED: Top Traders Panel ---
                auto top_traders_panel = vbox({
                    text("Top AI Traders (by Net Worth)") | bold | color(Color::Yellow),
                    [&] {
                        std::vector<const Trader*> sorted_traders;
                        for (const auto& trader : traders) {
                            if (trader->getId() != std::stoi(human_trader_id)) // Exclude human
                                sorted_traders.push_back(trader.get());
                        }
                        std::sort(sorted_traders.begin(), sorted_traders.end(), 
                            [current_price](const Trader* a, const Trader* b) {
                                return a->getNetWorth(current_price) > b->getNetWorth(current_price);
                            });
                        
                        std::vector<Element> trader_elements;
                        for (int i = 0; i < std::min(5, static_cast<int>(sorted_traders.size())); i++) {
                            const Trader* t = sorted_traders[i];
                            double net_worth = t->getNetWorth(current_price);
                            double profit = net_worth - config.initial_cash;
                            Color profit_color = profit >= 0 ? Color::Green : Color::Red;
                            
                            std::stringstream trader_info;
                            trader_info << std::fixed << std::setprecision(0);
                            trader_info << "#" << (i+1) << " | T" << t->getId() << " [" << t->getStrategyName() << "] ";
                            trader_info << "Worth: $" << net_worth;
                            trader_info << " (P: " << (profit >= 0 ? "+" : "") << "$" << profit << ")";
                            
                            trader_elements.push_back(text(trader_info.str()) | color(profit_color));
                        }
                        return vbox(std::move(trader_elements));
                    }()
                }) | border;

                // --- NEW: Add Human and AI panels side-by-side ---
                elements.push_back(
                    hbox({
                        human_panel | flex,
                        top_traders_panel | flex
                    })
                );
                
                return vbox(std::move(elements)) | border; });

            // Refresh thread
            std::thread refresh_thread([&]
                                       {
                while (running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    screen.Post(Event::Custom);
                } });

            screen.Loop(renderer);

            running = false;
            if (refresh_thread.joinable())
            {
                refresh_thread.join();
            }

            // --- (Final Summary Printout is the same) ---
            std::cout << "\n=== Simulation Complete ===\n\n";
            auto stats = simulation.getStats();
            std::cout << "Duration: " << stats.simulation_time << " seconds\n";
            std::cout << "Total Trades: " << stats.total_trades << "\n";
            std::cout << "Total Volume: $" << std::fixed << std::setprecision(2) << stats.total_volume << "\n";
            simulation.getLogger().flush();
            std::cout << "Logs saved to 'logs' directory.\n\n";
            const auto &traders = simulation.getTraders();
            std::vector<const Trader *> final_traders;
            for (const auto &trader : traders)
                final_traders.push_back(trader.get());
            double final_price = simulation.getMarket().getCurrentPrice();
            std::sort(final_traders.begin(), final_traders.end(), [](const Trader *a, const Trader *b)
                      { return a->getId() < b->getId(); });
            std::cout << "=== Final Trader Rankings (By ID) ===\n\n";
            for (const auto *t : final_traders)
            {
                double net_worth = t->getNetWorth(final_price);
                double profit = net_worth - config.initial_cash;
                std::cout << "Trader " << t->getId() << " [" << t->getStrategyName() << "]\n";
                std::cout << "   Net Worth: $" << std::fixed << std::setprecision(2) << net_worth << "\n";
                std::cout << "   Profit/Loss: " << (profit >= 0 ? "+" : "") << "$" << profit << "\n";
                std::cout << "   Trades Executed: " << t->getTradesExecuted() << "\n";
                std::cout << "   Holdings: " << t->getHoldings() << " shares\n";
                std::cout << "   Cash: $" << t->getCash() << "\n\n";
            }
        }
    }

    MPI_Finalize();
    return 0;
}