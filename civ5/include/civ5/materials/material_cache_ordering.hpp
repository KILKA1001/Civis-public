#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace civ5::materials {
// Original requested/completed states are compared as signed values;
// primary priority is unsigned, and the secondary Float32 priority descends.
struct MaterialCachePriority {
    std::int32_t requested_state, completed_state;
    std::uint32_t priority;
    float secondary_priority;
};
int material_cache_compare_priority(const MaterialCachePriority &a, const MaterialCachePriority &b);
struct MaterialCacheOrderComparison {
    std::uint32_t first, second;
    std::int32_t result;
};
using MaterialCacheOrderTrace = std::vector<MaterialCacheOrderComparison>;
// Sort patch indices using the original CRT partition/short-sort ordering.
// Equal priorities may change their entering order, as in original 644660.
// All indices are checked before mutation. The ordinary path allocates no
// storage; optional observations are cleared and record actual comparisons.
void material_cache_order(std::span<const MaterialCachePriority> priorities, std::span<std::uint32_t> order,
                          MaterialCacheOrderTrace *trace = nullptr);
} // namespace civ5::materials
