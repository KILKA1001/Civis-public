#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
// Issued only after genuine direct Lua/manager/GC/cursor/individual cleanup
// and the enclosing direct caller's normal lexical destruction.
class NotificationContextDirectLoadReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextDirectLoadReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextDirectLoadReturn();
    NotificationContextDirectLoadReturn(const NotificationContextDirectLoadReturn&)=delete;
    NotificationContextDirectLoadReturn&operator=(const NotificationContextDirectLoadReturn&)=delete;
};
}}
