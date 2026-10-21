#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// A real SAME setter return after original childGC and writer-history adoption.
// This receipt is partial history, not a complete457270/readiness grant.
class NotificationContextCursorRestoreReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextCursorRestoreReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextCursorRestoreReturn();
    NotificationContextCursorRestoreReturn(const NotificationContextCursorRestoreReturn&)=delete;
    NotificationContextCursorRestoreReturn&operator=(const NotificationContextCursorRestoreReturn&)=delete;
};
}
