#pragma once
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <string>
#include "Order.h"

class OrderBook {
public:
    OrderBook(const std::string& symbol);

    void addOrder(const std::shared_ptr<Order>& order);
    void removeOrder(const std::string& order_id, Order::Side side, double price);
    std::pair<double, double> getBBO() const; // (best_bid, best_ask)
    std::vector<std::pair<double, double>> getDepth(Order::Side side, int levels) const;
    std::string getMarketDepth(int levels) const; // JSON
    std::string getSnapshot() const; // JSON

    // Register a callback for real-time updates
    void setOnOrderBookChange(const std::function<void()>& cb);

    // Expose for MatchingEngine
    std::map<double, std::queue<std::shared_ptr<Order>>, std::greater<double>> bids_;
    std::map<double, std::queue<std::shared_ptr<Order>>, std::less<double>> asks_;
    mutable std::mutex mtx_;

private:
    std::string symbol_;
    double best_bid_ = 0.0;
    double best_ask_ = 0.0;
    std::function<void()> on_change_cb_;
    void updateBBO();
    void notifyChange();
}; 