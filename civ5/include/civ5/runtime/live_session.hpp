#pragma once
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_dirty_event.hpp"
#include "civ5/runtime/bnw_plot_gameplay_event.hpp"

#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/bnw_nearby_cities.hpp"
#include "civ5/runtime/bnw_city_flavor_recipients.hpp"
#include "civ5/runtime/bnw_city_strategy_runtime.hpp"
#include "civ5/runtime/bnw_player_flavor_recipients.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/bnw_adjacent_land.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/bnw_diplomacy_scratchpad_load.hpp"
#include "civ5/runtime/bnw_notification_load_transients.hpp"
#include "civ5/runtime/bnw_player_plot_list_load.hpp"
#include "civ5/runtime/bnw_player_read_tail_transients.hpp"
#include "civ5/runtime/bnw_game_plot_extras_load.hpp"
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/production_progress.hpp"
#include "civ5/runtime/bnw_city_production_turn.hpp"
#include "civ5/runtime/research_selection.hpp"
#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/science.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/runtime/treasury.hpp"
#include <atomic>
#include <stdexcept>
#include <cstdint>
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <thread>
#include <vector>

namespace civ5::ui {class SelectedBnwPreviewHost;class BnwClientSelectionControl;}
namespace civ5::ui {class BnwPlotYieldRefreshUiAdapter;class BnwCityProductionUiAdapter;}
namespace civ5::runtime {

class BnwResearchChoiceCommit;class BnwSelectedStockLuaSourceOwner;class BnwMajorPlayerTraitsOwner;
class BnwNativeUnitAllocatorCore;class BnwLiveGameContext;class BnwTwoLayerUnitPathFinder;class BnwPlayerTurnRuntimeOwner;class BnwUnitPathOwner;
class BnwMajorTeamHasTechPending;
class BnwUnitMovePending;class BnwUnitMoveRegistry;class BnwRetainedTurnCursor;
class BnwCommonPathfinderUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
class BnwCityFoodGrowthPending;class BnwFoodGrowthImportAdmission;class BnwCityFoodGrowthRegistry;
class BnwImportedPregameReadCache;
class BnwImportedPregameLocaleOwner;
class BnwNormalPlotYieldRules;
class BnwDisplayedPlotYield;
class BnwCityCurrentFoodRate;
struct BnwPlotTerrainCheckpointPair;
struct BnwPlotTerrainRestoredComponents;
class BnwPlotTerrainRulesSnapshot;
class BnwPlotYieldRefreshIssuer;
class BnwPlotYieldRefreshPending;
class BnwPlotYieldRefreshCheckpoint;
class BnwCityYieldRefresh;class BnwCityYieldRefreshPending;
class BnwCityYieldRefreshRegistry;class BnwCityYieldRefreshCheckpoint;

// Rules are borrowed only while restoring. The session owns its source and
// mutable world, but never a second, potentially divergent rules database.
struct LiveSessionImportRules {
    const content::RulesDatabase &database;
    const content::InfoTypeCatalog &types;
    const content::TechnologyRules &technologies;
    const content::GlobalDefineCache &defines;
    const CityYieldRules &city_yields;
    const ProductionProgressRules &production;
    const MaintenanceRules &maintenance;
    const TreasuryRules &treasury;
    SessionCheckpointIdentity identity;
    std::shared_ptr<const BnwImportedPregameLocaleOwner> actual_early_pregame_locale;
};

struct BnwGameClockPublication {
    std::int64_t turn=0;
    std::uint8_t next_step=0;
    // A throwing external callback may already have performed its effect.
    // Do not replay it until the host reconciles that uncertain outcome.
    bool uncertain=false;
};
struct BnwGameClockState {
    bool sent_auto_moves=false;
    std::optional<BnwGameClockPublication> pending;
};

struct BnwPlayerGoldPublication {
    std::int64_t player=-1;
    std::vector<std::uint8_t> dirty_kinds;
    std::size_t next_step=0;
    bool uncertain=false;
};
struct BnwPlayerResearchPublication {
    std::int64_t player=-1;
    std::vector<std::pair<std::int64_t,std::uint8_t>> dirty;
    std::size_t next_step=0;
    bool uncertain=false;
};
struct BnwPlayerEarlyPublication {
    std::int64_t player=-1;
    std::uint8_t dirty_kind=0;
    bool uncertain=false;
};

struct LiveSessionState {
    // The decoded source is copied so future map, city, unit and game writers
    // have an owned rollback target. The original source remains immutable.
    saves::SavedSessionSnapshot world;
    ResearchContext setup;
    std::vector<TeamTechnologies> technologies;
    PlayerResearchContext player_research;
    // CityYieldContext is authoritative for live city population and yields;
    // this owns BNW's additional happiness inputs needed to refresh its cache.
    CityYieldContext cities;
    BnwUnhappinessContext unhappiness;
    MaintenanceContext maintenance;
    TreasuryContext treasury;
    ScienceContext science;
    ResearchTurnState research_turn;
    SessionRandomStreams random;
    // Source civilization restrictions and advanced-start selection inputs.
    // Kept with the queue/flag owners for transactional research choices.
    ResearchSelectionState research_selection;
    BnwGameClockState game_clock;
    // Engine UI invalidations are staged by DoGold on detached state. A host
    // publishes them only after the enclosing action/player turn commits.
    std::optional<BnwPlayerGoldPublication> player_gold_publication;
    std::optional<BnwPlayerResearchPublication> player_research_publication;
    std::optional<BnwPlayerEarlyPublication> player_early_publication;
    // Derived CvCityManager proximity index. Rebuilt after BNW city load;
    // invalidated by arbitrary state writes and committed live actions.
    BnwNearbyCityCache nearby_cities;
    // Derived CvCity::read registrations. Native flavor dispatch remains a
    // separate unimplemented GameCore service.
    BnwCityFlavorRecipients city_flavor_recipients;
    // Opaque actual Init/Reset/Read arrays. Only registered CityEntry publishes
    // the paired raw/owned transition; no public constructor or apply grant.
    std::map<std::pair<std::int32_t,std::int32_t>,std::shared_ptr<const BnwCityStrategyRuntime>> city_strategy_runtime;
    // Derived CvPlayer::Read registrations; flavor dispatch remains blocked.
    BnwPlayerFlavorRecipients player_flavor_recipients;
    // CvPlot::read derives impassability from active terrain/feature rules.
    BnwLivePlotImpassability plot_impassability;
    // CvMap::Read restores this unserialized pathfinding input.
    BnwAdjacentLandCache adjacent_land;
    // CvPlot::read clamps source visibility counters before gameplay queries.
    BnwLivePlotVisibility plot_visibility;
    // CvDiplomacyAI::Reset recreates this transient array during Player::Read.
    // Any arbitrary write or committed action invalidates it until the exact
    // AI turn update and dependent load hooks are implemented.
    BnwDiplomacyScratchpadLoad diplomacy_scratchpad;
    // Derived operator>> flags for all loaded BNW notification slots.
    BnwNotificationLoadTransients notification_load_transients;
    // CvPlayer::Read pads its saved plot-list prefix to the complete map.
    BnwPlayerPlotListLoad player_plot_lists;
    // Unsaved CvDiplomacyRequests::Init state and active-turn count query.
    BnwPlayerReadTailTransients player_read_tail;
    // CvGame::Read restores source-ordered plot yield and movement cost extras.
    BnwGamePlotExtras game_plot_extras;
    // CvPlayer::Read calls reset(), which sets this field to zero but does
    // not read it from the BNW stream. Keep it outside decoded wire fields.
    std::map<std::int64_t,std::int32_t> player_culture_bomb_timers;
    // CvMilitaryAI::ResetCounters runs after player trade and before Grand
    // Strategy AI. The two request counters are not serialized by BNW.
    struct BnwMilitaryRequestCounters {
        std::int32_t land=0,naval=0;
        bool operator==(const BnwMilitaryRequestCounters&) const = default;
    };
    std::map<std::int64_t,BnwMilitaryRequestCounters> military_request_counters;
    // Append defaulted extensions after the legacy aggregate prefix.
    // Source-ordered renderer notifications; no renderer readiness permission.
    std::vector<BnwCityDirtyEvent> city_dirty_events;
    std::uint64_t city_dirty_next_occurrence=1;
    // Actual engine setter queue. Occurrences share the city setDirty order.
    std::vector<BnwSpecificCityInfoEvent> specific_city_info_events;
    // Actual source stores/notifications invalidate captured frames immediately.
    // This is owned mutation metadata, not a supplied freshness permission.
    std::uint64_t ui_source_revision=1;
    // Real typed GameplayYieldMightHaveChanged values; CP retains data, not
    // renderer completion. Old generation requires explicit reconciliation.
    std::vector<BnwPlotGameplayEvent> plot_gameplay_events;
    std::uint64_t plot_gameplay_next_occurrence=1;
    // Standalone configuration is persisted independently of original GameCore
    // flags. This does not remove any gameplay/persistence readiness blocker.
    saves::StandaloneServicesConfig standalone_services;
    // Immutable exact section43 bytes retained across snapshots/rollback. This
    // is unresolved imported journal data, not a resumed native Unit owner.
    std::shared_ptr<const saves::CivisSaveSection> bnw_unit_mission_continuation_quarantine = {};
};

enum class LiveSessionBlocker {
    bnw_query_caches_dirty,
    load_hooks_unimplemented,
    gamecore_services_unimplemented,
    full_turn_unimplemented,
    full_persistence_unimplemented,
    lua_turn_entrypoints_unimplemented,
    bnw_nearby_city_pointer_order_unavailable,
    bnw_unit_mission_continuation_restore_required,
};

struct LiveSessionReadiness {
    std::vector<LiveSessionBlocker> blockers;
    bool can_continue() const noexcept { return blockers.empty(); }
};

struct LiveActionResult;

// Import and checkpoints are diagnostic foundations, not a playable turn API.
// A future turn scheduler must check readiness before accepting commands.
class LiveSession {
    // Only restore_bnw executes this unsaved CvGame::uninit/Read prefix.
    // Generic decoded worlds cannot manufacture it. Raw and cached fields
    // remain separate; a future toggle must own every visibility/UI effect.
    struct BnwGameDebugReadState {
        SessionCheckpointIdentity identity;
        bool raw_mode, cached_mode;
        explicit BnwGameDebugReadState(const SessionCheckpointIdentity& source)
            : identity(source), raw_mode(false), cached_mode(false) {}
    };
    // Actual owned Game fields, never an issuer's copied debug bool. The
    // checkpoint retains the separate VALUE snapshot above, not this owner.
    struct BnwGameDebugReadStorage final:BnwGameDebugReadState {
        const std::thread::id thread=std::this_thread::get_id();
        std::atomic<bool> retired{false};
        explicit BnwGameDebugReadStorage(BnwGameDebugReadState value)
            : BnwGameDebugReadState(std::move(value)) {}
    };
  public:
    class Checkpoint {
      public:
        Checkpoint(const Checkpoint &) = default;
        Checkpoint &operator=(const Checkpoint &) = default;
      private:
        friend class LiveSession;
        std::map<BnwCityProductionTurnKey,std::shared_ptr<const BnwCityProductionTurnCheckpoint>> production_turn_pending_;
        Checkpoint(std::uint64_t owner, const LiveSessionState &state, bool happiness_current,
                   const std::optional<BnwGameDebugReadState>& debug)
            : owner_(owner), state_(state), happiness_current_(happiness_current), debug_(debug) {}
        std::uint64_t owner_;
        LiveSessionState state_;
        bool happiness_current_;
        std::optional<BnwGameDebugReadState> debug_;
        std::map<std::int64_t,std::shared_ptr<const BnwPlotYieldRefreshCheckpoint>>
            plot_yield_refresh_pending_;
        std::shared_ptr<const BnwCityYieldRefreshCheckpoint> city_yield_refresh_;
        std::map<BnwCityEntrySourceKey,std::shared_ptr<const BnwCityEntryCheckpoint>>
            city_entry_pending_;
    };

