#pragma once
#include "civ5/graphics/gl_bnw_system_font_atlas.hpp"
namespace civ5::graphics {class GlBnwSystemFontAtlas;}
namespace civ5::ui {class BnwSystemFontCpuOwner;class BnwSystemFontCacheOwner;}
namespace civ5::ui::detail {class BnwLabelSystemFontBorrower;class BnwLabelPaintTextLoan;}
namespace civ5::graphics::detail {
// Only a real Atlas nested lexical state may construct this bottom fence.
// The concrete state checks captured Paint + Metadata + Prepared Guards.
// This virtual call performs pure FIRST only; it grants no Source/GL authority.
class BnwOwnedFontPaintEffectFence {
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::ui::BnwSystemFontCpuOwner;
 friend class ::civ5::ui::BnwSystemFontCacheOwner;
 friend class ::civ5::ui::detail::BnwLabelSystemFontBorrower;
 class IssuerKey final {
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  IssuerKey()noexcept=default;
  ~IssuerKey()=default;
  IssuerKey(const IssuerKey&)=delete;
  IssuerKey&operator=(const IssuerKey&)=delete;
  IssuerKey(IssuerKey&&)=delete;
  IssuerKey&operator=(IssuerKey&&)=delete;
 };
 explicit BnwOwnedFontPaintEffectFence(const IssuerKey&)noexcept{}
 virtual ~BnwOwnedFontPaintEffectFence()=default;
 BnwOwnedFontPaintEffectFence(const BnwOwnedFontPaintEffectFence&)=delete;
 BnwOwnedFontPaintEffectFence&operator=(const BnwOwnedFontPaintEffectFence&)=delete;
 BnwOwnedFontPaintEffectFence(BnwOwnedFontPaintEffectFence&&)=delete;
 BnwOwnedFontPaintEffectFence&operator=(BnwOwnedFontPaintEffectFence&&)=delete;
 virtual void require_current_without_callbacks()const=0;
 // A captured Guard is compared before callbacks; never reread a raw facade.
 virtual void require_same_paint_guard_without_callbacks(const ::civ5::graphics::GlBnwSystemFontAtlas::PaintGuard&)const=0;
};
}
