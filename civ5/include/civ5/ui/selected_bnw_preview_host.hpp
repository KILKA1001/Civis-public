#pragma once

#include "civ5/content/saved_rules.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_notification_popup_owner.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <atomic>
#include <thread>

namespace civ5::graphics {class BnwInitialUnitShouldDimRendererLoan;}
namespace civ5::assets { class CivisResourceBankOwner; }
namespace civ5::content { class BnwGlobalsCacheSourceOwner; class BnwNotificationEntriesSourceOwner;class BnwGameInfoActionsOwner; }
namespace civ5::runtime { class BnwRetainedTurnCursor;class BnwLiveGameContext;class BnwNetMessageDebugManagerOwner;class BnwProcessedAutoMovesImportOwner; }
namespace civ5::runtime { class BnwSelectedStockLuaSourceOwner;class BnwImportedPregameLocaleOwner;class BnwWorldViewSourceOperationLoan; }
namespace civ5::runtime { class BnwResearchQueueSaveOwner; class BnwCivisApplicationStateOwner; }
namespace civ5::runtime { class BnwMajorPlayerTraitsOwner; }
namespace civ5::runtime { class BnwNativeUnitAllocatorCore;class BnwCityFoodGrowthPending;class BnwFoodPopulationGrowthCheckpoint; }
namespace civ5::runtime { class BnwMajorTeamHasTechPending; }


namespace civ5::runtime { class BnwUpdateMovesImportOwner; struct BnwTurnBasePartialExport; }

namespace civ5::ui {class UiScreenGlRenderer;}

namespace civ5::ui {
namespace detail {
class BnwNativeGlobalColorRegistryBank;
class BnwNativeCompletedResourceBankCapability;
class BnwNativeCompletedParentObservation;
class BnwNativeCompletedColorQuery;
class BnwNativeCompletedSelectedRead;
class BnwNativeCompletedUnitArtInputs;
class CivisSelectedBnwRulesPreparation;
}
class BnwNativePinnedInputEpoch;

class BnwTopPanelPreviewBridge;
class BnwOwnedCameraUiSourceInput;
class CivisBankAudioSourceOwner;
class BnwWorldViewFontSourceScope;
class BnwUnitPanelPreviewBridge;
class BnwNativePackageDiscoveryOwner;
class BnwNativeUiColorOwner;
struct SelectedBnwBootstrapResult;

// The caller identifies the selected source GameCore and supplies its decoded
// wire dimensions. A save header alone cannot prove these dimensions. This
// contract must come from a verified source profile, not host allocation caps.
enum class BnwSelectedImportPurpose {data_only_preview,imported_single_player_game};
struct BnwSourceDecodeContract {
    saves::SavedSessionDecodeContext context;
    std::filesystem::path gamecore_executable;
    std::string gamecore_sha256;
    std::shared_ptr<const runtime::BnwImportedPregameLocaleOwner> actual_early_pregame_locale;
    // Routing intent only. The full path still proves actual early inputs.
    BnwSelectedImportPurpose import_purpose=BnwSelectedImportPurpose::data_only_preview;
};

enum class SelectedBnwPreviewStage {
    selected_save, content_catalog, selected_mounts, activation, postprocess, source_contract,
    rule_cache, source_decode, live_restore, active_player
};
struct SelectedBnwPreviewBlocker {
    SelectedBnwPreviewStage stage;
    std::string reason;
};
struct SelectedBnwPreviewResult;

// Keep this object alive through the preview run. Its database, caches and
// LiveSession have stable addresses. Gameplay services are deliberately not
// invented by this read-only import factory.
class SelectedBnwPreviewHost {
  public:
    ~SelectedBnwPreviewHost();
    SelectedBnwPreviewHost(const SelectedBnwPreviewHost &) = delete;
    SelectedBnwPreviewHost &operator=(const SelectedBnwPreviewHost &) = delete;

