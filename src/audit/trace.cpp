#include "audit/trace.hpp"

namespace audit {

void Trace::append(const std::string& text) {
    events_.push_back(TraceEvent{text});
}

std::vector<TraceEvent> Trace::events() const {
    return events_;
}

void Trace::clear() {
    events_.clear();
}

}  // namespace audit
