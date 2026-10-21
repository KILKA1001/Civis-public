#pragma once
#include <memory>
#include "civ5/ui/detail/bnw_label_system_font_request.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {class NotificationContextGuiPort;class BnwLabelSystemFontSource;class BnwLabelSystemFontBorrower;class BnwLabelSystemFontRequest;}
namespace civ5::graphics::detail {
// Backend metadata only. Real atlas ownership remains in the renderer cache.
class BnwLabelSystemFontAtlasMetadata final {
 friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::ui::detail::NotificationContextGuiPort;
 friend class ::civ5::ui::detail::BnwLabelSystemFontBorrower;
 friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
 struct Impl;const std::shared_ptr<Impl> impl_;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::graphics::GlBnwFontShader;
 // Canonical resource metadata only. This is not a Label/Paint issuer.
 class Guard final {
  friend class BnwLabelSystemFontAtlasMetadata;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_actual_atlas_without_callbacks(const GlBnwSystemFontAtlas&)const;
  std::shared_ptr<GlBnwSystemFontAtlas> retain_atlas_for_physical_cleanup_without_callbacks()const;
 };
 Guard capture_guard_without_callbacks()const;
 explicit BnwLabelSystemFontAtlasMetadata(std::shared_ptr<Impl>);
 void require_current_without_callbacks()const;
 void require_pending_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
     const ui::detail::BnwLabelSystemFontRequest::Guard&)const;
 void require_cached_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
     const ui::detail::BnwLabelSystemFontSource&)const;
public:
 ~BnwLabelSystemFontAtlasMetadata();
 BnwLabelSystemFontAtlasMetadata(const BnwLabelSystemFontAtlasMetadata&)=delete;
 BnwLabelSystemFontAtlasMetadata&operator=(const BnwLabelSystemFontAtlasMetadata&)=delete;
};
}
