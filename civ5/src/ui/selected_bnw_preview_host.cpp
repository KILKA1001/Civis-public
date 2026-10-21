#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "detail/civis_selected_bnw_rules_preparation.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/runtime/bnw_imported_pregame_locale_owner.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_pinned_input_epoch.hpp"
#include "bnw_native_completed_resource_bank_capability.hpp"
#include "bnw_native_completed_parent_observation.hpp"
#include "bnw_native_completed_color_query.hpp"
#include "bnw_native_completed_selected_read.hpp"
#include "bnw_native_completed_unit_art_inputs.hpp"
#include "civ5/graphics/bnw_base_unit_art_compilation.hpp"
#include "../content/bnw_globals_cache_source_owner.hpp"
#include "../content/bnw_notification_entries_source_owner.hpp"
#include "civ5/ui/bnw_top_panel_preview_bridge.hpp"
#include "civ5/ui/bnw_unit_panel_preview_bridge.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/bnw_processed_auto_moves_import_owner.hpp"
#include "civ5/runtime/bnw_update_moves_prefix.hpp"
#include "civ5/runtime/bnw_ui_transient_checkpoint.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/saves/save_header.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <stdexcept>
#include <exception>
#include <algorithm>
#include <cctype>
#include <utility>

namespace civ5::ui {
SelectedBnwPreviewHost::SelectedBnwPreviewHost(std::optional<runtime::FileIdentity> actual)
    : gamecore_implementation_identity_(std::move(actual)) {}

runtime::BnwTurnBasePartialExport SelectedBnwPreviewHost::export_import_clock_checkpoint() const {
    require_restored_unit_mission_checkpoint_current_without_callbacks();
    if(std::this_thread::get_id()!=saved_database_thread_ || !session_ || !database_ ||
       !activation_owner_ || !update_moves_owner_ || !activation_owner_->busy_state)
        throw std::runtime_error("Selected BNW checkpoint lacks same-thread retained transient owners");
    const auto& a=*activation_owner_;
    if(!a.processed_auto_moves.empty()) {
        if(!bnw_processed_auto_moves_import_)
            throw std::runtime_error("BNW import-clock checkpoint processed-auto owner has no genuine original seal");
        bnw_processed_auto_moves_import_->require_pristine_checkpoint_without_callbacks(
            *this,*session_->read_state_lease(),session_->state_generation());
    }
    if(a.identity.payload_sha256!=identity_.payload_sha256 || a.identity.rules_sha256!=identity_.rules_sha256 ||
       !a.completed.empty() || !a.pending.empty() ||
       a.deferred_fog || a.city_connections || a.pathfinder_cache || a.builder_tasking ||
       a.diplomacy_requests || a.player_do_turn_prefix || a.player_do_turn_revolts ||
       a.player_do_turn_trade || a.player_do_turn_military || a.grand_military_average ||
       a.grand_world_averages || a.grand_guess_empty)
        throw std::runtime_error("BNW import-clock checkpoint lacks activation/publication history codecs");
    (void)session_->bnw_game_debug_mode();
    if(session_->has_bnw_operation_checkpoint_history())
        throw std::runtime_error("BNW import-clock checkpoint lacks once-operation journal codecs");
    const auto generation=session_->state_generation();
    const auto state=session_->read_state_lease();
    if(state->player_gold_publication || state->player_research_publication || state->player_early_publication ||
       !state->city_dirty_events.empty() || !state->plot_gameplay_events.empty() ||
       !state->city_strategy_runtime.empty())
        throw std::runtime_error("BNW import-clock checkpoint has unpublished effects or opaque strategy owners");
    const auto fingerprint=database_->gameplay_fingerprint_sha256();
    if(fingerprint!=identity_.rules_sha256)
        throw std::runtime_error("Selected BNW checkpoint rules changed");
    const auto services=saves::make_standalone_services_section(state->standalone_services);
    const auto transient=runtime::make_bnw_ui_transient_checkpoint(*state,session_->source(),
        identity_,*update_moves_owner_,*a.busy_state);
    auto result=runtime::make_bnw_live_turn_base_partial_export(*state,session_->source(),identity_,
        original_source_sha256_,content::saved_terrain_features(*database_,types_).resolve,
        *database_,technologies_,types_,defines_);
    const auto id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_ui_transients);
    std::erase_if(result.document.sections,[id](const auto& s){return s.id==id;});
    result.document.sections.push_back(transient);
    std::erase_if(result.document.sections,[&services](const auto& s){return s.id==services.id;});
    result.document.sections.push_back(services);
    if(session_->state_generation()!=generation || session_->read_state_lease().get()!=state.get() ||
       database_->gameplay_fingerprint_sha256()!=fingerprint)
        throw std::runtime_error("Selected BNW checkpoint owner changed during preparation");
    (void)session_->bnw_game_debug_mode();
    result.full_persistence=false;
    result.uncovered_domains.push_back("activation, AI delivery and UI selection/publication histories");
    return result;
}

namespace {
bool civis_container(const std::filesystem::path &path) {
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".civissave";
}

SelectedBnwPreviewResult blocked(SelectedBnwPreviewStage stage, std::string reason) {
    return {nullptr, SelectedBnwPreviewBlocker{stage, std::move(reason)}};
}
bool activation_complete(const content::SavedRulesActivationReport &report,
                         std::string &reason) {
    if (report.profile_kind == runtime::ContentProfileKind::vanilla ||
        !report.profile_can_activate || !report.rules_database_ready) {
        reason = "selected content is not a ready BNW profile";
        return false;
    }
    if (!report.base.schema_applied || report.base.failed ||
        report.base.applied != report.base.selected) {
        reason = "selected BNW base schema or gameplay sources failed activation";
        return false;
    }
    for (const auto &package : report.dlc.packages) {
        if (package.failed || package.missing || package.applied != package.requested) {
            reason = "selected DLC gameplay sources are incomplete: " + package.id;
            return false;
        }
    }
    if (!report.mod_plan.all_sources_present || !report.mods.all_succeeded) {
        reason = "selected mod database actions are incomplete";
        return false;
    }
    for (const auto &action : report.mods.actions) {
        if (!action.applied || !action.manifest_md5_match) {
            reason = "selected mod action or source identity is unverified: " + action.action.mod_id;
            return false;
        }
    }
    return true;
}
bool source_complete(const BnwSourceDecodeContract &source, std::string &reason,
                     std::optional<runtime::FileIdentity>& actual) {
    const auto &c = source.context;
    if (source.gamecore_executable.empty() || source.gamecore_sha256.size() != 64) {
        reason = "selected BNW source GameCore and wire dimensions are not verified";
        return false;
    }
    try {
        const auto measured=runtime::identify_file(source.gamecore_executable);
        if (measured.sha256 != source.gamecore_sha256) {
            reason = "selected BNW source GameCore executable identity changed";
            return false;
        }
        actual=measured; // Only actual file bytes mint private factory provenance.
    } catch (const std::exception &e) {
        reason = std::string("selected BNW source GameCore cannot be read: ") + e.what();
        return false;
    }
    if (c.layout != saves::GameStateLayout::expansion2_version1 ||
        !c.decode_map_plots || !c.decode_map_regions || !c.decode_teams ||
        !c.decode_all_players || !c.decode_supporting_classes || !c.all_players ||
        c.skip_non_sqlite_image_for_map_diagnostic) {
        reason = "selected BNW source contract does not request a complete expansion2 snapshot";
        return false;
    }
    if (!c.permitted_profile_sections.empty()) {
        reason = "common BNW preview cannot restore native GameCore profile sections";
        return false;
    }
    return true;
}
} // namespace

