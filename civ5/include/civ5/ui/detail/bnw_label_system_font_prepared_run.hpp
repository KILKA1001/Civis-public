#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;}
namespace civ5::ui::detail {
class BnwLabelSystemFontBorrower;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
// One actual410900 preparation on a live lexical C4 input. This metadata
// neither initializes Label nor owns the canonical CPU/cache/renderer.
class BnwLabelSystemFontPreparedRun final {
 friend class BnwLabelSystemFontBorrower;
 friend class NotificationLayoutEmbeddedLabelState;
 friend class NotificationLayoutStandaloneLabelState;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 struct Impl;const std::shared_ptr<const Impl> impl_;
 explicit BnwLabelSystemFontPreparedRun(std::shared_ptr<const Impl>);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
 ~BnwLabelSystemFontPreparedRun();
 BnwLabelSystemFontPreparedRun(const BnwLabelSystemFontPreparedRun&)=delete;
 BnwLabelSystemFontPreparedRun&operator=(const BnwLabelSystemFontPreparedRun&)=delete;
};
}
