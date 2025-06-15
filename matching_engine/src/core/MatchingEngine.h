#pragma once
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include "Order.h"
#include "Trade.h"
#include "OrderBook.h"

class MatchingEngine {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    MatchingEngine();
    std::vector<Trade> processOrder(const Order& order);
    
    // Register callback for trade notifications
    void setOnTrade(const TradeCallback& callback);
    
    // Expose order books for WebSocket server
    std::map<std::string, std::shared_ptr<OrderBook>> order_books_;

private:
    std::vector<Trade> matchMarketOrder(Order& order);
    std::vector<Trade> matchLimitOrder(Order& order);
    std::vector<Trade> matchIOCOrder(Order& order);
    std::vector<Trade> matchFOKOrder(Order& order);

    TradeCallback on_trade_cb_;
    void notifyTrade(const Trade& trade);
}; 