SelectedBnwPreviewHost::~SelectedBnwPreviewHost() = default;
std::shared_ptr<const content::BnwNotificationEntriesSourceOwner>
SelectedBnwPreviewHost::notification_entries_owned() const {
    if (saved_database_thread_ != std::this_thread::get_id() || !database_ || !notification_entries_)
        throw std::runtime_error("Selected NotificationEntries actual Host/creator/producer absent");
    const auto result = notification_entries_;
    result->require_current(*database_);
    return result;
}
void SelectedBnwPreviewHost::stage_globals_cache_source_owner() {
    if ((!early_package_ && !civis_resource_bank_) || globals_cache_source_owner_)
        throw std::runtime_error("BNW private GC source staging requires fresh authentic host lineage");
    if (civis_resource_bank_) require_same_civis_resources(*civis_resource_bank_);
    else require_same_early_package(*early_package_);
    auto staged = std::unique_ptr<content::BnwGlobalsCacheSourceOwner>(
        new content::BnwGlobalsCacheSourceOwner(database_));
    staged->require_current(*database_);
    // No CacheGameDatabaseData call, HEAVY read feed or completed lifecycle:
    // Config routing, info-type prefix and mandatory tail are still unowned.
    globals_cache_source_owner_ = std::move(staged);
}
runtime::PregameRoster &SelectedBnwPreviewHost::pregame_roster() noexcept { return *pregame_roster_; }
const runtime::PregameRoster &SelectedBnwPreviewHost::pregame_roster() const noexcept { return *pregame_roster_; }
runtime::ScriptPregameProvider SelectedBnwPreviewHost::pregame_services() {
    return runtime::lua_pregame_services(*pregame_roster_, *database_);
}
runtime::BnwGameActivationOwner &SelectedBnwPreviewHost::activation_owner() noexcept { return *activation_owner_; }
const runtime::BnwGameActivationOwner &SelectedBnwPreviewHost::activation_owner() const noexcept { return *activation_owner_; }

LiveBnwPreviewInput SelectedBnwPreviewHost::preview_input() {
    require_restored_unit_mission_current_without_callbacks();require_restored_food_growth_current();
    LiveSessionUiSources sources;
    sources.quotes = [owner=this, generation=std::make_shared<std::uint64_t>(0)](
        const runtime::LiveSession &session, std::uint64_t frame) mutable {
        if (&session!=owner->session_.get()) return LiveSessionUiQuotes{};
        if (*generation!=session.state_generation()) {
            auto replacement=std::make_unique<BnwTopPanelPreviewBridge>(session,
                *owner->database_,owner->identity_,owner->active_player_id_);
            owner->top_panel_bridge_=std::move(replacement);
            *generation=session.state_generation();
        }
        return owner->top_panel_bridge_->capture(session,frame);
    };
    sources.quotes_current = [owner=this](const runtime::LiveSession &session,
        const LiveSessionUiQuotes &quotes,std::uint64_t frame) {
        return &session==owner->session_.get() && owner->top_panel_bridge_ &&
            owner->top_panel_bridge_->matches(session,quotes,frame);
    };
    sources.database = database_.get();
    sources.technologies = &technologies_;
    if (notification_owner_)
        bind_bnw_notification_frame_sources(sources, *notification_owner_);
    sources.unit_stats_database = database_.get();
    sources.unit_stats_types = &types_;
    sources.unit_stats_identity = &identity_;
    sources.selected_unit_read_stats = [owner=this,generation=std::make_shared<std::uint64_t>(0)](
        const runtime::LiveSession &session, std::uint64_t frame, std::int64_t selected) mutable
        -> std::optional<LiveWorldViewUnitReadStatsFeed> {
        if (&session!=owner->session_.get()) return std::nullopt;
        if (*generation!=session.state_generation()) {
            auto replacement=std::make_unique<BnwUnitPanelPreviewBridge>(session,*owner->database_,
                owner->types_,owner->identity_,owner->active_player_id_);
            owner->unit_panel_bridge_=std::move(replacement);
            *generation=session.state_generation();
        }
        return owner->unit_panel_bridge_->capture(session, selected, frame);
    };
    sources.selected_unit_read_stats_current = [owner=this](
        const runtime::LiveSession &session, const LiveWorldViewUnitReadStatsFeed &feed,
        std::int64_t selected, std::uint64_t frame) {
        return &session==owner->session_.get() &&
            owner->unit_panel_bridge_->matches(session,feed,selected,frame);
    };
    // No production eligibility, Lua or external notification effects are supplied.
    return {*session_, active_player_id_, production_, std::move(sources), {}, {},
            notification_owner_.get(), {}, identity_, activation_owner_.get(), pregame_roster_.get()};
}

std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>
SelectedBnwPreviewHost::restore_unit_mission_before_publication(
 std::shared_ptr<const SelectedBnwPreviewHost> canonical) {
 if(saved_database_thread_!=std::this_thread::get_id() || !canonical || canonical.get()!=this || !session_)
    throw std::runtime_error("Mission join requires the actual unpublished canonical Host creator");
 const auto pending=session_->state().bnw_unit_mission_continuation_quarantine;
 if(!pending)return {};
 if(bnw_restored_mission_core_)throw std::runtime_error("Mission unpublished Core issuer already reserved");
 const auto core=runtime::BnwNativeUnitAllocatorCore::from_unpublished_mission_restore_host(canonical,*pending);
 session_->initialize_original_payload_mission_pathfinder(*core,*this);
 (void)core->restore_native_existing_continue_mission(*pending);
 core->require_restored_mission_checkpoint_current_without_callbacks(*pending);
 bnw_restored_mission_core_.emplace(core);
 session_->bind_restored_unit_mission_owner(core,*this);
 require_restored_unit_mission_current_without_callbacks();
 return core;
}
void SelectedBnwPreviewHost::require_restored_unit_mission_current_without_callbacks() const {
 if(saved_database_thread_!=std::this_thread::get_id() || !session_)
    throw std::runtime_error("Mission Host foreign creator or absent session");
 const auto& pending=session_->state().bnw_unit_mission_continuation_quarantine;
 if(!pending && !session_->source().source.live_bnw_unit_mission_continuation)return;
 if(!pending || !bnw_restored_mission_core_ || session_->has_bnw_unit_mission_continuation_quarantine())
    throw std::runtime_error("Mission Host requires the live external restored Core holder and earned join");
 runtime::BnwNativeUnitAllocatorCore::require_restored_mission_weak_owner_continuity_current_without_callbacks(
    *bnw_restored_mission_core_,*pending);
}
void SelectedBnwPreviewHost::require_restored_unit_mission_checkpoint_current_without_callbacks() const {
 require_restored_unit_mission_current_without_callbacks();
 const auto& pending=session_->state().bnw_unit_mission_continuation_quarantine;
 if(!pending)return;
 // Publication and export still require the original exact physical seal.
 // Ongoing owner continuity never authorizes stale journal bytes to save.
 runtime::BnwNativeUnitAllocatorCore::require_restored_mission_weak_owner_current_without_callbacks(
    *bnw_restored_mission_core_,*pending);
}
SelectedBnwReadOnlyPreview::SelectedBnwReadOnlyPreview(SelectedBnwPreviewResult result)
    : owner_(std::move(result.host)), restored_mission_core_(std::move(result.restored_mission_core)) {
    if (!owner_) throw std::invalid_argument("selected BNW preview has no restored host");
    owner_->require_restored_unit_mission_checkpoint_current_without_callbacks();
}

void SelectedBnwReadOnlyPreview::with_camera_input(
    const std::function<void(const LiveBnwPreviewInput *)> &render) const {
    if (!render) throw std::invalid_argument("BNW preview renderer callback is missing");
    const auto parent=owner_;const auto retained_core=restored_mission_core_;
    const auto callback=render;
    with_owned_bnw_camera_input(parent,[retained_core,callback](const LiveBnwPreviewInput* input){
        (void)retained_core;callback(input);
    });
}

void SelectedBnwReadOnlyPreview::with_owned_host_camera_input(const std::function<void(
    std::shared_ptr<const SelectedBnwPreviewHost>,const LiveBnwPreviewInput*)>&borrowed)const{
    // Pin real parent BEFORE callable target copying (which may invoke caller
    // code or release this facade). No subsequent access through this.
    const auto parent=owner_;
    const auto retained_core=restored_mission_core_;
    if(!parent)throw std::invalid_argument("selected preview parent is missing");
    const auto render=borrowed;
    if(!render)throw std::invalid_argument("owned BNW camera renderer missing");
    with_owned_bnw_camera_input(parent,[parent,retained_core,render](const LiveBnwPreviewInput*input){
        (void)retained_core;render(parent,input);
    });
}

const runtime::LiveSession &SelectedBnwReadOnlyPreview::session() const {
    if (!owner_) throw std::invalid_argument("selected BNW preview owner was moved");
    return owner_->session();
}

