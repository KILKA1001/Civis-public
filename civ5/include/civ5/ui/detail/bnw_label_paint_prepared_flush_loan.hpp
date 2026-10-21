#pragma once
#include <memory>
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;struct GlBnwSystemFontAtlasFrame;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class BnwLabelPaintTextLoan;
class BnwLabelPaintPositionedData;
// Completed actual 410900 preparation and physical 5A7250 whole-flush seal.
// This DATA never admits emission, membership, initialization or submission.
class BnwLabelPaintPreparedFlushLoan final {
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class BnwLabelPaintTextLoan;
 friend class BnwLabelPaintPositionedData;
 friend class ::civ5::graphics::GlBnwFontShader;
 struct IssuerKey final {private:IssuerKey()=default;friend class ::civ5::graphics::GlBnwSystemFontAtlas;};
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit BnwLabelPaintPreparedFlushLoan(IssuerKey,std::shared_ptr<Impl>);
 class Guard final {
  friend class BnwLabelPaintPreparedFlushLoan;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class BnwLabelPaintTextLoan;
  friend class BnwLabelPaintPositionedData;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_paint_without_callbacks(const BnwLabelPaintTextLoan&)const;
  void require_same_paint_guard_without_callbacks(const BnwLabelPaintTextLoan::Guard&)const;
  ::civ5::graphics::GlBnwSystemFontAtlasFrame accepted_pair_without_callbacks(
   const ::civ5::graphics::GlBnwSystemFontAtlas&)const;
 };
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwLabelPaintPreparedFlushLoan(); // Independent lexical authority retires FIRST.
 BnwLabelPaintPreparedFlushLoan(const BnwLabelPaintPreparedFlushLoan&)=delete;
 BnwLabelPaintPreparedFlushLoan&operator=(const BnwLabelPaintPreparedFlushLoan&)=delete;
};
}
