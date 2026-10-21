namespace civ5::ui {class BnwWorldViewGridToWorldLoan;class BnwClientSceneFrame;class SelectedBnwPreviewHost;class CivisInputPolicyOwner;class BnwWorldViewControlTree;struct CivisInputWindowOrigin;}
#pragma once
#include "civ5/runtime/bnw_context_lua_include_collection_loan.hpp"
#include "civ5/ui/detail/notification_context_root_small_ui_policy_loan.hpp"
#include "civ5/ui/detail/bnw_label_system_font_request.hpp"
#include "civ5/graphics/detail/bnw_label_system_font_atlas_metadata.hpp"
#include "civ5/ui/detail/bnw_label_system_font_language_data.hpp"
#include "civ5/runtime/detail/context_loader_resource_operand.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/ui/detail/notification_context_gui_port.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/graphics/bnw_clear_hex_highlight_owner.hpp"
#include <array>
#include <memory>
#include <vector>
#include <string_view>
namespace civ5::assets {struct CivisLoadedResource;}
namespace civ5::ui {class BnwWorldViewControlRenderPublication;class BnwWorldViewWindowMouseCacheLoan;class UiScreenGlRenderer;}
namespace civ5::ui::detail {class BnwOriginalNonFontDeviceInitializer;class BnwOriginalResourceSamplerDescriptorLoan;class BnwOriginalVertexDeclarationSetupLoan;class NotificationLayoutParentReceiver;class NotificationLayoutRuntimeDimensions;class NotificationLayoutResizeRegistration;class BnwLabelProcessModuleOwner;class NotificationLayoutEmbeddedLabelState;class NotificationLayoutStandaloneLabelState;
class BnwPositionedLabelRunLoan;class BnwStandalonePositionedLabelRunLoan;class BnwWindowedRendererMatrixDataLoan;class BnwNormalLabelPaintInvocation;class BnwNormalGuiBatchPrefix;class BnwNormalLabelMaterialLoan;class BnwNormalRootBatchHistory;class BnwLabelActualPaintCallFrame;class BnwNormalRootPaintCallFrame;class BnwNormalButtonSubobject118Backing;}
namespace civ5::ui::detail {class NotificationLayoutContextRealmRef;class NotificationLayoutContextOrdinaryState;class NotificationContextNativeConstructorPrefix;class NotificationContextNativeConstructorReturn;}
#include <utility>
namespace civ5::graphics {class BnwUnitSetupGraphicalQueue;}
namespace civ5::graphics::detail {class BnwFontTextureObservation;}
namespace civ5::ui::detail {class BnwFontTextureRequest;class BnwCompletedFontLoan;}
namespace civ5::content {class BnwGameInfoActionsOwner;class BnwNotificationEntriesSourceOwner;class RulesDatabase;}
namespace civ5::runtime {
namespace detail {class BnwFontResourceLoan;}
class BnwContextLuaProfileConfigLoan;
class BnwCivisApplicationStateOwner;
class LuaScriptHost;class ScriptQuery;
class ScriptUiChildSource;
class BnwSelectedWorldViewContextOwner;
class BnwSelectedStockLuaBootstrap;
class BnwWorldViewNativeContextIssuer;
class BnwUnitFlagHealthInstanceLoan;
enum class BnwUnitFlagHealthCommand;
// A borrowed fresh operation, not a retained activation bit or old serial.
// The actual native context lender encloses every use of this loan in real
// GUI/window loans and the currently active SAME Bootstrap source operation.
class BnwWorldViewSourceOperationLoan final {
 public:
    BnwWorldViewSourceOperationLoan(const BnwWorldViewSourceOperationLoan&)=delete;
    BnwWorldViewSourceOperationLoan& operator=(const BnwWorldViewSourceOperationLoan&)=delete;
    BnwWorldViewSourceOperationLoan(BnwWorldViewSourceOperationLoan&&)=delete;
    BnwWorldViewSourceOperationLoan& operator=(BnwWorldViewSourceOperationLoan&&)=delete;
    void require_current()const;
    std::uint64_t operation_serial()const;
    void require_operation(std::uint64_t)const;
    void require_same_source(const ScriptUiChildSource&)const;
    void require_same_vm(const LuaScriptHost&)const;
 private:
    friend class LuaScriptHost;
    std::shared_ptr<ScriptQuery> context_modding_activated_entrypoint_query_owned(std::string_view)const;
    void require_modding_activated_entrypoint_query_owned(const std::shared_ptr<ScriptQuery>&)const;
    std::shared_ptr<const content::BnwGameInfoActionsOwner> context_gameinfo_actions_owned()const;
    std::shared_ptr<const content::BnwNotificationEntriesSourceOwner> context_notification_entries_owned(
        const content::RulesDatabase** actual_database=nullptr)const;
    friend class BnwContextLuaProfileConfigLoan;
    std::shared_ptr<const BnwContextLuaProfileConfigLoan> context_lua_profile_config_owned()const;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::detail::BnwOriginalNonFontDeviceInitializer;
    friend class ::civ5::ui::detail::BnwOriginalResourceSamplerDescriptorLoan;
    friend class ::civ5::ui::detail::BnwOriginalVertexDeclarationSetupLoan;
    std::vector<std::shared_ptr<ui::BnwWorldViewControlTree>> selected_client_retained_context_trees_owned()const;
    friend class ::civ5::graphics::BnwUnitSetupGraphicalQueue;
    friend class BnwWorldViewScriptUiAdapter;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class BnwContextLuaIncludeCollectionLoan;
    std::shared_ptr<const BnwContextLuaIncludeCollectionLoan> context_lua_include_collection_owned()const;
    friend class BnwSelectedUserOptionsOwner;
    friend class ::civ5::ui::detail::NotificationContextRootSmallUiPolicyLoan;
    std::unique_ptr<const ui::detail::NotificationContextRootSmallUiPolicyLoan> context_small_ui_policy_owned()const;
    bool context_options_small_ui_assets_owned()const;
    bool context_options_minimap_game_setting_owned(std::string_view)const;
    friend class ::civ5::ui::detail::NotificationContextGuiPort;
    friend class detail::BnwFontResourceLoan;
    std::unique_ptr<detail::BnwFontResourceLoan> font_resource_loan_owned(std::string)const;
    std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan> context_xml_physical_source_owned(const assets::AssetKey&)const;
    std::shared_ptr<const assets::detail::CivisLoadedBytesPhysicalPathLoan> context_lua_physical_source_owned(const assets::AssetKey&)const;
    std::array<assets::CivisLoadedResource,3> selected_style_resources_owned()const;
    std::shared_ptr<const ::civ5::graphics::detail::BnwLabelSystemFontAtlasMetadata> system_font_atlas_query_owned(
        const ::civ5::ui::detail::BnwLabelSystemFontRequest&)const;
    // Graphics owns real Source/Grid receiver; no body/stub provided here.
    std::unique_ptr<::civ5::graphics::detail::BnwFontTextureObservation> font_texture_query_owned(
        const detail::BnwFontResourceLoan&,const ::civ5::ui::detail::BnwFontTextureRequest&)const;
    friend class ::civ5::ui::detail::BnwLabelProcessModuleOwner;
    friend class ::civ5::ui::detail::BnwCompletedFontLoan;
    std::shared_ptr<ui::detail::BnwLabelProcessModuleOwner> label_module_owned()const;
    ui::detail::NotificationContextGuiClientOperand context_gui_client_operand_owned()const;
    ui::detail::BnwLabelSystemFontLanguageData system_font_language_data_owned()const;
    std::shared_ptr<const assets::CivisResourceBankOwner> selected_provider_startup_bank_owned()const;
    friend class ::civ5::graphics::BnwClearHexHighlightOwner;
    std::weak_ptr<BnwSelectedWorldViewContextOwner> clear_hex_highlight_source_origin_without_callbacks_owned()const;
    std::shared_ptr<graphics::BnwClearHexHighlightOwner> clear_hex_highlight_owner_owned()const;
    std::array<assets::CivisLoadedResource,2> color_atlas_resources_owned()const;
    std::pair<std::string,std::uint8_t> selected_language_extended_font_owned()const;
    bool context_touch_bit8_owned()const;
    detail::ContextLoaderResourceOperand context_loader_resource_operand_owned(
        const std::shared_ptr<const ScriptUiChildSource>&)const;
    const LuaScriptHost& context_lua_host_owned()const;
    std::shared_ptr<BnwCivisApplicationStateOwner> context_application_state_owned()const;
    void require_context_gui_port_identity_without_callbacks(
        const std::weak_ptr<BnwSelectedWorldViewContextOwner>&,const std::weak_ptr<const ui::SelectedBnwPreviewHost>&,
        const std::weak_ptr<const LiveSessionState>&,const std::weak_ptr<ui::BnwClientSceneFrame>&,
        const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,std::uint64_t,
        std::uintptr_t,std::uintptr_t,std::uint64_t,std::uint32_t,std::uint32_t)const;
    enum class Purpose {render,unitflag_health};
    // Pure refusal fence: lifecycle lending cannot renew render authority.
    void require_render_operation_owned()const;
    void require_same_render_loans_without_callbacks(const ui::BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&)const;
    bool unitflag_health_operation_without_callbacks()const;
    void require_unitflag_health_control_owned(const ui::BnwWorldViewControlTree::Control&,
        BnwUnitFlagHealthCommand)const;
    friend class ::civ5::ui::detail::NotificationLayoutParentReceiver;
    friend class ::civ5::ui::detail::NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::detail::BnwNormalButtonSubobject118Backing;
    friend class ::civ5::ui::detail::NotificationLayoutResizeRegistration;
    friend class ::civ5::ui::detail::NotificationLayoutContextRealmRef;
    friend class ::civ5::ui::detail::NotificationLayoutContextOrdinaryState;
    friend class ::civ5::ui::detail::NotificationContextNativeConstructorPrefix;
    friend class ::civ5::ui::detail::NotificationContextNativeConstructorReturn;
    friend class ::civ5::ui::detail::NotificationContextNativeLoadNameInputLoan;
    friend class ::civ5::ui::detail::NotificationContextLoadNameLoan;
    friend class ::civ5::ui::detail::NotificationLayoutEmbeddedLabelState;
    friend class ::civ5::ui::detail::NotificationLayoutStandaloneLabelState;
    friend class ::civ5::ui::detail::BnwPositionedLabelRunLoan;
    friend class ::civ5::ui::detail::BnwStandalonePositionedLabelRunLoan;
    friend class ::civ5::ui::detail::BnwWindowedRendererMatrixDataLoan;
    friend class ::civ5::ui::detail::BnwNormalLabelPaintInvocation;
    friend class ::civ5::ui::detail::BnwNormalGuiBatchPrefix;
    friend class ::civ5::ui::detail::BnwNormalLabelMaterialLoan;
    friend class ::civ5::ui::detail::BnwNormalRootBatchHistory;
    friend class ::civ5::ui::detail::BnwLabelActualPaintCallFrame;
    friend class ::civ5::ui::detail::BnwNormalRootPaintCallFrame;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    std::shared_ptr<const ui::BnwWorldViewWindowMouseCacheLoan> mouse_cache_owned()const;
    std::pair<std::int64_t,std::int64_t> screen_size_owned()const;
    friend class ::civ5::ui::CivisInputPolicyOwner;
    std::weak_ptr<const ui::SelectedBnwPreviewHost> input_policy_host_origin_without_callbacks()const;
    ui::CivisInputWindowOrigin input_policy_window_origin_owned()const;
    void require_input_policy_window_owned(const ui::CivisInputWindowOrigin&)const;
    friend class ::civ5::ui::BnwWorldViewGridToWorldLoan;
    void require_grid_scene_origin_without_callbacks(const std::weak_ptr<ui::BnwClientSceneFrame>&)const;
    std::array<double,3> grid_to_world_owned(std::int32_t,std::int32_t)const;
    friend class BnwSpClientUpdateLoan;
    friend class BnwSelectedStockLuaSourceOwner;
    BnwStockLuaAttemptLease sp_client_source_owned()const;
    void require_sp_client_bootstrap_owned(const BnwSelectedStockLuaBootstrap&,
        const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,
        const std::shared_ptr<const LiveSessionState>&)const;
    friend class BnwWorldViewNativeContextIssuer;
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwSelectedWorldViewContextOwner;
    struct Impl;
    const std::unique_ptr<Impl> impl_;
    BnwWorldViewSourceOperationLoan(BnwSelectedWorldViewContextOwner&,
        const BnwStockLuaAttemptLease&,const BnwSelectedStockLuaBootstrap&,
        const LuaScriptHost&,const ui::BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,const ui::BnwWorldViewGridToWorldLoan&);
    BnwWorldViewSourceOperationLoan(BnwSelectedWorldViewContextOwner&,
        const BnwStockLuaAttemptLease&,const BnwSelectedStockLuaBootstrap&,
        const LuaScriptHost&,const ui::BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan&,
        const BnwUnitFlagHealthInstanceLoan&);
    void poison_owned()const noexcept;
    // Diagnostic only. Called exclusively while handling a caught exception.
    void diagnose_caught_primary_failure_owned()const noexcept;
    ~BnwWorldViewSourceOperationLoan();
    void require_origin_without_callbacks()const;
};
}