SelectedBnwPreviewResult SelectedBnwPreviewHost::prepare_rules_owned(
    const content::RulesDatabase& activated_database,std::optional<runtime::FileIdentity> actual_gamecore,
    bool seal_immutable,const std::function<void()>& current) {
    if(current)current();
    auto host = std::shared_ptr<SelectedBnwPreviewHost>(
        new SelectedBnwPreviewHost(std::move(actual_gamecore)));
    if(current)current();
    try {
        host->database_ = activated_database.clone_gameplay();
        if(current)current();
        // Activation/postprocessing completed above. Seal only this owned clone,
        // before read-only loaders; acquisition errors retain the normal blocker.
        if(seal_immutable)
            host->database_->seal_immutable_preview_fingerprints();
        if(current)current();
        host->types_ = host->database_->cache_info_types();
        if(current)current();
        if (!host->types_.report().all_succeeded())
            return blocked(SelectedBnwPreviewStage::rule_cache,
                           "selected BNW InfoType cache has SQL errors");
        host->defines_ = host->database_->cache_global_defines(content::GlobalDefineProfile::bnw);
        if(current)current();
        if (host->defines_.errors())
            return blocked(SelectedBnwPreviewStage::rule_cache,
                           "selected BNW GlobalDefine cache has SQL errors");
        // Only the existing sealed preview producer issues this immutable cache.
        if(seal_immutable){
            host->notification_entries_ = std::shared_ptr<const content::BnwNotificationEntriesSourceOwner>(
                new content::BnwNotificationEntriesSourceOwner(host->database_, host, current));
            if(current)current();
            host->notification_entries_->require_current(*host->database_);
        }
        host->technologies_ = content::load_technology_rules(*host->database_, host->types_,
            content::TechnologyProfile::bnw, 5);
        if(current)current();
        if (!host->technologies_.all_succeeded())
            return blocked(SelectedBnwPreviewStage::rule_cache,
                           "selected BNW technology rules are incomplete");
        host->city_yields_ = runtime::load_city_yield_rules(*host->database_, content::TechnologyProfile::bnw);
        if(current)current();
        host->production_ = runtime::load_production_progress_rules(*host->database_, content::TechnologyProfile::bnw);
        if(current)current();
        host->maintenance_ = runtime::load_maintenance_rules(*host->database_, content::TechnologyProfile::bnw, host->types_);
        if(current)current();
        host->treasury_ = runtime::load_treasury_rules(*host->database_, content::TechnologyProfile::bnw, host->types_);
        if(current)current();
    } catch (const std::exception &e) {
        return blocked(SelectedBnwPreviewStage::rule_cache,
                       std::string("selected BNW rule import failed: ") + e.what());
    }

    return {std::move(host),std::nullopt,{}};
}

SelectedBnwPreviewResult make_selected_bnw_preview_host(
    const std::filesystem::path &save_path,
    const content::RulesDatabase &activated_database,
    const content::SavedRulesActivationReport &activation,
    const content::GameplayPostprocessReport &postprocess,
    const BnwSourceDecodeContract &source,
    std::int64_t active_player_id) {
    return make_selected_bnw_preview_host(save_path,activated_database,activation,
        postprocess,source,active_player_id,false);
}

SelectedBnwPreviewResult make_selected_bnw_preview_host(
    const std::filesystem::path &save_path,
    const content::RulesDatabase &activated_database,
    const content::SavedRulesActivationReport &activation,
    const content::GameplayPostprocessReport &postprocess,
    const BnwSourceDecodeContract &source,
    std::int64_t active_player_id, bool immutable_preview_fingerprints) {
    return SelectedBnwPreviewHost::restore_selected_owned(save_path,activated_database,
        activation,postprocess,source,active_player_id,immutable_preview_fingerprints);
}

