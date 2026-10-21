#pragma once
#include <cstdint>
#include <memory>
namespace civ5::content { struct XmlNode; }
namespace civ5::runtime { class BnwWorldViewSourceOperationLoan; }
namespace civ5::ui { class BnwWorldViewControlTree; }
namespace civ5::ui::detail {
class NotificationContextRootReadChildrenLoan;
class NotificationLayoutOrdinaryReceiver;
class NotificationLayoutContextRealmRef;
class NotificationLayoutNativeChildControlsReturn;
// A real ordinary 529520/v44 return. Neither a tag nor a cold node issues it.
class NotificationContextRootOrdinaryFactoryReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutNativeChildControlsReturn;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextRootOrdinaryFactoryReturn(std::shared_ptr<const Impl>);
    enum class Subtype { image, box, button, label, stack, grid };
    class Guard final {
        friend class NotificationContextRootOrdinaryFactoryReturn;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationLayoutNativeChildControlsReturn;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_same_root_read_without_callbacks(const NotificationContextRootReadChildrenLoan&)const;
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_receiver_without_callbacks()const;
        Subtype actual_subtype_without_callbacks()const;
        std::uint64_t actual_control_id_without_callbacks()const;
        const content::XmlNode& actual_declaration_without_callbacks()const;
        std::shared_ptr<const NotificationLayoutContextRealmRef> actual_context_realm_without_callbacks()const;
    };
    Guard capture_guard_for_controls_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextRootReadChildrenLoan&)const;
public:
    ~NotificationContextRootOrdinaryFactoryReturn();
    NotificationContextRootOrdinaryFactoryReturn(const NotificationContextRootOrdinaryFactoryReturn&)=delete;
    NotificationContextRootOrdinaryFactoryReturn&operator=(const NotificationContextRootOrdinaryFactoryReturn&)=delete;
};
}