    LiveSession(const LiveSession &) = delete;
    LiveSession &operator=(const LiveSession &) = delete;
    LiveSession(LiveSession &&) noexcept;
    LiveSession &operator=(LiveSession &&) noexcept;
    ~LiveSession();

    // Restores all currently available common BNW contexts and validates each
    // supported Civis overlay before exposing any mutable state. Unsupported
    // profile sections and incomplete source snapshots fail explicitly.
    // Authentic data-only Read/Sync and supported overlay import. This entry
    // never creates a local SP client or lends gameplay-client authority.
    static LiveSession restore_bnw(saves::SavedSession source,
                                   const LiveSessionImportRules &rules);
    // Full imported-game entry: genuine early Bank/Locale and the complete
    // local SP startup are mandatory before native Pregame Read.
    static LiveSession restore_bnw_imported_game(saves::SavedSession source,
                                   const LiveSessionImportRules &rules);
    // For independently restored BNW worlds; still diagnostic-only.
    static LiveSession from_restored_bnw(saves::SavedSession source,
                                         LiveSessionState state);

    const saves::SavedSession &source() const noexcept { return *source_; }
    const LiveSessionState &state() const noexcept { return *state_; }
    // Pins this allocation across synchronous callbacks that can replace state.
    // It does not freeze in-place writes or authenticate a current publication;
    // readers must independently check generation and their semantic owners.
    std::shared_ptr<const LiveSessionState> read_state_lease() const noexcept { return state_; }
    // Successful state replacement advances this token even when allocation
    // reuses an old address or restores identical values. In-place writes still
    // require their semantic owner checks; this is not a complete world revision.
    std::uint64_t state_generation() const noexcept { return state_generation_; }
    // Conservative partial persistence guard: even an issued empty registry
    // requires its separate lifecycle codec before an import-clock export.
    bool has_bnw_operation_checkpoint_history() const noexcept {
        return bnw_retained_turn_history_ || static_cast<bool>(plot_yield_refresh_issuer_) ||
            !plot_yield_refresh_pending_.empty() || !city_entry_pending_.empty() ||
            !production_turn_pending_.empty() || static_cast<bool>(city_yield_refresh_registry_) ||
            static_cast<bool>(unit_move_registry_) || static_cast<bool>(food_growth_registry_);
    }
    // Read the current unsaved native Game cache, or refuse missing/stale
    // lifecycle authority. No caller bool or arbitrary reset receipt exists.
    bool bnw_game_debug_mode() const;
    // Read-only diagnostic of the actual original-import control plane.
    // Missing/retired/generic/checkpoint worlds refuse; never inferred false.
    bool bnw_common_pathfinder_force_reset() const;
    // Partial native-member diagnostic only, not mission/path/combat admission.
    // Queued dim intent is not completed original synchronous fanout.
    std::shared_ptr<const BnwUnitMovePending> begin_bnw_unit_move_prefix(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::pair<std::int64_t,std::int64_t>,std::int32_t x,std::int32_t y,bool show);
    // Conservative invalidation: arbitrary state writes can change the inputs
    // of query caches. Call rebuild_bnw_unhappiness_cache before using them.
    LiveSessionState &mutable_state() noexcept {
        retire_bnw_unit_move();
        retire_bnw_food_growth();
        retire_bnw_pathfinder();
        retire_plot_yield_refresh();
        retire_bnw_city_entry();
        retire_bnw_production_turn();
        state_->city_strategy_runtime.clear();
        retire_bnw_client_selection();
        retire_bnw_game_debug_owner();
        bnw_unhappiness_current_=false;
        state_->nearby_cities.current=false;
        state_->nearby_cities.order_gap.reset();
        state_->city_flavor_recipients.current=false;
        state_->player_flavor_recipients.current=false;
        state_->plot_impassability.current=false;
        state_->adjacent_land.current=false;
        state_->plot_visibility.current=false;
        state_->diplomacy_scratchpad.invalidate();
        state_->notification_load_transients.current=false;
        state_->player_plot_lists.current=false;
        state_->player_read_tail.current=false;
        state_->game_plot_extras.current=false;
        return *state_;
    }
    LiveSessionReadiness readiness() const;
    // Routing/refusal observation only. No caller can clear the source journal
    // or issue a completed native continuation through this API.
    bool has_bnw_unit_mission_continuation_quarantine() const;
    bool has_bnw_food_growth_import_quarantine() const;
    bool bnw_unhappiness_current() const noexcept { return bnw_unhappiness_current_; }
    void rebuild_bnw_unhappiness_cache(const BnwUnhappinessRules &rules,
                                       const content::GlobalDefineCache &defines);