SelectedBnwPreviewResult SelectedBnwPreviewHost::restore_selected_owned(
    const std::filesystem::path& save_path,const content::RulesDatabase& activated_database,
    const content::SavedRulesActivationReport& activation,const content::GameplayPostprocessReport& postprocess,
    const BnwSourceDecodeContract& source,std::int64_t active_player_id,bool immutable_preview_fingerprints,
    std::shared_ptr<SelectedBnwPreviewHost> prepared,saves::SavedTerrainFeatureResolver prepared_features) {
    if (!std::filesystem::is_regular_file(save_path))
        return blocked(SelectedBnwPreviewStage::selected_save,
                       "selected .Civ5Save file is missing");
    bool mission_restore_requested=false,serial_restore_requested=false,food_restore_requested=false;
    try {
        if (civis_container(save_path)){
            const auto imported=saves::read_loaded_save(save_path);
            mission_restore_requested=imported.live_bnw_unit_mission_continuation.has_value();
            serial_restore_requested=imported.live_bnw_unit_serial.has_value();
            food_restore_requested=imported.live_bnw_food_growth_journal.has_value();
        }
        else (void)saves::read_header(save_path);
    } catch (const std::exception &e) {
        return blocked(SelectedBnwPreviewStage::selected_save,
                       std::string("selected save header cannot be read: ") + e.what());
    }
    std::string reason;
    if (!activation_complete(activation, reason))
        return blocked(SelectedBnwPreviewStage::activation, std::move(reason));
    if (!postprocess.all_succeeded || !postprocess.transaction_released ||
        !postprocess.strategic_resources_applied ||
        !postprocess.luxury_resources_applied) {
        return blocked(SelectedBnwPreviewStage::postprocess,
                       "selected BNW gameplay postprocessing is incomplete");
    }
    std::optional<runtime::FileIdentity> actual_gamecore;
    if (!source_complete(source, reason, actual_gamecore))
        return blocked(SelectedBnwPreviewStage::source_contract, std::move(reason));

    auto host=std::move(prepared);
    if(host){
        const auto& held=host->gamecore_implementation_identity_;
        if(host->saved_database_thread_!=std::this_thread::get_id()||host->session_||
           !held||!actual_gamecore||held->bytes!=actual_gamecore->bytes||held->sha256!=actual_gamecore->sha256||
           !host->database_||!host->database_->immutable_preview_fingerprints_current()||
           host->database_->gameplay_fingerprint_sha256()!=activated_database.gameplay_fingerprint_sha256()||
           !host->types_.report().all_succeeded()||!prepared_features)
            return blocked(SelectedBnwPreviewStage::source_contract,"Prepared Civis Host/rules cache differs or was already restored");
    }else{
        auto cached=prepare_rules_owned(activated_database,std::move(actual_gamecore),
            immutable_preview_fingerprints||mission_restore_requested||serial_restore_requested||food_restore_requested);
        if(!cached)return cached;host=std::move(cached.host);
    }

    auto decode = source.context;
    decode.permit_live_bnw_unit_serial_overlay=serial_restore_requested;
    decode.permit_live_bnw_food_growth_original_decode=food_restore_requested;
    try {
        decode.terrain_features = prepared_features ? std::move(prepared_features) :
            content::saved_terrain_features(*host->database_, host->types_).resolve;
        const auto input_file_sha=runtime::identify_file(save_path).sha256;
        auto saved = saves::read_saved_session(save_path, decode);
        if(runtime::identify_file(save_path).sha256!=input_file_sha)
            throw std::runtime_error("Selected BNW source file changed during decode");
        host->original_source_sha256_=saved.source.source_sha256.value_or(input_file_sha);
        // observe_loaded_session already validated the exact original DB boundary
        // and any ID6 replacement. Bind only AFTER genuine full LiveSession restore.
        host->identity_ = {runtime::identify_bytes(saved.source.bytes).sha256,
            host->database_->gameplay_fingerprint_sha256()};
        runtime::LiveSessionImportRules rules{*host->database_, host->types_,
            host->technologies_, host->defines_, host->city_yields_, host->production_,
            host->maintenance_, host->treasury_, host->identity_,source.actual_early_pregame_locale};
        try {
            host->session_ = std::make_unique<runtime::LiveSession>(
                source.import_purpose==BnwSelectedImportPurpose::imported_single_player_game ?
                runtime::LiveSession::restore_bnw_imported_game(std::move(saved), rules) :
                runtime::LiveSession::restore_bnw(std::move(saved), rules));
        } catch (const std::exception &e) {
            return blocked(SelectedBnwPreviewStage::live_restore,
                           std::string("selected BNW live restore failed: ") + e.what());
        }
    } catch (const std::exception &e) {
        return blocked(SelectedBnwPreviewStage::source_decode,
                       std::string("selected BNW source decode failed: ") + e.what());
    }
    if(host->session_->source().source.live_bnw_food_growth_journal){
        try {host->restore_food_growth_before_publication(host);}
        catch(const std::exception& error){return blocked(SelectedBnwPreviewStage::live_restore,std::string("Food47 unpublished restore failed: ")+error.what());}
    }
    std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> restored_mission_core;
    if(host->session_->has_bnw_unit_mission_continuation_quarantine()) {
        try {restored_mission_core=host->restore_unit_mission_before_publication(host);}
        catch(const std::exception& error) {
            return blocked(SelectedBnwPreviewStage::live_restore,
                std::string("BNW mission unpublished restore join failed: ")+error.what());
        }
    }
    const auto &players = host->session_->state().setup.players;
    const auto player = players.find(active_player_id);
    if (active_player_id < 0 || player == players.end() || !player->second.alive)
        return blocked(SelectedBnwPreviewStage::active_player,
                       "explicit local active player is absent or dead in restored BNW session");
    host->activation_owner_ = std::make_unique<runtime::BnwGameActivationOwner>();
    host->activation_owner_->identity = host->identity_;
    if(host->session_->source().source.container==saves::SaveContainerKind::civ5){
        try {
            host->stage_processed_auto_moves_original_read(host);
            const auto& imported=host->session_->source();
            auto moves=runtime::bind_bnw_update_moves_original_import(imported,host->identity_);
            auto busy=runtime::bind_bnw_busy_state_original_import(imported,host->identity_);
            host->update_moves_owner_=std::make_unique<runtime::BnwUpdateMovesImportOwner>(std::move(moves));
            host->activation_owner_->busy_state=std::move(busy);
        } catch(const std::exception& error) {
            return blocked(SelectedBnwPreviewStage::live_restore,
                std::string("Selected original BNW transient import failed: ")+error.what());
        }
    }
    else if(const auto& section=host->session_->source().source.live_bnw_ui_transients;section){
        try {
            auto restored=runtime::restore_bnw_ui_transient_checkpoint(*section,
                host->session_->state(),host->session_->source(),host->identity_);
            host->update_moves_owner_=std::make_unique<runtime::BnwUpdateMovesImportOwner>(std::move(restored.moves));
            host->activation_owner_->busy_state=std::move(restored.busy);
        } catch(const std::exception& error) {
            return blocked(SelectedBnwPreviewStage::live_restore,
                std::string("Selected BNW transient restore failed: ")+error.what());
        }
    }
    // Legacy Civis sources have no receipt: keep both owners absent.
    host->active_player_id_ = active_player_id;
    host->top_panel_bridge_ = std::make_unique<BnwTopPanelPreviewBridge>(
        *host->session_, *host->database_, host->identity_, active_player_id);
    host->unit_panel_bridge_ = std::make_unique<BnwUnitPanelPreviewBridge>(
        *host->session_, *host->database_, host->types_, host->identity_, active_player_id);
    if (!host->session_->state().world.pregame)
        return blocked(SelectedBnwPreviewStage::live_restore, "Current BNW pregame owner is absent");
    host->pregame_roster_ = std::make_unique<runtime::PregameRoster>(
        runtime::restore_pregame_roster(*host->session_->state().world.pregame, *host->database_));
    host->notification_owner_ = std::make_unique<BnwNotificationPopupOwner>(
        *host->session_, active_player_id);
    try {
        const auto& saved_db=host->session_->source().snapshot.saved_database;
        if(!saved_db.image.empty()) {
            if(!saved_db.quick_check_passed || saved_db.non_sqlite_image_skipped)
                throw std::runtime_error("Selected saved database has no verified source boundary");
            host->saved_database_=content::RulesDatabase::from_saved_image(saved_db.image);
        }
        // Absent native image remains absent; no synthetic SimpleValues reset.
    } catch(const std::exception& error) {
        return blocked(SelectedBnwPreviewStage::source_decode,
                       std::string("selected saved database import failed: ")+error.what());
    }
    // Active-player/activation/notification/database providers above returned.
    // The external strong Core holder stays alive across this final pure seal.
    try {host->require_restored_unit_mission_checkpoint_current_without_callbacks();host->require_restored_food_growth_publication_current();}
    catch(const std::exception& error) {
        return blocked(SelectedBnwPreviewStage::live_restore,
            std::string("BNW mission final publication seal failed: ")+error.what());
    }
    return {std::move(host), std::nullopt, std::move(restored_mission_core)};
}

namespace {
struct CivisBankHostBootstrapInputs final {
    std::shared_ptr<const assets::CivisResourceBankOwner> bank;
    assets::CivisResourceBankCapability capability;
    assets::CivisResourceBankSourceRecipe recipe;
    explicit CivisBankHostBootstrapInputs(
        std::shared_ptr<const assets::CivisResourceBankOwner> actual)
        : bank(std::move(actual)), capability(bank->capability()),
          recipe(bank->source_recipe()) {}
    void current() const {
        capability.require_current();
        bank->require_boundary();
        capability.require_current();
        if (recipe.recipe_identity.empty())
            throw std::runtime_error("Civis selected resource recipe has no actual identity");
    }
};
struct CompletedHostBootstrapInputs final {
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package;
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> resources;
    std::shared_ptr<const detail::BnwNativeCompletedParentObservation> observation;
    std::shared_ptr<const detail::BnwNativeCompletedColorQuery> color;
    std::shared_ptr<const detail::BnwNativeCompletedSelectedRead> read;
    std::shared_ptr<const detail::BnwNativeCompletedUnitArtInputs> art;
    std::shared_ptr<const BnwNativePinnedInputEpoch> epoch;
    std::filesystem::path save_path,game_root;
    content::Catalog catalog;
    runtime::SavedContentProfile profile;
    void current()const {
        if(!package||!resources||!observation||!color||!read||!art||!epoch)
            throw std::runtime_error("Completed BNW bootstrap actual retained inputs absent");
        observation->require_same_parent(*package);resources->require_identity_current();
        color->require_same_parent(*package);read->require_same_parent(*package);art->require_same_parent(*package);
        observation->with_actual_parent_loan([&](const auto& loan){
            loan.with_actual_pinned_input_epoch([&](const BnwNativePinnedInputEpoch& actual){
                if(&actual!=epoch.get())throw std::runtime_error("Completed BNW bootstrap substituted actual Epoch");
                actual.require_current();
                if(actual.save_path()!=save_path||actual.game_root()!=game_root)
                    throw std::runtime_error("Completed BNW bootstrap actual Epoch roots differ");
                loan.require_current();
            });
            loan.require_current();
        });
        art->require_same_profile(profile);epoch->require_current();observation->require_same_parent(*package);
    }
};
}

struct detail::CivisRulesPreparationSink final {
    detail::CivisSelectedBnwRulesPreparation& output;
    SelectedBnwBootstrapResult consume(std::unique_ptr<content::RulesDatabase>&& database,
        std::optional<content::LooseResourcePlan>&& mounts,SelectedBnwBootstrapResult&& metadata,
        BnwSourceDecodeContract&& source,const runtime::FileIdentity& save,const saves::SaveHeader& header){
        return output.adopt_activated_owned(std::move(database),std::move(mounts),std::move(metadata),
            std::move(source),save,header);
    }
};
static SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_impl(
    const std::filesystem::path&, const std::filesystem::path&, std::optional<std::int64_t>, bool,
    const std::shared_ptr<const BnwNativePackageDiscoveryOwner>&, std::string*,
    const CompletedHostBootstrapInputs* = nullptr,
    const CivisBankHostBootstrapInputs* = nullptr, bool installed_base_unit_art = false,
    detail::CivisRulesPreparationSink* = nullptr);

SelectedBnwBootstrapResult bootstrap_selected_bnw_preview(
    const std::filesystem::path &save_path,
    const std::filesystem::path &game_root,
    std::int64_t active_player_id) {
    return bootstrap_selected_bnw_preview(save_path,game_root,active_player_id,false);
}

