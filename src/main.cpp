#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "../include/simulation.hpp"

using namespace ftxui;

struct Config
{
    int num_traders = 12;
    double initial_price = 100.0;
    double initial_cash = 10000.0;
    int duration_seconds = 60;
    bool show_help = false;
};

void printHelp()
{
    std::cout << "Algorithmic Trading Simulation\n\n";
    std::cout << "Usage: ftxui_demo [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t, --traders <num>     Number of trader agents (default: 12)\n";
    std::cout << "  -d, --duration <sec>    Simulation duration in seconds (default: 60)\n";
    std::cout << "  -p, --price <value>     Initial asset price (default: 100.0)\n";
    std::cout << "  -c, --cash <value>      Initial cash per trader (default: 10000.0)\n";
    std::cout << "  -h, --help              Show this help message\n\n";
    std::cout << "Example:\n";
    std::cout << "  ftxui_demo -t 20 -d 120 -p 150.0\n\n";
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
    }

    return config;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    Config config = parseArguments(argc, argv);

    if (config.show_help)
    {
        printHelp();
        return 0;
    }

    // Create simulation
    TradingSimulation simulation(config.num_traders, config.initial_price, config.initial_cash);

    auto screen = ScreenInteractive::Fullscreen();

    bool running = true;
    auto start_time = std::chrono::steady_clock::now();
    auto last_update = std::chrono::steady_clock::now();

    // Create quit button
    auto quit_button = Button("Stop Simulation (or press 'q')", [&]
                              { 
        running = false;
        screen.ExitLoopClosure()(); });

    auto container = Container::Vertical({quit_button});

    // Handle keyboard events
    container |= CatchEvent([&](Event event)
                            {
        if (event == Event::Character('q') || event == Event::Character('Q')) {
            running = false;
            screen.ExitLoopClosure()();
            return true;
        }
        return false; });

    // Create renderer
    auto renderer = Renderer(container, [&]
                             {
        // Check elapsed time
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        
        // Stop simulation after duration
        if (elapsed >= config.duration_seconds && running) {
            running = false;
            screen.ExitLoopClosure()();
        }
        
        // Run simulation steps
        auto step_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
        if (step_elapsed >= 100 && running) {
            simulation.step();
            last_update = now;
        }
        
        std::vector<Element> elements;
        
        // Title
        elements.push_back(
            text("🏦 Algorithmic Trading Simulation") | bold | color(Color::Cyan) | center
        );
        elements.push_back(separator());
        
        // Simulation info
        int remaining = config.duration_seconds - elapsed;
        std::stringstream ss;
        ss << "Time: " << elapsed << "s / " << config.duration_seconds << "s";
        if (remaining > 0) {
            ss << " (Remaining: " << remaining << "s)";
        } else {
            ss << " [COMPLETED]";
        }
        
        elements.push_back(
            hbox({
                text("Traders: " + std::to_string(config.num_traders)) | color(Color::Yellow),
                text(" | "),
                text(ss.str()) | color(running ? Color::Green : Color::Red),
            }) | center
        );
        elements.push_back(separator());
        
        // Market price graph
        const auto& market = simulation.getMarket();
        double current_price = market.getCurrentPrice();
        auto price_history = market.getRecentHistory(60);
        
        elements.push_back(
            hbox({
                text("Current Price: ") | bold,
                text("$" + std::to_string(static_cast<int>(current_price))) | 
                    color(Color::GreenLight) | bold,
            })
        );
        
        // Price graph
        std::vector<Element> price_bars;
        if (!price_history.empty()) {
            double min_price = *std::min_element(price_history.begin(), price_history.end());
            double max_price = *std::max_element(price_history.begin(), price_history.end());
            double range = max_price - min_price;
            if (range < 1.0) range = 1.0;
            
            for (double price : price_history) {
                double normalized = (price - min_price) / range;
                int bar_height = static_cast<int>(normalized * 8);
                std::string bar_char;
                if (bar_height == 0) bar_char = "▁";
                else if (bar_height <= 2) bar_char = "▂";
                else if (bar_height <= 4) bar_char = "▄";
                else if (bar_height <= 6) bar_char = "▆";
                else bar_char = "█";
                
                price_bars.push_back(text(bar_char) | color(Color::Cyan));
            }
        }
        elements.push_back(hbox(std::move(price_bars)) | border);
        elements.push_back(separator());
        
        // Trader statistics
        elements.push_back(text("Top Traders") | bold | color(Color::Yellow));
        
        const auto& traders = simulation.getTraders();
        std::vector<const Trader*> sorted_traders;
        for (const auto& trader : traders) {
            sorted_traders.push_back(trader.get());
        }
        
        // Sort by net worth
        std::sort(sorted_traders.begin(), sorted_traders.end(), 
            [current_price](const Trader* a, const Trader* b) {
                return a->getNetWorth(current_price) > b->getNetWorth(current_price);
            });
        
        // Show top 5 traders
        for (int i = 0; i < std::min(5, static_cast<int>(sorted_traders.size())); i++) {
            const Trader* t = sorted_traders[i];
            double net_worth = t->getNetWorth(current_price);
            double profit = net_worth - config.initial_cash;
            Color profit_color = profit >= 0 ? Color::Green : Color::Red;
            
            std::stringstream trader_info;
            trader_info << std::fixed << std::setprecision(0);
            trader_info << "T" << t->getId() << " [" << t->getStrategyName() << "] ";
            trader_info << "Worth: $" << net_worth;
            trader_info << " | Profit: " << (profit >= 0 ? "+" : "") << "$" << profit;
            trader_info << " | Trades: " << t->getTradesExecuted();
            
            elements.push_back(
                text(trader_info.str()) | color(profit_color)
            );
        }
        elements.push_back(separator());
        
        // Overall statistics
        auto stats = simulation.getStats();
        elements.push_back(text("Market Statistics") | bold | color(Color::Yellow));
        elements.push_back(
            text("Total Trades: " + std::to_string(stats.total_trades))
        );
        elements.push_back(
            text("Total Volume: $" + std::to_string(static_cast<int>(stats.total_volume)))
        );
        elements.push_back(
            text("Avg Trade Price: $" + std::to_string(static_cast<int>(stats.avg_price)))
        );
        elements.push_back(
            text("Price Volatility: $" + std::to_string(static_cast<int>(stats.price_volatility)))
        );
        
        elements.push_back(separator());
        elements.push_back(quit_button->Render() | center);
        elements.push_back(separator());
        elements.push_back(
            text("Press 'q' to stop | OpenMP enabled for parallel trader decisions") | dim | center
        );
        
        return vbox(std::move(elements)) | border; });

    // Refresh thread
    std::thread refresh_thread([&]
                               {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            screen.Post(Event::Custom);
        } });

    // Run the simulation
    screen.Loop(renderer);

    running = false;
    if (refresh_thread.joinable())
    {
        refresh_thread.join();
    }

    // Print final summary
    std::cout << "\n=== Simulation Complete ===\n\n";

    auto stats = simulation.getStats();
    std::cout << "Duration: " << stats.simulation_time << " seconds\n";
    std::cout << "Total Trades: " << stats.total_trades << "\n";
    std::cout << "Total Volume: $" << std::fixed << std::setprecision(2) << stats.total_volume << "\n";
    std::cout << "Average Price: $" << stats.avg_price << "\n";
    std::cout << "Price Volatility: $" << stats.price_volatility << "\n\n";

    // Final trader rankings
    std::cout << "=== Final Trader Rankings ===\n\n";

    const auto &traders = simulation.getTraders();
    std::vector<const Trader *> sorted_traders;
    for (const auto &trader : traders)
    {
        sorted_traders.push_back(trader.get());
    }

    double final_price = simulation.getMarket().getCurrentPrice();
    std::sort(sorted_traders.begin(), sorted_traders.end(),
              [final_price](const Trader *a, const Trader *b)
              {
                  return a->getNetWorth(final_price) > b->getNetWorth(final_price);
              });

    for (int i = 0; i < sorted_traders.size(); i++)
    {
        const Trader *t = sorted_traders[i];
        double net_worth = t->getNetWorth(final_price);
        double profit = net_worth - config.initial_cash;

        std::cout << (i + 1) << ". Trader " << t->getId()
                  << " [" << t->getStrategyName() << "]\n";
        std::cout << "   Net Worth: $" << std::fixed << std::setprecision(2) << net_worth << "\n";
        std::cout << "   Profit/Loss: " << (profit >= 0 ? "+" : "") << "$" << profit << "\n";
        std::cout << "   Trades Executed: " << t->getTradesExecuted() << "\n";
        std::cout << "   Holdings: " << t->getHoldings() << " shares\n";
        std::cout << "   Cash: $" << t->getCash() << "\n\n";
    }

    return 0;
}
