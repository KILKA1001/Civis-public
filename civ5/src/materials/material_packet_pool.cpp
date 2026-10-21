#include "civ5/materials/material_packet_pool.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::materials {
MaterialCachePacket &MaterialPacketPool::allocate(std::uint32_t key,
                                                  std::uint32_t &handle) {
  const bool recycled = !available_.empty();
  if (!recycled && next_ > last_fresh_handle)
    throw std::overflow_error("Original packet handle limit");
  const auto id = recycled ? available_.back() : next_;
  auto &entry = entries_[id];
  if (entry.references)
    throw std::logic_error("Packet free stack contains a live resource");
  entry.packet = {};
  entry.packet.key = key;
  entry.references = 1;
  if (recycled)
    available_.pop_back();
  else
    ++next_;
  handle = id;
  return entry.packet;
}
void MaterialPacketPool::retain(std::uint32_t handle) {
  if (!handle)
    return;
  const auto at = entries_.find(handle);
  if (at == entries_.end() || !at->second.references)
    throw std::invalid_argument("Unknown or returned packet");
  if (at->second.references == std::numeric_limits<std::uint32_t>::max())
    throw std::overflow_error("Packet reference overflow");
  ++at->second.references;
}
void MaterialPacketPool::release(std::uint32_t handle) {
  if (!handle)
    return;
  const auto at = entries_.find(handle);
  if (at == entries_.end() || !at->second.references)
    throw std::invalid_argument("Unknown or returned packet");
  if (at->second.references == 1)
    pending_[frame_].push_back(handle);
  --at->second.references;
}
void MaterialPacketPool::advance_frame() {
  const auto next = (frame_ + 3u) & 3u;
  const auto &queue = pending_[next];
  retired_.insert(retired_.end(), queue.begin(), queue.end());
  pending_[next].clear();
  frame_ = next;
}
void MaterialPacketPool::collect() {
  available_.insert(available_.end(), retired_.begin(), retired_.end());
  retired_.clear();
}
const MaterialCachePacket &
MaterialPacketPool::packet(std::uint32_t handle) const {
  const auto at = entries_.find(handle);
  if (at == entries_.end() || !at->second.references)
    throw std::invalid_argument("Unknown or returned packet");
  return at->second.packet;
}
std::uint32_t MaterialPacketPool::references(std::uint32_t handle) const {
  if (!handle)
    return 0;
  const auto at = entries_.find(handle);
  if (at == entries_.end())
    throw std::invalid_argument("Unknown packet");
  return at->second.references;
}
std::span<const std::uint32_t>
MaterialPacketPool::pending(unsigned slot) const {
  if (slot >= pending_.size())
    throw std::invalid_argument("Invalid packet frame slot");
  return pending_[slot];
}
} // namespace civ5::materials
