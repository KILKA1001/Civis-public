#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwNormalLabelMaterialLoan;
//Real OWN arena allocation/current payload lease only. No Font-plane record,
//material-descriptor finalizer, Material, draw or Root18 completion authority.
class BnwNormalFontPayloadPrefix final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalLabelMaterialLoan;
 struct Impl;const std::shared_ptr<Impl> impl_;
 explicit BnwNormalFontPayloadPrefix(std::shared_ptr<Impl>);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
 ~BnwNormalFontPayloadPrefix(); //lease retirement FIRST; arena storage stays owned
 BnwNormalFontPayloadPrefix(const BnwNormalFontPayloadPrefix&)=delete;
 BnwNormalFontPayloadPrefix&operator=(const BnwNormalFontPayloadPrefix&)=delete;
};
}
