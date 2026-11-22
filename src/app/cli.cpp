#include "app/cli.hpp"

#include <cctype>
#include <iostream>
#include <string>

namespace app {
namespace {

bool is_space(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

bool valid_nonempty_trimmed(const std::string& text) {
    return !text.empty() && !is_space(text.front()) && !is_space(text.back());
}

}  // namespace

std::string normalize_sku(const std::string& sku) {
    std::size_t first = 0;
    while (first < sku.size() && is_space(sku[first])) {
        ++first;
    }
    std::size_t last = sku.size();
    while (last > first && is_space(sku[last - 1])) {
        --last;
    }
    std::string result;
    result.reserve(last - first);
    for (std::size_t i = first; i < last; ++i) {
        result.push_back(
            static_cast<char>(std::toupper(static_cast<unsigned char>(sku[i]))));
    }
    return result;
}

bool valid_station_id(const std::string& station_id) {
    return valid_nonempty_trimmed(station_id);
}

bool valid_order_id(const std::string& order_id) {
    return valid_nonempty_trimmed(order_id);
}

int run() {
    std::cout << "Use the ROS 2 demo package for the end-to-end runtime.\n";
    return 0;
}

int print_trace(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--trace") {
        std::cout << "TRACE command is reserved for grading\n";
        return 0;
    }
    return run();
}

}  // namespace app
