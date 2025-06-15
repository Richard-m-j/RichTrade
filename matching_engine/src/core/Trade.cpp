#include "Trade.h"
#include <nlohmann/json.hpp>

std::string Trade::toJSON() const {
    nlohmann::json j = {
        {"trade_id", trade_id},
        {"timestamp", timestamp},
        {"symbol", symbol},
        {"price", price},
        {"quantity", quantity},
        {"aggressor_side", aggressor_side},
        {"maker_order_id", maker_order_id},
        {"taker_order_id", taker_order_id}
    };
    return j.dump();
} 