SelectedBnwBootstrapResult bootstrap_selected_bnw_preview(
    const std::filesystem::path &save_path,
    const std::filesystem::path &game_root,
    std::int64_t active_player_id, bool immutable_preview_fingerprints) {
    return bootstrap_selected_bnw_preview_impl(save_path, game_root, active_player_id,
        immutable_preview_fingerprints, {}, nullptr);
}

SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_with_installed_base_unit_art(
    const std::filesystem::path& save_path, const std::filesystem::path& game_root,
    std::int64_t active_player_id) {
    return bootstrap_selected_bnw_preview_impl(save_path, game_root, active_player_id,
        false, {}, nullptr, nullptr, nullptr, true);
}

static SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_impl(
    const std::filesystem::path& save_path, const std::filesystem::path& game_root,
    std::optional<std::int64_t> active_player_id, bool immutable_preview_fingerprints,
    const std::shared_ptr<const BnwNativePackageDiscoveryOwner>& early_package,
    std::string* final_art_fingerprint, const CompletedHostBootstrapInputs* completed,
    const CivisBankHostBootstrapInputs* civis, bool installed_base_unit_art,
    detail::CivisRulesPreparationSink* preparation_output) {
    if((preparation_output&&(!civis||active_player_id))||(!preparation_output&&!active_player_id))
        throw std::invalid_argument("Civis rules preparation and actual Host restore inputs differ");
    if (installed_base_unit_art && (civis || completed || early_package))
        throw std::invalid_argument("Offline installed art cannot adopt a native/resource lane");
    if (civis && (completed || early_package))
        throw std::invalid_argument("Civis and native resource bootstrap lanes cannot be mixed");
    SelectedBnwBootstrapResult result;
    const auto fail = [&](SelectedBnwPreviewStage stage, std::string reason) {
        result.preview = blocked(stage, std::move(reason));
        return std::move(result);
    };
    runtime::FileIdentity selected_save;
    saves::SaveHeader header;
    std::optional<saves::LoadedSave> civis_preflight;
    try {
        if (!std::filesystem::is_regular_file(save_path))
            return fail(SelectedBnwPreviewStage::selected_save,
                        "selected .Civ5Save or .CivisSave file is missing");
        selected_save = runtime::identify_file(save_path);
        if (civis_container(save_path)) {
            civis_preflight = saves::read_loaded_save(save_path);
            header = civis_preflight->header;
            if (!civis_preflight->source_prefix)
                return fail(SelectedBnwPreviewStage::source_contract,
                            "Civis import lacks original setup prefix");
            // ID6 is saved SQL data, NOT activated gameplay rules. Its actual
            // source boundary and SQLite integrity are checked by full decode.
            if (!civis_preflight->profile_sections.empty())
                return fail(SelectedBnwPreviewStage::source_contract,
                            "Civis GameCore profile sections have no BNW runtime consumer");
        } else {
            header = saves::read_header(save_path);
        }
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::selected_save,
                    std::string("cannot inspect selected save: ") + e.what());
    }
    if (!std::filesystem::is_regular_file(game_root / "CivilizationV.exe") ||
        !std::filesystem::is_directory(game_root / "Assets/DLC"))
        return fail(SelectedBnwPreviewStage::content_catalog,
                    "installation root requires real CivilizationV.exe and Assets/DLC");
    content::Catalog catalog;
    try {
        if(completed) {
            completed->current();
            catalog=completed->catalog;
            result.profile=runtime::resolve_saved_content_profile(header,catalog);
            completed->art->require_same_profile(result.profile);
            completed->current();
        } else if (civis) {
            civis->current();
            if (civis->recipe.save_path != save_path || civis->recipe.game_root != game_root)
                throw std::runtime_error("Civis selected host input differs from its actual bank");
            catalog = civis->recipe.catalog;
            result.profile = runtime::resolve_saved_content_profile(header, catalog);
            if (result.profile.enabled_dlc != civis->recipe.profile.enabled_dlc ||
                result.profile.enabled_mods != civis->recipe.profile.enabled_mods ||
                result.profile.kind != civis->recipe.profile.kind)
                throw std::runtime_error("Civis selected save/profile differs from its captured bank");
            civis->current();
        } else {
            catalog = content::discover_packages({game_root / "Assets/DLC"});
            result.profile = runtime::resolve_saved_content_profile(header, catalog);
        }
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::content_catalog,
                    std::string("selected DLC discovery failed: ") + e.what());
    }
    if (installed_base_unit_art && !result.profile.enabled_mods.empty())
        return fail(SelectedBnwPreviewStage::content_catalog,
                    "Offline installed BASE art restart requires the selected zero-mod profile");
    constexpr std::string_view expansion2_id = "6da07636-4123-4018-b643-6575b4ec336b";
    const auto expansion2_selected = std::any_of(result.profile.enabled_dlc.begin(),
        result.profile.enabled_dlc.end(), [&](std::size_t index) {
            const auto &package = catalog.packages.at(index);
            return package.kind == content::PackageKind::dlc &&
                   content::canonical_guid(package.id) == expansion2_id;
        });
    if (!expansion2_selected)
        return fail(SelectedBnwPreviewStage::content_catalog,
                    "selected save does not activate Expansion2 DLC");
    if (!result.profile.can_activate) {
        for (const auto &requirement : result.profile.requirements)
            if (requirement.status != runtime::RequirementStatus::located)
                return fail(SelectedBnwPreviewStage::content_catalog,
                            "selected package missing or ambiguous: " + requirement.name);
        return fail(SelectedBnwPreviewStage::content_catalog,
                    result.profile.native_mods_require_port
                        ? "selected native mod GameCore has no x64 profile"
                        : "selected DLC or mod files are incomplete");
    }

    const auto gamecore = game_root / "Assets/DLC/Expansion2/CvGameCore_Expansion2.dll";
    constexpr std::uint64_t pinned_bytes = 4061696;
    constexpr std::string_view pinned_sha256 =
        "c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5";
    try {
        const auto identity = runtime::identify_file(gamecore);
        result.gamecore_sha256 = identity.sha256;
        if (identity.bytes != pinned_bytes || identity.sha256 != pinned_sha256)
            return fail(SelectedBnwPreviewStage::source_contract,
                        "selected Expansion2 GameCore differs from pinned BNW wire source");
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::source_contract,
                    std::string("selected Expansion2 GameCore cannot be read: ") + e.what());
    }

    auto activated_database=std::make_unique<content::RulesDatabase>();
    auto& database=*activated_database;
    std::optional<content::LooseResourcePlan> retained_rules_mounts;
    try {
        retained_rules_mounts.emplace(content::build_selected_loose_resource_plan(
            game_root, result.profile.enabled_dlc, catalog));
        auto& mounts=*retained_rules_mounts;
        result.indexed_resources = mounts.indexed_files;
        if(completed) {
            completed->current();
            const auto art=graphics::BnwBaseUnitArtCompilation::compile_from_completed_native_inputs(
                completed->art,completed->profile);
            // Existing independently built Civis loose rules plan remains a
            // rules/localization input. It is not the native art winner table.
            result.activation=content::activate_saved_rules_with_completed_base_unit_art(
                database,game_root,completed->profile,completed->catalog,mounts.resources,art,completed->art);
            completed->current();
        } else if (civis) {
            civis->current();
            const auto& art_resources = civis->bank->resources(civis->capability);
            const auto art = graphics::BnwBaseUnitArtCompilation::compile(
                art_resources, civis->recipe.profile);
            result.activation = content::activate_saved_rules_with_base_unit_art(
                database, game_root, civis->recipe.profile, civis->recipe.catalog,
                mounts.resources, art, art_resources);
            civis->current();
        } else if (early_package) {
            const auto lease = early_package->early_input_lease();
            lease->require_current();
            const auto& early_resources = early_package->resources();
            const auto art = graphics::BnwBaseUnitArtCompilation::compile(
                early_resources, lease->profile());
            result.activation = content::activate_saved_rules_with_base_unit_art(
                database, game_root, lease->profile(), lease->catalog(), mounts.resources,
                art, early_resources);
            lease->require_current();
        } else if (installed_base_unit_art) {
            // A distinct offline read of genuine installed sources, not a Bank
            // surrogate or a copied fingerprint. The existing compiler pins
            // each selected payload before applying its real SQL statements.
            auto installed = std::make_shared<assets::LooseResourceIndex>(game_root);
            for (const auto* path : {"Assets/Units/Civ5ArtDefines_UnitMembers.xml",
                                     "Assets/Units/Civ5ArtDefines_Units.xml",
                                     "Assets/Units/Civ5ArtDefines_UnitMemberTemplates.xml"})
                installed->add_file(path);
            assets::ResourceMountTable art_resources(assets::resource_lookup(installed),
                assets::resource_lookup_all(installed), assets::resource_keys(installed));
            const auto art = graphics::BnwBaseUnitArtCompilation::compile(art_resources, result.profile);
            result.activation = content::activate_saved_rules_with_base_unit_art(
                database, game_root, result.profile, catalog, mounts.resources, art, art_resources);
        } else {
            result.activation = content::activate_saved_rules(database, game_root,
                result.profile, catalog, mounts.resources);
        }
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::activation,
                    std::string("selected BNW rule activation failed: ") + e.what());
    }
    if (!result.activation->rules_database_ready)
        return fail(SelectedBnwPreviewStage::activation,
                    "selected BNW base, localization, DLC, or mod actions are incomplete");

    try {
        constexpr content::GameplayDefine bnw_defines[] = {
            {"MAX_CIV_PLAYERS", 63}, {"MAX_CIV_TEAMS", 63},
            {"MAX_MAJOR_CIVS", 22}, {"MAX_MINOR_CIVS", 41},
            {"MAX_PLAYERS", 64}, {"MAX_TEAMS", 64},
            {"BARBARIAN_PLAYER", 63}, {"BARBARIAN_TEAM", 63},
            {"NUM_CITY_PLOTS", 37}, {"CITY_HOME_PLOT", 0},
            {"CITY_PLOTS_RADIUS", 3}, {"CITY_PLOTS_DIAMETER", 7}};
        result.postprocess = database.postprocess_gameplay(
            content::GameplayPostprocessProfile::bnw, bnw_defines);
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::postprocess,
                    std::string("selected BNW postprocess failed: ") + e.what());
    }
    BnwSourceDecodeContract source;
    if(civis){
        try{
            civis->current();
            if(!retained_rules_mounts)throw std::runtime_error("Pregame early Locale actual activated rules mount lifetime absent");
            source.actual_early_pregame_locale=runtime::BnwImportedPregameLocaleOwner::capture(
                civis->bank,database,retained_rules_mounts->resources);
            source.actual_early_pregame_locale->require_current();civis->current();
            source.import_purpose=BnwSelectedImportPurpose::imported_single_player_game;
        }catch(const std::exception& error){return fail(SelectedBnwPreviewStage::postprocess,std::string("Actual early Pregame Locale init failed: ")+error.what());}
    }
    source.gamecore_executable = gamecore;
    source.gamecore_sha256 = result.gamecore_sha256;
    source.context.decode_map_regions = true;
    source.context.decode_teams = true;
    source.context.decode_all_players = true;
    source.context.decode_supporting_classes = true;
    source.context.all_players = saves::AllPlayersContext{};
    if (civis_preflight) {
        const auto &c = *civis_preflight;
        source.context.permit_live_production_overlay = c.live_city_production.has_value();
        source.context.permit_live_research_overlay = c.live_team_research.has_value();
        source.context.permit_live_treasury_overlay = c.live_treasury.has_value();
        source.context.permit_live_research_turn_overlay = c.live_research_turn.has_value();
        source.context.permit_live_research_selection_overlay = c.live_research_selection.has_value();
        source.context.permit_live_random_overlay = c.live_simulation_random.has_value();
        source.context.permit_live_bnw_game_clock_overlay = c.live_bnw_game_clock.has_value();
        source.context.permit_live_bnw_game_supporting_overlay =
            c.live_bnw_game_supporting.has_value();
        source.context.permit_live_bnw_game_score_replay_overlay =
            c.live_bnw_game_score_replay.has_value();
        source.context.permit_live_bnw_game_deals_overlay =
            c.live_bnw_game_deals.has_value();
        source.context.permit_live_bnw_team_turn_overlay =
            c.live_bnw_team_turn.has_value();
        source.context.permit_live_bnw_ui_transients_overlay = c.live_bnw_ui_transients.has_value();
        source.context.permit_live_bnw_map_plots_overlay = c.live_bnw_map_plots.has_value();
        source.context.permit_live_bnw_units_overlay = c.live_bnw_units.has_value();
        source.context.permit_live_bnw_unit_serial_overlay = c.live_bnw_unit_serial.has_value();
        source.context.permit_live_bnw_food_growth_original_decode = c.live_bnw_food_growth_journal.has_value();
        source.context.permit_live_bnw_area_units_overlay = c.live_bnw_area_units.has_value();
        source.context.permit_live_bnw_player_proximity_overlay =
            c.live_bnw_player_proximity.has_value();
        source.context.permit_live_bnw_team_meet_overlay = c.live_bnw_team_meet.has_value();
        source.context.permit_live_bnw_exploration_dirty_overlay =
            c.live_bnw_exploration_dirty.has_value();
        source.context.permit_live_bnw_player_strategy_overlay =
            c.live_bnw_player_strategy.has_value();
        source.context.permit_live_bnw_player_core_overlay = c.live_bnw_player_core.has_value();
        source.context.permit_live_bnw_city_core_overlay = c.live_bnw_city_core.has_value();
        source.context.permit_live_bnw_city_production_overlay =
            c.live_bnw_city_production.has_value();
        source.context.permit_live_bnw_city_management_overlay =
            c.live_bnw_city_management.has_value();
        source.context.permit_live_bnw_city_tail_overlay = c.live_bnw_city_tail.has_value();
        source.context.permit_live_bnw_treasury_balance_overlay =
            c.live_bnw_treasury_balance.has_value();
        source.context.permit_live_bnw_team_progress_overlay =
            c.live_bnw_team_progress.has_value();
        source.context.permit_live_bnw_culture_bomb_timer_overlay =
            c.live_bnw_culture_bomb_timer.has_value();
        source.context.permit_live_bnw_research_queue_overlay =
            c.live_bnw_research_queue.has_value();
        source.context.permit_live_bnw_maintenance_expense_overlay =
            c.live_bnw_maintenance_expense.has_value();
        source.context.permit_live_bnw_player_knowledge_overlay =
            c.live_bnw_player_knowledge.has_value();
        source.context.permit_live_bnw_player_administration_overlay =
            c.live_bnw_player_administration.has_value();
        civis_preflight.reset();
    }
    try {
        if (runtime::identify_file(save_path).sha256 != selected_save.sha256)
            return fail(SelectedBnwPreviewStage::selected_save,
                        "selected save changed during BNW content activation");
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::selected_save,
                    std::string("selected save cannot be rechecked: ") + e.what());
    }
    if(completed) {
        completed->current();
        if(!final_art_fingerprint)
            return fail(SelectedBnwPreviewStage::activation,"Completed BNW private art lineage output missing");
        *final_art_fingerprint=database.gameplay_fingerprint_sha256();
    } else if (civis) {
        civis->current();
        if (!final_art_fingerprint)
            return fail(SelectedBnwPreviewStage::activation, "Civis private art lineage output missing");
        *final_art_fingerprint = database.gameplay_fingerprint_sha256();
    } else if (early_package) {
        early_package->early_input_lease()->require_current();
        if (!final_art_fingerprint)
            return fail(SelectedBnwPreviewStage::activation, "BNW private art lineage output missing");
        *final_art_fingerprint = database.gameplay_fingerprint_sha256();
    }
    if(preparation_output){
        return preparation_output->consume(std::move(activated_database),std::move(retained_rules_mounts),
            std::move(result),std::move(source),selected_save,header);
    }
    result.preview = make_selected_bnw_preview_host(save_path, database,
        *result.activation, *result.postprocess, source, *active_player_id,
        immutable_preview_fingerprints);
    try {
        if (runtime::identify_file(save_path).sha256 != selected_save.sha256)
            return fail(SelectedBnwPreviewStage::selected_save,
                        "selected save changed during BNW live restore");
    } catch (const std::exception &e) {
        return fail(SelectedBnwPreviewStage::selected_save,
                    std::string("selected save cannot be rechecked after restore: ") + e.what());
    }
    result.original_source_file_verified = !civis_container(save_path) &&
        static_cast<bool>(result.preview);
    return result;
}

