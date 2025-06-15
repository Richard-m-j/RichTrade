#include "OrderBook.h"
#include <algorithm>
#include <nlohmann/json.hpp>

OrderBook::OrderBook(const std::string& symbol) : symbol_(symbol), best_bid_(0.0), best_ask_(0.0) {}

void OrderBook::addOrder(const std::shared_ptr<Order>& order) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (order->getSide() == Order::Side::BUY) {
        bids_[order->getPrice()].push(order);
        if (order->getPrice() > best_bid_ || best_bid_ == 0.0) {
            best_bid_ = order->getPrice();
        }
    } else {
        asks_[order->getPrice()].push(order);
        if (best_ask_ == 0.0 || order->getPrice() < best_ask_) {
            best_ask_ = order->getPrice();
        }
    }
    notifyChange();
}

void OrderBook::removeOrder(const std::string& order_id, Order::Side side, double price) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (side == Order::Side::BUY) {
        auto it = bids_.find(price);
        if (it != bids_.end()) {
            std::queue<std::shared_ptr<Order>>& q = it->second;
            std::queue<std::shared_ptr<Order>> new_q;
            while (!q.empty()) {
                auto o = q.front();
                q.pop();
                if (o->getOrderId() != order_id) {
                    new_q.push(o);
                }
            }
            if (new_q.empty()) {
                bids_.erase(it);
            } else {
                it->second = std::move(new_q);
            }
        }
    } else {
        auto it = asks_.find(price);
        if (it != asks_.end()) {
            std::queue<std::shared_ptr<Order>>& q = it->second;
            std::queue<std::shared_ptr<Order>> new_q;
            while (!q.empty()) {
                auto o = q.front();
                q.pop();
                if (o->getOrderId() != order_id) {
                    new_q.push(o);
                }
            }
            if (new_q.empty()) {
                asks_.erase(it);
            } else {
                it->second = std::move(new_q);
            }
        }
    }
    updateBBO();
    notifyChange();
}

std::pair<double, double> OrderBook::getBBO() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return {best_bid_, best_ask_};
}

std::vector<std::pair<double, double>> OrderBook::getDepth(Order::Side side, int levels) const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::pair<double, double>> depth;
    if (side == Order::Side::BUY) {
        int count = 0;
        for (const auto& entry : bids_) {
            double price = entry.first;
            const auto& queue = entry.second;
            double qty = 0.0;
            std::queue<std::shared_ptr<Order>> q = queue;
            while (!q.empty()) {
                qty += q.front()->getQuantity();
                q.pop();
            }
            depth.emplace_back(price, qty);
            if (++count >= levels) break;
        }
    } else {
        int count = 0;
        for (const auto& entry : asks_) {
            double price = entry.first;
            const auto& queue = entry.second;
            double qty = 0.0;
            std::queue<std::shared_ptr<Order>> q = queue;
            while (!q.empty()) {
                qty += q.front()->getQuantity();
                q.pop();
            }
            depth.emplace_back(price, qty);
            if (++count >= levels) break;
        }
    }
    return depth;
}

std::string OrderBook::getMarketDepth(int levels) const {
    std::lock_guard<std::mutex> lock(mtx_);
    nlohmann::json j;
    j["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    j["symbol"] = symbol_;
    j["asks"] = nlohmann::json::array();
    j["bids"] = nlohmann::json::array();
    // Asks (price ascending)
    int count = 0;
    for (const auto& entry : asks_) {
        if (count++ >= levels) break;
        double price = entry.first;
        double qty = 0.0;
        std::queue<std::shared_ptr<Order>> q = entry.second;
        while (!q.empty()) {
            qty += q.front()->getQuantity();
            q.pop();
        }
        j["asks"].push_back({nlohmann::json::array({price, qty})});
    }
    // Bids (price descending)
    count = 0;
    for (const auto& entry : bids_) {
        if (count++ >= levels) break;
        double price = entry.first;
        double qty = 0.0;
        std::queue<std::shared_ptr<Order>> q = entry.second;
        while (!q.empty()) {
            qty += q.front()->getQuantity();
            q.pop();
        }
        j["bids"].push_back({nlohmann::json::array({price, qty})});
    }
    return j.dump();
}

std::string OrderBook::getSnapshot() const {
    std::lock_guard<std::mutex> lock(mtx_);
    nlohmann::json j;
    j["symbol"] = symbol_;
    j["bids"] = nlohmann::json::array();
    j["asks"] = nlohmann::json::array();
    for (const auto& entry : bids_) {
        double price = entry.first;
        double qty = 0.0;
        std::queue<std::shared_ptr<Order>> q = entry.second;
        while (!q.empty()) {
            qty += q.front()->getQuantity();
            q.pop();
        }
        j["bids"].push_back({nlohmann::json::array({price, qty})});
    }
    for (const auto& entry : asks_) {
        double price = entry.first;
        double qty = 0.0;
        std::queue<std::shared_ptr<Order>> q = entry.second;
        while (!q.empty()) {
            qty += q.front()->getQuantity();
            q.pop();
        }
        j["asks"].push_back({nlohmann::json::array({price, qty})});
    }
    return j.dump();
}

void OrderBook::setOnOrderBookChange(const std::function<void()>& cb) {
    std::lock_guard<std::mutex> lock(mtx_);
    on_change_cb_ = cb;
}

void OrderBook::updateBBO() {
    // Assumes mtx_ is already locked
    best_bid_ = bids_.empty() ? 0.0 : bids_.begin()->first;
    best_ask_ = asks_.empty() ? 0.0 : asks_.begin()->first;
}

void OrderBook::notifyChange() {
    if (on_change_cb_) on_change_cb_();
} 