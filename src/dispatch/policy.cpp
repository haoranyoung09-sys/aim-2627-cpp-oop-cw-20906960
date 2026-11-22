#include "dispatch/policy.hpp"

#include <stdexcept>

namespace dispatch {
namespace {

class PriorityPolicy final : public DispatchPolicy {
public:
    std::string name() const override;
    std::optional<std::string> select(
        const warehouse::Order& order,
        const std::vector<warehouse::PickStation>& candidates) const override;
};

class StockPolicy final : public DispatchPolicy {
public:
    std::string name() const override;
    std::optional<std::string> select(
        const warehouse::Order& order,
        const std::vector<warehouse::PickStation>& candidates) const override;
};

}  // namespace

std::unique_ptr<DispatchPolicy> make_policy(const std::string& name) {
    if (name == "priority") {
        return std::make_unique<PriorityPolicy>();
    }
    if (name == "stock") {
        return std::make_unique<StockPolicy>();
    }
    throw std::invalid_argument("unknown dispatch policy: " + name);
}

std::string PriorityPolicy::name() const { return "priority"; }

std::optional<std::string> PriorityPolicy::select(
    const warehouse::Order& order,
    const std::vector<warehouse::PickStation>& candidates) const {
    (void)order;
    if (candidates.empty()) {
        return std::nullopt;
    }
    return candidates.front().id;
}

std::string StockPolicy::name() const { return "stock"; }

std::optional<std::string> StockPolicy::select(
    const warehouse::Order& order,
    const std::vector<warehouse::PickStation>& candidates) const {
    (void)order;
    if (candidates.empty()) {
        return std::nullopt;
    }
    const warehouse::PickStation* best = &candidates.front();
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        const warehouse::PickStation& candidate = candidates[i];
        if (candidate.stock > best->stock ||
            (candidate.stock == best->stock && candidate.id < best->id)) {
            best = &candidate;
        }
    }
    return best->id;
}

}  // namespace dispatch
