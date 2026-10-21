#pragma once
#include "material_cache_passes.hpp"
#include <map>

namespace civ5::materials {
// Render-thread counterpart of the original packet allocator and its four
// deferred queues. advance_frame() retires the preceding ring slot; collect()
// transfers retired IDs to the free stack. Neither operation runs implicitly.
// A packet is readable only while it has a live reference. Its storage remains
// stable until its ID is allocated again, when all semantic fields are zeroed.
class MaterialPacketPool {
public:
  static constexpr std::uint32_t last_fresh_handle = 0x1fffe;
  MaterialCachePacket &allocate(std::uint32_t key, std::uint32_t &handle);
  void retain(std::uint32_t handle);
  void release(std::uint32_t handle);
  void advance_frame();
  void collect();
  const MaterialCachePacket &packet(std::uint32_t handle) const;
  std::uint32_t references(std::uint32_t handle) const;
  std::uint32_t next_handle() const { return next_; }
  unsigned frame_slot() const { return frame_; }
  std::span<const std::uint32_t> pending(unsigned slot) const;
  std::span<const std::uint32_t> retired() const { return retired_; }
  std::span<const std::uint32_t> available() const { return available_; }

private:
  struct Entry {
    MaterialCachePacket packet{};
    std::uint32_t references = 0;
  };
  std::map<std::uint32_t, Entry> entries_;
  std::array<std::vector<std::uint32_t>, 4> pending_;
  std::vector<std::uint32_t> retired_, available_;
  std::uint32_t next_ = 1;
  unsigned frame_ = 0;
};
} // namespace civ5::materials
