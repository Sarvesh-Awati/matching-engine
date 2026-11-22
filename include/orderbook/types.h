#pragma once

#include <cstdint>
#include <string>

namespace orderbook {

// ---------------------------------------------------------------------------
// Prices are stored as integer "ticks" rather than floating-point values.
//
// Why?
//   1. Floating-point types (float, double) cannot represent many decimal
//      values exactly.  For example, 0.1 in IEEE-754 is actually
//      0.1000000000000000055511151231257827021181583404541015625.
//   2. Accumulated rounding errors during matching and fill calculations
//      can cause two prices that *should* be equal to compare as unequal,
//      leading to missed matches or incorrect trade prices.
//   3. Integer arithmetic is exact: 100 + 100 == 200, always.  Comparisons
//      (==, <, >) are reliable and free of epsilon-tolerance hacks.
//   4. Modern exchanges universally use integer ticks internally.  The tick
//      size is defined per instrument (e.g. 1 tick = $0.01).
//
// In this project one tick equals the smallest price increment of the
// instrument.  Converting to/from a human-readable decimal price is an
// I/O concern, not a matching-engine concern.
// ---------------------------------------------------------------------------
using Price    = std::int64_t;
using Quantity = std::uint64_t;
using OrderId  = std::uint64_t;

// ---------------------------------------------------------------------------
// Side: which side of the book the order sits on.
// ---------------------------------------------------------------------------
enum class Side : std::uint8_t {
    BUY,
    SELL
};

inline std::string to_string(Side s) {
    return s == Side::BUY ? "BUY" : "SELL";
}

// ---------------------------------------------------------------------------
// OrderType: determines whether the order specifies a price limit.
//
//   LIMIT  – rests in the book if not fully matched immediately.
//   MARKET – matches at the best available price(s); any unmatched
//            remainder is cancelled (never rests in the book).
// ---------------------------------------------------------------------------
enum class OrderType : std::uint8_t {
    LIMIT,
    MARKET
};

inline std::string to_string(OrderType t) {
    return t == OrderType::LIMIT ? "LIMIT" : "MARKET";
}

// ---------------------------------------------------------------------------
// Order: everything the matching engine needs to know about one order.
//
//   `remaining_qty` starts equal to the original quantity and decreases
//   with each partial fill.  When it reaches zero the order is fully filled.
//
//   For MARKET orders `price` is unused (set to 0 by convention).
// ---------------------------------------------------------------------------
struct Order {
    OrderId   id;
    Side      side;
    OrderType type;
    Price     price;           // meaningful only for LIMIT orders
    Quantity  remaining_qty;   // quantity still to be filled

    bool is_filled() const { return remaining_qty == 0; }
};

// ---------------------------------------------------------------------------
// Trade: the record produced whenever a buy and a sell order match.
//
// The trade price is always the price of the *resting* (passive) order,
// because the resting order arrived first and set the price.
// ---------------------------------------------------------------------------
struct Trade {
    OrderId  buy_order_id;
    OrderId  sell_order_id;
    Price    price;
    Quantity quantity;
};

} // namespace orderbook
