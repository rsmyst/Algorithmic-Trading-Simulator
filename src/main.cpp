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
#include <mpi.h> 
#include <omp.h>

#include "../include/simulation.hpp"

using namespace ftxui;

// --- (Config struct, printHelp, parseArguments are IDENTICAL to Part 1) ---
struct Config {
    int num_traders = 12;
    double initial_price = 170.0;
    double initial_cash = 10000.0;
    int duration_seconds = 60;
    double time_scale = 1.0;
    bool show_help = false;
    int ensemble_count = 0;
    unsigned int base_seed = 12345;
};
void printHelp() {
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
Config parseArguments(int argc, char *argv[]) {
    Config config;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            config.show_help = true; return config;
        } else if ((arg == "-t" || arg == "--traders") && i + 1 < argc) {
            config.num_traders = std::stoi(argv[++i]);
        } else if ((arg == "-d" || arg == "--duration") && i + 1 < argc) {
            config.duration_seconds = std::stoi(argv[++i]);
        } else if ((arg == "-p" || arg == "--price") && i + 1 < argc) {
            config.initial_price = std::stod(argv[++i]);
        } else if ((arg == "-c" || arg == "--cash") && i + 1 < argc) {
            config.initial_cash = std::stod(argv[++i]);
        } else if ((arg == "-s" || arg == "--speed") && i + 1 < argc) {
            config.time_scale = std::stod(argv[++i]);
            if (config.time_scale <= 0.0) config.time_scale = 1.0;
        } else if ((arg == "-E" || arg == "--ensemble") && i + 1 < argc) {
            config.ensemble_count = std::stoi(argv[++i]);
        } else if ((arg == "--seed") && i + 1 < argc) {
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

    if (config.show_help) {
        if (mpi_rank == 0) printHelp();
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
        if (mpi_rank == 0) {
            std::cout << "=== Running Ensemble Mode ===\n" << "Total Simulations: " << n << "\n"
                      << "MPI Processes: " << mpi_size << "\n" << "----------------------------------\n";
        }
        long long local_total_trades = 0;
        double local_total_volume = 0.0;
        double local_sum_avg_price = 0.0;
        for (int i = 0; i < sims_for_this_rank; i++) {
            int global_sim_index = start_index + i;
            unsigned int sim_seed = config.base_seed + global_sim_index;
            std::cout << "[Rank " << mpi_rank << "] Starting sim " << global_sim_index << " (seed " << sim_seed << ")..." << std::endl;
            TradingSimulation sim(config.num_traders, config.initial_price, config.initial_cash, sim_seed);
            sim.setTimeScale(config.time_scale);
            sim.getLogger().initialize(true, mpi_rank, mpi_size, global_sim_index);
            SimulationStats stats = sim.runHeadless(config.duration_seconds);
            local_total_trades += stats.total_trades;
            local_total_volume += stats.total_volume;
            local_sum_avg_price += stats.avg_price;
            std::cout << "[Rank " << mpi_rank << "] Finished sim " << global_sim_index << " (Trades: " << stats.total_trades << ")" << std::endl;
        }
        long long global_total_trades = 0;
        double global_total_volume = 0.0;
        double global_sum_avg_price = 0.0;
        MPI_Reduce(&local_total_trades, &global_total_trades, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_total_volume, &global_total_volume, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_sum_avg_price, &global_sum_avg_price, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        if (mpi_rank == 0) {
            std::cout << "\n=== Ensemble Summary ===\n" << std::endl;
            std::cout << "Total Simulations Ran: " << config.ensemble_count << std::endl;
            std::cout << "Grand Total Trades: " << global_total_trades << std::endl;
            std::cout << "Grand Total Volume: $" << std::fixed << std::setprecision(2) << global_total_volume << std::endl;
            std::cout << "----------------------------------\n";
            std::cout << "Average Trades per Sim: " << (double)global_total_trades / config.ensemble_count << std::endl;
            std::cout << "Average Volume per Sim: $" << (double)global_total_volume / config.ensemble_count << std::endl;
            std::cout << "Ensemble Avg. Price: $" << (double)global_sum_avg_price / config.ensemble_count << std::endl;
            std::cout << "\nEnsemble run complete. Logs saved to 'logs/' directory.\n";
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
            
            auto human_trade_action = [&](OrderType type) {
                try {
                    double price = std::stod(human_price_str);
                    int qty = std::stoi(human_qty_str);
                    int trader_id = std::stoi(human_trader_id);
                    
                    if (qty <= 0) {
                        last_action_msg = "Error: Qty must be > 0";
                        return;
                    }

                    // Get current time from simulation
                    double timestamp = simulation.getStats().simulation_time;

                    Order human_order(0, trader_id, type, price, qty, timestamp);
                    simulation.addHumanOrder(human_order); // Inject the order
                    
                    last_action_msg = (type == OrderType::BUY ? "BUY" : "SELL");
                    last_action_msg += " order for " + human_qty_str + " @ $" + human_price_str + " sent!";

                } catch (const std::exception& e) {
                    last_action_msg = "Error: Invalid price or qty";
                }
            };

            Component buy_button = Button("  BUY  ", [&]() { human_trade_action(OrderType::BUY); }, ButtonOption::Animated(Color::Green));
            Component sell_button = Button("  SELL  ", [&]() { human_trade_action(OrderType::SELL); }, ButtonOption::Animated(Color::Red));

            // --- NEW: Add components to a container for focus ---
            auto main_container = Container::Vertical({
                price_input,
                qty_input,
                buy_button,
                sell_button
            });

            // Handle 'q' to quit
            main_container |= CatchEvent([&](Event event) {
                if (event == Event::Character('q') || event == Event::Character('Q')) {
                    running = false;
                    screen.ExitLoopClosure()();
                    return true;
                }
                return false; 
            });

            // Create renderer
            auto renderer = Renderer(main_container, [&] {
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
                    double initial_price = config.initial_price;
                    double max_price_seen = *std::max_element(price_history.begin(), price_history.end());
                    double min_price_seen = *std::min_element(price_history.begin(), price_history.end());
                    max_price_seen = std::max(max_price_seen, initial_price); min_price_seen = std::min(min_price_seen, initial_price);
                    double price_range = max_price_seen - min_price_seen; if (price_range < 1.0) price_range = 1.0;
                    double buffer = price_range * 0.1; double max_price = max_price_seen + buffer; double min_price = min_price_seen - buffer;
                    double range = max_price - min_price; if (range < 1.0) range = 1.0;
                    const int graph_height = 15;
                    double initial_price_normalized = (initial_price - min_price) / range;
                    int baseline_pos = static_cast<int>(initial_price_normalized * (graph_height - 1));
                    baseline_pos = std::max(0, std::min(graph_height - 1, baseline_pos));
                    std::vector<Element> columns;
                    for (size_t i = 0; i < price_history.size(); i++) {
                        double price = price_history[i];
                        double normalized = (price - min_price) / range;
                        int filled_lines = static_cast<int>(normalized * (graph_height - 1));
                        filled_lines = std::max(0, std::min(graph_height - 1, filled_lines));
                        Color bar_color = (price > initial_price) ? Color::Green : Color::Red;
                        std::vector<Element> vertical_blocks;
                        for (int line = 0; line < graph_height; line++) {
                            int line_from_bottom = graph_height - 1 - line;
                            if (line_from_bottom == baseline_pos) {
                                if (line_from_bottom == filled_lines) vertical_blocks.push_back(text("◆") | color(bar_color) | bold);
                                else vertical_blocks.push_back(text("─") | color(Color::Yellow));
                            } else if (line_from_bottom == filled_lines) {
                                vertical_blocks.push_back(text("●") | color(bar_color));
                            } else vertical_blocks.push_back(text(" "));
                        }
                        columns.push_back(vbox(std::move(vertical_blocks)));
                    }
                    elements.push_back(hbox(std::move(columns)) | border | flex);
                }
                elements.push_back(separator());
                
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
                    text(last_action_msg) | center | dim
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
                
                return vbox(std::move(elements)) | border; 
            });

            // Refresh thread
            std::thread refresh_thread([&] {
                while (running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    screen.Post(Event::Custom);
                } 
            });

            screen.Loop(renderer);

            running = false;
            if (refresh_thread.joinable()) {
                refresh_thread.join();
            }

            // --- (Final Summary Printout is the same) ---
            std::cout << "\n=== Simulation Complete ===\n\n";
            auto stats = simulation.getStats();
            std::cout << "Duration: " << stats.simulation_time << " seconds\n";
            std::cout << "Total Trades: " << stats.total_trades << "\n";
            std::cout << "Total Volume: $" << std::fixed << std::setprecision(2) << stats.total_volume << "\n";
            simulation.getLogger().flush();
            simulation.getLogger().exportToJSON("simulation_summary.json");
            std::cout << "Logs saved to 'logs' directory and 'simulation_summary.json'\n\n";
            const auto &traders = simulation.getTraders();
            std::vector<const Trader *> final_traders;
            for (const auto &trader : traders) final_traders.push_back(trader.get());
            double final_price = simulation.getMarket().getCurrentPrice();
            std::sort(final_traders.begin(), final_traders.end(), [](const Trader *a, const Trader *b) { return a->getId() < b->getId(); });
            std::cout << "=== Final Trader Rankings (By ID) ===\n\n";
            for (const auto *t : final_traders) {
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