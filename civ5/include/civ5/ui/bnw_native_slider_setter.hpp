#pragma once
#include "civ5/ui/bnw_civilopedia_right_tree_owner.hpp"
namespace civ5::ui {
struct BnwNativeSliderFields {
 float width=0,height=0,shuttle_width=0,shuttle_height=0;
 float offset_x=0,offset_y=0,x=0,y=0,edge_x=0,edge_y=0,value=-1;
 std::int16_t gutter=0,travel=0;
 bool vertical=false;
};
struct BnwNativeSliderSizePrefix {
 BnwNativeSliderFields fields;
 BnwGraphicPair track_size;
 // Source0x6058D0 has not returned until anchor and bounds callbacks complete.
 std::string required_callback="Control.anchor0x79D300 then Control.bounds0x6EBC30";
 bool native_setter_complete=false;
};
// Exact source arithmetic, not a production init receipt. Synthetic fields
// never initialize a Slider, establish its ratio or resume a stock VM.
BnwNativeSliderSizePrefix reduce_bnw_native_slider_size_prefix(
 const BnwNativeSliderFields&,float requested_width,float requested_height);
struct BnwNativeSliderJournalFrame {
 runtime::BnwStockLuaClientExtent client;
 std::string control,xml_sha256,styles_sha256;
 BnwNativeSliderFields constructor_parser_fields;
 // These are PRIVATE constructed embedded Graphics, not children at+0x78.
 std::shared_ptr<const BnwSliderGraphicCacheOwner> embedded_graphics;
 std::size_t normal_children=0;
 std::optional<BnwNativeSliderFields> current_fields;
 std::array<float,4> control_bounds{},parent_panel_bounds{};
 float initial_panel_content_height=0,initial_ratio=0;
 std::uint64_t native_revision=0;
 std::uint32_t slider_flags=0,track_flags=0,shuttle_flags=0;
 bool size_geometry_dependencies_complete=false,shader_draw_available=false;
 bool special_panel_registration=false,embedded_shuttle_parent_linked=false;
 bool constructor_parser_complete=false,initial_shuttle_ratio_known=false;
 bool native_setter_complete=false;
 std::string pending_source,pending_dependency;
};
// Keeps actual ctor/parser state attached to SAME stock shell/right tree/HWND.
// Initial ScrollPanel ratio must come from initialized direct child caches.
class BnwNativeSliderJournalOwner {
 public:
 static std::shared_ptr<BnwNativeSliderJournalOwner>create(
  std::uintptr_t actual_hwnd,runtime::BnwStockLuaClientExtentProvider,
  const std::filesystem::path&,const assets::ResourceMountTable&,
  const BnwCivilopediaScrollBarOwner&,
  std::shared_ptr<BnwCivilopediaRightTreeOwner>,std::string_view control="ScrollBar");
 ~BnwNativeSliderJournalOwner();
 const BnwNativeSliderJournalFrame&current()const;
 std::shared_ptr<const BnwCivilopediaRightTreeOwner> retained_right_tree()const;
 BnwGraphicPair get_native_size()const;
 // Existing integer-only Lua seam refuses nonintegral native extents.
 CivisControlSizeResult get_size()const;
 // Refuses before any publication when initial native shuttle ratio is unknown.
 // Metadata dimensions never authorize the mandatory native size effects.
 BnwNativeSliderSizePrefix stage_set_size(float width,float height)const;
 // Actual selected empty LeftStack branch; no source/global readiness claim.
 std::shared_ptr<BnwNativeSliderJournalOwner> fork_initialize_empty_left()const;
 std::shared_ptr<BnwNativeSliderJournalOwner> fork_set_size(float width,float height)const;
 private:
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit BnwNativeSliderJournalOwner(std::unique_ptr<Impl>);
};
}
