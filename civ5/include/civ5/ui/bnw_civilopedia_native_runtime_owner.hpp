#pragma once
#include "civ5/ui/bnw_civilopedia_native_initial_graph.hpp"
namespace civ5::runtime { class BnwSelectedStockLuaBootstrap; }
namespace civ5::assets { class CivisResourceBankOwner; }
namespace civ5::ui { class CivisResourceBankUiSnapshot; class CivisResourceBankFontColorSnapshot; }
namespace civ5::ui { class SelectedBnwPreviewHost; }
namespace civ5::graphics { class BnwOwnedCameraRendererWindowLease; }
namespace civ5::ui { class BnwOwnedCameraGuiScreen; class BnwNativePackageDiscoveryOwner; namespace detail { class BnwNativeCompletedColorQuery; } }
namespace civ5::ui {
// Raw HWND overload is diagnostic and borrowed. The typed renderer overload
// retains the canonical physical owner issued by its actual stack Loan.
// Neither overload grants completed native UI, resources, GPU or Lua activation.
class BnwCivilopediaNativeWindowBinding {
 public:
 ~BnwCivilopediaNativeWindowBinding();
 BnwCivilopediaNativeWindowBinding(const BnwCivilopediaNativeWindowBinding&)=delete;
 BnwCivilopediaNativeWindowBinding&operator=(const BnwCivilopediaNativeWindowBinding&)=delete;
 std::optional<runtime::BnwStockLuaClientExtent> current_extent()const;
 std::uintptr_t native_window()const;
 std::uintptr_t occurrence()const;
 void retire()noexcept;
 private:
 friend class runtime::BnwSelectedStockLuaBootstrap;
 friend class BnwCivilopediaNativeRuntimeOwner;
 struct Impl;std::shared_ptr<Impl>impl_;
 explicit BnwCivilopediaNativeWindowBinding(std::shared_ptr<Impl>);
 static std::shared_ptr<BnwCivilopediaNativeWindowBinding> attach(
  std::uintptr_t,runtime::BnwStockLuaClientExtentProvider);
 static std::shared_ptr<BnwCivilopediaNativeWindowBinding> attach_renderer(
  std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,
  runtime::BnwStockLuaClientExtentProvider);
 // Producer facts are observations only; no DTO grants window ownership.
 struct RendererFacts {
  std::uintptr_t window=0,dc=0,wgl=0,occurrence=0;
  std::uint32_t process=0,thread=0;
  std::uint64_t revision=0;
 };
 void require_owned_renderer_current()const;
 void require_same_gui_screen(const BnwOwnedCameraGuiScreen&)const;
 static std::shared_ptr<BnwCivilopediaNativeWindowBinding> attach_impl(
  std::uintptr_t,runtime::BnwStockLuaClientExtentProvider,
  std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,RendererFacts);
};
enum class BnwCivilopediaNativeOperation {
 root_ordinary_child,ordinary_box_factory,box_read,color_read,named_color_lookup,base_box_read,
 civis_color_read,civis_palette_color_lookup
};
struct BnwCivilopediaNativeCall {
 BnwCivilopediaNativeOperation operation;
 std::uint32_t source_address=0;
 std::size_t source_ordinal=0;
 std::string source_path;
};
struct BnwCivilopediaNativeBoxFrame {
 std::uintptr_t receiver_occurrence=0,read_context_occurrence=0;
 std::string path,id,pending_color_token;
 BnwNativeChildCache cache;
 std::array<float,4> color{1,1,1,1};
 std::uint32_t native_kind=14,flags=0;
 // Read's actual context argument is retained. context+3C is written only
 // by entered BaseRead; parent+38/normal AddChild remain unexecuted.
 bool parent_field_written=false,context_field_written=false;
 bool read_returned=false,source_append_completed=false;
 // Observed completed Color interval only, never full Box/native readiness.
 bool color_lookup_attempted=false,color_read_returned=false,numeric_color_miss=false;
 // Own CPU palette/value receiver only; never Original completed Color authority.
 bool civis_color_receiver=false;
 std::uint32_t color_lookup_key=0;
};
struct BnwCivilopediaNativeRuntimeFrame {
 runtime::BnwStockLuaClientExtent client;
 std::string xml_sha256,styles_sha256;
 std::uintptr_t window_observer_occurrence=0,right_tree_occurrence=0,root_context_occurrence=0;
 std::size_t root_source_cursor=0,instance_metadata_records=0;
 std::uint64_t ordinary_factory_executions=0,box_read_entries=0;
 std::vector<BnwCivilopediaNativeCall> call_stack;
 std::optional<BnwCivilopediaNativeBoxFrame> box;
 std::string pending_dependency;
 // Actual INITIAL physical renderer facts, not completed production UI.
 // A copied frame never grants the canonical owner retained internally.
 std::uintptr_t initial_renderer_window_occurrence=0;
 std::uintptr_t initial_renderer_dc=0,initial_renderer_wgl=0;
 std::uint64_t initial_renderer_viewport_revision=0;
 // No completed production WindowLease/native UI, setter or epoch here.
 static constexpr bool production_window_lease_bound=false;
 static constexpr bool native_slider_setter_complete=false,stock_loading_complete=false;
};
// Created only by the retained bootstrap from its SAME shell/tree/mount table.
// Owns the actual pending source receiver/call stack; no completion setters.
class BnwCivilopediaNativeRuntimeOwner {
 public:
 ~BnwCivilopediaNativeRuntimeOwner();
 BnwCivilopediaNativeRuntimeFrame advance_initialization();
 BnwCivilopediaNativeRuntimeFrame current()const;
 void retire()noexcept;
 private:
 friend class runtime::BnwSelectedStockLuaBootstrap;
 struct Impl;std::shared_ptr<Impl>impl_;
 explicit BnwCivilopediaNativeRuntimeOwner(std::shared_ptr<Impl>);
 void bind_gui_screen(std::shared_ptr<const BnwOwnedCameraGuiScreen>);
 void bind_completed_color_query(std::shared_ptr<const detail::BnwNativeCompletedColorQuery>,
  std::weak_ptr<const BnwNativePackageDiscoveryOwner>);
 void bind_civis_color_snapshot(const std::shared_ptr<const CivisResourceBankFontColorSnapshot>&,
  const std::shared_ptr<assets::CivisResourceBankOwner>&,
  const std::shared_ptr<const CivisResourceBankUiSnapshot>&,
  const std::weak_ptr<const SelectedBnwPreviewHost>&);
 static std::shared_ptr<BnwCivilopediaNativeRuntimeOwner> create(
  std::shared_ptr<BnwCivilopediaNativeWindowBinding>,const std::filesystem::path&,
  const assets::ResourceMountTable&,const BnwCivilopediaScrollBarOwner&,
  std::shared_ptr<BnwCivilopediaRightTreeOwner>);
};
}
