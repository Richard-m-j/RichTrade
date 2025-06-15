#include <gtest/gtest.h>
#include "../src/core/OrderBook.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <atomic>

TEST(OrderBookTest, AddAndBBO) {
    OrderBook ob("BTC-USDT");
    auto buy1 = std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z");
    auto sell1 = std::make_shared<Order>("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50100.0, "2025-06-14T10:00:01.000000Z");
    ob.addOrder(buy1);
    ob.addOrder(sell1);
    auto bbo = ob.getBBO();
    EXPECT_DOUBLE_EQ(bbo.first, 50000.0);
    EXPECT_DOUBLE_EQ(bbo.second, 50100.0);
}

TEST(OrderBookTest, RemoveOrder) {
    OrderBook ob("BTC-USDT");
    auto buy1 = std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z");
    ob.addOrder(buy1);
    ob.removeOrder("b1", Order::Side::BUY, 50000.0);
    auto bbo = ob.getBBO();
    EXPECT_DOUBLE_EQ(bbo.first, 0.0);
}

TEST(OrderBookTest, GetDepth) {
    OrderBook ob("BTC-USDT");
    for (int i = 0; i < 5; ++i) {
        auto buy = std::make_shared<Order>("b" + std::to_string(i), "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0 - i * 10, "2025-06-14T10:00:00.000000Z");
        ob.addOrder(buy);
    }
    auto depth = ob.getDepth(Order::Side::BUY, 3);
    ASSERT_EQ(depth.size(), 3);
    EXPECT_DOUBLE_EQ(depth[0].first, 50000.0);
    EXPECT_DOUBLE_EQ(depth[1].first, 49990.0);
    EXPECT_DOUBLE_EQ(depth[2].first, 49980.0);
}

// --- NEW TESTS FOR STEP 2 ---

TEST(OrderBookTest, BBOUpdatesIncrementally) {
    OrderBook ob("BTC-USDT");
    auto buy1 = std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z");
    ob.addOrder(buy1);
    EXPECT_DOUBLE_EQ(ob.getBBO().first, 50000.0);
    auto buy2 = std::make_shared<Order>("b2", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50100.0, "2025-06-14T10:00:01.000000Z");
    ob.addOrder(buy2);
    EXPECT_DOUBLE_EQ(ob.getBBO().first, 50100.0);
    ob.removeOrder("b2", Order::Side::BUY, 50100.0);
    EXPECT_DOUBLE_EQ(ob.getBBO().first, 50000.0);
}

TEST(OrderBookTest, MarketDepthJSONFormat) {
    OrderBook ob("BTC-USDT");
    ob.addOrder(std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 2.5, 50000.0, "2025-06-14T10:00:00.000000Z"));
    ob.addOrder(std::make_shared<Order>("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.8, 50150.0, "2025-06-14T10:00:01.000000Z"));
    std::string json_str = ob.getMarketDepth(2);
    auto j = nlohmann::json::parse(json_str);
    EXPECT_EQ(j["symbol"], "BTC-USDT");
    ASSERT_EQ(j["bids"].size(), 1);
    ASSERT_EQ(j["asks"].size(), 1);
    EXPECT_DOUBLE_EQ(j["bids"][0][0][0], 50000.0);
    EXPECT_DOUBLE_EQ(j["bids"][0][0][1], 2.5);
    EXPECT_DOUBLE_EQ(j["asks"][0][0][0], 50150.0);
    EXPECT_DOUBLE_EQ(j["asks"][0][0][1], 1.8);
}

TEST(OrderBookTest, SnapshotJSONFormat) {
    OrderBook ob("BTC-USDT");
    ob.addOrder(std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    ob.addOrder(std::make_shared<Order>("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 2.0, 50100.0, "2025-06-14T10:00:01.000000Z"));
    std::string snap = ob.getSnapshot();
    auto j = nlohmann::json::parse(snap);
    EXPECT_EQ(j["symbol"], "BTC-USDT");
    ASSERT_EQ(j["bids"].size(), 1);
    ASSERT_EQ(j["asks"].size(), 1);
    EXPECT_DOUBLE_EQ(j["bids"][0][0][0], 50000.0);
    EXPECT_DOUBLE_EQ(j["bids"][0][0][1], 1.0);
    EXPECT_DOUBLE_EQ(j["asks"][0][0][0], 50100.0);
    EXPECT_DOUBLE_EQ(j["asks"][0][0][1], 2.0);
}

TEST(OrderBookTest, CallbackIsInvokedOnChange) {
    OrderBook ob("BTC-USDT");
    std::atomic<int> call_count{0};
    ob.setOnOrderBookChange([&]() { call_count++; });
    ob.addOrder(std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    ob.addOrder(std::make_shared<Order>("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50100.0, "2025-06-14T10:00:01.000000Z"));
    ob.removeOrder("b1", Order::Side::BUY, 50000.0);
    EXPECT_EQ(call_count, 3);
}

TEST(OrderBookTest, EdgeCases_EmptyBookAndSingleOrder) {
    OrderBook ob("BTC-USDT");
    // Empty book
    auto bbo = ob.getBBO();
    EXPECT_DOUBLE_EQ(bbo.first, 0.0);
    EXPECT_DOUBLE_EQ(bbo.second, 0.0);
    // Add single order
    ob.addOrder(std::make_shared<Order>("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    bbo = ob.getBBO();
    EXPECT_DOUBLE_EQ(bbo.first, 50000.0);
    EXPECT_DOUBLE_EQ(bbo.second, 0.0);
} 