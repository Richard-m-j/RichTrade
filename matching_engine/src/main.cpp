#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include "utils/Logger.h"
#include "core/MatchingEngine.h"
#include "api/RestServer.h"
#include "api/WebSocketServer.h"

std::shared_ptr<MatchingEngine> engine;
std::shared_ptr<RestServer> rest_server;
std::shared_ptr<WebSocketServer> ws_server;
std::atomic<bool> running(true);
std::condition_variable cv;
std::mutex cv_mutex;
bool servers_ready = false;

void signal_handler(int signal) {
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down...");
    running = false;
    cv.notify_all();
}

int main() {
    try {
        // Set up signal handling
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // Initialize logging
        Logger::setLevel(Logger::Level::INFO);
        Logger::info("Matching Engine starting up...");

        // Create matching engine instance
        engine = std::make_shared<MatchingEngine>();

        // Start REST server on port 8080
        rest_server = std::make_shared<RestServer>(engine, 8080);
        std::thread rest_thread([&]() {
            try {
                Logger::info("Starting REST server on port 8080...");
                rest_server->start();
            } catch (const std::exception& e) {
                Logger::err("REST server error: " + std::string(e.what()));
                running = false;
                cv.notify_all();
            }
        });

        // Start WebSocket server on port 9002
        ws_server = std::make_shared<WebSocketServer>(engine, 9002);
        std::thread ws_thread([&]() {
            try {
                Logger::info("Starting WebSocket server on port 9002...");
                ws_server->start();
                
                // Notify that servers are ready
                {
                    std::lock_guard<std::mutex> lock(cv_mutex);
                    servers_ready = true;
                }
                cv.notify_all();
            } catch (const std::exception& e) {
                Logger::err("WebSocket server error: " + std::string(e.what()));
                running = false;    
                cv.notify_all();
            }
        });

        // Wait for servers to be ready or error
        {
            std::unique_lock<std::mutex> lock(cv_mutex);
            cv.wait(lock, [&]() { return servers_ready || !running; });
        }

        if (!running) {
            Logger::err("Failed to start servers");
            return 1;
        }

        // Main loop
        Logger::info("Matching Engine is running. Press Ctrl+C to stop.");
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Cleanup
        Logger::info("Shutting down servers...");
        if (ws_server) ws_server->stop();

        // Wait for server threads to finish
        if (rest_thread.joinable()) rest_thread.join();
        if (ws_thread.joinable()) ws_thread.join();

        Logger::info("Matching Engine shutdown complete.");
        return 0;

    } catch (const std::exception& e) {
        Logger::err("Fatal error: " + std::string(e.what()));
        return 1;
    }
} 