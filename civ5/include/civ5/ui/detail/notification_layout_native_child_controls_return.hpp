#pragma once
#include <memory>
#include <string_view>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class ScriptUiChildSource;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutOrdinaryReceiver;
class NotificationLayoutContextOrdinaryState;
class NotificationContextLuaControlsNamedReference;
class NotificationContextChildControlsExportReturn;
class NotificationContextRootReadChildrenLoan;
class NotificationContextRootChildFactoryReturn;
class NotificationContextRootOrdinaryFactoryReturn;
class NotificationContextRootOrdinaryControlsExportReturn;
// Required genuine childv64(ControlsRef,true) return AFTER subtypev7C,
// duplicate/map/stack effects and the actual78 traversal. PrefixName1 supplies
// none of these constructors. Root attachment may not synthesize this return.
class NotificationLayoutNativeChildControlsReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextChildControlsExportReturn;
    friend class NotificationContextRootOrdinaryControlsExportReturn;
    enum class Origin {distinct_context,ordinary_control};
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationLayoutNativeChildControlsReturn(std::shared_ptr<const Impl>);
    void require_export_return_origin_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextChildControlsExportReturn&)const;
    void require_export_return_operands_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextChildControlsExportReturn&,const NotificationContextLuaControlsNamedReference&,
        const NotificationLayoutContextOrdinaryState&,const ::civ5::ui::BnwWorldViewControlTree&,
        const runtime::ScriptUiChildSource&,const NotificationLayoutOrdinaryReceiver&,std::string_view)const;
    void require_ordinary_export_return_origin_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextRootOrdinaryControlsExportReturn&)const;
    void require_ordinary_export_return_operands_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextRootOrdinaryControlsExportReturn&,const NotificationContextLuaControlsNamedReference&,
        const void*,const ::civ5::ui::BnwWorldViewControlTree&,const NotificationLayoutOrdinaryReceiver&,
        std::string_view,std::string_view)const;
    class Guard final {
        friend class NotificationLayoutNativeChildControlsReturn;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_same_root_read_without_callbacks(const NotificationContextRootReadChildrenLoan&)const;
        const NotificationContextRootChildFactoryReturn& actual_factory_return_without_callbacks()const;
        const NotificationContextRootOrdinaryFactoryReturn& actual_ordinary_factory_return_without_callbacks()const;
        Origin actual_origin_without_callbacks()const;
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_receiver_without_callbacks()const;
    };
    Guard capture_guard_for_attach_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationContextRootReadChildrenLoan&)const;
public:
    ~NotificationLayoutNativeChildControlsReturn();
    NotificationLayoutNativeChildControlsReturn(const NotificationLayoutNativeChildControlsReturn&)=delete;
    NotificationLayoutNativeChildControlsReturn&operator=(const NotificationLayoutNativeChildControlsReturn&)=delete;
};
}
