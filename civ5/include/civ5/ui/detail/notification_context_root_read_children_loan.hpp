#pragma once
#include <memory>
namespace civ5::content {struct XmlNode;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextRootChildFactoryReturn;
class NotificationContextRootOrdinaryFactoryReturn;
class NotificationLayoutNativeChildControlsReturn;
class NotificationLayoutOrdinaryReceiver;
class NotificationLayoutContextRealmRef;
class NotificationContextRootReadCompletionSlot;
// SEALED lexical Root7A9C30 child cursor, NOT ordinary/id0 Runtime or a518 token.
// Source23 issues only current parsed XML84 cursor/parentTHIS DATA AFTER real
// setup/5D/8F85A0. Childfactory, pending completion-slot and wholeRead return
// remain required separate producers. Retention cannot renew the Read frame.
class NotificationContextRootReadChildrenLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutNativeChildControlsReturn;
    friend class NotificationContextRootChildFactoryReturn;
    friend class NotificationContextRootOrdinaryFactoryReturn;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootReadChildrenLoan(std::shared_ptr<Impl>);
    class Guard final {
        friend class NotificationContextRootReadChildrenLoan;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationLayoutNativeChildControlsReturn;
        friend class NotificationContextRootChildFactoryReturn;
        friend class NotificationContextRootOrdinaryFactoryReturn;
        const std::shared_ptr<Impl> impl_;
        explicit Guard(std::shared_ptr<Impl>);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_child_factory_return_identity_without_callbacks(const NotificationContextRootChildFactoryReturn&)const;
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_root_receiver_without_callbacks()const;
        std::shared_ptr<const NotificationLayoutContextRealmRef> actual_context_realm_without_callbacks()const;
        const content::XmlNode& actual_declaration_without_callbacks()const;
        std::shared_ptr<const NotificationContextRootReadCompletionSlot> capture_completion_slot_without_callbacks()const;
    };
    Guard capture_guard_without_callbacks()const;
public:
    ~NotificationContextRootReadChildrenLoan();
    NotificationContextRootReadChildrenLoan(const NotificationContextRootReadChildrenLoan&)=delete;
    NotificationContextRootReadChildrenLoan& operator=(const NotificationContextRootReadChildrenLoan&)=delete;
};
// Actual529520 result after selected concretev44/commonreturn and real lexical
// Binding DTOR. Existing CommonChildFactoryReturn alone does not mint this.
class NotificationContextRootChildFactoryReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextRootReadChildrenLoan;
    friend class NotificationLayoutNativeChildControlsReturn;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootChildFactoryReturn(std::shared_ptr<Impl>);
public:
    ~NotificationContextRootChildFactoryReturn();
    NotificationContextRootChildFactoryReturn(const NotificationContextRootChildFactoryReturn&)=delete;
    NotificationContextRootChildFactoryReturn& operator=(const NotificationContextRootChildFactoryReturn&)=delete;
};
}
