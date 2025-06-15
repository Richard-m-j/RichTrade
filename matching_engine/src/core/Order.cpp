#include "Order.h"

Order::Order(const std::string& order_id,
             const std::string& symbol,
             Type type,
             Side side,
             double quantity,
             double price,
             const std::string& timestamp)
    : order_id_(order_id), symbol_(symbol), type_(type), side_(side),
      quantity_(quantity), price_(price), timestamp_(timestamp), status_(Status::NEW) {}

const std::string& Order::getOrderId() const { return order_id_; }
const std::string& Order::getSymbol() const { return symbol_; }
Order::Type Order::getType() const { return type_; }
Order::Side Order::getSide() const { return side_; }
double Order::getQuantity() const { return quantity_; }
double Order::getPrice() const { return price_; }
const std::string& Order::getTimestamp() const { return timestamp_; }
Order::Status Order::getStatus() const { return status_; }

void Order::setStatus(Status status) { status_ = status; }
void Order::setQuantity(double quantity) { quantity_ = quantity; } 