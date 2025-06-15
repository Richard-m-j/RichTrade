#pragma once
#include <string>
#include <cstdint>
#include <chrono>

class Order {
public:
    enum class Type { MARKET, LIMIT, IOC, FOK };
    enum class Side { BUY, SELL };
    enum class Status { NEW, PARTIALLY_FILLED, FILLED, CANCELLED };

    Order(const std::string& order_id,
          const std::string& symbol,
          Type type,
          Side side,
          double quantity,
          double price,
          const std::string& timestamp);

    // Getters
    const std::string& getOrderId() const;
    const std::string& getSymbol() const;
    Type getType() const;
    Side getSide() const;
    double getQuantity() const;
    double getPrice() const;
    const std::string& getTimestamp() const;
    Status getStatus() const;

    // Setters
    void setStatus(Status status);
    void setQuantity(double quantity);

private:
    std::string order_id_;
    std::string symbol_;
    Type type_;
    Side side_;
    double quantity_;
    double price_;
    std::string timestamp_;
    Status status_;
}; 