#include "MatchingEngine.h"
#include <stdexcept>
#include <mutex>

MatchingEngine::MatchingEngine() {}

void MatchingEngine::setOnTrade(const TradeCallback& callback) {
    on_trade_cb_ = callback;
}

void MatchingEngine::notifyTrade(const Trade& trade) {
    if (on_trade_cb_) {
        on_trade_cb_(trade);
    }
}

std::vector<Trade> MatchingEngine::processOrder(const Order& order) {
    if (order_books_.find(order.getSymbol()) == order_books_.end()) {
        order_books_[order.getSymbol()] = std::make_shared<OrderBook>(order.getSymbol());
    }
    Order::Type type = order.getType();
    std::vector<Trade> trades;
    
    switch (type) {
        case Order::Type::MARKET:
            trades = matchMarketOrder(const_cast<Order&>(order));
            break;
        case Order::Type::LIMIT:
            trades = matchLimitOrder(const_cast<Order&>(order));
            break;
        case Order::Type::IOC:
            trades = matchIOCOrder(const_cast<Order&>(order));
            break;
        case Order::Type::FOK:
            trades = matchFOKOrder(const_cast<Order&>(order));
            break;
        default:
            throw std::invalid_argument("Unknown order type");
    }

    // Notify about each trade
    for (const auto& trade : trades) {
        notifyTrade(trade);
    }

    return trades;
}

std::vector<Trade> MatchingEngine::matchMarketOrder(Order& order) {
    std::vector<Trade> trades;
    auto& book = order_books_[order.getSymbol()];
    std::unique_lock<std::mutex> lock(book->mtx_);
    double remaining_qty = order.getQuantity();
    auto side = order.getSide();
    if (side == Order::Side::BUY) {
        // BUY: match against asks_
        for (auto it = book->asks_.begin(); it != book->asks_.end() && remaining_qty > 0;) {
            double price = it->first;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "buy";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->asks_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // SELL: match against bids_
        for (auto it = book->bids_.begin(); it != book->bids_.end() && remaining_qty > 0;) {
            double price = it->first;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "sell";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->bids_.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (remaining_qty == 0) {
        order.setStatus(Order::Status::FILLED);
    } else if (remaining_qty < order.getQuantity()) {
        order.setStatus(Order::Status::PARTIALLY_FILLED);
        order.setQuantity(remaining_qty);
    } else {
        order.setStatus(Order::Status::NEW);
    }
    return trades;
}

std::vector<Trade> MatchingEngine::matchLimitOrder(Order& order) {
    std::vector<Trade> trades;
    auto& book = order_books_[order.getSymbol()];
    std::unique_lock<std::mutex> lock(book->mtx_);
    double remaining_qty = order.getQuantity();
    double limit_price = order.getPrice();
    auto side = order.getSide();
    if (side == Order::Side::BUY) {
        for (auto it = book->asks_.begin(); it != book->asks_.end() && remaining_qty > 0;) {
            double price = it->first;
            if (price > limit_price) break;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "buy";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->asks_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        for (auto it = book->bids_.begin(); it != book->bids_.end() && remaining_qty > 0;) {
            double price = it->first;
            if (price < limit_price) break;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "sell";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->bids_.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (remaining_qty > 0) {
        if (remaining_qty < order.getQuantity()) {
            order.setStatus(Order::Status::PARTIALLY_FILLED);
        } else {
            order.setStatus(Order::Status::NEW);
        }
        order.setQuantity(remaining_qty);
        lock.unlock();
        book->addOrder(std::make_shared<Order>(order));
    } else {
        order.setStatus(Order::Status::FILLED);
    }
    return trades;
}

std::vector<Trade> MatchingEngine::matchIOCOrder(Order& order) {
    std::vector<Trade> trades;
    auto& book = order_books_[order.getSymbol()];
    std::unique_lock<std::mutex> lock(book->mtx_);
    double remaining_qty = order.getQuantity();
    double limit_price = order.getPrice();
    auto side = order.getSide();
    if (side == Order::Side::BUY) {
        for (auto it = book->asks_.begin(); it != book->asks_.end() && remaining_qty > 0;) {
            double price = it->first;
            if (price > limit_price) break;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "buy";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->asks_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        for (auto it = book->bids_.begin(); it != book->bids_.end() && remaining_qty > 0;) {
            double price = it->first;
            if (price < limit_price) break;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "sell";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->bids_.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (remaining_qty == 0) {
        order.setStatus(Order::Status::FILLED);
    } else if (remaining_qty < order.getQuantity()) {
        order.setStatus(Order::Status::PARTIALLY_FILLED);
        order.setQuantity(remaining_qty);
    } else {
        order.setStatus(Order::Status::CANCELLED);
    }
    return trades;
}

std::vector<Trade> MatchingEngine::matchFOKOrder(Order& order) {
    std::vector<Trade> trades;
    auto& book = order_books_[order.getSymbol()];
    std::unique_lock<std::mutex> lock(book->mtx_);
    double remaining_qty = order.getQuantity();
    double limit_price = order.getPrice();
    auto side = order.getSide();
    double available_qty = 0.0;
    if (side == Order::Side::BUY) {
        for (auto it = book->asks_.begin(); it != book->asks_.end() && available_qty < remaining_qty; ++it) {
            double price = it->first;
            if (price > limit_price) break;
            std::queue<std::shared_ptr<Order>> queue = it->second;
            while (!queue.empty() && available_qty < remaining_qty) {
                available_qty += queue.front()->getQuantity();
                queue.pop();
            }
        }
    } else {
        for (auto it = book->bids_.begin(); it != book->bids_.end() && available_qty < remaining_qty; ++it) {
            double price = it->first;
            if (price < limit_price) break;
            std::queue<std::shared_ptr<Order>> queue = it->second;
            while (!queue.empty() && available_qty < remaining_qty) {
                available_qty += queue.front()->getQuantity();
                queue.pop();
            }
        }
    }
    if (available_qty < remaining_qty) {
        order.setStatus(Order::Status::CANCELLED);
        return trades;
    }
    remaining_qty = order.getQuantity();
    if (side == Order::Side::BUY) {
        for (auto it = book->asks_.begin(); it != book->asks_.end() && remaining_qty > 0;) {
            double price = it->first;
            if (price > limit_price) break;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "buy";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->asks_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        for (auto it = book->bids_.begin(); it != book->bids_.end() && remaining_qty > 0;) {
            double price = it->first;
            if (price < limit_price) break;
            auto& queue = it->second;
            while (!queue.empty() && remaining_qty > 0) {
                auto& resting_order = queue.front();
                double match_qty = std::min(remaining_qty, resting_order->getQuantity());
                Trade trade;
                trade.trade_id = std::to_string(rand());
                trade.timestamp = order.getTimestamp();
                trade.symbol = order.getSymbol();
                trade.price = resting_order->getPrice();
                trade.quantity = match_qty;
                trade.aggressor_side = "sell";
                trade.maker_order_id = resting_order->getOrderId();
                trade.taker_order_id = order.getOrderId();
                trades.push_back(trade);
                remaining_qty -= match_qty;
                double new_resting_qty = resting_order->getQuantity() - match_qty;
                resting_order->setQuantity(new_resting_qty);
                if (new_resting_qty == 0) {
                    resting_order->setStatus(Order::Status::FILLED);
                    queue.pop();
                } else {
                    resting_order->setStatus(Order::Status::PARTIALLY_FILLED);
                }
            }
            if (queue.empty()) {
                it = book->bids_.erase(it);
            } else {
                ++it;
            }
        }
    }
    order.setStatus(Order::Status::FILLED);
    return trades;
} 