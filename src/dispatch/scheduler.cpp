#include "dispatch/scheduler.hpp"

#include "app/cli.hpp"

#include <cctype>
#include <stdexcept>

namespace dispatch {
namespace {

bool valid_sku(const std::string& sku) {
    return !sku.empty() &&
           std::isspace(static_cast<unsigned char>(sku.front())) == 0 &&
           std::isspace(static_cast<unsigned char>(sku.back())) == 0;
}

}  // namespace

Scheduler::Scheduler(warehouse::Inventory& inventory,
                     std::unique_ptr<DispatchPolicy> policy,
                     audit::Trace& trace)
    : inventory_(inventory), policy_(std::move(policy)), trace_(trace) {
    if (!policy_) {
        throw std::invalid_argument("dispatch policy cannot be null");
    }
}

DispatchDecision Scheduler::submit(const warehouse::Order& order) {
    if (!app::valid_order_id(order.id)) {
        return {false, order.id, "", "invalid order id"};
    }
    if (!valid_sku(order.sku)) {
        return {false, order.id, "", "invalid sku"};
    }
    if (order.quantity <= 0) {
        return {false, order.id, "", "invalid quantity"};
    }
    const auto candidates = inventory_.available_for(order);
    if (candidates.empty()) {
        return {false, order.id, "", "no available station"};
    }
    const auto selected = policy_->select(order, candidates);
    if (!selected.has_value()) {
        return {false, order.id, "", "policy selected no station"};
    }
    if (!inventory_.reserve(order, *selected)) {
        return {false, order.id, "", "reservation failed"};
    }
    trace_.append("DISPATCH " + order.id + " " + *selected);
    return {true, order.id, *selected, "accepted"};
}

std::vector<DispatchDecision> Scheduler::submit_all(
    const std::vector<warehouse::Order>& orders) {
    std::vector<DispatchDecision> decisions;
    decisions.reserve(orders.size());
    for (const auto& order : orders) {
        decisions.push_back(submit(order));
    }
    return decisions;
}

bool Scheduler::complete(const std::string& station_id,
                         const std::string& order_id,
                         std::string& failure_reason) {
    if (!app::valid_station_id(station_id)) {
        failure_reason = "invalid station id";
        return false;
    }
    if (!app::valid_order_id(order_id)) {
        failure_reason = "invalid order id";
        return false;
    }
    const auto st = inventory_.station(station_id);
    if (!st.has_value()) {
        failure_reason = "unknown station";
        return false;
    }
    if (st->robot_state != warehouse::RobotState::busy) {
        failure_reason = "station is not busy";
        return false;
    }
    if (st->current_order_id != order_id) {
        failure_reason = "order id mismatch";
        return false;
    }
    if (!inventory_.finish(station_id, order_id)) {
        failure_reason = "finish failed";
        return false;
    }
    failure_reason.clear();
    trace_.append("COMPLETE " + order_id + " " + station_id);
    return true;
}

const warehouse::Inventory& Scheduler::inventory() const {
    return inventory_;
}

const audit::Trace& Scheduler::trace() const {
    return trace_;
}

}  // namespace dispatch
