#include "orderbook/order_book.h"

namespace orderbook {

// Stub implementations — matching logic comes in stage (b).

AddResult OrderBook::add_order(Order order) {
    // Validate input before anything else.
    if (order.remaining_qty == 0) {
        return {Status::REJECTED_ZERO_QUANTITY, {}};
    }
    if (order_locations_.count(order.id)) {
        return {Status::REJECTED_DUPLICATE_ID, {}};
    }

    // Matching and resting will be implemented in stage (b).
    // For now, limit orders go straight to the book; market orders
    // are rejected (no liquidity).
    if (order.type == OrderType::MARKET) {
        return {Status::REJECTED_MARKET_NO_LIQUIDITY, {}};
    }

    rest_order(std::move(order));
    return {Status::OK, {}};
}

Status OrderBook::cancel_order(OrderId id) {
    auto it = order_locations_.find(id);
    if (it == order_locations_.end()) {
        return Status::REJECTED_UNKNOWN_ID;
    }

    const auto& loc = it->second;
    if (loc.side == Side::BUY) {
        loc.bid_loc.level_it->second.remove_order(loc.bid_loc.order_it);
        if (loc.bid_loc.level_it->second.empty()) {
            bids_.erase(loc.bid_loc.level_it);
        }
    } else {
        loc.ask_loc.level_it->second.remove_order(loc.ask_loc.order_it);
        if (loc.ask_loc.level_it->second.empty()) {
            asks_.erase(loc.ask_loc.level_it);
        }
    }

    order_locations_.erase(it);
    return Status::OK;
}

// Place a limit order into the resting book.
void OrderBook::rest_order(Order order) {
    if (order.side == Side::BUY) {
        // emplace into the bid map; if the price level doesn't exist, create it.
        auto [level_it, _] = bids_.emplace(order.price, PriceLevel{order.price});
        auto order_it = level_it->second.add_order(std::move(order));

        LocationInfo loc;
        loc.side = Side::BUY;
        loc.bid_loc = {level_it, order_it};
        order_locations_[order_it->id] = loc;
    } else {
        auto [level_it, _] = asks_.emplace(order.price, PriceLevel{order.price});
        auto order_it = level_it->second.add_order(std::move(order));

        LocationInfo loc;
        loc.side = Side::SELL;
        loc.ask_loc = {level_it, order_it};
        order_locations_[order_it->id] = loc;
    }
}

std::vector<Trade> OrderBook::match_order(Order& /*incoming*/) {
    // Implemented in stage (b).
    return {};
}

// remove_filled_front is now a template defined in the header.

// --- Query helpers ----------------------------------------------------------

std::vector<OrderBook::LevelInfo> OrderBook::top_bids(std::size_t depth) const {
    std::vector<LevelInfo> result;
    result.reserve(depth);
    for (auto it = bids_.begin(); it != bids_.end() && result.size() < depth; ++it) {
        result.push_back({it->second.price(), it->second.total_quantity(), it->second.order_count()});
    }
    return result;
}

std::vector<OrderBook::LevelInfo> OrderBook::top_asks(std::size_t depth) const {
    std::vector<LevelInfo> result;
    result.reserve(depth);
    for (auto it = asks_.begin(); it != asks_.end() && result.size() < depth; ++it) {
        result.push_back({it->second.price(), it->second.total_quantity(), it->second.order_count()});
    }
    return result;
}

} // namespace orderbook
