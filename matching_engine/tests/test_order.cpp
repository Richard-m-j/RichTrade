#include <gtest/gtest.h>
#include "../src/core/Order.h"

TEST(OrderTest, ConstructionAndGetters) {
    Order order("id1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.5, 50000.0, "2025-06-14T10:30:45.123456Z");
    EXPECT_EQ(order.getOrderId(), "id1");
    EXPECT_EQ(order.getSymbol(), "BTC-USDT");
    EXPECT_EQ(order.getType(), Order::Type::LIMIT);
    EXPECT_EQ(order.getSide(), Order::Side::BUY);
    EXPECT_DOUBLE_EQ(order.getQuantity(), 1.5);
    EXPECT_DOUBLE_EQ(order.getPrice(), 50000.0);
    EXPECT_EQ(order.getTimestamp(), "2025-06-14T10:30:45.123456Z");
    EXPECT_EQ(order.getStatus(), Order::Status::NEW);
}

TEST(OrderTest, Setters) {
    Order order("id2", "ETH-USDT", Order::Type::MARKET, Order::Side::SELL, 2.0, 0.0, "2025-06-14T10:31:00.000000Z");
    order.setStatus(Order::Status::PARTIALLY_FILLED);
    EXPECT_EQ(order.getStatus(), Order::Status::PARTIALLY_FILLED);
    order.setQuantity(1.0);
    EXPECT_DOUBLE_EQ(order.getQuantity(), 1.0);
} 