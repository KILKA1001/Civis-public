#include "civ5/materials/material_cache_free_lists.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::materials {
namespace {
constexpr std::array<std::uint32_t, 4> widths{64, 128, 256, 512};
int slot(std::uint32_t width) {
    const auto found = std::find(widths.begin(), widths.end(), width);
    return found == widths.end() ? -1 : static_cast<int>(found - widths.begin());
}
} // namespace
void MaterialCacheFreeLists::add_texture(std::uint32_t handle, std::uint32_t width, bool available) {
    const auto index = slot(width);
    if (!handle || index < 0 || entries_.contains(handle))
        throw std::invalid_argument("Invalid or duplicate material pool texture");
    // All registered textures of this width may return simultaneously.
    free_[index].reserve(counts_[index] + 1);
    entries_.emplace(handle, Entry{static_cast<unsigned>(index), available});
    ++counts_[index];
    if (available)
        free_[index].push_back(handle);
}
std::uint32_t MaterialCacheFreeLists::take(std::uint32_t requested_size, std::uint32_t &actual_size) {
    // Original unsupported requests fail without writing the output size.
    for (auto index = slot(requested_size); index >= 0; --index) {
        auto &items = free_[index];
        if (!items.empty()) {
            const auto handle = items.back();
            items.pop_back();
            entries_.at(handle).available = false;
            actual_size = widths[index];
            return handle;
        }
    }
    return 0;
}
void MaterialCacheFreeLists::give(std::uint32_t handle) {
    if (!handle)
        return;
    const auto found = entries_.find(handle);
    if (found == entries_.end() || found->second.available)
        throw std::invalid_argument("Unknown material texture or duplicate pool return");
    auto &entry = found->second;
    free_[entry.slot].push_back(handle);
    entry.available = true;
}
bool MaterialCacheFreeLists::contains(std::uint32_t handle) const { return entries_.contains(handle); }
bool MaterialCacheFreeLists::available(std::uint32_t handle) const { return entries_.at(handle).available; }
std::uint32_t MaterialCacheFreeLists::texture_width(std::uint32_t handle) const {
    return widths[entries_.at(handle).slot];
}
std::span<const std::uint32_t> MaterialCacheFreeLists::free_handles(std::uint32_t width) const {
    const auto index = slot(width);
    if (index < 0)
        throw std::invalid_argument("Unsupported material pool width");
    return free_[index];
}
} // namespace civ5::materials
