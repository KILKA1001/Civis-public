#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class BnwSelectedUserOptionsOwner;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// Genuine effective InterfaceSettings on the SAME selected UserSettings owner.
// The only admitted subset is explicit Small=0,Auto=0 after actual registration.
// No default, caller flag, Bank MISS or read-only source snapshot can issue it.
class NotificationContextRootSmallUiPolicyLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class ::civ5::runtime::BnwSelectedUserOptionsOwner;
    struct Impl;
public:
    class Guard final {
        friend class NotificationContextRootSmallUiPolicyLoan;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class ::civ5::runtime::BnwSelectedUserOptionsOwner;
        struct Capture;
        const std::shared_ptr<Impl> impl_;
        const std::shared_ptr<const Capture> capture_;
        Guard(std::shared_ptr<Impl>,std::shared_ptr<const Capture>);
        void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        bool branch_taken_without_callbacks()const;
    public:
        Guard(const Guard&)=default;
        Guard(Guard&&)=default;
        Guard&operator=(const Guard&)=delete;
    };
private:
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootSmallUiPolicyLoan(std::shared_ptr<Impl>);
    void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    bool branch_taken_without_callbacks()const;
    bool small_ui_assets_without_callbacks()const;
    Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void retire_owned()const noexcept;
public:
    ~NotificationContextRootSmallUiPolicyLoan();
    NotificationContextRootSmallUiPolicyLoan(const NotificationContextRootSmallUiPolicyLoan&)=delete;
    NotificationContextRootSmallUiPolicyLoan&operator=(const NotificationContextRootSmallUiPolicyLoan&)=delete;
};
}
