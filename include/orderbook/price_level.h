#pragma once

#include "orderbook/types.h"

#include <list>

namespace orderbook {

// ---------------------------------------------------------------------------
// PriceLevel: all resting orders at a single price, in FIFO order.
//
// Why std::list?
//   - We need stable iterators: when we store an iterator in the
//     OrderId → location lookup map, that iterator must remain valid even
//     as other orders at the same price are added or removed.
//   - std::list (doubly-linked list) guarantees iterator stability on
//     insert and erase of *other* elements.
//   - Removal of a known order is O(1) given its iterator.
//   - Matching always takes from the front (FIFO), which is O(1).
//
// Trade-off:
//   std::list has poor cache locality compared to std::vector or
//   std::deque.  For a portfolio project correctness and clear semantics
//   matter more; a production system might use an intrusive list or a
//   flat structure with tombstones to improve cache behavior.
// ---------------------------------------------------------------------------
class PriceLevel {
public:
    using OrderList = std::list<Order>;
    using Iterator  = OrderList::iterator;

    explicit PriceLevel(Price price) : price_{price} {}

    // --- Accessors ----------------------------------------------------------

    Price    price()          const { return price_; }
    bool     empty()          const { return orders_.empty(); }
    Quantity total_quantity()  const { return total_qty_; }
    std::size_t order_count() const { return orders_.size(); }

    // --- Mutators -----------------------------------------------------------

    // Append an order to the back of the queue (FIFO).
    // Returns an iterator that serves as the order's "location handle".
    Iterator add_order(Order order) {
        total_qty_ += order.remaining_qty;
        orders_.push_back(std::move(order));
        return std::prev(orders_.end());
    }

    // Remove an order identified by its iterator.  O(1).
    void remove_order(Iterator it) {
        total_qty_ -= it->remaining_qty;
        orders_.erase(it);
    }

    // Peek at the front order (the one with the highest time priority).
    Order&       front()       { return orders_.front(); }
    const Order& front() const { return orders_.front(); }

    // Pop the front order after it has been fully filled.
    void pop_front() {
        total_qty_ -= orders_.front().remaining_qty;
        orders_.pop_front();
    }

    // Reduce the front order's remaining quantity after a partial fill.
    void reduce_front_qty(Quantity filled) {
        orders_.front().remaining_qty -= filled;
        total_qty_ -= filled;
    }

    // --- Iteration (for PRINT / debugging) ----------------------------------

    OrderList::const_iterator begin() const { return orders_.cbegin(); }
    OrderList::const_iterator end()   const { return orders_.cend(); }

private:
    Price     price_;
    OrderList orders_;
    Quantity  total_qty_ = 0;
};

} // namespace orderbook
