#include "TradingClient.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void printHelp() {
    std::cout << "\nAvailable commands:" << std::endl;
    std::cout << "  help                    - Show this help message" << std::endl;
    std::cout << "  connect                 - Connect to the server" << std::endl;
    std::cout << "  disconnect              - Disconnect from the server" << std::endl;
    std::cout << "  order <symbol> <type> <side> <quantity> [price] - Place an order" << std::endl;
    std::cout << "    types: market, limit, ioc, fok" << std::endl;
    std::cout << "    sides: buy, sell" << std::endl;
    std::cout << "  subscribe <symbol>      - Subscribe to market data" << std::endl;
    std::cout << "  unsubscribe <symbol>    - Unsubscribe from market data" << std::endl;
    std::cout << "  quit                    - Exit the program" << std::endl;
}

void handleMessage(const std::string& message) {
    std::cout << "\nReceived: " << message << std::endl;
}

void handleConnectionStatus(bool connected) {
    std::cout << "\nConnection status: " << (connected ? "Connected" : "Disconnected") << std::endl;
}

int main() {
    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Create trading client
    TradingClient client;
    client.setMessageHandler(handleMessage);
    client.setConnectionStatusHandler(handleConnectionStatus);

    std::string command;
    printHelp();

    while (running) {
        std::cout << "\nEnter command (or 'help'): ";
        std::getline(std::cin, command);

        if (command == "quit") {
            running = false;
        }
        else if (command == "help") {
            printHelp();
        }
        else if (command == "connect") {
            if (client.connect()) {
                std::cout << "Successfully connected to server" << std::endl;
            } else {
                std::cout << "Failed to connect to server" << std::endl;
            }
        }
        else if (command == "disconnect") {
            client.disconnect();
            std::cout << "Disconnected from server" << std::endl;
        }
        else if (command.substr(0, 5) == "order") {
            std::string symbol, type, side;
            double quantity, price = 0.0;
            std::istringstream iss(command.substr(6));
            
            if (iss >> symbol >> type >> side >> quantity) {
                if (iss >> price) {
                    // Price was provided
                }
                
                if (client.placeOrder(symbol, type, side, quantity, price)) {
                    std::cout << "Order placed successfully" << std::endl;
                } else {
                    std::cout << "Failed to place order" << std::endl;
                }
            } else {
                std::cout << "Invalid order command format. Use: order <symbol> <type> <side> <quantity> [price]" << std::endl;
            }
        }
        else if (command.substr(0, 9) == "subscribe") {
            std::string symbol;
            std::istringstream iss(command.substr(10));
            
            if (iss >> symbol) {
                if (client.subscribeToMarketData(symbol)) {
                    std::cout << "Subscribed to " << symbol << " market data" << std::endl;
                } else {
                    std::cout << "Failed to subscribe to market data" << std::endl;
                }
            } else {
                std::cout << "Invalid subscribe command format. Use: subscribe <symbol>" << std::endl;
            }
        }
        else if (command.substr(0, 11) == "unsubscribe") {
            std::string symbol;
            std::istringstream iss(command.substr(12));
            
            if (iss >> symbol) {
                if (client.unsubscribeFromMarketData(symbol)) {
                    std::cout << "Unsubscribed from " << symbol << " market data" << std::endl;
                } else {
                    std::cout << "Failed to unsubscribe from market data" << std::endl;
                }
            } else {
                std::cout << "Invalid unsubscribe command format. Use: unsubscribe <symbol>" << std::endl;
            }
        }
        else {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }

    // Cleanup
    client.disconnect();
    std::cout << "Trading client shutdown complete." << std::endl;
    return 0;
} 