    runtime::LiveSession &session() noexcept { return *session_; }
    const runtime::LiveSession &session() const noexcept { return *session_; }
    const content::RulesDatabase &database() const noexcept { return *database_; }
    std::shared_ptr<const content::BnwNotificationEntriesSourceOwner> notification_entries_owned() const;
    const content::TechnologyRules &technologies() const noexcept { return technologies_; }
    const content::InfoTypeCatalog &types() const noexcept { return types_; }
    const content::GlobalDefineCache &defines() const noexcept { return defines_; }
    const runtime::ProductionProgressRules &production_rules() const noexcept { return production_; }
    std::int64_t active_player_id() const noexcept { return active_player_id_; }
    const runtime::SessionCheckpointIdentity &identity() const noexcept { return identity_; }
    LiveBnwPreviewInput preview_input();
    // Partial import-clock checkpoint, never a full-turn continuation grant.
    runtime::BnwTurnBasePartialExport export_import_clock_checkpoint() const;
    runtime::PregameRoster &pregame_roster() noexcept;
    const runtime::PregameRoster &pregame_roster() const noexcept;
    runtime::ScriptPregameProvider pregame_services();
    runtime::BnwGameActivationOwner &activation_owner() noexcept;
    const runtime::BnwGameActivationOwner &activation_owner() const noexcept;
    // Read-only proof check. Only the source-ordered bootstrap can install the
    // private early package/BASE-to-final lineage; no public bind exists.
    void require_same_early_package(const BnwNativePackageDiscoveryOwner&) const;
    // Own Civis resource lineage, disjoint from native completed-bank receipts.
    void require_same_civis_resources(const assets::CivisResourceBankOwner&) const;
    // Routing discriminator only; grants no font/audio/native readiness.
    bool uses_civis_resource_bank() const;

