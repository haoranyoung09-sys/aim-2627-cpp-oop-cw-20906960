#include "warehouse/domain.hpp"

#include <cctype>
#include <stdexcept>

namespace warehouse {
namespace {

bool is_space(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

bool valid_token(const std::string& text) {
    return !text.empty() && !is_space(text.front()) && !is_space(text.back());
}

bool valid_order(const Order& order) {
    return valid_token(order.id) && valid_token(order.sku) &&
           order.quantity > 0;
}

}  // namespace

void Inventory::add_station(const std::string& station_id, int stock) {
    if (!valid_token(station_id)) {
        throw std::invalid_argument("invalid station id");
    }
    if (stock < 0) {
        throw std::invalid_argument("station stock cannot be negative");
    }
    if (stations_.find(station_id) != stations_.end()) {
        throw std::invalid_argument("duplicate station id: " + station_id);
    }
    stations_.emplace(station_id,
                      PickStation{station_id, stock, RobotState::idle, ""});
}

std::optional<PickStation> Inventory::station(
    const std::string& station_id) const {
    const auto it = stations_.find(station_id);
    if (it == stations_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<PickStation> Inventory::stations() const {
    std::vector<PickStation> result;
    result.reserve(stations_.size());
    for (const auto& entry : stations_) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<PickStation> Inventory::available_for(const Order& order) const {
    std::vector<PickStation> candidates;
    if (!valid_order(order)) {
        return candidates;
    }
    for (const auto& entry : stations_) {
        const PickStation& st = entry.second;
        if (st.robot_state == RobotState::idle && st.stock >= order.quantity) {
            candidates.push_back(st);
        }
    }
    return candidates;
}

bool Inventory::reserve(const Order& order, const std::string& station_id) {
    if (!valid_order(order) || !valid_token(station_id)) {
        return false;
    }
    const auto it = stations_.find(station_id);
    if (it == stations_.end()) {
        return false;
    }
    PickStation& st = it->second;
    if (st.robot_state != RobotState::idle || st.stock < order.quantity) {
        return false;
    }
    st.stock -= order.quantity;
    st.robot_state = RobotState::busy;
    st.current_order_id = order.id;
    return true;
}

bool Inventory::finish(const std::string& station_id,
                       const std::string& order_id) {
    if (!valid_token(station_id) || !valid_token(order_id)) {
        return false;
    }
    const auto it = stations_.find(station_id);
    if (it == stations_.end()) {
        return false;
    }
    PickStation& st = it->second;
    if (st.robot_state != RobotState::busy || st.current_order_id != order_id) {
        return false;
    }
    st.robot_state = RobotState::idle;
    st.current_order_id.clear();
    return true;
}

}  // namespace warehouse
