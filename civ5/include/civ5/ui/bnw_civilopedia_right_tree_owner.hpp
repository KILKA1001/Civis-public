#pragma once
#include "civ5/ui/bnw_civilopedia_scrollbar_owner.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/ui/bnw_civilopedia_grid_graphic_cache.hpp"
namespace civ5::ui {
class BnwCivilopediaLabelMetricsOwner;
struct BnwNativeChildCache {
 float width=0,height=0,x=0,y=0,edge_x=0,edge_y=0;
 std::array<float,4> bounds{};
 bool hidden=false,outside_x=false,outside_y=false;
};
struct BnwBottomStackCache {
 float width=0,height=0,edge_x=0,edge_y=0;
 std::array<float,4> bounds{};
 std::vector<std::array<float,2>> translations;
};
// Source-scoped Bottom/WrapWidth=0 reducer. Synthetic inputs are fixtures and
// cannot create a production right-tree or authorize panel/Lua readiness.
BnwBottomStackCache reduce_bnw_bottom_stack_cache(
 const std::vector<BnwNativeChildCache>&,float padding,float offset_x,float offset_y,float position_x=0,float position_y=0);
// Narrow source metric quote: only the actual two ASCII source literals.
// A descriptor without explicit ascadj is outside this selected profile.
std::array<std::int32_t,2> inspect_bnw_initial_literal_native_size(const GlyphGenFont &,std::string_view);
struct BnwCreatedRightControl {
 std::string path,element,id,style,font,text,anchor,offset,size;
 std::vector<std::pair<std::string,std::string>> source_attributes;
 std::optional<std::size_t> parent;
 std::vector<std::size_t> children;
 std::vector<content::XmlNode> special_source_groups;
 bool hidden=false,has_explicit_hidden=false,hidden_resolved=false,show_on_mouse_over=false;
 bool literal_metrics_complete=false;
 // Meaningful only when literal_metrics_complete. Parent Stack placement is
 // deliberately not supplied by an authored/source inventory rectangle.
 BnwScrollBarLayoutRect parent_local_rectangle;
 float cached_edge_x=0,cached_edge_y=0;
 std::optional<GlyphGenLine> glyph_line;
};
struct BnwRightLiteralImage {
 std::size_t image=0,label=0;
 std::shared_ptr<const assets::UiTextureBinding> texture;
};
struct BnwRightTreeFrame {
 runtime::BnwStockLuaClientExtent client;
 std::uint64_t shell_revision=0;
 std::string xml_sha256,styles_sha256,font_sha256,font_key;
 GlyphGenGeometry font_geometry;
 std::int32_t native_font_ascent=0;
 // Explicit XML factory instances only; style-created/embedded children still
 // need their owners and are not counted as initialized controls.
 std::vector<BnwCreatedRightControl> controls;
 std::size_t source_node_count=0,metadata_group_count=0;
 std::array<std::size_t,3> stacks{};
 std::vector<BnwRightLiteralImage> literal_images;
 std::shared_ptr<const BnwCivilopediaGridGraphicCacheOwner> grid_graphics;
 std::shared_ptr<const BnwCivilopediaLabelMetricsOwner> label_metrics;
 std::vector<std::string> pending_locale_keys;
 std::size_t completed_literal_labels=0;
 std::string pending_path,pending_child_path,pending_dependency;
 bool literal_parent_bounds_propagation_complete=false;
 bool native_child_creation_complete=false,all_child_bounds_ready=false,initial_panel_calculation_complete=false;
 bool native_slider_setter_complete=false;
};
// Retain this owner with the same stock VM/shell epoch. Creation allocates each
// selected right subtree node exactly once, including anonymous controls and
// empty Stack children. It executes the supported native literal metric scope.
// A repeated advance only returns its retained frontier; no source/Lua replay.
class BnwCivilopediaRightTreeOwner {
 public:
 static std::shared_ptr<BnwCivilopediaRightTreeOwner> create(
  const std::filesystem::path &,const assets::ResourceMountTable &,
  const BnwScrollBarLayoutFrame &actual_shell);
 const BnwRightTreeFrame &snapshot()const noexcept{return frame_;}
 const BnwRightTreeFrame &advance(const runtime::BnwStockLuaClientExtent &,std::uint64_t shell_revision)const;
 const BnwCreatedRightControl *control(const runtime::BnwStockLuaClientExtent &,
  std::uint64_t shell_revision,std::string_view id)const noexcept;
 private:
 explicit BnwCivilopediaRightTreeOwner(BnwRightTreeFrame f):frame_(std::move(f)){}
 BnwRightTreeFrame frame_;
};
} // namespace civ5::ui