void SelectedBnwPreviewHost::require_same_early_package(
    const BnwNativePackageDiscoveryOwner& package) const {
    // Existing immutable creator identity precedes completed mutable fields.
    if(saved_database_thread_!=std::this_thread::get_id())
        throw std::runtime_error("BNW adopted host foreign creator thread");
    if(completed_observation_) {
        // Retain all authentic storage before provider/file currentness. No
        // field of this is read after those potentially failing bookends.
        const auto actual_package=early_package_;const auto resources=completed_resources_;
        const auto observation=completed_observation_;const auto color=completed_color_query_;
        const auto read=completed_selected_read_;const auto art=completed_unit_art_inputs_;
        const auto epoch=completed_input_epoch_;const auto database=database_;
        const auto fingerprint=early_art_final_fingerprint_;
        if(!actual_package||actual_package.get()!=&package||!resources||!color||!read||!art||!epoch||
           !database||fingerprint.empty())
            throw std::runtime_error("BNW completed host has no SAME actual retained input/clone lineage");
        observation->require_same_parent(package);resources->require_identity_current();
        color->require_same_parent(package);read->require_same_parent(package);art->require_same_parent(package);
        observation->with_actual_parent_loan([&](const auto& loan){
            loan.with_actual_pinned_input_epoch([&](const BnwNativePinnedInputEpoch& actual){
                if(&actual!=epoch.get())throw std::runtime_error("BNW completed host actual Epoch relation differs");
                actual.require_current();loan.require_current();
            });
            loan.require_current();
        });
        if(!database->immutable_preview_fingerprints_current()||
           database->gameplay_fingerprint_sha256()!=fingerprint)
            throw std::runtime_error("BNW adopted final completed art/rules lineage changed");
        epoch->require_current();observation->require_same_parent(package);return;
    }
    if (!early_package_ || early_package_.get() != &package || early_art_final_fingerprint_.empty())
        throw std::runtime_error("BNW host has no SAME privately adopted early package/art lineage");
    early_package_->early_input_lease()->require_current();
    if (!database_->immutable_preview_fingerprints_current() ||
        database_->gameplay_fingerprint_sha256() != early_art_final_fingerprint_)
        throw std::runtime_error("BNW adopted final art/rules lineage changed");
}


