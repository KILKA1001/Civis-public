#pragma once
#include "civ5/ui/bnw_native_slider_setter.hpp"
#include "civ5/ui/detail/bnw_native_label_field_storage.hpp"
#include "civ5/ui/detail/bnw_native_root_context_fields.hpp"
namespace civ5::ui {
class BnwNativeUiColorOwner;
// Owned native numeric constructor/base-parser fields, distinctly BEFORE
// 0x5761E0 initialization. Dimensions are nominal; position/edges/bounds0.
struct BnwNativeRightDirectPrior {
 std::size_t control_index=0;
 std::string path,element,id;
 std::uint32_t geometry_flags=0;
 float offset_x=0,offset_y=0;
 BnwNativeChildCache cache;
 bool geometry_constructor_parser_complete=false,native_initialized=false;
};
enum class BnwNativePanelSpecialCallback { scroll_up,scroll_down,slider_value_changed };
struct BnwNativeSpecialCallbackBinding {
 BnwNativePanelSpecialCallback kind=BnwNativePanelSpecialCallback::scroll_up;
 std::uint32_t native_target=0;
 std::shared_ptr<const std::uint8_t> panel_identity;
 bool dispatch_available=false;
};
struct BnwNativeSpecialButtonGeometry {
 std::string id,style;
 std::shared_ptr<const assets::UiTextureBinding> texture;
 std::uint32_t flags=0x8028,state=0;
 BnwNativeChildCache cache;
 BnwGraphicPair texture_offset{},state_increment{},texture_size{},draw_size{},uv_start{},uv_span{};
 bool constructor_parser_complete=false,geometry_prefix_complete=false;
};
struct BnwNativePreRatioSpecialGeometry {
 std::array<BnwNativeSpecialButtonGeometry,2> buttons;
 BnwNativeSliderFields slider;
 BnwNativeChildCache slider_cache,track_cache,shuttle_cache;
 std::shared_ptr<const BnwSliderGraphicCacheOwner> embedded_graphics;
 std::uint32_t slider_flags=0,track_flags=0,shuttle_flags=0,slider_button_state=0;
 std::array<BnwNativeSpecialCallbackBinding,3> callbacks;
 // Source writes registration slots only; this is not callback execution.
 std::vector<std::string> ordered_operations;
 bool set_value_same_offset_branch=false,geometry_prefix_complete=false;
 bool button_events_complete=false,shader_draw_available=false;
};
// Real scoped Box constructor instances. Source declarations are retained
// separately: pending Color providers prevent claiming successful native Read.
struct BnwNativeEnclosingBoxConstructor {
 std::string path;
 std::vector<std::pair<std::string,std::string>> source_attributes;
 std::vector<std::string> source_normal_child_order;
 // Source quote for a later parser: never substitutes cache+30/+34 or+70/+74.
 std::optional<float> authored_nominal_width,authored_nominal_height;
 BnwGraphicPair authored_offset;
 bool authored_full_width=false,authored_full_height=false;
 std::shared_ptr<const std::uint8_t> identity;
 std::uint32_t native_kind=14,flags=0;
 BnwNativeChildCache cache;
 float offset_x=0,offset_y=0,branch_alpha=1;
 std::array<float,4> color{1,1,1,1};
 bool constructor_complete=false,parser_complete=false,subtype_color_read_complete=false;
 bool normal_registration_complete=false,initialized=false;
};
struct BnwNativeEnclosingParentBinding {
 std::shared_ptr<const std::uint8_t> parent_identity,child_identity;
 std::size_t source_normal_ordinal=0;
 // The actual SAME scoped owners exist. Native AddChild still waits for
 // preceding sibling factories; no sparse vector is a successful append.
 std::uint32_t required_native_add_child=0x44bf40;
 bool source_append_complete=false;
};
struct BnwNativeEnclosingConstructorGraph {
 std::array<BnwNativeEnclosingBoxConstructor,2> boxes; // outer then immediate
 std::array<BnwNativeEnclosingParentBinding,2> bindings; // outer->inner->Panel
 std::shared_ptr<const std::uint8_t> panel_identity;
 bool constructor_instances_created=false,parent_bounds_dispatch_available=false;
 std::string pending_source,pending_dependency;
};
struct BnwNativeInitialGraphFrame {
 runtime::BnwStockLuaClientExtent client;
 std::string xml_sha256,styles_sha256;
 std::array<BnwNativeRightDirectPrior,3> stacks;
 std::array<std::vector<BnwNativeRightDirectPrior>,3> direct_children;
 float panel_nominal_width=1002,panel_nominal_height=980;
 float panel_internal_width=0,panel_internal_height=0;
 std::array<BnwNativeSpecialButtonGeometry,2> special_button_priors;
 std::optional<BnwNativePreRatioSpecialGeometry> pre_ratio_special_geometry;
 std::optional<BnwNativeEnclosingConstructorGraph> enclosing_constructors;
 std::uint64_t geometry_revision=0;
 bool direct_geometry_constructor_parser_complete=false;
 bool special_initialization_complete=false,normal_initialization_complete=false;
 bool initial_ratio_complete=false,stock_loading_complete=false;
 std::string pending_source,pending_dependency;
};
// Actual selected source prefix frontier, not completed native Read.
// Source-predicate/local-field stage only. No original Read invocation,
// temporary native string allocation, or ordered factory traversal is attested.
struct BnwNativeLabelXmlReadPrefixFrame {
 std::size_t control_index=0;
 std::string source_path,xml_sha256;
 std::optional<detail::BnwLabelFieldSnapshot> fields;
 bool absent_style_branch_complete=false,subtype_read_entry_modeled=false;
 //5E3AD9 must write actual supplied +3C before any Font/Color setters.
 std::uint32_t pending_source_address=0x5e3ad9;
 std::string pending_dependency;
 static constexpr bool actual_original_read_executed=false;
 static constexpr bool actual_context_write=false,subtype_read_complete=false;
 static constexpr bool base_read_complete=false,parent_append_complete=false;
 static constexpr bool native_initialized=false,native_label_ready=false;
};
class BnwCivilopediaNativeInitialGraph {
 public:
 static std::shared_ptr<const BnwCivilopediaNativeInitialGraph> create(
  std::uintptr_t actual_hwnd,runtime::BnwStockLuaClientExtentProvider,
  const std::filesystem::path&,const assets::ResourceMountTable&,
  std::shared_ptr<const BnwCivilopediaRightTreeOwner>,
  std::shared_ptr<const BnwNativeSliderJournalOwner>);
 ~BnwCivilopediaNativeInitialGraph();
 const BnwNativeInitialGraphFrame&current()const;
 // Detached source-ordered geometry fork. Normal traversal and panel ratio
 // remain unexecuted. Repeating this transition rejects rather than replaying.
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_initialize_pre_ratio_special_geometry()const;
 // Retains real constructor fields once, with SAME source/Panel identities.
 // It does not execute unknown Color/full-size/child factories or AddChild.
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_create_enclosing_box_constructors()const;
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_read_outer_box_color_prefix(std::shared_ptr<const BnwNativeUiColorOwner>)const;
 // Detached subtype constructor fields attached to SAME retained source graph.
 // XML Read/AddChild/initialize are still unavailable; no metrics/paint input.
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_create_label_constructor_fields()const;
 // Private SAME root Context component, distinct from Panel/source tokens.
 // Constructor fields and empty declarations do NOT supply child Read+3C,
 // ordered factory+44/name+64/AddChild+38 or native initialization authority.
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_create_root_context_constructor_fields()const;
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_read_root_context_empty_declaration()const;
 std::optional<detail::BnwRootContextFieldSnapshot> staged_root_context_fields()const;
 // SAME +A0 metadata cell/source cursor; stops before ordinary child factory.
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_register_root_instance_metadata_prefix()const;
 std::optional<detail::BnwRootContextInstanceTemplateQuote>
  staged_root_instance_template(std::string_view name)const;
 std::vector<std::size_t> staged_label_control_indices()const;
 detail::BnwLabelFieldSnapshot staged_label_fields(std::size_t control_index)const;
 // Mutates SAME shared field cell and retires earlier snapshots across forks.
 // Source/context failure after reservation is uncertain/fail-closed.
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
  fork_enter_label_xml_read_prefix(std::size_t control_index)const;
 std::optional<BnwNativeLabelXmlReadPrefixFrame>
  staged_label_xml_read_prefix(std::size_t control_index)const;
 BnwGraphicPair enclosing_panel_anchor_nominal_input()const;
 void dispatch_enclosing_parent_bounds()const;
 // Typed registered native receiver; refuses until its actual scroll/normal
 // prerequisites are owned. A slot registration is never an event receipt.
 void dispatch_special_callback(BnwNativePanelSpecialCallback)const;
 private:
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit BnwCivilopediaNativeInitialGraph(std::unique_ptr<Impl>);
};
}
