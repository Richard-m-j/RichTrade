#include <gtest/gtest.h>
#include "../src/core/MatchingEngine.h"
#include "../src/core/Order.h"

// --- LIMIT ORDER MATCHING ---
TEST(MatchingEngineTest, LimitOrder_MatchAndAddRemainder) {
    MatchingEngine engine;
    // Add resting sell at 50000
    Order sell("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z");
    engine.processOrder(sell);
    // Incoming buy at 50000 for 2.0 (should match 1.0, remainder 1.0 added)
    Order buy("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 2.0, 50000.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(buy);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 1.0);
    EXPECT_EQ(trades[0].price, 50000.0);
    EXPECT_EQ(trades[0].aggressor_side, "buy");
    EXPECT_EQ(trades[0].taker_order_id, "b1");
    EXPECT_EQ(trades[0].maker_order_id, "s1");
    // The buy order should be PARTIALLY_FILLED and remainder on book
    EXPECT_EQ(buy.getStatus(), Order::Status::PARTIALLY_FILLED);
}

// --- MARKET ORDER MATCHING ---
TEST(MatchingEngineTest, MarketOrder_MatchBestPrice) {
    MatchingEngine engine;
    // Add two resting buys at 50000 and 49900
    engine.processOrder(Order("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    engine.processOrder(Order("b2", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 2.0, 49900.0, "2025-06-14T10:00:01.000000Z"));
    // Incoming market sell for 2.5 (should fill 1.0 at 50000, 1.5 at 49900)
    Order sell("s1", "BTC-USDT", Order::Type::MARKET, Order::Side::SELL, 2.5, 0.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(sell);
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].price, 50000.0);
    EXPECT_EQ(trades[0].quantity, 1.0);
    EXPECT_EQ(trades[1].price, 49900.0);
    EXPECT_EQ(trades[1].quantity, 1.5);
    EXPECT_EQ(sell.getStatus(), Order::Status::FILLED);
}

// --- IOC ORDER MATCHING ---
TEST(MatchingEngineTest, IOCOrder_PartialFillAndCancelRest) {
    MatchingEngine engine;
    // Add resting sell at 50000 for 1.0
    engine.processOrder(Order("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    // Incoming IOC buy for 2.0 at 50000 (should fill 1.0, cancel 1.0)
    Order buy("b1", "BTC-USDT", Order::Type::IOC, Order::Side::BUY, 2.0, 50000.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(buy);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 1.0);
    EXPECT_EQ(buy.getStatus(), Order::Status::PARTIALLY_FILLED);
    EXPECT_DOUBLE_EQ(buy.getQuantity(), 1.0); // Remaining unfilled
}

// --- FOK ORDER MATCHING ---
TEST(MatchingEngineTest, FOKOrder_FillOrKill) {
    MatchingEngine engine;
    // Add resting sell at 50000 for 1.0
    engine.processOrder(Order("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    // Incoming FOK buy for 2.0 at 50000 (should be cancelled, not enough liquidity)
    Order buy("b1", "BTC-USDT", Order::Type::FOK, Order::Side::BUY, 2.0, 50000.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(buy);
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(buy.getStatus(), Order::Status::CANCELLED);
    // Now try FOK buy for 1.0 (should fill)
    Order buy2("b2", "BTC-USDT", Order::Type::FOK, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:01:10.000000Z");
    auto trades2 = engine.processOrder(buy2);
    ASSERT_EQ(trades2.size(), 1);
    EXPECT_EQ(trades2[0].quantity, 1.0);
    EXPECT_EQ(buy2.getStatus(), Order::Status::FILLED);
}

// --- PRICE-TIME PRIORITY ---
TEST(MatchingEngineTest, PriceTimePriority_FIFO) {
    MatchingEngine engine;
    // Add two resting sells at same price, different times
    engine.processOrder(Order("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    engine.processOrder(Order("s2", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:01.000000Z"));
    // Incoming buy for 2.0 at 50000 (should fill s1 then s2)
    Order buy("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 2.0, 50000.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(buy);
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].maker_order_id, "s1");
    EXPECT_EQ(trades[1].maker_order_id, "s2");
}

// --- NO TRADE-THROUGHS ---
TEST(MatchingEngineTest, NoTradeThroughs) {
    MatchingEngine engine;
    // Add resting sell at 49900 and 50000
    engine.processOrder(Order("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 49900.0, "2025-06-14T10:00:00.000000Z"));
    engine.processOrder(Order("s2", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:01.000000Z"));
    // Incoming buy at 50000 (should fill 49900 first)
    Order buy("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 1.0, 50000.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(buy);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 49900.0);
    EXPECT_EQ(trades[0].maker_order_id, "s1");
}

// --- EDGE CASES ---
TEST(MatchingEngineTest, EmptyBook_NoMatch) {
    MatchingEngine engine;
    // Market order on empty book
    Order buy("b1", "BTC-USDT", Order::Type::MARKET, Order::Side::BUY, 1.0, 0.0, "2025-06-14T10:00:00.000000Z");
    auto trades = engine.processOrder(buy);
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(buy.getStatus(), Order::Status::NEW);
}

TEST(MatchingEngineTest, PartialFill_MultipleLevels) {
    MatchingEngine engine;
    // Add two resting sells at 50000 (1.0) and 50100 (2.0)
    engine.processOrder(Order("s1", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 1.0, 50000.0, "2025-06-14T10:00:00.000000Z"));
    engine.processOrder(Order("s2", "BTC-USDT", Order::Type::LIMIT, Order::Side::SELL, 2.0, 50100.0, "2025-06-14T10:00:01.000000Z"));
    // Incoming buy for 2.5 at 50100 (should fill 1.0 at 50000, 1.5 at 50100)
    Order buy("b1", "BTC-USDT", Order::Type::LIMIT, Order::Side::BUY, 2.5, 50100.0, "2025-06-14T10:01:00.000000Z");
    auto trades = engine.processOrder(buy);
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].price, 50000.0);
    EXPECT_EQ(trades[1].price, 50100.0);
    EXPECT_EQ(trades[1].quantity, 1.5);
    EXPECT_EQ(buy.getStatus(), Order::Status::FILLED);
} 