std::shared_ptr<const SelectedBnwPreviewHost>
SelectedBnwPreviewHost::retain_civis_resource_host_owned() const {
    if (saved_database_thread_ != std::this_thread::get_id())
        throw std::runtime_error("Civis selected host owner belongs to another creator thread");
    const auto actual = civis_canonical_self_.lock();
    if (!actual || actual.get() != this || actual.owner_before(civis_canonical_self_) ||
        civis_canonical_self_.owner_before(actual) || !civis_resource_bank_)
        throw std::runtime_error("Civis selected host canonical factory owner retired");
    require_same_civis_resources(*civis_resource_bank_);
    if (actual.get() != this || actual.owner_before(civis_canonical_self_) ||
        civis_canonical_self_.owner_before(actual))
        throw std::runtime_error("Civis selected host canonical factory control changed");
    require_same_civis_resources(*civis_resource_bank_);
    return actual;
}
bool SelectedBnwPreviewHost::uses_civis_resource_bank() const {
    if (saved_database_thread_ != std::this_thread::get_id())
        throw std::runtime_error("Civis resource lane queried from another creator thread");
    if (civis_resource_bank_) civis_resource_bank_->require_current();
    return static_cast<bool>(civis_resource_bank_);
}

void SelectedBnwPreviewHost::require_same_civis_resources(
    const assets::CivisResourceBankOwner& bank) const {
    if (saved_database_thread_ != std::this_thread::get_id())
        throw std::runtime_error("Civis selected host resource lineage belongs to another thread");
    if (!civis_resource_bank_ || civis_resource_bank_.get() != &bank ||
        early_package_ || completed_resources_ || completed_observation_ ||
        civis_art_final_fingerprint_.empty() || civis_resource_recipe_identity_.empty())
        throw std::runtime_error("Civis selected host has no same own-bank lineage");
    civis_resource_bank_->require_current();
    if (!database_->immutable_preview_fingerprints_current() ||
        database_->gameplay_fingerprint_sha256() != civis_art_final_fingerprint_)
        throw std::runtime_error("Civis selected host final art/rules clone lineage changed");
    civis_resource_bank_->require_current();
}
SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_civis_resources(
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied,
    std::int64_t active_player_id) {
    if (!supplied) throw std::invalid_argument("Civis selected host requires an actual resource bank");
    const auto bank = supplied->retain_owner();
    if (!bank || bank.get() != supplied.get() || bank.owner_before(supplied) ||
        supplied.owner_before(bank))
        throw std::invalid_argument("Civis selected host requires the canonical bank control block");
    bank->require_boundary();
    // Exact documents used by the existing five-table base art compiler.
    // Repeated compiler reads now consume the bank's immutable actual bytes.
    for (const auto* key : {"Assets/Units/Civ5ArtDefines_UnitMembers.xml",
                           "Assets/Units/Civ5ArtDefines_Units.xml",
                           "Assets/Units/Civ5ArtDefines_UnitMemberTemplates.xml"})
        bank->load(assets::AssetKey(key), assets::CivisResourceParse::xml);
    const CivisBankHostBootstrapInputs actual(bank);
    actual.current();
    std::string final_art_fingerprint;
    auto result = bootstrap_selected_bnw_preview_impl(
        actual.recipe.save_path, actual.recipe.game_root, active_player_id,
        true, {}, &final_art_fingerprint, nullptr, &actual);
    if (!result.preview) return result;
    actual.current();
    auto& host = *result.preview.host;
    if (final_art_fingerprint.empty() || !host.database_->immutable_preview_fingerprints_current() ||
        host.database_->gameplay_fingerprint_sha256() != final_art_fingerprint)
        throw std::runtime_error("Civis actual BASE-to-final rules clone differs");
    host.civis_resource_bank_ = bank;
    host.civis_art_final_fingerprint_ = std::move(final_art_fingerprint);
    host.civis_resource_recipe_identity_ = actual.recipe.recipe_identity;
    host.civis_canonical_self_ = result.preview.host;
    host.require_same_civis_resources(*bank);
    host.stage_globals_cache_source_owner();
    actual.current();
    host.require_same_civis_resources(*bank);
    return result;
}

SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_early_package(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package, std::int64_t active_player_id) {
    if (!package) throw std::invalid_argument("BNW early package is missing");
    // Retain the actual factory facade before any Early/provider callbacks.
    // Equal raw addresses do not prove the same shared ownership authority.
    const auto canonical_package = package->retain_factory_owner();
    if (!canonical_package || canonical_package.get() != package.get() ||
        canonical_package.owner_before(package) || package.owner_before(canonical_package))
        throw std::runtime_error("BNW host adoption requires SAME canonical package control block");
    package = canonical_package;
    package->require_selected_host_adoption_available();
    const auto lease = package->early_input_lease();
    lease->require_current();
    const auto& frame = package->current();
    if (!frame.primary_dx9_registration_complete || !frame.common_high_quality_registration_complete)
        throw std::runtime_error("BNW early source Common registration is incomplete");
    std::string final_art_fingerprint;
    auto result = bootstrap_selected_bnw_preview_impl(lease->save_path(), lease->game_root(),
        active_player_id, true, package, &final_art_fingerprint);
    if (!result.preview) return result;
    // This is the sole adoption site. Both final clone and fingerprint were
    // produced inside this ordered call; callers cannot bind an equal DB/host.
    lease->require_current();
    auto& host = *result.preview.host;
    if (final_art_fingerprint.empty() || !host.database_->immutable_preview_fingerprints_current() ||
        host.database_->gameplay_fingerprint_sha256() != final_art_fingerprint)
        throw std::runtime_error("BNW real BASE-to-final art clone lineage differs");
    host.early_art_final_fingerprint_ = std::move(final_art_fingerprint);
    host.early_package_ = std::move(package);
    host.require_same_early_package(*host.early_package_);
    host.stage_globals_cache_source_owner();
    host.early_package_->consume_selected_host_adoption(host);
    static_assert(std::is_nothrow_move_constructible_v<SelectedBnwBootstrapResult>);
    return result;
}


