#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutEmbeddedLabelState;
// Only a real allocated child's complete constructor return can issue this.
// This does not grant parsed Font/text/geometry, initialization or destruction.
class NotificationLayoutEmbeddedLabelFactoryReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class BnwPositionedLabelRunLoan;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutEmbeddedLabelFactoryReturn(std::shared_ptr<Impl>);
public:
    ~NotificationLayoutEmbeddedLabelFactoryReturn();
    NotificationLayoutEmbeddedLabelFactoryReturn(const NotificationLayoutEmbeddedLabelFactoryReturn&)=delete;
    NotificationLayoutEmbeddedLabelFactoryReturn& operator=(const NotificationLayoutEmbeddedLabelFactoryReturn&)=delete;
};
}
