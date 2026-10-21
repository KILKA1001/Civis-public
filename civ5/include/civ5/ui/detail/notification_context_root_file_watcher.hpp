#pragma once
#include <memory>
#include <string>
#include <array>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// Own x64 port of5657B0/71C790/750FC0/456C70. This is a field owner, not a
// RootRead/Controls/factory/initialization return. Only the genuine Root reader
// may drive the ordered stages from its actual resolved510 VALUE.
class NotificationContextRootFileWatcher final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootFileWatcher(std::shared_ptr<Impl>);
    static std::shared_ptr<NotificationContextRootFileWatcher> allocate_first_owned(std::array<char16_t,260>);
    void capture_file_attributes_owned()const;
    void open_change_notification_owned()const;
    void register_wait_owned()const;
    void require_reached_without_callbacks()const;
    bool changed_without_callbacks()const;
public:
    ~NotificationContextRootFileWatcher();
    NotificationContextRootFileWatcher(const NotificationContextRootFileWatcher&)=delete;
    NotificationContextRootFileWatcher&operator=(const NotificationContextRootFileWatcher&)=delete;
};
}
