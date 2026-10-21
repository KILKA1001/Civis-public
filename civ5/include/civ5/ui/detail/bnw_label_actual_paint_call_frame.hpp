#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;struct ComposedUiFrame;}
namespace civ5::ui::detail {
class BnwLabelPaintTextLoan;
class BnwPositionedLabelRunLoan;
class BnwStandalonePositionedLabelRunLoan;
class BnwNormalLabelPaintEmission;
class BnwNormalGuiBatchPrefix;
class BnwNormalLabelMaterialLoan;
class NotificationContextGuiPort;
class BnwNormalRootBatchHistory;
class BnwNormalItemOccurrence;
class BnwLabelSystemFontBorrower;
class BnwLabelSystemFontPreparedRun;
class BnwNormalLabelChildScopeHealthBridge;
// Lexical actual591020 renderer callframe. Physical storage does not prolong
// WorldViewPaintScope, frame/publication, Source/gui/window or renderer life.
// It grants no emitted glyph membership, target program, upload or draw.
class BnwLabelActualPaintCallFrame final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
 friend class BnwPositionedLabelRunLoan;
 friend class BnwStandalonePositionedLabelRunLoan;
 friend class BnwNormalLabelPaintEmission;
 friend class BnwNormalGuiBatchPrefix;
 friend class BnwNormalLabelMaterialLoan;
 friend class NotificationContextGuiPort;
 friend class BnwNormalRootBatchHistory;
 friend class BnwNormalItemOccurrence;
 friend class BnwLabelPaintTextLoan;
 friend class BnwLabelSystemFontBorrower;
 friend class BnwLabelSystemFontPreparedRun;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::graphics::GlBnwFontShader;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 // Other frame consumers may name this key, but only the renderer can mint it.
 class IssuerKey final {
  friend class ::civ5::ui::UiScreenGlRenderer;
  IssuerKey()=default;
  IssuerKey(const IssuerKey&)=delete;
 };
 explicit BnwLabelActualPaintCallFrame(const IssuerKey&,std::shared_ptr<Impl>);
 class Guard;
 //A separate capability extracted under the original ACTIVE Frame's FULL.
 //Later checks use actual root life/scope/Source and OWN frame storage. They
 //never call old Frame::first or require another current item/publication.
public:
 // Only the existing health TYPE NAME is public; capture and readers stay private.
 class RootScopeHealthGuard final {
  friend class Guard;
  friend class BnwNormalLabelChildScopeHealthBridge;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class NotificationContextGuiPort;
  friend class BnwNormalRootBatchHistory;
  friend class BnwNormalItemOccurrence;
  friend class BnwNormalLabelMaterialLoan;
  friend class BnwNormalGuiBatchPrefix;
  friend class ::civ5::graphics::GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  class CaptureKey final {
   friend class Guard;
   CaptureKey()=default;
   CaptureKey(const CaptureKey&)=delete;
  };
  struct Impl;
  const std::shared_ptr<const Impl> impl_;
  explicit RootScopeHealthGuard(std::shared_ptr<const Impl>,const CaptureKey&);
  void require_physical_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_root_scope_without_callbacks(const RootScopeHealthGuard&)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  //Publication is an immutable association only here, with no raw dereference.
  //The later Root/Item holder must retain SAME original Publication separately.
  void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
  void require_actual_frame_owner_without_callbacks(const std::shared_ptr<const ::civ5::ui::ComposedUiFrame>&)const;
  std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
 public:
  RootScopeHealthGuard(const RootScopeHealthGuard&)=default;
  RootScopeHealthGuard&operator=(const RootScopeHealthGuard&)=delete;
  ~RootScopeHealthGuard()=default;
 };
private:
 class Guard final {
  friend class BnwLabelActualPaintCallFrame;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
  friend class BnwPositionedLabelRunLoan;
  friend class BnwStandalonePositionedLabelRunLoan;
  friend class BnwNormalLabelPaintEmission;
  friend class BnwNormalGuiBatchPrefix;
  friend class BnwNormalLabelMaterialLoan;
  friend class NotificationContextGuiPort;
  friend class BnwNormalRootBatchHistory;
  friend class BnwNormalItemOccurrence;
  friend class BnwLabelPaintTextLoan;
  friend class BnwLabelSystemFontBorrower;
  friend class BnwLabelSystemFontPreparedRun;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  std::shared_ptr<const BnwWorldViewControlRenderPublication> original_publication_without_callbacks()const;
  void require_actual_paint_item_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_actual_control_without_callbacks(const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  RootScopeHealthGuard capture_root_scope_health_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  // Real raw x87 CW and MXCSR captured at this renderer entry, not caller DATA.
  std::array<std::uint32_t,2> actual_numeric_profile_without_callbacks()const;
 };
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwLabelActualPaintCallFrame(); // Retires lexical authority FIRST.
 BnwLabelActualPaintCallFrame(const BnwLabelActualPaintCallFrame&)=delete;
 BnwLabelActualPaintCallFrame&operator=(const BnwLabelActualPaintCallFrame&)=delete;
};
}