const std::shared_ptr<const detail::BnwNativeCompletedParentObservation>&
SelectedBnwPreviewHost::completed_observation_without_callbacks()const {
    if(saved_database_thread_!=std::this_thread::get_id())
        throw std::runtime_error("BNW completed Host discriminator foreign creator thread");
    return completed_observation_;
}

SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_completed_resources(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> resources,
    std::shared_ptr<const detail::BnwNativeCompletedColorQuery> color,
    std::shared_ptr<const detail::BnwNativeCompletedSelectedRead> read,
    std::int64_t active_player_id) {
    if(!package)throw std::invalid_argument("BNW completed package is missing");
    // Genuine canonical package relation BEFORE any provider/file callbacks.
    const auto canonical_package=package->retain_factory_owner();
    if(!canonical_package||canonical_package.get()!=package.get()||
       canonical_package.owner_before(package)||package.owner_before(canonical_package))
        throw std::runtime_error("BNW completed adoption requires SAME canonical Parent control");
    package=canonical_package;
    // Parent103 proves genuine cap/read factory controls before supplied read
    // currentness; it then mints the closed native-winner art input, not a table.
    const auto art=package->capture_native_owned_completed_unit_art_inputs(resources,read);
    if(!color)throw std::invalid_argument("BNW completed Color input is missing");
    const auto canonical_color=color->retain_owner();
    if(!canonical_color||canonical_color.get()!=color.get()||canonical_color.owner_before(color)||
       color.owner_before(canonical_color))
        throw std::runtime_error("BNW completed adoption requires canonical Color input");
    color=canonical_color;color->require_same_parent(*package);
    const auto canonical_read=read->retain_owner();
    if(canonical_read.get()!=read.get()||canonical_read.owner_before(read)||read.owner_before(canonical_read))
        throw std::runtime_error("BNW completed adoption requires canonical read input");
    read=canonical_read;
    const auto observation=package->capture_native_owned_completed_parent_observation(resources);
    package->require_selected_host_adoption_available_completed(*observation);
    CompletedHostBootstrapInputs actual;
    actual.package=package;actual.resources=resources;actual.observation=observation;
    actual.color=color;actual.read=read;actual.art=art;
    resources->with_actual_pinned_input_epoch([&](const auto& epoch){
        if(!epoch||actual.epoch)throw std::runtime_error("BNW completed bootstrap actual Epoch absent/replayed");
        actual.epoch=epoch;
    });
    // Copy only from the actual SAME loan/Epoch. Close this loan BEFORE native
    // read/Art/Host guards, which may open their own completed source operation.
    observation->with_actual_parent_loan([&](const auto& loan){
        loan.with_actual_pinned_input_epoch([&](const BnwNativePinnedInputEpoch& epoch){
            if(&epoch!=actual.epoch.get())throw std::runtime_error("BNW completed bootstrap Epoch source differs");
            actual.save_path=epoch.save_path();actual.game_root=epoch.game_root();
            actual.catalog=epoch.catalog();actual.profile=epoch.profile();loan.require_current();
        });
        loan.require_current();
    });
    actual.current();std::string final_art_fingerprint;
    auto result=bootstrap_selected_bnw_preview_impl(actual.save_path,actual.game_root,
        active_player_id,true,{},&final_art_fingerprint,&actual);
    if(!result.preview)return result;
    actual.current();auto& host=*result.preview.host;
    if(final_art_fingerprint.empty()||!host.database_->immutable_preview_fingerprints_current()||
       host.database_->gameplay_fingerprint_sha256()!=final_art_fingerprint)
        throw std::runtime_error("BNW completed real BASE-to-final art clone lineage differs");
    // Sole closed adoption site; real Host, not a caller-supplied equal clone.
    host.early_art_final_fingerprint_=std::move(final_art_fingerprint);host.early_package_=package;
    host.completed_resources_=resources;host.completed_observation_=observation;
    host.completed_color_query_=color;host.completed_selected_read_=read;
    host.completed_unit_art_inputs_=art;host.completed_input_epoch_=actual.epoch;
    host.require_same_early_package(*package);host.stage_globals_cache_source_owner();
    actual.current();package->consume_selected_host_adoption_completed(host,*observation);
    static_assert(std::is_nothrow_move_constructible_v<SelectedBnwBootstrapResult>);
    return result;
}

} // namespace civ5::ui

namespace civ5::ui {
void SelectedBnwPreviewHost::stage_processed_auto_moves_original_read(std::shared_ptr<const SelectedBnwPreviewHost>actual){
 if(saved_database_thread_!=std::this_thread::get_id())throw std::runtime_error("ProcessedAutoMoves Host foreign factory creator");
 if(!actual||actual.get()!=this||bnw_processed_auto_moves_import_||!activation_owner_||!session_||
    session_->source().source.container!=saves::SaveContainerKind::civ5)
  throw std::runtime_error("ProcessedAutoMoves requires actual shared fresh original Host factory");
 // Called only by the original make_selected factory's genuine shared_ptr
 // allocation. This is independent of later civis_canonical_self_ publication.
 auto seal=std::unique_ptr<runtime::BnwProcessedAutoMovesImportOwner>(new runtime::BnwProcessedAutoMovesImportOwner(std::move(actual)));
 bnw_processed_auto_moves_import_=std::move(seal);
}
}

namespace civ5::ui {
void SelectedBnwPreviewHost::restore_food_growth_before_publication(const std::shared_ptr<SelectedBnwPreviewHost>& canonical){
 if(saved_database_thread_!=std::this_thread::get_id()||!canonical||canonical.get()!=this||!session_||activation_owner_||!bnw_food_canonical_self_.expired())
  throw std::runtime_error("Food47 join requires the true unpublished canonical Host creator");
 bnw_food_canonical_self_=canonical;
 const auto& in=session_->source().source;
 const auto part=[](std::uint32_t id,const auto& value){if(!value)throw std::runtime_error("Food47 actual input companion missing");return saves::CivisSaveSection{id,0,*value};};
 runtime::BnwPopulationTotalsCheckpointBundle b{part(46,in.live_bnw_population_totals),part(24,in.live_bnw_player_core),{part(25,in.live_bnw_city_core),part(26,in.live_bnw_city_production),part(27,in.live_bnw_city_management),part(28,in.live_bnw_city_tail)}};
 (void)runtime::BnwCityFoodGrowthPending::restore_growth_before_plot_from_unpublished_host(canonical,part(47,in.live_bnw_food_growth_journal),b,part(16,in.live_bnw_game_clock),part(13,in.live_simulation_random));
 require_restored_food_growth_publication_current();
}
void SelectedBnwPreviewHost::require_restored_food_growth_current()const{
 if(saved_database_thread_!=std::this_thread::get_id()||!session_)throw std::runtime_error("Food47 Host foreign creator/session");
 if(!session_->source().source.live_bnw_food_growth_journal)return;
 if(session_->has_bnw_food_growth_import_quarantine())throw std::runtime_error("Food47 Host actual once owner remains quarantined");
 const auto real=bnw_food_canonical_self_.lock();if(!real||real.get()!=this||real.owner_before(bnw_food_canonical_self_)||bnw_food_canonical_self_.owner_before(real))throw std::runtime_error("Food47 canonical Host history expired");
}
void SelectedBnwPreviewHost::require_restored_food_growth_publication_current()const{
 require_restored_food_growth_current();const auto& bytes=session_->source().source.live_bnw_food_growth_journal;if(!bytes)return;
 // The owner lookup is private. Never reconstruct a Pending from wire operands.
 const auto record=runtime::BnwFoodPopulationGrowthCheckpoint::decode({47,0,*bytes});
 const auto pending=session_->bnw_city_food_growth_pending(record.actor,record.city);
 if(!pending)throw std::runtime_error("Food47 registered restored parent absent");pending->require_restored_growth_publication_current();
}
}

namespace civ5::ui {
#include "detail/civis_selected_bnw_rules_preparation_core.inc"
}