    // Actual early-zero calculateYield branch only. Stores source plot/city
    // caches in order and retains a once cursor before mandatory UI delivery.
    // Returned const owner provides diagnostics, not a public acknowledgement.
    std::shared_ptr<const BnwPlotYieldRefreshPending>
        begin_bnw_plot_early_zero_yield_refresh(std::int64_t plot);
    // Actual supported normal calculation; the SAME once cursor and mandatory
    // UI/symbol effects. Identity comes from the private current import owner.
    std::shared_ptr<const BnwPlotYieldRefreshPending>
        begin_bnw_plot_normal_yield_refresh(std::int64_t plot,
                                            const BnwNormalPlotYieldRules& rules);
    std::shared_ptr<const BnwPlotYieldRefreshPending>
        bnw_plot_yield_refresh_pending(std::int64_t plot) const;

    // Genuine imported standalone component: damage==0 native skip then three
    // city-entry reset calls. No player/city scheduler predecessor or growth
    // completion is issued. Retained once slot precedes the first raw store.
    std::shared_ptr<const BnwCityEntryPending>
        begin_bnw_city_entry_damage_zero(std::int64_t owner,std::int64_t city);
    // Continues ONLY an existing genuine retained city-entry predecessor.
    // This endpoint never creates/replays the three reset calls.
    std::shared_ptr<const BnwCityEntryPending>
        continue_bnw_city_entry_feature_surrounded(std::int64_t owner,std::int64_t city,
            const BnwCityEntryFeatureRules&);