  private:
    friend struct SelectedBnwPreviewResult;
    friend SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_civis_resources(
        const std::shared_ptr<assets::CivisResourceBankOwner>&, std::int64_t);
    friend class UiScreenGlRenderer;
    friend class BnwClientFrameOwner;
    friend class BnwAudioCompletedResourceOwner;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    friend class runtime::BnwCivisApplicationStateOwner;
    mutable std::weak_ptr<runtime::BnwCivisApplicationStateOwner> bnw_civis_application_state_;
    friend class runtime::BnwWorldViewSourceOperationLoan;
    friend class runtime::BnwResearchQueueSaveOwner;
    friend class runtime::BnwMajorPlayerTraitsOwner;
    friend class runtime::BnwNativeUnitAllocatorCore;
    friend class runtime::BnwCityFoodGrowthPending;friend class runtime::BnwFoodPopulationGrowthCheckpoint;
    std::weak_ptr<const SelectedBnwPreviewHost> bnw_food_canonical_self_;
    void restore_food_growth_before_publication(const std::shared_ptr<SelectedBnwPreviewHost>&);
    void require_restored_food_growth_current()const;
    void require_restored_food_growth_publication_current()const;
    friend class runtime::LiveSession;
    friend class SelectedBnwReadOnlyPreview;
    // Engaged expiry is sticky: it never authorizes a clean sibling Core.
    std::optional<std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore>> bnw_restored_mission_core_;
    std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>
        restore_unit_mission_before_publication(std::shared_ptr<const SelectedBnwPreviewHost>);
    void require_restored_unit_mission_current_without_callbacks() const;
    void require_restored_unit_mission_checkpoint_current_without_callbacks() const;
    friend class runtime::BnwMajorTeamHasTechPending;
    friend class BnwNativeUiColorOwner;
    friend class runtime::BnwRetainedTurnCursor;
    friend class runtime::BnwLiveGameContext;
    friend class runtime::BnwNetMessageDebugManagerOwner;
    friend class ::civ5::graphics::BnwInitialUnitShouldDimRendererLoan;
    mutable std::shared_ptr<runtime::BnwLiveGameContext> bnw_live_game_context_;
    friend class detail::BnwNativeGlobalColorRegistryBank;
    friend class BnwOwnedCameraUiSourceInput;
    friend class CivisBankAudioSourceOwner;
    friend class BnwWorldViewTitleFontIssuer;
    friend class BnwWorldViewFontSourceScope;
    friend class BnwNativePackageDiscoveryOwner;
    friend SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_completed_resources(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>,
        std::shared_ptr<const detail::BnwNativeCompletedColorQuery>,
        std::shared_ptr<const detail::BnwNativeCompletedSelectedRead>,std::int64_t);
    friend SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_early_package(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>, std::int64_t);
    friend SelectedBnwPreviewResult make_selected_bnw_preview_host(
        const std::filesystem::path &, const content::RulesDatabase &,
        const content::SavedRulesActivationReport &,
        const content::GameplayPostprocessReport &,
        const BnwSourceDecodeContract &, std::int64_t);
    friend SelectedBnwPreviewResult make_selected_bnw_preview_host(
        const std::filesystem::path &, const content::RulesDatabase &,
        const content::SavedRulesActivationReport &,
        const content::GameplayPostprocessReport &,
        const BnwSourceDecodeContract &, std::int64_t, bool);
    friend class detail::CivisSelectedBnwRulesPreparation;
    static SelectedBnwPreviewResult prepare_rules_owned(
        const content::RulesDatabase&,std::optional<runtime::FileIdentity>,bool,
        const std::function<void()>& current={});
    static SelectedBnwPreviewResult restore_selected_owned(
        const std::filesystem::path&,const content::RulesDatabase&,
        const content::SavedRulesActivationReport&,const content::GameplayPostprocessReport&,
        const BnwSourceDecodeContract&,std::int64_t,bool,
        std::shared_ptr<SelectedBnwPreviewHost> prepared={},
        saves::SavedTerrainFeatureResolver features={});
    // Factory-only measured file identity, immutable for this Host lifetime.
    // Never inferred from the caller hash, saved header or a default profile.
    explicit SelectedBnwPreviewHost(std::optional<runtime::FileIdentity> actual);
    const std::optional<runtime::FileIdentity> gamecore_implementation_identity_;
    // Shared ownership is real: the private GC child retains this exact clone,
    // never a no-op-deleter alias to a wrapper owned elsewhere.
    std::shared_ptr<content::RulesDatabase> database_;
    std::shared_ptr<const content::BnwNotificationEntriesSourceOwner> notification_entries_;
    mutable std::shared_ptr<const content::BnwGameInfoActionsOwner> bnw_gameinfo_actions_;
    mutable bool bnw_gameinfo_actions_querying_=false;
    void stage_globals_cache_source_owner();
    std::unique_ptr<content::BnwGlobalsCacheSourceOwner> globals_cache_source_owner_;
    content::InfoTypeCatalog types_;
    content::GlobalDefineCache defines_;
    content::TechnologyRules technologies_;
    runtime::CityYieldRules city_yields_;
    runtime::ProductionProgressRules production_;
    runtime::MaintenanceRules maintenance_;
    runtime::TreasuryRules treasury_;
    runtime::SessionCheckpointIdentity identity_;
    std::string original_source_sha256_;
    std::unique_ptr<runtime::LiveSession> session_;
    // Distinct from activated rules: verified per-save SQLite backing only.
    std::shared_ptr<content::RulesDatabase> saved_database_;
    const std::thread::id saved_database_thread_=std::this_thread::get_id();
    mutable std::atomic<bool> saved_database_claimed_{false};
    std::unique_ptr<BnwNotificationPopupOwner> notification_owner_;
    std::int64_t active_player_id_ = -1;
    std::unique_ptr<runtime::BnwGameActivationOwner> activation_owner_;
    friend class runtime::BnwProcessedAutoMovesImportOwner;
    std::unique_ptr<runtime::BnwProcessedAutoMovesImportOwner> bnw_processed_auto_moves_import_;
    void stage_processed_auto_moves_original_read(std::shared_ptr<const SelectedBnwPreviewHost>);
    // Original Read transients retained once by the SAME host, never per input.
    std::unique_ptr<runtime::BnwUpdateMovesImportOwner> update_moves_owner_;
    std::unique_ptr<BnwTopPanelPreviewBridge> top_panel_bridge_;
    std::unique_ptr<BnwUnitPanelPreviewBridge> unit_panel_bridge_;
    std::unique_ptr<runtime::PregameRoster> pregame_roster_;
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> early_package_;
    std::string early_art_final_fingerprint_;
    std::shared_ptr<const assets::CivisResourceBankOwner> civis_resource_bank_;
    std::weak_ptr<const SelectedBnwPreviewHost> civis_canonical_self_;
    std::shared_ptr<const SelectedBnwPreviewHost> retain_civis_resource_host_owned() const;
    std::string civis_art_final_fingerprint_, civis_resource_recipe_identity_;
    // Installed only by the genuine completed bootstrap after actual Art/Rules
    // clone lineage. Parent's inverse adoption edge remains non-owning.
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> completed_resources_;
    std::shared_ptr<const detail::BnwNativeCompletedParentObservation> completed_observation_;
    std::shared_ptr<const detail::BnwNativeCompletedColorQuery> completed_color_query_;
    std::shared_ptr<const detail::BnwNativeCompletedSelectedRead> completed_selected_read_;
    std::shared_ptr<const detail::BnwNativeCompletedUnitArtInputs> completed_unit_art_inputs_;
    std::shared_ptr<const BnwNativePinnedInputEpoch> completed_input_epoch_;
    // Creator-first callback-free discriminator for the actual private Parent.
    // A nonnull value is not readiness: full observer/epoch/clone checks follow.
    const std::shared_ptr<const detail::BnwNativeCompletedParentObservation>&
        completed_observation_without_callbacks()const;

};

struct SelectedBnwPreviewResult {
    // Mint one real shared control block by moving the uniquely restored
    // host. The result stays move-only; every downstream owner reuses it.
    std::shared_ptr<SelectedBnwPreviewHost> host;
    // Strong external owner; Core retains Host, so Host itself owns only weak.
    std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> restored_mission_core;
    SelectedBnwPreviewResult() = default;
    SelectedBnwPreviewResult(std::unique_ptr<SelectedBnwPreviewHost> restored,
        std::optional<SelectedBnwPreviewBlocker> why = {})
        : host(std::move(restored)), blocker(std::move(why)) {}
    SelectedBnwPreviewResult(std::shared_ptr<SelectedBnwPreviewHost> restored,
        std::optional<SelectedBnwPreviewBlocker> why,
        std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> core)
        : host(std::move(restored)), restored_mission_core(std::move(core)), blocker(std::move(why)) {}
    SelectedBnwPreviewResult(const SelectedBnwPreviewResult&) = delete;
    SelectedBnwPreviewResult& operator=(const SelectedBnwPreviewResult&) = delete;
    SelectedBnwPreviewResult(SelectedBnwPreviewResult&&) noexcept = default;
    SelectedBnwPreviewResult& operator=(SelectedBnwPreviewResult&&) noexcept = default;
    std::optional<SelectedBnwPreviewBlocker> blocker;
    explicit operator bool() const noexcept { return static_cast<bool>(host); }
};

// The input borrows session/rules. Retain the owner across the entire
// synchronous renderer call, including if the caller releases its own owner
// while rendering. The callback must not keep the input pointer afterward.
template <class Owner, class Render>
void with_owned_bnw_camera_input(std::shared_ptr<Owner> owner, Render &&render) {
    if (!owner) throw std::invalid_argument("BNW preview owner is missing");
    auto input = owner->preview_input();
    std::invoke(std::forward<Render>(render), &input);
}

// This is the explicit read-only bridge into run_camera_preview's final
// LiveBnwPreviewInput argument. A successful selected host is required; the
// renderer binds its live frame provider/backend only inside the callback.
class SelectedBnwReadOnlyPreview {
  public:
    explicit SelectedBnwReadOnlyPreview(SelectedBnwPreviewResult result);
    SelectedBnwReadOnlyPreview(const SelectedBnwReadOnlyPreview &) = delete;
    SelectedBnwReadOnlyPreview &operator=(const SelectedBnwReadOnlyPreview &) = delete;
    SelectedBnwReadOnlyPreview(SelectedBnwReadOnlyPreview &&) noexcept = default;
    SelectedBnwReadOnlyPreview &operator=(SelectedBnwReadOnlyPreview &&) noexcept = default;

