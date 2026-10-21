#pragma once
#include "civ5/ui/bnw_slider_graphic_cache.hpp"
namespace civ5::ui {
struct BnwRightTreeFrame;
struct BnwGridGraphicBinding {
 std::size_t control_index=0;
 std::string path,id;
 BnwGraphicNineSlice graphic;
};
struct BnwGridGraphicCacheFrame {
 runtime::BnwStockLuaClientExtent client;
 std::uint64_t shell_revision=0;
 std::string xml_sha256,styles_sha256;
 std::vector<BnwGridGraphicBinding> staged_graphics;
 std::vector<std::size_t> pending_size_controls;
 // Preparing geometry/UV data does not insert it into native initialization
 // order. Child/base/font/bounds propagation remains required.
 bool native_initialization_complete=false;
};
class BnwCivilopediaGridGraphicCacheOwner {
 public:
 static std::shared_ptr<const BnwCivilopediaGridGraphicCacheOwner> create(
  const std::filesystem::path&,const assets::ResourceMountTable&,const BnwRightTreeFrame&);
 const BnwGridGraphicCacheFrame&snapshot()const noexcept{return frame_;}
 const BnwGridGraphicBinding*graphic(const runtime::BnwStockLuaClientExtent&,
  std::uint64_t revision,std::size_t control_index)const noexcept;
 private:
 explicit BnwCivilopediaGridGraphicCacheOwner(BnwGridGraphicCacheFrame f):frame_(std::move(f)){}
 BnwGridGraphicCacheFrame frame_;
};
// Generic native ControlBase+0x94 union is deliberately unlike Stack filtering:
// EVERY actual child bounds contributes, even Hidden/outside/zero-size.
// A result is data, not an initialization/readiness receipt.
std::array<float,4> update_bnw_native_base_bounds(float x,float y,float width,float height,
 const std::vector<std::array<float,4>>&actual_child_bounds);
} // namespace civ5::ui
