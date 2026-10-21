#pragma once
#include <cstdint>
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwNormalFontPayloadPrefix;
class BnwNormalLabelMaterialLoan;
class BnwNormalRootBatchHistory;
// Actual executed OWN normal-zero batch beginning only. This is not an
// Original current-batch observation, completed root18, Label material, a
// selector/Font attachment, or a new Paint/DATA/Frame membership authority.
class BnwNormalGuiBatchPrefix final {
 friend class NotificationContextGuiPort;
 friend class BnwNormalFontPayloadPrefix;
 friend class BnwNormalLabelMaterialLoan;
 friend class BnwNormalRootBatchHistory;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
 struct Impl;const std::shared_ptr<Impl> impl_;
 explicit BnwNormalGuiBatchPrefix(std::shared_ptr<Impl>);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
 ~BnwNormalGuiBatchPrefix(); // Retirement FIRST; no Source/GL/world callbacks.
 BnwNormalGuiBatchPrefix(const BnwNormalGuiBatchPrefix&)=delete;
 BnwNormalGuiBatchPrefix&operator=(const BnwNormalGuiBatchPrefix&)=delete;
};
}
