#pragma once
#include "civ5/ui/detail/bnw_label_system_font_language_data.hpp"
#include "civ5/runtime/detail/context_loader_resource_operand.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/content/manifest.hpp"
#include <thread>
#include <map>
#include <array>
#include <utility>
namespace civ5::assets {struct CivisLoadedResource;class CivisResourceBankOwner;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwEngineUiTurnOwner;}
namespace civ5::ui::detail {class NotificationLayoutRuntimeDimensions;class NotificationLayoutResizeRegistration;class NotificationContextRootReadB4Guard;class NotificationContextNativeLoadNameInputLoan;class NotificationContextRootSmallUiPolicyLoan;class NotificationContextLuaControlsNamedRetirement;}
namespace civ5::runtime {
namespace detail {class BnwWorldViewCanvasXmlMetadata;class BnwFontResourceLoan;}
class BnwContextLuaProfileConfigLoan;
class BnwSelectedStockLuaBootstrap;
class BnwCivisOwnedStockSourceBundle;
class BnwWorldViewSourceOperationLoan;
class BnwWorldViewNativeContextIssuer;
class BnwUnitFlagHealthInstanceLoan;
// Retained source metadata is separate from the short current native operation.
// Only the actual native issuer can lend the checked active operation scope.
class BnwSelectedWorldViewContextOwner final : public ScriptUiContextSourceOwner {
 public:
    ~BnwSelectedWorldViewContextOwner();
    BnwSelectedWorldViewContextOwner(const BnwSelectedWorldViewContextOwner&)=delete;
    BnwSelectedWorldViewContextOwner& operator=(const BnwSelectedWorldViewContextOwner&)=delete;
    void require_metadata_current()const;
    void require_sources_current()const;
    const BnwWorldViewSourceOperationLoan& current_operation_loan()const;
    void require_current(std::string_view,const ScriptUiChildSource*)const override;
    std::shared_ptr<const ScriptUiChildSource> resolve_child(
        std::string_view,const ScriptUiChildSource*,std::string_view)override;
    void require_native_activation()const;
 private:
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwWorldViewSourceOperationLoan;
    friend class detail::BnwFontResourceLoan;
    friend class BnwWorldViewNativeContextIssuer;
    friend class LuaScriptHost;
    friend class BnwContextLuaProfileConfigLoan;
    std::shared_ptr<const BnwContextLuaProfileConfigLoan> selected_lua_profile_config_owned(const BnwWorldViewSourceOperationLoan&)const;
    friend class BnwUnitFlagHealthInstanceLoan;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::detail::NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::detail::NotificationLayoutResizeRegistration;
    friend class ::civ5::ui::detail::NotificationContextRootReadB4Guard;
    friend class ::civ5::ui::detail::NotificationContextNativeLoadNameInputLoan;
    friend class BnwContextLuaIncludeCollectionLoan;
    friend class ::civ5::ui::detail::NotificationContextRootSmallUiPolicyLoan;
    friend class ::civ5::ui::detail::NotificationContextLuaControlsNamedRetirement;
    friend class BnwSelectedUserOptionsOwner;
    struct State;class Child;
    const std::shared_ptr<State> state_;
    std::vector<std::shared_ptr<const Child>> children_;
    explicit BnwSelectedWorldViewContextOwner(std::shared_ptr<State>);
    // Called synchronously inside the SAME retained Bootstrap operation.
    // The mount table is borrowed only during capture, never stored/copy-minted.
    static std::shared_ptr<BnwSelectedWorldViewContextOwner> capture_owned(
        const std::filesystem::path&,const assets::ResourceMountTable&,
        const BnwStockLuaAttemptLease&,std::shared_ptr<const BnwCivisOwnedStockSourceBundle> = {});
    std::uint64_t lend_operation_scope(const BnwWorldViewSourceOperationLoan&);
    void retire_operation_scope(const BnwWorldViewSourceOperationLoan&,std::uint64_t)noexcept;
    static void retire_operation_storage(const std::shared_ptr<State>&,const BnwWorldViewSourceOperationLoan&,std::uint64_t)noexcept;
    void require_operation_origin_without_callbacks(const BnwWorldViewSourceOperationLoan&,std::uint64_t)const;
    std::uint64_t capture_active_operation_scope_serial_without_callbacks(const BnwWorldViewSourceOperationLoan*)const;
    std::weak_ptr<BnwSelectedWorldViewContextOwner> canonical_owner_without_callbacks()const;
    void seal_initial_native_generation_without_callbacks(const std::weak_ptr<const LiveSessionState>&,std::uint64_t)const;
    void require_world_and_generation_without_callbacks(const std::weak_ptr<const LiveSessionState>&,std::uint64_t)const;
    const BnwStockLuaAttemptLease& stored_source_lineage_without_callbacks()const;
    void require_source_member_without_callbacks(const ScriptUiChildSource&)const;
    // Storage retention alone is not admission to a new operation. The
    // authentic membership keeps its independent VM/Tree/Stock witnesses.
    static std::shared_ptr<State> retain_health_source_storage_without_callbacks(
        const BnwSelectedWorldViewContextOwner&,
        const std::weak_ptr<const LiveSessionState>&,std::uint64_t);
    // nullptr/0 is prebind and requires no active Source. A bound operation
    // checks the ONE existing metadata slot, without calling that Source.
    static void require_health_source_storage_without_callbacks(
        const std::shared_ptr<State>&,const BnwSelectedWorldViewContextOwner&,
        const std::weak_ptr<const LiveSessionState>&,std::uint64_t,
        const BnwWorldViewSourceOperationLoan*,std::uint64_t);
    // Retained declaration metadata only; the active source occurrence guards
    // its capture/read. No native active-child or initialization grant exists.
    std::shared_ptr<const detail::BnwWorldViewCanvasXmlMetadata> canvas_xml_metadata_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    std::unique_ptr<detail::BnwFontResourceLoan> font_resource_loan_owned(
        const BnwWorldViewSourceOperationLoan&,std::string)const;
    detail::ContextLoaderResourceOperand context_loader_resource_operand_owned(
        const BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const ScriptUiChildSource>&)const;
    void publish_selected_client_context_slot1_owned(const BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&);
    std::shared_ptr<ui::BnwWorldViewControlTree> selected_client_context_slot1_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    // Closed initial-load supplier; metadata cannot mint a Style query borrower.
    std::array<assets::CivisLoadedResource,3> selected_style_resources_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    std::map<std::string,content::XmlNode,std::less<>> selected_client_styles_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const assets::CivisResourceBankOwner> selected_provider_startup_bank_owned(const BnwWorldViewSourceOperationLoan&)const;
    std::array<assets::CivisLoadedResource,2> color_atlas_resources_owned(const BnwWorldViewSourceOperationLoan&)const;
    std::array<float,4> image_color_value_owned(const BnwWorldViewSourceOperationLoan&,std::string_view)const;
    static ui::detail::BnwLabelSystemFontLanguageData system_font_language_data_from_owned_weak(
        const std::weak_ptr<BnwSelectedWorldViewContextOwner>&,const BnwWorldViewSourceOperationLoan&);
    void bind_tooltip_localization_owned(const std::weak_ptr<ui::BnwEngineUiTurnOwner>&,const BnwStockLuaAttemptLease&);
    std::string tooltip_localized_text_owned(const BnwWorldViewSourceOperationLoan&,std::string_view)const;
    std::string action_info_localized_text_owned(const BnwWorldViewSourceOperationLoan&,std::string_view)const;
    std::pair<std::string,std::uint8_t> selected_language_extended_font_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    const content::XmlNode& authored_layout(std::string_view)const;
    std::shared_ptr<const ScriptUiChildSource> prepared_source(std::string_view)const;
    std::string selected_context_declared_parent_owned(std::string_view)const;
    std::vector<std::shared_ptr<ui::BnwWorldViewControlTree>> selected_client_retained_context_trees_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    std::vector<std::int64_t> native_mouse_callback_events()const;
};
}
