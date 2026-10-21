#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextInitialLuaGcReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextInitialLuaGcReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextInitialLuaGcReturn();
    NotificationContextInitialLuaGcReturn(const NotificationContextInitialLuaGcReturn&)=delete;
    NotificationContextInitialLuaGcReturn& operator=(const NotificationContextInitialLuaGcReturn&)=delete;
};
}}
