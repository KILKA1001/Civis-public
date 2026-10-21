#pragma once
#include <memory>
#include <utility>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// Only the genuine typed child+10 dispatcher can issue this occurrence.
// Its Impl keeps weak canonical domains and actual post-DTOR history.
class NotificationLayoutChildSlot10Completion final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationLayoutChildSlot10Completion(std::shared_ptr<const Impl>);
    NotificationLayoutChildSlot10Completion(const NotificationLayoutChildSlot10Completion&)=delete;
    NotificationLayoutChildSlot10Completion&operator=(const NotificationLayoutChildSlot10Completion&)=delete;
public:
    ~NotificationLayoutChildSlot10Completion()=default;
};
}