    // The exact registered Pending pointer chooses its private current ID.
    // No caller strategy ID, trigger boolean, Lua result or resume flag exists.
    std::shared_ptr<const BnwCityEntryPending> continue_bnw_city_entry_strategy(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::shared_ptr<const BnwCityEntryPending>);

    // Only the same registered strategy-loop owner may enter native Citizens.
    std::shared_ptr<const BnwCityEntryPending> continue_bnw_city_citizens_verify(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::shared_ptr<const BnwCityEntryPending>);

    // Imported component only: native human/manual/suppressed MAINTAIN branch.
    std::shared_ptr<const BnwCityProductionTurnPending> begin_bnw_city_production_process_suppressed(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
    std::shared_ptr<const BnwCityProductionTurnPending> continue_bnw_city_production_process_suppressed(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::shared_ptr<const BnwCityProductionTurnPending>);
    std::shared_ptr<const BnwCityProductionTurnPending> begin_bnw_city_production_train(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
    std::shared_ptr<const BnwCityProductionTurnPending> begin_bnw_city_production_project(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
    // Imported doGrowth component only, not a scheduler predecessor grant.
    // Retains once history before the first native read/store; no caller delta.
    std::shared_ptr<const BnwCityFoodGrowthPending> begin_bnw_city_food_growth_prefix(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
    std::shared_ptr<const BnwCityFoodGrowthPending> bnw_city_food_growth_pending(
        std::int64_t owner,std::int64_t city) const;
    Checkpoint checkpoint() const;
    // A foreign checkpoint is rejected. Replacement is all-or-nothing even if
    // copying a large world fails; references into the old state expire.
    void restore(const Checkpoint &checkpoint);

  private:
    friend class BnwMajorPlayerTraitsOwner;
    friend class BnwNativeUnitAllocatorCore;
    friend class ui::SelectedBnwPreviewHost;
    // Closed unpublished Host join only. No generic clearQuarantine exists.
    void initialize_original_payload_mission_pathfinder(const BnwNativeUnitAllocatorCore&,
        const ui::SelectedBnwPreviewHost&);
    void bind_restored_unit_mission_owner(
        const std::shared_ptr<const BnwNativeUnitAllocatorCore>&,
        const ui::SelectedBnwPreviewHost&);
    std::optional<std::weak_ptr<const BnwNativeUnitAllocatorCore>> bnw_restored_mission_core_;
    const LiveSessionState* bnw_restored_mission_state_=nullptr;
    const saves::SavedSession* bnw_restored_mission_source_=nullptr;
    std::uint64_t bnw_restored_mission_generation_=0;
    friend class BnwMajorTeamHasTechPending;
    friend class BnwPolicyAdoptionPending;
    friend class BnwResearchChoiceCommit;
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwUnitMovePending;
    friend class BnwRetainedTurnCursor;
    friend class BnwGameTurnTimerOwner;
    // Identity is issued only by this actual facade. It retains no world or
    // session; alias controlblock expiry is not a destruction receipt.
    struct RetainedFacadeLifetime final {
        const LiveSession* const facade;
        const std::thread::id thread;
        mutable std::atomic<bool> poisoned{false};
        std::atomic<bool> retired{false},destroyed{false};
        RetainedFacadeLifetime(const LiveSession* actual,std::thread::id owner)
            :facade(actual),thread(owner){}
        void require_current() const {
            if(std::this_thread::get_id()!=thread){
                poisoned.store(true);
                throw std::runtime_error("Retained facade foreign thread before live borrow");
            }
            if(retired.load()||destroyed.load()||poisoned.load())
                throw std::runtime_error("Retained facade lifetime retired before live borrow");
        }
    };
    mutable std::shared_ptr<RetainedFacadeLifetime> retained_facade_lifetime_;
    std::shared_ptr<RetainedFacadeLifetime> retain_bnw_facade_lifetime() const;
    void retire_bnw_facade_lifetime(bool destroyed=false) noexcept;
    void mark_bnw_retained_timer_history() const {
        if(plot_yield_refresh_thread_!=std::this_thread::get_id())
            throw std::runtime_error("Retained timer history belongs to another session thread");
        // Generic timer-only diagnostics may have no debug owner, including
        // after retirement clears it. That absence never authorizes UI or
        // continuation; the UI phase/export separately require a current owner.
        // Any debug owner still present must remain current.
        if(debug_){
            if(!bnw_game_debug_owner_current())
                throw std::runtime_error("Retained timer genuine Game debug owner retired");
            (void)bnw_game_debug_mode();
        }
        bnw_retained_turn_history_=true;
    }
    // Sticky unsaved lifecycle, recorded BEFORE retained timer construction.
    mutable bool bnw_retained_turn_history_=false;
    friend class BnwCityFoodGrowthPending;
    friend class ui::BnwPlotYieldRefreshUiAdapter;
    friend class ui::BnwCityProductionUiAdapter;
    friend class BnwPlotYieldRefreshIssuer;
    friend class BnwLuaCalculatedPlotYieldOwner;
    friend class BnwPlotYieldRefreshPending;
    friend class BnwDisplayedPlotYield;
    friend class BnwCityCurrentFoodRate;
    friend class BnwCityEntryPending;friend class BnwCityProductionTurnPending;
    friend class BnwCityYieldRefresh;friend class BnwCityYieldRefreshPending;
    friend class ui::BnwClientSelectionControl;
    friend BnwPlotTerrainCheckpointPair make_bnw_plot_terrain_checkpoint_pair(
        const LiveSession&,const SessionCheckpointIdentity&,
        const BnwPlotTerrainRulesSnapshot&);
    friend BnwPlotTerrainRestoredComponents restore_bnw_plot_terrain_checkpoint_pair(
        const BnwPlotTerrainCheckpointPair&,const LiveSession&,
        const SessionCheckpointIdentity&,const BnwPlotTerrainRulesSnapshot&);
    friend LiveActionResult apply_live_action(
        LiveSession &, const std::function<LiveActionResult(LiveSessionState &)> &);
    static LiveSession restore_bnw_impl(saves::SavedSession,
        const LiveSessionImportRules&,bool require_local_sp_startup);
    LiveSession(saves::SavedSession source, LiveSessionState state);
    mutable std::weak_ptr<ui::BnwClientSelectionControl> city_selection_;
    void retire_bnw_client_selection()noexcept;
    struct BnwUnitSerialRestoreStorage;
    std::shared_ptr<BnwUnitSerialRestoreStorage> bnw_unit_serial_restore_;
    void retire_bnw_unit_serial_restore()noexcept;
    std::int32_t bnw_unit_serial_factory_start(std::int32_t actor)const;
    static LiveSession from_restored_bnw_owned_serial(saves::SavedSession,
        LiveSessionState,std::shared_ptr<BnwUnitSerialRestoreStorage>,
        std::shared_ptr<BnwFoodGrowthImportAdmission> = {});
    std::shared_ptr<const saves::SavedSession> source_;
    std::shared_ptr<LiveSessionState> state_;
    // Actual private imported Pregame Read->Sync result; generic DTO restores cannot mint it.
    std::shared_ptr<BnwImportedPregameReadCache> bnw_imported_pregame_read_cache_;
    std::uint64_t owner_ = 0;
    friend class BnwLiveGameContext;
    friend class BnwPlayerTurnRuntimeOwner;
    // Successful actual restore/read only; DTO construction has no issuer.
    void qualify_bnw_game_read_final_member();
    bool bnw_game_read_final_qualified_=false;
    bool bnw_game_read_final_initialized_=false;
    const LiveSessionState* bnw_game_read_final_state_=nullptr;
    std::uint64_t bnw_game_read_final_generation_=0;
    bool bnw_unhappiness_current_ = false;
    std::uint64_t state_generation_ = 1;
    std::shared_ptr<BnwGameDebugReadStorage> debug_;
    const LiveSessionState* debug_state_ = nullptr;
    std::uint64_t debug_generation_ = 0;
    std::shared_ptr<BnwPlotYieldRefreshIssuer> plot_yield_refresh_issuer_;
    std::thread::id plot_yield_refresh_thread_=std::this_thread::get_id();
    std::map<std::int64_t,std::shared_ptr<BnwPlotYieldRefreshPending>>
        plot_yield_refresh_pending_;
    std::map<BnwCityEntrySourceKey,std::shared_ptr<BnwCityEntryPending>> city_entry_pending_;
    std::shared_ptr<BnwCityYieldRefreshRegistry> city_yield_refresh_registry_;
    std::shared_ptr<BnwCityYieldRefreshPending> begin_bnw_city_yield_refresh(
        const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,std::int64_t,std::int64_t,
        const std::shared_ptr<BnwCityEntryPending>&);
    void begin_bnw_city_plot_yield_child(BnwCityYieldRefreshPending&);
    std::shared_ptr<BnwPlotYieldRefreshPending> current_bnw_city_plot_child(const BnwCityYieldRefreshPending&)const;
    void continue_bnw_city_plot_yield_child(BnwCityYieldRefreshPending&,
        const std::shared_ptr<const ui::SelectedBnwPreviewHost>&);
    std::map<BnwCityProductionTurnKey,std::shared_ptr<BnwCityProductionTurnPending>> production_turn_pending_;
    std::shared_ptr<BnwUnitMoveRegistry> unit_move_registry_;
    struct BnwPathfinderOwnedStorage;
    std::shared_ptr<BnwPathfinderOwnedStorage> pathfinder_;
    void require_bnw_pathfinder()const;
    std::shared_ptr<BnwTwoLayerUnitPathFinder> bnw_common_unit_pathfinder_for_native_unit(const BnwNativeUnitAllocatorCore&)const;
    BnwUnitPathOwner& bnw_native_unit_paths_for_core(const BnwNativeUnitAllocatorCore&,const BnwTwoLayerUnitPathFinder&) const;
    void require_bnw_native_unit_paths_current_without_callbacks(const BnwUnitPathOwner&,const BnwTwoLayerUnitPathFinder&,const BnwNativeUnitAllocatorCore&) const;
    void require_bnw_common_unit_pathfinder_native_current_without_callbacks(const BnwTwoLayerUnitPathFinder&,const BnwNativeUnitAllocatorCore&)const;
    void force_reset_bnw_common_pathfinder();
    void retire_bnw_pathfinder()noexcept;
    std::shared_ptr<BnwCityFoodGrowthRegistry> food_growth_registry_;
    std::shared_ptr<BnwFoodGrowthImportAdmission> bnw_food_growth_import_;
    void retire_bnw_food_growth()noexcept;
    void retire_bnw_unit_move()noexcept;
    void retire_bnw_production_turn()noexcept;
    void retire_bnw_city_entry()noexcept;
    void retire_plot_yield_refresh() noexcept;
    void retire_bnw_game_debug_owner() noexcept {
        if(debug_)debug_->retired.store(true);
        debug_.reset();debug_state_=nullptr;debug_generation_=0;
    }
    bool bnw_game_debug_owner_current() const noexcept {
        if(plot_yield_refresh_thread_!=std::this_thread::get_id())return false;
        return source_ && state_ && debug_ && !debug_->retired.load() &&
            debug_->thread==plot_yield_refresh_thread_ && debug_state_ == state_.get() &&
            debug_generation_ == state_generation_ &&
            !debug_->identity.payload_sha256.empty() && !debug_->identity.rules_sha256.empty();
    }
    void bind_bnw_game_debug_owner() noexcept {
        debug_state_ = state_.get(); debug_generation_ = state_generation_;
    }
    void advance_state_generation();
};

} // namespace civ5::runtime
