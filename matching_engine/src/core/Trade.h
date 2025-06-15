#pragma once
#include <string>

struct Trade {
    std::string trade_id;
    std::string timestamp;
    std::string symbol;
    double price;
    double quantity;
    std::string aggressor_side; // "buy" or "sell"
    std::string maker_order_id;
    std::string taker_order_id;

    std::string toJSON() const;
}; 