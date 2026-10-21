#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <utility>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
// Distinct concrete OWN value created by the GUI's actual7A5F20 constructor
// lane. Known tooltip fields are a view of this value, not its CData identity.
class NotificationContextTooltipManagerValue final {
    friend class NotificationContextGuiPort;
    friend class NotificationContextTooltipManagerOperand;
    friend class ::civ5::runtime::LuaScriptHost;
    struct Impl;const std::shared_ptr<Impl> impl_;
    explicit NotificationContextTooltipManagerValue(std::shared_ptr<Impl> p):impl_(std::move(p)){}
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void retire_owned()const noexcept;
public:
    ~NotificationContextTooltipManagerValue();
    NotificationContextTooltipManagerValue(const NotificationContextTooltipManagerValue&)=delete;
    NotificationContextTooltipManagerValue&operator=(const NotificationContextTooltipManagerValue&)=delete;
};
// Closed actual value loan. No loaded-registry, provider or GUI-ready grant.
class NotificationContextTooltipManagerOperand final {
    friend class NotificationContextGuiPort;
    friend class ::civ5::runtime::LuaScriptHost;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<const NotificationContextTooltipManagerValue> value_;
    const void*const cdata_;
    NotificationContextTooltipManagerOperand(std::shared_ptr<const NotificationContextTooltipManagerValue> value,const void* cdata)
      :value_(std::move(value)),cdata_(cdata){}
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
    ~NotificationContextTooltipManagerOperand()=default;
    NotificationContextTooltipManagerOperand(const NotificationContextTooltipManagerOperand&)=delete;
    NotificationContextTooltipManagerOperand&operator=(const NotificationContextTooltipManagerOperand&)=delete;
};
}
