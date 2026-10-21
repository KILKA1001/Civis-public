#pragma once
#include <memory>
#include <utility>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextLuaStatePreparation;
// Actual supported RootRead5D2920 return, tied to the still-live SAME read
// frame, newly parsed XML84 and installed518. Not wholeRead or Setup authority.
class NotificationContextRootDefaultFieldsReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::runtime::LuaScriptHost;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootDefaultFieldsReturn(std::shared_ptr<Impl>);
public:
    class Guard final {
        friend class NotificationContextRootDefaultFieldsReturn;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class ::civ5::runtime::LuaScriptHost;
        const std::shared_ptr<Impl> impl_;
        explicit Guard(std::shared_ptr<Impl> value):impl_(std::move(value)){}
        // Pure FIRST, no Host query/NativeScope recursion or Source callbacks.
        void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
        // Call strictly outside Host NativeScope. Snapshot operands by VALUE
        // before callbacks; repeat pure FIRST after every Source boundary.
        void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
    };
private:
    Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
public:
    ~NotificationContextRootDefaultFieldsReturn();
    NotificationContextRootDefaultFieldsReturn(const NotificationContextRootDefaultFieldsReturn&)=delete;
    NotificationContextRootDefaultFieldsReturn&operator=(const NotificationContextRootDefaultFieldsReturn&)=delete;
};
}}
