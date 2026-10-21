#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextLuaStatePreparation;class NotificationContextRootDefaultFieldsReturn;class NotificationContextLuaControlsNamedRetirement;
// The private issuer owns the actual VM table/reference. These are neither
// registrar-completion nor whole Setup/RootRead authority.
class NotificationContextLuaControlsTableReturn final {
    friend class ::civ5::runtime::LuaScriptHost;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextLuaControlsTableReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextLuaControlsTableReturn();
    NotificationContextLuaControlsTableReturn(const NotificationContextLuaControlsTableReturn&)=delete;
    NotificationContextLuaControlsTableReturn&operator=(const NotificationContextLuaControlsTableReturn&)=delete;
};
class NotificationContextLuaControlsNamedReference final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextLuaControlsNamedReference(std::shared_ptr<const Impl>);
    // REQUIRED Host-owned enteredRead implementation, NOT an issued guard.
    class Guard final {
        friend class NotificationContextLuaControlsNamedReference;
        friend class ::civ5::runtime::LuaScriptHost;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>);
        void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const NotificationContextLuaStatePreparation>&,
            const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn>&)const;
        void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const NotificationContextLuaStatePreparation>&,
            const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn>&)const;
    };
    Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn>&)const;
public:
    ~NotificationContextLuaControlsNamedReference();
    NotificationContextLuaControlsNamedReference(const NotificationContextLuaControlsNamedReference&)=delete;
    NotificationContextLuaControlsNamedReference&operator=(const NotificationContextLuaControlsNamedReference&)=delete;
};
}}
