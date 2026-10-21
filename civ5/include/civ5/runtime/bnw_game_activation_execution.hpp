#pragma once
#include "civ5/runtime/bnw_game_activation_preflight.hpp"
#include "civ5/runtime/bnw_deferred_fog.hpp"
#include "civ5/runtime/bnw_activation_connections.hpp"
#include "civ5/runtime/bnw_pathfinder_cache_controls.hpp"
#include "civ5/runtime/bnw_busy_state.hpp"
#include "civ5/runtime/bnw_activation_happiness.hpp"
#include "civ5/runtime/bnw_activation_builder_tasking.hpp"
#include "civ5/runtime/bnw_diplomacy_turn_end.hpp"
#include "civ5/runtime/bnw_player_do_turn_prefix.hpp"
#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include "civ5/runtime/bnw_player_do_turn_trade.hpp"
#include "civ5/runtime/bnw_player_do_turn_military.hpp"
#include "civ5/runtime/bnw_player_grand_military_average.hpp"
#include "civ5/runtime/bnw_grand_guess_empty.hpp"
#include "civ5/runtime/bnw_grand_first_priority.hpp"
#include "civ5/runtime/live_session.hpp"
#include <map>
#include <memory>
#include <set>

namespace civ5::runtime {
struct BnwGameActivationPublication {
    BnwGameActivationStep step;
    std::int64_t player=-1;
    std::string message;
};
// An enclosing game transaction owns this alongside LiveSessionState. Keeping
// the completed epoch after deactivation prevents a second doTurn for the same
// player and turn. It needs explicit checkpoint integration before continuation.
struct BnwGameActivationOwner {
    SessionCheckpointIdentity identity;
    std::set<std::pair<std::int64_t,std::int64_t>> completed; // turn, player
    std::map<std::int64_t,bool> processed_auto_moves;
    std::vector<BnwGameActivationPublication> pending;
    // A tracked native renderer baseline allows deferred fog to stage on the
    // same private activation owner. No actual FOW client is touched here.
    std::optional<BnwDeferredFogOwner> deferred_fog;
    std::optional<BnwActivationConnectionsOwner> city_connections;
    std::optional<BnwPathfinderCacheControlOwner> pathfinder_cache;
    // Native mission/combat transients belong to the same enclosing world
    // owner, not a second independently reset copy during updateMoves.
    std::optional<BnwBusyStateOwner> busy_state;
    // SAME Builder Read/city-strategy caches, bound once at original import.
    std::optional<BnwActivationBuilderOwner> builder_tasking;
    // Same original Read request registry through activation and deactivation.
    std::optional<BnwDiplomacyTurnOwner> diplomacy_requests;
    // Actual incomplete player DoTurn source prefix and SAME staged UI bit.
    // Once present it must never be reset/replayed by a later suffix retry.
    std::optional<BnwPlayerDoTurnPrefixJournal> player_do_turn_prefix;
    std::optional<BnwPlayerDoTurnRevoltsJournal> player_do_turn_revolts;
    std::optional<BnwPlayerDoTurnTradeJournal> player_do_turn_trade;
    std::optional<BnwPlayerDoTurnMilitaryJournal> player_do_turn_military;
    std::optional<BnwGrandMilitaryAverageJournal> grand_military_average;
    std::optional<BnwGrandWorldAverageJournal> grand_world_averages;
    std::optional<BnwGrandGuessEmptyJournal> grand_guess_empty;
};
struct BnwGameActivationPlayerPublication {
    BnwTurnPhase phase;
    std::function<LiveActionResult(LiveSession &)> publish;
};
struct BnwGameActivationDraft {
    LiveSessionState state;
    BnwGameActivationOwner owner;
    std::vector<BnwGameActivationPlayerPublication> player_publications;
};
// Retains all real preceding native owners/publication journals together with
// the sole happiness VM/world owner. Its state is the authoritative pending
// happiness world, not the earlier activation rollback snapshot.
class BnwGrandCulturePriorityPending;
class BnwGrandFirstRollJournal;
class BnwGameActivationHappinessPending {
 public:
    const LiveSessionState &state()const{return happiness_->state();}
    const BnwGameActivationOwner &activation_owner()const noexcept{return activation_->owner;}
    const BnwActivationHappinessOwner &happiness()const noexcept{return *happiness_;}
    const auto &completed_prefix()const noexcept{return completed_prefix_;}
    bool builder_complete()const noexcept{return builder_complete_;}
    bool diplomacy_begin_complete()const noexcept{return diplomacy_begin_complete_;}
    const BnwGrandFirstPriorityQuote *grand_first_prefix()const noexcept{return grand_first_prefix_?&*grand_first_prefix_:nullptr;}
 private:
    friend struct BnwGameActivationPendingAccess;
    friend BnwGrandSelectionPending stage_bnw_grand_selection(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &,std::shared_ptr<const BnwGrandMinimumAgeOwner>);
    friend BnwGrandPressureJournal stage_bnw_grand_pressure(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandConquestPriorityJournal stage_bnw_grand_conquest_category(
        const BnwGameActivationHappinessPending &,const BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandFourthRollJournal stage_bnw_grand_fourth_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandSpaceshipPriorityJournal stage_bnw_grand_spaceship_category(
        const BnwGameActivationHappinessPending &,const BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandThirdRollJournal stage_bnw_grand_third_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandSecondRollJournal stage_bnw_grand_second_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    friend BnwGrandFirstRollJournal stage_bnw_grand_first_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    friend BnwGrandUnitedNationsPriorityJournal stage_bnw_grand_united_nations_category(
        const BnwGameActivationHappinessPending &,const BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    friend std::unique_ptr<BnwGrandCulturePriorityPending> stage_bnw_grand_culture_priority(
        const BnwGameActivationHappinessPending &,std::unique_ptr<BnwTeamTechLuaEpoch> &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    std::unique_ptr<BnwGameActivationDraft> activation_;
    std::unique_ptr<BnwActivationHappinessOwner> happiness_;
    BnwGameActivationTicket ticket_;
    std::vector<BnwGameActivationStep> completed_prefix_;
    bool builder_complete_=false,diplomacy_begin_complete_=false;
    std::optional<BnwGrandFirstPriorityQuote> grand_first_prefix_;
};
struct BnwGameActivationExecution {
    // The source and input owner are always unchanged. A complete draft still
    // needs the outer game commit and source-ordered external publication.
    std::unique_ptr<BnwGameActivationDraft> draft;
    // Native source prefix and subsequent happiness/Builder stages stay here.
    // Full draft stays null; completed_steps records only actual native stages.
    std::unique_ptr<BnwGameActivationHappinessPending> pending_happiness;
    std::vector<BnwGameActivationStep> completed_steps;
    std::vector<BnwTurnPhase> completed_player_phases;
    // Legacy callback path counts entry attempts, including rejected handlers.
    // Native retained path increments only after accepted real DoTurn prefix;
    // a rejected rollback trial cannot strand its recovery journal. Completion
    // receipts are completed steps/phases; only draft means all succeeded.
    std::uint32_t player_turn_calls=0, unit_turn_calls=0;
    std::string blocker;
};

// Executes the source activation order on one private copy. Native owned
// operations are bound here; all other operations require real caller services.
// Missing owners reject at their source boundary, with no successful default.
// Callbacks may mutate only the supplied draft, never external engine/UI/Lua.
// Their publication must be staged. Caller callbacks for native owned steps
// are rejected to preserve one owner. Player after_commit hooks are returned
// as obligations, never run inside the rollbackable activation.
BnwGameActivationExecution execute_bnw_game_activation_draft(
    const LiveSessionState &source,const BnwGameActivationOwner &owner,
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity,
    std::span<const BnwTurnPhase> completed_game_prefix,
    std::span<const BnwTurnHandler> player_handlers,
    std::span<const BnwGameActivationService> services={});

// Execute the SAME source loop but stop at the real happiness owner. Earlier
// fog/pathfinder/connections/gold must be native retained owners, never caller
// callbacks. No imported-active shortcut, DoTurn entry or full publication.
BnwGameActivationExecution execute_bnw_game_activation_until_happiness(
    const LiveSessionState &,const BnwGameActivationOwner &,
    const content::RulesDatabase &,const SessionCheckpointIdentity &,
    std::span<const BnwTurnPhase> completed_game_prefix,
    std::span<const BnwTurnHandler> player_handlers,
    std::span<const BnwGameActivationService> services={});
// Resume only this retained native stage after actual stock Lua becomes ready.
// The prior source prefix and player DoTurn are never executed by this call.
// Actual happiness native completion is followed by actual BuilderTasking on
// the SAME epoch allocation/owners. Only real successful native stages add a
// receipt. Actual BeginTurn uses the SAME diplomacy owner next. Player/unit
// DoTurn, cache restore, UI and outer publication remain mandatory barriers.
LiveActionResult continue_bnw_game_activation_happiness(
    BnwGameActivationExecution &,const SavedContentProfile &,
    BnwGameplayLuaEpochOwner &,const content::RulesDatabase &);

// Retry only the independent Builder trial after accepted native happiness;
// never forks Lua or replays DoUpdate/the earlier activation prefix.
LiveActionResult continue_bnw_game_activation_builder(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// Resume only the independent native BeginTurn trial after actual happiness
// and Builder; no original Read rebind, queue reset, AI evaluation or DoTurn.
LiveActionResult continue_bnw_game_activation_diplomacy_begin(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// Enter the real player DoTurn prefix ONLY after retained native BeginTurn.
// No scheduler callbacks or repeated happiness/Builder/BeginTurn invocation.
// Records cache/AI receipts and scalar/cramped substeps, not player_do_turn or
// player_timers_and_revolts completion. Uprisings is the next required boundary.
LiveActionResult continue_bnw_game_activation_player_prefix(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// Continue only Uprisings/CityRevolts/Maya after accepted real scalar/cramped
// prefix. Same epoch, native rules, RNG and journals; no prefix or Lua replay.
// Native suffix receipt is not timers/revolts phase or UI delivery completion.
LiveActionResult continue_bnw_game_activation_player_revolts(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// Resume actual Trade::DoTurn only after accepted native revolts. Retains the
// SAME epoch/history; explicit route effect gates, no phase3/step15 receipt.
LiveActionResult continue_bnw_game_activation_player_trade(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// Exact source ResetCounters after SAME accepted Trade. Human GrandStrategy
// and shadow Diplomacy are mandatory downstream, not certified by this stage.
LiveActionResult continue_bnw_game_activation_player_military(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// First actual GrandStrategy query on SAME retained Trade/Military epoch.
// Source cached branch only; stale requires BOTH native calculators. Culture,
// tourism, technologies, RNG priorities and decisions remain mandatory next.
LiveActionResult continue_bnw_game_activation_grand_military_average(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// Source DoGuess remaining averages computed once by private producer, then
// empty eligible-target loop on SAME seals/epoch. No Grand/phase3/step15 grant.
LiveActionResult continue_bnw_game_activation_grand_world_averages(
    BnwGameActivationExecution &,const content::RulesDatabase &);
LiveActionResult continue_bnw_game_activation_grand_guess_empty(
    BnwGameActivationExecution &,const content::RulesDatabase &);

// After accepted source DoGuess, retain GetBase/Culture prefix once, then
// run GetTotalJONSCulturePerTurn on the SAME world/VM. Pre-dispatch refusals
// retain its pending query; a dispatched failure forbids replay/publication.
// Tourism/foreign comparisons/influence and JonRand remain separate stages.
LiveActionResult continue_bnw_game_activation_grand_own_culture(
    BnwGameActivationExecution &,const SavedContentProfile &,
    const content::RulesDatabase &);
// Continue own tourism, the native63 foreign scan and influence22 in the
// same sole epoch. Retained source locals survive pure refusals; callbacks
// are not replayed. Completion is a category receipt before native JonRand.
LiveActionResult continue_bnw_game_activation_grand_culture_priority(
    BnwGameActivationExecution &,const SavedContentProfile &,
    const content::RulesDatabase &);
// The actual completed first category authorizes exactly one gameplay draw
// and priorities[0] store on the retained source epoch. Later slots and the
// final Grand decision remain mandatory; this is not a full-turn receipt.
LiveActionResult continue_bnw_game_activation_grand_first_roll(
    BnwGameActivationExecution &,const SavedContentProfile &,
    const content::RulesDatabase &);
// Retains the real slot1 GetBase/category locals after priorities[0]. It does
// not execute the second random draw or complete Grand/player/game phases.
LiveActionResult continue_bnw_game_activation_grand_united_nations_category(
    BnwGameActivationExecution &,const SavedContentProfile &,
    const content::RulesDatabase &);
LiveActionResult continue_bnw_game_activation_grand_second_roll(
    BnwGameActivationExecution &,const SavedContentProfile &,
    const content::RulesDatabase &);
LiveActionResult continue_bnw_game_activation_grand_spaceship_category(
    BnwGameActivationExecution &,const SavedContentProfile &,const content::RulesDatabase &);
LiveActionResult continue_bnw_game_activation_grand_third_roll(
    BnwGameActivationExecution &,const SavedContentProfile &,const content::RulesDatabase &);
// Conquest obtains a fresh current military average at its own gated source
// point, after GetBase; the earlier DoGuess journal is not a substitute.
LiveActionResult continue_bnw_game_activation_grand_conquest_category(
    BnwGameActivationExecution &,const SavedContentProfile &,const content::RulesDatabase &);
LiveActionResult continue_bnw_game_activation_grand_fourth_roll(
    BnwGameActivationExecution &,const SavedContentProfile &,const content::RulesDatabase &);
LiveActionResult continue_bnw_game_activation_grand_pressure(
    BnwGameActivationExecution &,const SavedContentProfile &,const content::RulesDatabase &);
// Retains selection at its first mandatory logger. An existing active strategy
// requires a source-issued cached GC threshold; no scalar/SQL substitute.
LiveActionResult continue_bnw_game_activation_grand_selection(
    BnwGameActivationExecution &,const SavedContentProfile &,const content::RulesDatabase &,
    std::shared_ptr<const BnwGrandMinimumAgeOwner> minimum_age={});

// The existing source-backed human cache/AI/early-timer phases are connected.
// Later phases carry explicit rejection handlers until real owners are supplied.
std::vector<BnwTurnHandler> make_bnw_game_activation_source_player_handlers(
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity,
    std::int64_t active_player);
} // namespace civ5::runtime
