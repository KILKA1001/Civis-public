#pragma once
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include <array>
#include <cstdint>
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::graphics {class GlBnwFontShader;}
namespace civ5::graphics::detail {
// Current actual FBO0 row phase only. No material/depth-policy/draw issuer.
// Requires Root's narrow outer friendship for naming its private DATA Guard.
class BnwLabelDefaultFontTargetLoan final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::GlBnwFontShader;
 class IssuerKey final {
  friend class ::civ5::ui::UiScreenGlRenderer;
  IssuerKey()noexcept=default;
 };
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit BnwLabelDefaultFontTargetLoan(const IssuerKey&,std::shared_ptr<Impl>)noexcept;
 struct CapturedTargetData final {
  std::array<std::int32_t,2> drawable_extent;
  std::array<double,2> preserved_depth;
 };
 class RowsRestorationWitness;
 class Guard final {
  friend class BnwLabelDefaultFontTargetLoan;
  friend class RowsRestorationWitness;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  // Captures an already Ui-issued opaque Impl; never constructs new authority.
  explicit Guard(std::shared_ptr<Impl>)noexcept;
  void require_current_without_callbacks()const;
  void require_source_owned(const ::civ5::runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_active_owned()const;
  CapturedTargetData captured_target_data_without_callbacks()const;
  void require_same_positioned_data_without_callbacks(
   ::civ5::ui::detail::BnwLabelPaintPositionedData::Guard)const;
 };
 // Closed evidence issued ONLY by actual Ui Rows finish. The original
 // retired FontTarget Guard is retained as identity, never active authority.
 class RowsRestorationWitness final {
  friend class BnwLabelDefaultFontTargetLoan;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwFontShader;
  struct Impl;
  const std::shared_ptr<const Impl> impl_;
  explicit RowsRestorationWitness(const IssuerKey&,std::shared_ptr<const Impl>)noexcept;
  void require_current_without_callbacks()const;
  void require_source_owned(const ::civ5::runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_original_font_target_without_callbacks(Guard)const;
 public:
  RowsRestorationWitness(const RowsRestorationWitness&)=default;
  RowsRestorationWitness&operator=(const RowsRestorationWitness&)=delete;
  ~RowsRestorationWitness();
 };
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwLabelDefaultFontTargetLoan()noexcept;
 BnwLabelDefaultFontTargetLoan(const BnwLabelDefaultFontTargetLoan&)=delete;
 BnwLabelDefaultFontTargetLoan&operator=(const BnwLabelDefaultFontTargetLoan&)=delete;
};
}
