#pragma once
#include <array>
#include "civ5/ui/detail/notification_context_tooltip_manager_operand.hpp"
#include "civ5/ui/detail/bnw_windowed_renderer_matrix_data_loan.hpp"
#include <cstdint>
#include <memory>
#include <utility>
#include <functional>
#include <optional>
#include <variant>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class BnwSelectedWorldViewContextOwner;struct LiveSessionState;}
namespace civ5::graphics {class BnwOwnedCameraRendererWindowLease;}
namespace civ5::ui {class BnwWorldViewControlRenderPublication;class BnwWorldViewControlTree;class BnwClientSceneFrame;class SelectedBnwPreviewHost;}
namespace civ5::ui::detail {
class NotificationLayoutResizeRegistration;
class NotificationLayoutContextOrdinaryState;class NotificationContextNativeConstructorReturn;class NotificationContextLoadNameLoan;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutParentReceiver;
class NotificationContextGuiPort;
class BnwOriginalNonFontDeviceInitializer;
class BnwWindowedRendererMatrixDataLoan;
class BnwNormalGuiBatchPrefix;
class BnwNormalFontPayloadPrefix;
class BnwNormalLabelMaterialLoan;
class BnwNormalRootBatchHistory;
class BnwNormalQueueDrainPermission;
struct NotificationContextFontRegistryFields;
class BnwFontTextureRequest;
class BnwCompletedFontLoan;
class BnwLabelSystemFontSource;
class BnwLabelSystemFontBorrower;
struct BnwLabelSystemFontRegistryFields;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class NotificationContextCursorLoadPrefixReturn;
// This value has one private issuer: the actual entered Source borrower after
// SAME created client-profile HWND GetClientRect. A public viewport/rectangle
// cannot construct it. It is neither an Original pointer nor a mode-table loan.
class NotificationContextGuiClientOperand final {
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class NotificationContextGuiPort;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source_;
    const std::weak_ptr<const SelectedBnwPreviewHost> host_;
    const std::weak_ptr<const runtime::LiveSessionState> world_;
    const std::weak_ptr<BnwClientSceneFrame> scene_;
    const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window_;
    const std::uint64_t generation_,source_serial_,viewport_revision_;
    const std::uintptr_t hwnd_,occurrence_;
    const std::uint32_t process_,ui_thread_;
    const std::int32_t width_,height_;
    NotificationContextGuiClientOperand(
        std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source,
        std::weak_ptr<const SelectedBnwPreviewHost> host,
        std::weak_ptr<const runtime::LiveSessionState> world,std::weak_ptr<BnwClientSceneFrame> scene,
        std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window,
        std::uint64_t generation,std::uint64_t serial,std::uint64_t revision,
        std::uintptr_t hwnd,std::uintptr_t occurrence,std::uint32_t process,std::uint32_t thread,
        std::int32_t width,std::int32_t height)
        :source_(source),host_(host),world_(world),scene_(scene),window_(window),generation_(generation),
         source_serial_(serial),viewport_revision_(revision),hwnd_(hwnd),occurrence_(occurrence),
         process_(process),ui_thread_(thread),width_(width),height_(height){}
public:
    NotificationContextGuiClientOperand(const NotificationContextGuiClientOperand&)=delete;
    NotificationContextGuiClientOperand&operator=(const NotificationContextGuiClientOperand&)=delete;
    ~NotificationContextGuiClientOperand()=default;
};
// One canonical OWN GUI-manager namespace for the supported Civis client
// profile. Its dimensions are real ordered stores; its arithmetic pair is a
// mutable property initialized by the PE zero-fill port, not an immutable
// constant and not a receipt for Original globals2DC1B1C/05AB6DB0.
class NotificationContextGuiPort final {
    friend class BnwOriginalNonFontDeviceInitializer;
    friend class BnwNormalGuiBatchPrefix;
    friend class BnwNormalRootBatchHistory;
    friend class BnwNormalQueueDrainPermission;
    friend class BnwNormalFontPayloadPrefix;
    friend class BnwNormalLabelMaterialLoan;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class BnwWindowedRendererMatrixDataLoan;
    friend class BnwWindowedRendererSharedDescriptorIdentity;
    friend class NotificationLayoutParentReceiver;
    friend class NotificationContextCursorLoadPrefixReturn;
    friend class NotificationContextUiManagerValueLoan;
    friend class NotificationContextCursorRestoreReturn;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutResizeRegistration;
    friend class NotificationContextLoadNameLoan;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class NotificationLayoutStandaloneLabelState;
    friend class NotificationContextTooltipManagerValue;
    friend class BnwFontTextureRequest;
    friend class BnwCompletedFontLoan;
    friend class BnwLabelSystemFontSource;
    friend class BnwLabelSystemFontBorrower;
    std::shared_ptr<const BnwLabelSystemFontBorrower> begin_system_font_lookup_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwCompletedFontLoan&);
    // Genuine scoped96CC known-field prefix only. No positive Font loan.
    std::unique_ptr<BnwCompletedFontLoan> begin_font_lookup_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::optional<std::string_view>);
    // Real known5A4 field read only. Called in the SAME canonical parser
    // scope after its actual localized54/OR4000 prefix; no input/Box grant.
    void refresh_tooltip_if_hovered_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t actual_control_id);
    void dispatch_resize_event_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::int32_t);
    struct CursorChange {std::int32_t previous,current;std::uint64_t before_revision,after_revision;};
    CursorChange set_cursor_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::int32_t);
    std::shared_ptr<const NotificationContextTooltipManagerOperand> capture_tooltip_manager_operand_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextGuiPort(std::shared_ptr<Impl>);
    static std::shared_ptr<NotificationContextGuiPort> capture_owned(
        const runtime::BnwWorldViewSourceOperationLoan&);
    std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> capture_windowed_renderer_matrix_data_owned(
        const runtime::BnwWorldViewSourceOperationLoan&);
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::array<std::int32_t,2> dimensions_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::array<float,2> arithmetic_pair_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void refresh_language_980a_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    std::uint8_t label_language_980a_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void set_arithmetic_pair_owned(const runtime::BnwWorldViewSourceOperationLoan&,const std::array<float,2>&);
public:
    ~NotificationContextGuiPort();
    NotificationContextGuiPort(const NotificationContextGuiPort&)=delete;
    NotificationContextGuiPort&operator=(const NotificationContextGuiPort&)=delete;
};
// Issued only AFTER actual physical touch-bit and canonical cursor return.
class NotificationContextCursorLoadPrefixReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextCursorLoadPrefixReturn(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
public:
    ~NotificationContextCursorLoadPrefixReturn()=default;
    NotificationContextCursorLoadPrefixReturn(const NotificationContextCursorLoadPrefixReturn&)=delete;
    NotificationContextCursorLoadPrefixReturn& operator=(const NotificationContextCursorLoadPrefixReturn&)=delete;
};
// Prefix only: real parsed flag10000000 decision, and for nondeferred loads
// genuine GUI644/648→CVTSI2SS30/34→OR300000. No VM/environment/initialLua return.
class NotificationContextSlot10LoadOrigin final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextSlot10LoadOrigin(std::shared_ptr<const Impl> value):impl_(std::move(value)){}
public:
    ~NotificationContextSlot10LoadOrigin()=default;
    NotificationContextSlot10LoadOrigin(const NotificationContextSlot10LoadOrigin&)=delete;
    NotificationContextSlot10LoadOrigin&operator=(const NotificationContextSlot10LoadOrigin&)=delete;
};
}