    void with_camera_input(
        const std::function<void(const LiveBnwPreviewInput *)> &render) const;
    // Additive retained-parent route for source-scoped child owners. Legacy
    // callbacks stay available. This handoff itself mints no source authority.
    void with_owned_host_camera_input(const std::function<void(
        std::shared_ptr<const SelectedBnwPreviewHost>,const LiveBnwPreviewInput*)>&)const;
    const runtime::LiveSession &session() const;
    // Actual private owned source producer. EARLY parent is required; source
    // callable storage and its genuine parent survive the renderer callback.
    void with_owned_camera_ui_source_input(const std::function<void(
        std::shared_ptr<BnwOwnedCameraUiSourceInput>)>&)const;

  private:
    std::shared_ptr<SelectedBnwPreviewHost> owner_;
    std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> restored_mission_core_;
};

// The active database must already have passed selected base/DLC/mod
// activation and BNW postprocessing. This function clones it and checks each
// dependency before exposing a restored, read-only preview host. The returned
// host still has LiveSession readiness blockers until load hooks, GameCore,
// full turns and persistence are implemented.
SelectedBnwPreviewResult make_selected_bnw_preview_host(
    const std::filesystem::path &save_path,
    const content::RulesDatabase &activated_database,
    const content::SavedRulesActivationReport &activation,
    const content::GameplayPostprocessReport &postprocess,
    const BnwSourceDecodeContract &source,
    std::int64_t active_player_id);
// Explicit one-way immutable preview scope. Legacy factory remains mutable.
SelectedBnwPreviewResult make_selected_bnw_preview_host(
    const std::filesystem::path &save_path,
    const content::RulesDatabase &activated_database,
    const content::SavedRulesActivationReport &activation,
    const content::GameplayPostprocessReport &postprocess,
    const BnwSourceDecodeContract &source,
    std::int64_t active_player_id, bool immutable_preview_fingerprints);

struct SelectedBnwBootstrapResult {
    runtime::SavedContentProfile profile;
    std::optional<content::SavedRulesActivationReport> activation;
    std::optional<content::GameplayPostprocessReport> postprocess;
    std::uint64_t indexed_resources = 0;
    std::string gamecore_sha256;
    // A Civis container owns its payload, but metadata naming the original
    // external .Civ5Save cannot prove that file still exists or has same bytes.
    bool original_source_file_verified = false;
    SelectedBnwPreviewResult preview;
};

// The installation root contains the real CivilizationV.exe and Assets tree.
// A split local installation may be supplied through a caller-owned private
// junction/hardlink view; this function never changes original game files.
// Only DLC named in this save is mounted. The exact local Expansion2 GameCore
// binary is pinned before standard BNW wire dimensions are admitted.
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview(
    const std::filesystem::path &save_path,
    const std::filesystem::path &game_root,
    std::int64_t active_player_id);
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview(
    const std::filesystem::path &save_path,
    const std::filesystem::path &game_root,
    std::int64_t active_player_id, bool immutable_preview_fingerprints);

// Offline selected restart only: read the three actual installed BASE unit-art
// XML documents and compile all five tables before selected DLC activation.
// No HWND, Bank/native completed receipt, VM, renderer or continuation is issued.
// Existing checkpoint fingerprints still authenticate the resulting rules.
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_with_installed_base_unit_art(
    const std::filesystem::path &save_path,
    const std::filesystem::path &game_root, std::int64_t active_player_id);

// Imports the actual selected session with the own bank's three verified art
// documents. Rules activation/decode/postprocess/DB sealing remain mandatory;
// this does not issue a native VM, font, GUI, renderer or game-ready receipt.
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_civis_resources(
    const std::shared_ptr<assets::CivisResourceBankOwner>&, std::int64_t active_player_id);

// Requires completed real early Common registration. Compiles all five base
// unit art tables before DLC/mod activation, clones/seals the actual final DB,
// then adopts exactly that early package. Failed stages expose no adopted host.
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_early_package(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner>, std::int64_t active_player_id);

// Completion-owned actual inputs, not a table/HWND/boolean handoff. The real
// factory validates canonical Parent/cap/read controls, mints the closed art
// input, preserves independent Civis loose rules policy and final DB lineage.
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_completed_resources(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>,
        std::shared_ptr<const detail::BnwNativeCompletedColorQuery>,
        std::shared_ptr<const detail::BnwNativeCompletedSelectedRead>,std::int64_t);

} // namespace civ5::ui
