#include "civ5/materials/material_cache_ordering.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::materials {
int material_cache_compare_priority(const MaterialCachePriority &a, const MaterialCachePriority &b) {
    if (a.requested_state != b.requested_state)
        return a.requested_state > b.requested_state ? 1 : -1;
    if (a.completed_state != b.completed_state)
        return a.completed_state > b.completed_state ? 1 : -1;
    if (a.priority != b.priority)
        return a.priority < b.priority ? 1 : -1;
    if (b.secondary_priority > a.secondary_priority)
        return 1;
    if (a.secondary_priority > b.secondary_priority)
        return -1;
    return 0;
}
void material_cache_order(std::span<const MaterialCachePriority> priorities, std::span<std::uint32_t> order,
                          MaterialCacheOrderTrace *trace) {
    using Index = std::ptrdiff_t;
    if (order.size() > static_cast<std::size_t>(std::numeric_limits<Index>::max()))
        throw std::length_error("Material ordering exceeds signed index range");
    for (const auto index : order)
        if (index >= priorities.size())
            throw std::out_of_range("Material ordering references an unknown patch");
    if (trace)
        trace->clear();
    if (order.size() < 2)
        return;
    const auto compare = [&](Index a, Index b) {
        const auto first = order[a], second = order[b];
        const auto result = material_cache_compare_priority(priorities[first], priorities[second]);
        if (trace)
            trace->push_back({first, second, result});
        return result;
    };
    // The original always processes the smaller interval next. At most one
    // deferred interval per index bit is needed, without heap allocation.
    std::array<std::pair<Index, Index>, std::numeric_limits<std::size_t>::digits> deferred;
    std::size_t depth = 0;
    Index low = 0, high = static_cast<Index>(order.size()) - 1;
    for (;;) {
        const auto count = high - low + 1;
        if (count <= 8) {
            // Original shortsort chooses the first strict maximum, then swaps
            // it to the last slot. This intentionally moves equal-key items.
            for (auto end = high; end > low; --end) {
                auto maximum = low;
                for (auto i = low + 1; i <= end; ++i)
                    if (compare(i, maximum) > 0)
                        maximum = i;
                std::swap(order[maximum], order[end]);
            }
        } else {
            auto middle = low + count / 2;
            if (compare(low, middle) > 0)
                std::swap(order[low], order[middle]);
            if (compare(low, high) > 0)
                std::swap(order[low], order[high]);
            if (compare(middle, high) > 0)
                std::swap(order[middle], order[high]);
            auto left = low, right = high;
            for (;;) {
                if (middle > left)
                    do {
                        ++left;
                    } while (left < middle && compare(left, middle) <= 0);
                if (middle <= left)
                    do {
                        ++left;
                    } while (left <= high && compare(left, middle) <= 0);
                do {
                    --right;
                } while (right > middle && compare(right, middle) > 0);
                if (left > right)
                    break;
                std::swap(order[left], order[right]);
                if (middle == right)
                    middle = left;
            }
            ++right;
            if (middle < right)
                do {
                    --right;
                } while (right > middle && compare(right, middle) == 0);
            if (middle >= right)
                do {
                    --right;
                } while (right > low && compare(right, middle) == 0);
            if (right - low >= high - left) {
                if (low < right)
                    deferred[depth++] = {low, right};
                if (left < high) {
                    low = left;
                    continue;
                }
            } else {
                if (left < high)
                    deferred[depth++] = {left, high};
                if (low < right) {
                    high = right;
                    continue;
                }
            }
        }
        if (!depth)
            return;
        const auto interval = deferred[--depth];
        low = interval.first;
        high = interval.second;
    }
}
} // namespace civ5::materials
