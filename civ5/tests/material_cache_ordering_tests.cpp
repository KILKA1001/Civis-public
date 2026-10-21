#include "civ5/materials/material_cache_ordering.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace civ5::materials;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        const std::array<MaterialCachePriority, 2> priorities{{{3, 3, 1, 1}, {3, 3, 2, 1}}};
        std::array<std::uint32_t, 3> invalid{0, 1, 2};
        const auto before = invalid;
        MaterialCacheOrderTrace trace{{7, 8, -1}};
        bool rejected = false;
        try {
            material_cache_order(priorities, invalid, &trace);
        } catch (const std::out_of_range &) {
            rejected = true;
        }
        require(rejected && invalid == before, "Invalid index must fail before reordering valid entries");
        require(trace.size() == 1 && trace[0].first == 7 && trace[0].second == 8 && trace[0].result == -1,
                "Rejected input must preserve the caller's observations");
        material_cache_order(priorities, {}, &trace);
        require(trace.empty(), "Empty ordering must clear observations");
        std::array<std::uint32_t, 1> single{1};
        material_cache_order(priorities, single, &trace);
        require(single[0] == 1 && trace.empty(), "Single element must need no comparisons");
        // Repeated references are valid; the API sorts references, not ownership.
        std::array<std::uint32_t, 4> repeated{0, 1, 0, 1};
        material_cache_order(priorities, repeated);
        require(repeated == std::array<std::uint32_t, 4>{1, 1, 0, 0}, "Repeated patch references lost");
        std::cout << "PASS material ordering input and observation boundaries\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
