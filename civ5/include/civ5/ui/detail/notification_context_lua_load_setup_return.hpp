#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextLuaStatePreparation;
// Required supplier owned by LuaScriptHost. No issuer is provided here.
// StateName alone is not this return: real45731B..45747C including Controls={}
// and all reached registrations must have returned on the SAME installed518.
class NotificationContextLuaLoadSetupReturn final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextLuaLoadSetupReturn(std::shared_ptr<Impl>);
    class Guard final {
        friend class NotificationContextLuaLoadSetupReturn;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class ::civ5::runtime::LuaScriptHost;
        const std::shared_ptr<Impl> impl_;
        explicit Guard(std::shared_ptr<Impl>);
        void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
        void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
    };
    Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
public:
    ~NotificationContextLuaLoadSetupReturn();
    NotificationContextLuaLoadSetupReturn(const NotificationContextLuaLoadSetupReturn&)=delete;
    NotificationContextLuaLoadSetupReturn&operator=(const NotificationContextLuaLoadSetupReturn&)=delete;
};
}
