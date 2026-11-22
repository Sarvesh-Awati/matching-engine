// Stage (a) tests: verify core types, PriceLevel, and basic OrderBook
// operations (add to book, cancel, input validation).

#include <catch2/catch_test_macros.hpp>
#include "orderbook/order_book.h"

using namespace orderbook;

// ── Order struct ────────────────────────────────────────────────────────────

TEST_CASE("Order::is_filled returns true when remaining_qty is zero", "[types]") {
    Order o{1, Side::BUY, OrderType::LIMIT, 100, 0};
    REQUIRE(o.is_filled());
}

TEST_CASE("Order::is_filled returns false when quantity remains", "[types]") {
    Order o{1, Side::BUY, OrderType::LIMIT, 100, 10};
    REQUIRE_FALSE(o.is_filled());
}

// ── PriceLevel ──────────────────────────────────────────────────────────────

TEST_CASE("PriceLevel starts empty", "[price_level]") {
    PriceLevel level{100};
    REQUIRE(level.empty());
    REQUIRE(level.total_quantity() == 0);
    REQUIRE(level.order_count() == 0);
    REQUIRE(level.price() == 100);
}

TEST_CASE("PriceLevel tracks orders in FIFO order", "[price_level]") {
    PriceLevel level{100};
    Order a{1, Side::BUY, OrderType::LIMIT, 100, 10};
    Order b{2, Side::BUY, OrderType::LIMIT, 100, 20};

    level.add_order(a);
    level.add_order(b);

    REQUIRE(level.order_count() == 2);
    REQUIRE(level.total_quantity() == 30);
    REQUIRE(level.front().id == 1); // first in, first out
}

TEST_CASE("PriceLevel::remove_order removes by iterator and updates totals", "[price_level]") {
    PriceLevel level{100};
    Order a{1, Side::BUY, OrderType::LIMIT, 100, 10};
    Order b{2, Side::BUY, OrderType::LIMIT, 100, 20};

    auto it_a = level.add_order(a);
    level.add_order(b);

    level.remove_order(it_a);
    REQUIRE(level.order_count() == 1);
    REQUIRE(level.total_quantity() == 20);
    REQUIRE(level.front().id == 2);
}

TEST_CASE("PriceLevel::reduce_front_qty works for partial fills", "[price_level]") {
    PriceLevel level{100};
    level.add_order({1, Side::BUY, OrderType::LIMIT, 100, 50});

    level.reduce_front_qty(30);
    REQUIRE(level.front().remaining_qty == 20);
    REQUIRE(level.total_quantity() == 20);
}

// ── OrderBook: input validation ─────────────────────────────────────────────

TEST_CASE("OrderBook rejects zero-quantity orders", "[orderbook]") {
    OrderBook book;
    auto result = book.add_order({1, Side::BUY, OrderType::LIMIT, 100, 0});
    REQUIRE(result.status == Status::REJECTED_ZERO_QUANTITY);
    REQUIRE(book.empty());
}

TEST_CASE("OrderBook rejects duplicate order ids", "[orderbook]") {
    OrderBook book;
    book.add_order({1, Side::BUY, OrderType::LIMIT, 100, 10});

    auto result = book.add_order({1, Side::SELL, OrderType::LIMIT, 200, 5});
    REQUIRE(result.status == Status::REJECTED_DUPLICATE_ID);
    REQUIRE(book.order_count() == 1);
}

TEST_CASE("OrderBook cancel rejects unknown ids", "[orderbook]") {
    OrderBook book;
    REQUIRE(book.cancel_order(999) == Status::REJECTED_UNKNOWN_ID);
}

// ── OrderBook: resting and cancel ───────────────────────────────────────────

TEST_CASE("Limit orders rest in the book when no match exists", "[orderbook]") {
    OrderBook book;

    book.add_order({1, Side::BUY, OrderType::LIMIT, 100, 10});
    book.add_order({2, Side::BUY, OrderType::LIMIT, 101, 5});
    book.add_order({3, Side::SELL, OrderType::LIMIT, 200, 20});

    REQUIRE(book.order_count() == 3);

    // Best bid should be 101 (highest), best ask 200 (lowest).
    auto bids = book.top_bids(5);
    auto asks = book.top_asks(5);

    REQUIRE(bids.size() == 2);
    REQUIRE(bids[0].price == 101);
    REQUIRE(bids[1].price == 100);

    REQUIRE(asks.size() == 1);
    REQUIRE(asks[0].price == 200);
}

TEST_CASE("Cancel removes an order and cleans up empty levels", "[orderbook]") {
    OrderBook book;
    book.add_order({1, Side::BUY, OrderType::LIMIT, 100, 10});
    book.add_order({2, Side::BUY, OrderType::LIMIT, 100, 20});

    REQUIRE(book.cancel_order(1) == Status::OK);
    REQUIRE(book.order_count() == 1);
    REQUIRE(book.has_order(1) == false);
    REQUIRE(book.has_order(2) == true);

    // Level still exists (order 2 is there).
    auto bids = book.top_bids(5);
    REQUIRE(bids.size() == 1);
    REQUIRE(bids[0].total_qty == 20);

    // Cancel the last order at that level — level should be removed.
    REQUIRE(book.cancel_order(2) == Status::OK);
    REQUIRE(book.empty());
    REQUIRE(book.top_bids(5).empty());
}
