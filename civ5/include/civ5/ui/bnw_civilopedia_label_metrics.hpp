#pragma once
#include "civ5/ui/bnw_civilopedia_right_tree_owner.hpp"
namespace civ5::ui {
struct BnwNativePlainLabelMetric {
 std::size_t control_index=0;
 std::string path,text,font_key,font_sha256,font_style;
 std::int32_t width=0,height=0,wrap_width=-1,leading_offset=0;
 bool force_ime=false,force_non_ime=false;
 GlyphGenLine glyph_line;
};
struct BnwNativePlainLabelFrame {
 runtime::BnwStockLuaClientExtent client;
 std::uint64_t shell_revision=0;
 std::string xml_sha256;
 std::vector<BnwNativePlainLabelMetric> staged_metrics;
 std::vector<std::string> missing_font_keys,localized_paths;
 // Label.initialize and synchronous parent callbacks are separate producers.
 bool native_initialization_complete=false;
};
// Actual selected source/VFS binding, not arbitrary text or an implicit locale.
class BnwCivilopediaLabelMetricsOwner {
 public:
 static std::shared_ptr<const BnwCivilopediaLabelMetricsOwner> create(
  const std::filesystem::path&,const assets::ResourceMountTable&,const BnwRightTreeFrame&);
 const BnwNativePlainLabelFrame&snapshot()const noexcept{return frame_;}
 const BnwNativePlainLabelMetric*metric(const runtime::BnwStockLuaClientExtent&,
  std::uint64_t revision,std::size_t control_index)const noexcept;
 private:
 explicit BnwCivilopediaLabelMetricsOwner(BnwNativePlainLabelFrame f):frame_(std::move(f)){}
 BnwNativePlainLabelFrame frame_;
};
// Source-scoped algebra test seam. Prior caches must come from actual ordered
// initializers to be used in production; synthetic caches mint no receipt.
struct BnwLiteralImageChainInput {
 BnwNativeChildCache label,image;
 std::vector<BnwNativeChildCache> stack_children;
 std::size_t image_index=0;
 float stack_x=0,stack_y=0,stack_offset_x=0,stack_offset_y=0,padding=12;
 std::array<float,4> previous_stack_bounds{};
};
struct BnwLiteralImageChainResult {
 BnwNativeChildCache label,image;
 std::optional<BnwBottomStackCache> stack;
 std::vector<std::string> calls;
 bool mandatory_outer_callback=false;
 // If true, the size writer is suspended INSIDE the parent callback; its
 // later recursive anchor must not execute until this callback completes.
 bool size_writer_suspended=false;
};
BnwLiteralImageChainResult stage_bnw_literal_image_parent_chain(
 const BnwLiteralImageChainInput&,std::int32_t new_width,std::int32_t new_height,
 float label_offset_x,float label_offset_y);
}
