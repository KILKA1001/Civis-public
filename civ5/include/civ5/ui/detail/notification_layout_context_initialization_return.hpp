#pragma once
#include <memory>
#include <utility>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// Defined by the actual construction/load owner, never a caller DTO. It must
// retain genuine parent/attachment, initial GUI dimension and native-zero
// supplier provenance. Those suppliers remain mandatory, separately owned.
class NotificationLayoutContextConstructionReturn;
class NotificationLayoutChildSlot10Completion;
class NotificationLayoutContextInitializationReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationLayoutContextInitializationReturn(std::shared_ptr<const Impl> actual)
        :impl_(std::move(actual)){}
    NotificationLayoutContextInitializationReturn(const NotificationLayoutContextInitializationReturn&)=delete;
    NotificationLayoutContextInitializationReturn&operator=(const NotificationLayoutContextInitializationReturn&)=delete;
public:
    ~NotificationLayoutContextInitializationReturn()=default;
};
}
