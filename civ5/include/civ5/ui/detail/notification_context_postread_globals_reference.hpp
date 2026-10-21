#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <utility>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextLuaStatePreparation;class NotificationLayoutContextOrdinaryState;
// Sole Tree issuer is the real457509 caller after completed RootRead and
// returned514/2E4. Closures borrow weak current owners; they cannot load a VM,
// substitute a filename, accept caller booleans or mint Context completion.
class NotificationContextPostreadGlobalsInputLoan final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::weak_ptr<const void> core_;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> root_;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> prepared_;
    const std::function<void()> first_,full_;
    NotificationContextPostreadGlobalsInputLoan(std::weak_ptr<const void> core,
        std::shared_ptr<const NotificationContextLuaStatePreparation> prepared,
        std::weak_ptr<NotificationLayoutContextOrdinaryState> root,
        std::function<void()> first,std::function<void()> full)
        :core_(std::move(core)),root_(std::move(root)),prepared_(std::move(prepared)),first_(std::move(first)),full_(std::move(full)){}
public:
    ~NotificationContextPostreadGlobalsInputLoan()=default;
    NotificationContextPostreadGlobalsInputLoan(const NotificationContextPostreadGlobalsInputLoan&)=delete;
    NotificationContextPostreadGlobalsInputLoan&operator=(const NotificationContextPostreadGlobalsInputLoan&)=delete;
};
// Own local1C Globals and distinct local10 ContextPtr-name refs, physically
// acquired on the SAME prepared coroutine. Host retains partial allocations
// across Lua longjmp; this view owns no exposed raw lua_State or registry index.
// Destruction invalidates the view. Original normal8F8820 callsite ordering
// remains a later required caller; failed/dropped views stay quarantined until
// real protected Host teardown (which cannot issue that caller return).
class NotificationContextPostreadGlobalsReference final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextPostreadGlobalsReference(std::shared_ptr<const Impl>);
public:
    ~NotificationContextPostreadGlobalsReference();
    NotificationContextPostreadGlobalsReference(const NotificationContextPostreadGlobalsReference&)=delete;
    NotificationContextPostreadGlobalsReference&operator=(const NotificationContextPostreadGlobalsReference&)=delete;
};
}}
