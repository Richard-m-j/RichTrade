#pragma once
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include "../core/MatchingEngine.h"

class RestServer {
public:
    RestServer(std::shared_ptr<MatchingEngine> engine, int port = 8080);
    ~RestServer();
    
    void start();
    void stop();
    
private:
    std::shared_ptr<MatchingEngine> engine_;
    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    void registerHandlers();
}; 