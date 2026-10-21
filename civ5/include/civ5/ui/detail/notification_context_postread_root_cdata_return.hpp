#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextUiManagerValueLoan;
class NotificationContextTooltipManagerOperand;
class NotificationContextDirectLuaTailInput;
class NotificationContextCursorRestoreReturn;
class NotificationContextLoaderCleanupReturn;
// SAME actual current Context Root reflection value returned by42F060.
// Cache-MISS must complete its real LuaContext517C20 definition before issue.
// Neither child-Controls output nor a generic ContextPtr table is accepted.
class NotificationContextPostreadRootCDataReturn final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextPostreadRootCDataReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextPostreadRootCDataReturn();
    // Actual4575718F8C60/copy/local-DTOR owner; same creator result only.
    void acquire_contextptr_lookup_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextUiManagerValueLoan>&,
        const std::shared_ptr<const NotificationContextTooltipManagerOperand>&)const;
    void require_contextptr_lookup_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void register_postread_environment_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void invoke_direct_initial_lua_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextDirectLuaTailInput>&)const;
    void require_direct_initial_lua_state_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextDirectLuaTailInput>&)const;
    void invoke_direct_initial_showhide_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextDirectLuaTailInput>&,
        const std::shared_ptr<const NotificationContextCursorRestoreReturn>&,bool)const;
    std::shared_ptr<const NotificationContextLoaderCleanupReturn> cleanup_direct_loader_temporaries_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const NotificationContextDirectLuaTailInput>&,
        const std::shared_ptr<const NotificationContextCursorRestoreReturn>&)const;
    NotificationContextPostreadRootCDataReturn(const NotificationContextPostreadRootCDataReturn&)=delete;
    NotificationContextPostreadRootCDataReturn&operator=(const NotificationContextPostreadRootCDataReturn&)=delete;
};
}}
