#include "civ5/ui/bnw_client_selection_control.hpp"
#include <limits>
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_imported_pregame_read_cache.hpp"
#include "civ5/runtime/bnw_unit_mission_continuation_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/bnw_pathfinder_cache_controls.hpp"
#include "civ5/runtime/bnw_two_layer_unit_pathfinder.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_food_growth_import_admission.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_yield_refresh.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/bnw_game_supporting_checkpoint.hpp"
#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include "civ5/runtime/bnw_team_turn_pair.hpp"
#include "civ5/runtime/bnw_game_read_hooks.hpp"
#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_area_unit_checkpoint.hpp"
#include "civ5/runtime/bnw_player_proximity_checkpoint.hpp"
#include "civ5/runtime/bnw_team_meet_checkpoint.hpp"
#include "civ5/runtime/bnw_exploration_dirty_checkpoint.hpp"
#include "civ5/runtime/bnw_player_strategy_checkpoint.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/bnw_team_progress_checkpoint.hpp"
#include "civ5/runtime/bnw_player_culture_bomb_turn.hpp"
#include "civ5/runtime/bnw_culture_bomb_timer_checkpoint.hpp"
#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include "civ5/runtime/bnw_research_queue_checkpoint.hpp"
#include "civ5/runtime/bnw_maintenance_expense_checkpoint.hpp"
#include "civ5/runtime/bnw_player_knowledge_live_bundle.hpp"
#include "civ5/runtime/bnw_player_administration_live_bundle.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/runtime/production_checkpoint.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include "civ5/runtime/research_selection_checkpoint.hpp"
#include "civ5/runtime/research_turn_checkpoint.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/runtime/treasury_checkpoint.hpp"
#include <atomic>
#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace civ5::runtime {
namespace {
std::atomic<std::uint64_t> next_session_owner{1};
constexpr auto bnw_layout=saves::GameStateLayout::expansion2_version1;
constexpr auto bnw_profile=content::TechnologyProfile::bnw;

saves::CivisSaveSection section(saves::CivisSaveSectionId id,
                                const std::vector<std::uint8_t> &bytes) {
    return {static_cast<std::uint32_t>(id),0,bytes};
}

std::int64_t player_land_score(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_iTotalLandScored") {
            if(found) throw std::invalid_argument("Duplicate BNW player land score");
            found=&field;
        }
    if(!found || found->values.size()!=1)
        throw std::invalid_argument("BNW player land score is unavailable");
    return found->values.front();
}

std::int64_t city_id(const saves::SavedCityObservation &city) {
    for(const auto &field:city.core.fields)
        if(field.name && std::string_view(field.name)=="m_iID" &&
           field.values.size()==1) return field.values.front();
    throw std::invalid_argument("BNW city checkpoint ID is unavailable");
}

template<class Member>
void move_city_domain(saves::SavedSessionSnapshot &target,
                      saves::SavedSessionSnapshot &restored,Member member) {
    if(!target.all_players || !restored.all_players ||
       target.all_players->players.size()!=restored.all_players->players.size())
        throw std::invalid_argument("BNW city checkpoint player roster changed");
    auto &to_players=target.all_players->players;
    auto &from_players=restored.all_players->players;
    for(std::size_t p=0;p<to_players.size();++p) {
        auto &to=to_players[p].cities.cities;
        auto &from=from_players[p].cities.cities;
        if(to_players[p].saved_player_id!=from_players[p].saved_player_id ||
           to.size()!=from.size())
            throw std::invalid_argument("BNW city checkpoint roster changed");
        for(std::size_t c=0;c<to.size();++c) {
            if(to[c].index!=from[c].index || city_id(to[c])!=city_id(from[c]))
                throw std::invalid_argument("BNW city checkpoint order or ID changed");
            to[c].*member=std::move(from[c].*member);
        }
    }
}

void validate_import(const saves::SavedSession &source,
                     const LiveSessionImportRules &rules) {
    const auto &s=source.snapshot;
    if(s.game_state.layout!=bnw_layout || !s.pregame || s.pregame->layout!=bnw_layout ||
       !s.map_plots || !s.map_regions || !s.teams || !s.all_players ||
       !s.supporting_classes || s.payload_end_offset!=source.source.bytes.size())
        throw std::invalid_argument("BNW live session requires a complete expansion2 snapshot");
    if(rules.technologies.profile!=bnw_profile || rules.city_yields.profile!=bnw_profile ||
       rules.production.profile!=bnw_profile || rules.maintenance.profile!=bnw_profile ||
       rules.treasury.profile!=bnw_profile)
        throw std::invalid_argument("BNW live session requires one BNW rules profile");
    if(rules.identity.payload_sha256.empty() || rules.identity.rules_sha256.empty())
        throw std::invalid_argument("BNW live session requires payload and rules identities");
    if(identify_bytes(source.source.bytes).sha256!=rules.identity.payload_sha256)
        throw std::invalid_argument("BNW live session payload identity does not match decoded source");
    if(identify_bytes(rules.database.gameplay_fingerprint_data()).sha256!=
       rules.identity.rules_sha256)
        throw std::invalid_argument("BNW live session rules identity does not match activated rules");
    if(!source.source.profile_sections.empty())
        throw std::invalid_argument("BNW live session cannot consume GameCore profile sections");
}
} // namespace

struct LiveSession::BnwPathfinderOwnedStorage {
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> retired{false};
    std::shared_ptr<const saves::SavedSession> source;
    std::shared_ptr<LiveSessionState> state;
    const std::uint64_t generation;
    std::function<bool()> rules_current;
    BnwPathfinderCacheControlOwner controls;
    std::unique_ptr<BnwTwoLayerUnitPathFinder>common_unit_tree;
    std::optional<BnwUnitPathOwner>native_unit_paths;
    const saves::PlotObservation* plot_storage;
    const saves::SavedMapCellProjection* projection_storage;
    BnwPathfinderOwnedStorage(std::shared_ptr<const saves::SavedSession> original,
        std::shared_ptr<LiveSessionState> world,std::uint64_t gen,
        std::function<bool()> actual_rules,BnwPathfinderCacheControlOwner initialized)
        :source(std::move(original)),state(std::move(world)),generation(gen),
         rules_current(std::move(actual_rules)),controls(std::move(initialized)),
         plot_storage(state->world.map_plots->plots.data()),
         projection_storage(state->world.map_projection->cells.data()) {}
};

struct LiveSession::BnwUnitSerialRestoreStorage {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> retired{false};
    saves::CivisSaveSection section;
    SessionCheckpointIdentity identity;
    std::array<std::int32_t,64> starts{};
    std::weak_ptr<const saves::SavedSession> source;
    std::weak_ptr<LiveSessionState> state;
    std::uint64_t generation=0;
};
void LiveSession::retire_bnw_unit_serial_restore()noexcept {
    if(bnw_unit_serial_restore_)bnw_unit_serial_restore_->retired.store(true);
    bnw_unit_serial_restore_.reset();
}
std::int32_t LiveSession::bnw_unit_serial_factory_start(std::int32_t actor)const {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id()||!source_||!state_||
       !bnw_game_debug_owner_current()||actor<0||actor>=64||!source_->snapshot.all_players)
        throw BnwNativeUnitAllocatorUnavailable("Serial factory lacks genuine current session/actor");
    const auto&original=source_->snapshot.all_players->players.at(static_cast<std::size_t>(actor));
    if(original.index!=static_cast<std::uint64_t>(actor)||original.units.header.fields.size()!=5||
       !original.units.header.fields[4].name||std::string_view(original.units.header.fields[4].name)!="current_id")
        throw BnwNativeUnitAllocatorUnavailable("Serial factory original actor/FLTA identity differs");
    const auto&pending=source_->source.live_bnw_unit_serial;
    if(!pending){
        if(bnw_unit_serial_restore_)throw BnwNativeUnitAllocatorUnavailable("Serial factory has unrelated restore lineage");
        const auto value=original.units.header.fields[4].value;
        if(value<8192||value>INT32_MAX||value%8192)throw BnwNativeUnitAllocatorUnavailable("Serial factory original native counter invalid");
        return static_cast<std::int32_t>(value);
    }
    const auto&p=bnw_unit_serial_restore_;
    const auto held_source=p?p->source.lock():std::shared_ptr<const saves::SavedSession>{};
    const auto held_state=p?p->state.lock():std::shared_ptr<LiveSessionState>{};
    if(!p||p->creator!=std::this_thread::get_id()||p->retired.load()||
       held_source.get()!=source_.get()||held_source.owner_before(source_)||source_.owner_before(held_source)||
       held_state.get()!=state_.get()||held_state.owner_before(state_)||state_.owner_before(held_state)||p->generation!=state_generation_||
       p->section.id!=45||p->section.flags!=0||p->section.bytes!=*pending||
       p->identity.payload_sha256!=debug_->identity.payload_sha256||
       p->identity.rules_sha256!=debug_->identity.rules_sha256)
        throw BnwNativeUnitAllocatorUnavailable("Serial factory authentic restored source retired/replaced");
    return p->starts.at(static_cast<std::size_t>(actor));
}

LiveSession::LiveSession(saves::SavedSession source, LiveSessionState state)
    : source_(std::make_shared<const saves::SavedSession>(std::move(source))),
      state_(std::make_shared<LiveSessionState>(std::move(state))),
      owner_(next_session_owner.fetch_add(1, std::memory_order_relaxed)) {}

LiveSession LiveSession::restore_bnw(saves::SavedSession source,
                                     const LiveSessionImportRules &rules) {
    return restore_bnw_impl(std::move(source),rules,false);
}
LiveSession LiveSession::restore_bnw_imported_game(saves::SavedSession source,
                                     const LiveSessionImportRules &rules) {
    return restore_bnw_impl(std::move(source),rules,true);
}
LiveSession LiveSession::restore_bnw_impl(saves::SavedSession source,
                                     const LiveSessionImportRules &rules,bool require_local_sp_startup) {
    if(source.source.live_bnw_population_totals&&!source.source.live_bnw_food_growth_journal)throw std::invalid_argument("Population totals checkpoint lacks genuine once-owner restore");
    validate_import(source,rules);
    // Original Pregame::read precedes CvGame::Read/reset. Re-read genuine
    // immutable SlotHints/Archive bytes, rebuild hash, execute native22 Sync.
    // Later leader/civ/reseat callbacks are not licensed by this cache result.
    auto world=source.snapshot;
    auto imported_pregame=std::shared_ptr<BnwImportedPregameReadCache>(new BnwImportedPregameReadCache(
        source,*world.pregame,rules.database,rules.identity,require_local_sp_startup,rules.actual_early_pregame_locale));
    // CvGame::Read enters reset()->uninit() before consuming Game state.
    // Execute the two unsaved debug writes here before replaying the live
    // world/load hooks. Publish this private state only if restoration succeeds.
    BnwGameDebugReadState debug_read(rules.identity);
    const auto &loaded=source.source;
    saves::StandaloneServicesConfig standalone_services;
    if(loaded.container==saves::SaveContainerKind::civis) {
        if(!loaded.standalone_services)
            throw std::invalid_argument("BNW live restore requires saved standalone services configuration");
        standalone_services=*loaded.standalone_services;
    } else if(loaded.standalone_services) {
        // Public input DTOs must not smuggle unsupported values into an original import.
        saves::require_supported_standalone_services(*loaded.standalone_services);
    }
    saves::require_supported_standalone_services(standalone_services);
    std::shared_ptr<BnwFoodGrowthImportAdmission> food_restore;
    if(loaded.live_bnw_food_growth_journal)food_restore=std::shared_ptr<BnwFoodGrowthImportAdmission>(new BnwFoodGrowthImportAdmission(loaded,rules.identity));
    std::shared_ptr<BnwUnitSerialRestoreStorage> serial_restore;
    if(loaded.live_bnw_unit_serial){
        if(loaded.container!=saves::SaveContainerKind::civis||loaded.live_bnw_units||
           loaded.live_bnw_area_units||loaded.live_bnw_unit_mission_continuation)
            throw std::invalid_argument("Serial45 restore requires standalone original-unit domain");
        serial_restore=std::make_shared<BnwUnitSerialRestoreStorage>();
        serial_restore->section=section(saves::CivisSaveSectionId::live_bnw_unit_serial,*loaded.live_bnw_unit_serial);
        serial_restore->identity=rules.identity;
        restore_bnw_unit_serial_checkpoint(serial_restore->section,world,rules.identity);
        for(std::size_t i=0;i<64;++i)serial_restore->starts[i]=static_cast<std::int32_t>(world.all_players->players.at(i).units.header.fields.at(4).value);
    }
    if(loaded.live_bnw_player_knowledge) {
        if(loaded.live_bnw_research_queue)
            throw std::invalid_argument("BNW full player knowledge and ID33 research queue overlap");
        // The narrow prophet-only bundle is validated after every owned
        // research context has been restored. Other ID35 mutations reject.
    }
    if(loaded.live_bnw_player_administration) {
        if(loaded.live_team_research || loaded.live_treasury ||
           loaded.live_research_turn || loaded.live_research_selection ||
           loaded.live_bnw_treasury_balance || loaded.live_bnw_research_queue ||
           loaded.live_bnw_maintenance_expense)
            throw std::invalid_argument(
                "BNW full player administration overlaps research, treasury or maintenance sections");
        // Only the ring and two history flags can be restored against the
        // current owned economy/queue contexts. Validate after those contexts.
    }
    if(loaded.live_bnw_player_core && loaded.live_bnw_player_proximity)
        throw std::invalid_argument("BNW player proximity and full player core sections cannot coexist");
    std::optional<saves::SavedSessionSnapshot> restored_player_core;
    if(loaded.live_bnw_player_core&&!food_restore) {
        restored_player_core.emplace(source.snapshot);
        restore_bnw_player_core_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_player_core,
                    *loaded.live_bnw_player_core),
            *restored_player_core,source.snapshot,rules.identity);
    }
    if(loaded.live_bnw_area_units) {
        if(!loaded.live_bnw_map_plots || !loaded.live_bnw_units)
            throw std::invalid_argument("BNW area section requires paired map and unit sections");
        const auto map=section(saves::CivisSaveSectionId::live_bnw_map_plots,
                               *loaded.live_bnw_map_plots);
        const auto units=section(saves::CivisSaveSectionId::live_bnw_units,
                                 *loaded.live_bnw_units);
        const auto area=section(saves::CivisSaveSectionId::live_bnw_area_units,
                                *loaded.live_bnw_area_units);
        restore_bnw_map_unit_area_checkpoint_triplet(
            &map,&units,&area,world,rules.identity,
            content::saved_terrain_features(rules.database,rules.types).resolve,
            &rules.database,&rules.types,&rules.defines);
    } else if(loaded.live_bnw_units) {
        if(!loaded.live_bnw_map_plots)
            throw std::invalid_argument("BNW unit section requires its paired map section");
        const auto map=section(saves::CivisSaveSectionId::live_bnw_map_plots,
                               *loaded.live_bnw_map_plots);
        const auto units=section(saves::CivisSaveSectionId::live_bnw_units,
                                 *loaded.live_bnw_units);
        restore_bnw_map_unit_checkpoint_pair(
            &map,&units,world,rules.identity,
            content::saved_terrain_features(rules.database,rules.types).resolve,
            &rules.database,&rules.types,&rules.defines);
    } else if(loaded.live_bnw_map_plots)
        restore_bnw_map_plot_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_map_plots,
                    *loaded.live_bnw_map_plots),world,rules.identity,
            content::saved_terrain_features(rules.database,rules.types).resolve,
            &rules.database,&rules.types,&rules.defines);
    // Original CvMap::Read/setup initializes unsaved finder controls. Reuse
    // the existing ctor/Initialize/setup control port at this map-load point.
    // Civis continuation has unknown runtime cache history: do not mint zeros.
    std::optional<BnwPathfinderCacheControlOwner> imported_pathfinder;
    auto pathfinder_rules_current=rules.database.immutable_preview_fingerprint_currentness();
    if(source.source.container==saves::SaveContainerKind::civ5&&pathfinder_rules_current){
        if(!pathfinder_rules_current())throw BnwCommonPathfinderUnavailable("Original map setup immutable rules retired");
        imported_pathfinder.emplace(make_bnw_pathfinder_cache_control_import_owner(source,rules.identity));
    }
    if(loaded.live_bnw_player_proximity)
        restore_bnw_player_proximity_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_player_proximity,
                    *loaded.live_bnw_player_proximity),world,source.snapshot,rules.identity);
    if(restored_player_core) {
        auto &target=world.all_players->players;
        auto &restored=restored_player_core->all_players->players;
        if(target.size()!=restored.size())
            throw std::invalid_argument("BNW player core roster changed during map restore");
        if(loaded.live_bnw_map_plots)
            for(std::size_t i=0;i<target.size();++i)
                if(player_land_score(target[i])!=player_land_score(restored[i]))
                    throw std::invalid_argument("BNW map and player core land scores disagree");
        for(std::size_t i=0;i<target.size();++i)
            target[i].core=std::move(restored[i].core);
    }
    if(loaded.live_bnw_team_meet)
        restore_bnw_team_meet_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_team_meet,
                    *loaded.live_bnw_team_meet),world,source.snapshot,rules.identity);
    if(loaded.live_bnw_exploration_dirty&&loaded.live_bnw_player_strategy)
        throw std::invalid_argument("BNW exploration dirty and full player strategy sections cannot coexist");
    if(loaded.live_bnw_exploration_dirty)
        restore_bnw_exploration_dirty_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_exploration_dirty,
                    *loaded.live_bnw_exploration_dirty),world,source.snapshot,rules.identity);
    if(loaded.live_bnw_player_strategy)
        restore_bnw_player_strategy_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_player_strategy,
                    *loaded.live_bnw_player_strategy),world,source.snapshot,rules.identity);
    if(loaded.live_bnw_city_core&&!food_restore) {
        auto restored=source.snapshot;
        restore_bnw_city_core_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_city_core,
                    *loaded.live_bnw_city_core),
            restored,source.snapshot,rules.identity);
        move_city_domain(world,restored,&saves::SavedCityObservation::core);
    }
    if(loaded.live_bnw_city_production&&!food_restore) {
        auto restored=source.snapshot;
        restore_bnw_city_production_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_city_production,
                    *loaded.live_bnw_city_production),
            restored,source.snapshot,rules.identity);
        move_city_domain(world,restored,&saves::SavedCityObservation::production);
    }
    if(loaded.live_bnw_city_management&&!food_restore) {
        auto restored=source.snapshot;
        restore_bnw_city_management_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_city_management,
                    *loaded.live_bnw_city_management),
            restored,source.snapshot,rules.identity);
        move_city_domain(world,restored,&saves::SavedCityObservation::management);
    }
    if(loaded.live_bnw_city_tail&&!food_restore) {
        auto restored=source.snapshot;
        restore_bnw_city_tail_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_city_tail,
                    *loaded.live_bnw_city_tail),
            restored,source.snapshot,rules.identity);
        move_city_domain(world,restored,&saves::SavedCityObservation::tail);
    }
    BnwGameClockState game_clock;
    if(loaded.live_bnw_game_clock&&!food_restore)
        restore_bnw_game_clock_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_game_clock,
                    *loaded.live_bnw_game_clock),world,game_clock,rules.identity);
    if(loaded.live_bnw_game_supporting)
        restore_bnw_game_supporting_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_game_supporting,
                    *loaded.live_bnw_game_supporting),world,rules.identity);
    if(loaded.live_bnw_game_score_replay)
        restore_bnw_game_score_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_game_score_replay,
                    *loaded.live_bnw_game_score_replay),world,
            source.snapshot,rules.identity);
    if(loaded.live_bnw_game_deals)
        restore_bnw_game_deals_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_game_deals,
                    *loaded.live_bnw_game_deals),world,
            source.snapshot,rules.identity);
    auto setup=restore_research_context(world,rules.database);
    if(setup.profile==content::TechnologyProfile::bnw)
        restore_bnw_policy_membership(world,setup,BnwPolicyHappinessRules::load(rules.database));
    if(setup.profile!=bnw_profile)
        throw std::invalid_argument("Restored setup is not BNW");
    auto technologies=restore_team_technologies(*world.teams,rules.technologies,rules.types);
    auto player_research=restore_player_research_context(world,setup,rules.defines);
    if(loaded.live_bnw_research_queue &&
       (!loaded.live_team_research || !loaded.live_research_turn ||
        loaded.live_bnw_team_progress || loaded.live_research_selection))
        throw std::invalid_argument("BNW research queue requires exclusive ID8 and ID11 companions");
    if(loaded.live_bnw_team_turn) {
        if(!loaded.live_team_research || loaded.live_bnw_team_progress ||
           loaded.live_bnw_research_queue)
            throw std::invalid_argument(
                "BNW team turn requires exclusive raw ID40 and owned ID8 pair");
        BnwTeamTurnCheckpointPair pair{
            section(saves::CivisSaveSectionId::live_bnw_team_turn,
                    *loaded.live_bnw_team_turn),
            section(saves::CivisSaveSectionId::live_team_research,
                    *loaded.live_team_research)};
        auto validated=restore_bnw_team_turn_checkpoint_pair(
            pair,source.snapshot,setup,technologies,player_research,
            rules.technologies,rules.types,rules.identity);
        restore_bnw_team_turn_checkpoint(
            pair.raw,world,source.snapshot,rules.identity);
        technologies=std::move(validated.technologies);
        player_research=std::move(validated.player_research);
    } else if(loaded.live_bnw_team_progress) {
        if(!loaded.live_team_research)
            throw std::invalid_argument("BNW raw team progress requires paired live research section");
        BnwTeamProgressCheckpointPair pair{
            section(saves::CivisSaveSectionId::live_bnw_team_progress,
                    *loaded.live_bnw_team_progress),
            section(saves::CivisSaveSectionId::live_team_research,
                    *loaded.live_team_research)};
        auto validated=restore_bnw_team_progress_checkpoint_pair(
            pair,source.snapshot,setup,technologies,player_research,
            rules.technologies,rules.types,rules.identity);
        restore_bnw_team_progress_checkpoint(
            pair.raw,world,source.snapshot,rules.identity);
        technologies=std::move(validated.technologies);
        player_research=std::move(validated.player_research);
    } else if(loaded.live_team_research && !loaded.live_bnw_research_queue)
        restore_research_checkpoint(section(saves::CivisSaveSectionId::live_team_research,
            *loaded.live_team_research),std::nullopt,technologies,player_research,
            setup,rules.technologies,rules.identity);
    auto cities=restore_city_yields(world,setup,rules.database,rules.city_yields,rules.types);
    const auto unhappiness_rules=load_bnw_unhappiness_rules(rules.database);
    auto unhappiness=restore_bnw_unhappiness(world,setup,cities,unhappiness_rules,rules.types);
    (void)bind_bnw_unhappiness(unhappiness,setup,cities,unhappiness_rules,rules.defines);
    restore_production_progress(world,rules.production,rules.types,cities);
    auto research_turn=restore_research_turn(world,setup,rules.technologies,rules.types,rules.database);
    auto research_selection=restore_research_selection(world,setup,rules.database);
    auto random=restore_session_random(world.progression);
    if(loaded.live_bnw_research_queue) {
        // ID33 is the raw companion of exactly this ID8+ID11 pair. Validate
        // against the unmodified owned baseline before applying any part of
        // the choice to the composed world. ID31 has an exclusive ID8 owner.
        LiveSessionState baseline{
            world,setup,technologies,player_research,cities,unhappiness,{},
            {},{},research_turn,random,research_selection,game_clock,
            {},{},{},{},{},{},{},{},{},{},{},{},{},{}};
        const BnwResearchQueueCheckpointBundle bundle{
            section(saves::CivisSaveSectionId::live_bnw_research_queue,
                    *loaded.live_bnw_research_queue),
            section(saves::CivisSaveSectionId::live_team_research,
                    *loaded.live_team_research),
            section(saves::CivisSaveSectionId::live_research_turn,
                    *loaded.live_research_turn)};
        auto validated=restore_bnw_research_queue_checkpoint_bundle(
            bundle,baseline,source.snapshot,rules.technologies,rules.types,
            rules.identity);
        restore_bnw_research_queue_checkpoint(
            bundle.raw,world,source.snapshot,rules.identity);
        technologies=std::move(validated.technologies);
        player_research=std::move(validated.players);
        research_turn=std::move(validated.turn);
    }
    auto maintenance_world=MaintenanceWorld{setup,player_research,cities,technologies};
    auto maintenance=restore_maintenance(world,maintenance_world,rules.maintenance,rules.types);
    auto treasury=restore_treasury(world,maintenance_world,rules.treasury,rules.types);
    auto science=restore_science(world,setup,maintenance_world,rules.database);

    // Restorers mutate only these local values. A malformed or mismatched
    // overlay cannot publish a partially restored LiveSession.
    if(loaded.live_city_production &&
       (loaded.live_bnw_city_core || loaded.live_bnw_city_production)) {
        const auto from_raw=make_production_checkpoint(cities,rules.production,rules.types);
        if(from_raw.bytes!=*loaded.live_city_production)
            throw std::invalid_argument(
                "BNW raw city production and live production checkpoint disagree");
    }
    if(loaded.live_city_production)
        restore_production_checkpoint(section(saves::CivisSaveSectionId::live_city_production,
            *loaded.live_city_production),rules.production,rules.types,cities);
    if(loaded.live_bnw_treasury_balance) {
        if(!loaded.live_treasury)
            throw std::invalid_argument("BNW raw treasury balance requires paired live treasury section");
        BnwTreasuryBalanceCheckpointPair pair{
            section(saves::CivisSaveSectionId::live_bnw_treasury_balance,
                    *loaded.live_bnw_treasury_balance),
            section(saves::CivisSaveSectionId::live_treasury,
                    *loaded.live_treasury)};
        // Validate the pair against the immutable source and unmodified
        // treasury baseline before applying it to the composed live world.
        auto validated=restore_bnw_treasury_balance_checkpoint_pair(
            pair,source.snapshot,treasury,setup,rules.identity,
            loaded.live_bnw_game_clock ? &world : nullptr);
        restore_bnw_treasury_balance_checkpoint(
            pair.raw,world,source.snapshot,rules.identity);
        treasury=std::move(validated.treasury);
    } else if(loaded.live_treasury)
        restore_treasury_checkpoint(section(saves::CivisSaveSectionId::live_treasury,
            *loaded.live_treasury),treasury,setup,rules.identity);
    if(loaded.live_bnw_maintenance_expense) {
        std::optional<saves::CivisSaveSection> gold_companion;
        if(loaded.live_bnw_treasury_balance)
            gold_companion=section(saves::CivisSaveSectionId::live_bnw_treasury_balance,
                                   *loaded.live_bnw_treasury_balance);
        auto restored=restore_bnw_maintenance_expense_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_maintenance_expense,
                    *loaded.live_bnw_maintenance_expense),
            world,source.snapshot,maintenance,maintenance,rules.identity,
            gold_companion ? &*gold_companion : nullptr);
        world=std::move(restored.world);
        maintenance=std::move(restored.maintenance);
    }
    if(loaded.live_research_turn && !loaded.live_bnw_research_queue)
        restore_research_turn_checkpoint(section(saves::CivisSaveSectionId::live_research_turn,
            *loaded.live_research_turn),research_turn,setup,rules.technologies,rules.identity);
    if(loaded.live_research_selection)
        restore_research_selection_checkpoint(
            section(saves::CivisSaveSectionId::live_research_selection,
                    *loaded.live_research_selection),
            research_selection,player_research,research_turn,setup,
            rules.technologies,rules.identity,
            loaded.live_team_research.has_value(),
            loaded.live_research_turn.has_value());
    if(loaded.live_simulation_random&&!food_restore)
        restore_simulation_random_checkpoint(section(saves::CivisSaveSectionId::live_simulation_random,
            *loaded.live_simulation_random),random,rules.identity);

    LiveSessionState live_state{
        std::move(world),std::move(setup),std::move(technologies),
        std::move(player_research),std::move(cities),std::move(unhappiness),std::move(maintenance),
        std::move(treasury),std::move(science),std::move(research_turn),std::move(random),
        std::move(research_selection),std::move(game_clock), {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    live_state.standalone_services=standalone_services;
    if(loaded.live_bnw_player_administration) {
        auto bundle=restore_bnw_player_administration_notification_bundle(
            section(saves::CivisSaveSectionId::live_bnw_player_administration,
                    *loaded.live_bnw_player_administration),
            live_state,source,rules.database,rules.identity);
        live_state=std::move(bundle.state);
    }
    if(loaded.live_bnw_player_knowledge) {
        auto bundle=restore_bnw_player_knowledge_prophet_bundle(
            section(saves::CivisSaveSectionId::live_bnw_player_knowledge,
                    *loaded.live_bnw_player_knowledge),
            live_state,source,rules.technologies,rules.types,rules.database,
            rules.identity);
        live_state=std::move(bundle.state);
    }
    initialize_bnw_player_read_transients(live_state);
    if(loaded.live_bnw_culture_bomb_timer)
        restore_bnw_culture_bomb_timer_checkpoint(
            section(saves::CivisSaveSectionId::live_bnw_culture_bomb_timer,
                    *loaded.live_bnw_culture_bomb_timer),
            live_state.player_culture_bomb_timers,source.snapshot,rules.identity);
    if(loaded.live_bnw_military_requests && !loaded.live_bnw_player_strategy)
        throw std::invalid_argument("Military checkpoint restore requires actual ID23 companion");
    if(loaded.live_bnw_military_requests)
        restore_bnw_military_request_checkpoint(
            {bnw_military_request_checkpoint_section_id,0,*loaded.live_bnw_military_requests},
            live_state,source.snapshot,rules.identity);
    // Actual city plot links are independently resolvable after all players
    // Read. Native allocation-address tie order is not serialized by BNW.
    resolve_bnw_city_plot_links(live_state,true);
    try {
        rebuild_bnw_nearby_cities(live_state,true);
    } catch (const BnwNearbyCityOrderUnavailable &e) {
        // Retain the exact typed readiness frontier, not a successful order.
        // Malformed profiles/links and all other load failures still escape.
        if (live_state.nearby_cities.current ||
            !live_state.nearby_cities.order_gap ||
            *live_state.nearby_cities.order_gap!=e.gap()) throw;
    }
    rebuild_bnw_city_flavor_recipients(live_state);
    rebuild_bnw_player_flavor_recipients(live_state);
    rebuild_bnw_live_plot_impassability(live_state,rules.database,rules.types,
                                        rules.identity);
    rebuild_bnw_adjacent_land(live_state);
    rebuild_bnw_live_plot_visibility(live_state);
    live_state.diplomacy_scratchpad=
        restore_bnw_diplomacy_scratchpad_after_player_read(
            source,live_state,rules.database,rules.identity);
    // The scratchpad needs the original clock; CvGame::Read then exposes its
    // normalized scalar state to the live world, leaving the source intact.
    apply_bnw_game_read_scalar_hooks(live_state.world.game_state);
    apply_bnw_game_read_session_hook(live_state.world);
    apply_bnw_game_read_active_player_hook(live_state.world,live_state.setup);
    live_state.notification_load_transients=
        rebuild_bnw_notification_load_transients(live_state,source,rules.identity);
    live_state.player_plot_lists=
        rebuild_bnw_player_plot_lists(live_state,source,rules.identity);
    live_state.player_read_tail=
        rebuild_bnw_player_read_tail_transients(live_state,source,rules.identity);
    live_state.game_plot_extras=
        restore_bnw_game_plot_extras(live_state,source,rules.identity);
    if(loaded.live_bnw_unit_mission_continuation) {
        // All local world/clock/RNG overlays and load hooks above returned.
        // Validate source/rules only, retain exact bytes, NEVER invoke Core
        // restore or replay ClearPathCache/Flags/Game effects at this stage.
        auto pending=section(saves::CivisSaveSectionId::live_bnw_unit_mission_continuation,
                             *loaded.live_bnw_unit_mission_continuation);
        validate_bnw_unit_mission_continuation_checkpoint_binding(pending,rules.identity);
        live_state.bnw_unit_mission_continuation_quarantine=
            std::make_shared<const saves::CivisSaveSection>(std::move(pending));
    }
    auto session=from_restored_bnw_owned_serial(std::move(source),std::move(live_state),std::move(serial_restore),std::move(food_restore));
    imported_pregame->bind_actual_restored(session.source_,session.state_,session.state_generation_);
    session.bnw_imported_pregame_read_cache_=std::move(imported_pregame);
    session.bnw_unhappiness_current_=true;
    static_assert(std::is_nothrow_move_constructible_v<BnwGameDebugReadState>);
    session.debug_=std::make_shared<BnwGameDebugReadStorage>(std::move(debug_read));
    session.bind_bnw_game_debug_owner();
    if(imported_pathfinder){
        auto current=std::move(pathfinder_rules_current);
        if(!current||!current())throw BnwCommonPathfinderUnavailable("Original map setup immutable rules retired");
        if(!session.state_->world.map_plots||!session.state_->world.map_projection)
            throw BnwCommonPathfinderUnavailable("Original map setup actual plot allocation unavailable");
        session.pathfinder_=std::make_shared<BnwPathfinderOwnedStorage>(session.source_,session.state_,
            session.state_generation_,std::move(current),std::move(*imported_pathfinder));
        session.require_bnw_pathfinder();
    }
    session.qualify_bnw_game_read_final_member();
    return session;
}

void LiveSession::qualify_bnw_game_read_final_member() {
    if(!bnw_game_debug_owner_current()||!source_||!state_)
        throw std::invalid_argument("Game Read requires actual restored debug lifetime");
    const auto& original=source_->snapshot.game_state;
    const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&original.prefix);
    const auto& bytes=source_->source.bytes;
    constexpr std::uint64_t wire_prefix=4+29*4;
    if(original.layout!=bnw_layout||!prefix||prefix->version!=1||
       original.start_offset>bytes.size()||bytes.size()-original.start_offset<wire_prefix+3||
       prefix->end_offset!=original.start_offset+wire_prefix||
       original.end_offset<=prefix->end_offset+2||original.end_offset>bytes.size())
        throw std::invalid_argument("Game final member wire span unavailable");
    const auto at=static_cast<std::size_t>(original.start_offset);
    if(bytes[at]!=1||bytes[at+1]!=0||bytes[at+2]!=0||bytes[at+3]!=0)
        throw std::invalid_argument("Game final member unverified BNW version wire");
    const auto wire=bytes[static_cast<std::size_t>(prefix->end_offset+2)];
    if(wire>1)throw std::invalid_argument("Game final member unverified boolean wire");
    const auto exact=[](const saves::GameStateObservation& game){
        const saves::NamedSaveFlag* found=nullptr;
        for(const auto& flag:game.flags)if(flag.name&&std::string_view(flag.name)=="final_initialized"){
            if(found)throw std::invalid_argument("Game final member duplicated");found=&flag;
        }
        if(!found)throw std::invalid_argument("Game final member missing");return found->value;
    };
    if(exact(original)!=(wire!=0)||exact(state_->world.game_state)!=(wire!=0))
        throw std::invalid_argument("Actual Game Read final member differs from immutable wire");
    // Read/reset and retained load hooks returned. This qualifies one member,
    // never full-game readiness or a fresh-game init2 publisher.
    bnw_game_read_final_initialized_=wire!=0;
    bnw_game_read_final_state_=state_.get();
    bnw_game_read_final_generation_=state_generation_;
    bnw_game_read_final_qualified_=true;
}

LiveSession LiveSession::from_restored_bnw(saves::SavedSession source,
                                          LiveSessionState state) {
    return from_restored_bnw_owned_serial(std::move(source),std::move(state),{});
}
LiveSession LiveSession::from_restored_bnw_owned_serial(saves::SavedSession source,
    LiveSessionState state,std::shared_ptr<BnwUnitSerialRestoreStorage> serial,
    std::shared_ptr<BnwFoodGrowthImportAdmission> food) {
    if(source.snapshot.game_state.layout!=bnw_layout ||
       state.world.game_state.layout!=bnw_layout ||
       state.setup.profile!=bnw_profile ||
       state.player_research.profile!=bnw_profile ||
       state.cities.profile!=bnw_profile ||
       state.maintenance.profile!=bnw_profile ||
       state.treasury.profile!=bnw_profile ||
       state.science.profile!=bnw_profile ||
       state.research_turn.profile!=bnw_profile ||
       state.research_selection.profile!=bnw_profile)
        throw std::invalid_argument("Restored BNW live state has mixed profiles");
    const auto& pending=state.bnw_unit_mission_continuation_quarantine;
    const auto& imported=source.source.live_bnw_unit_mission_continuation;
    if(imported || pending) {
        if(!imported || !pending || pending->bytes!=*imported)
            throw std::invalid_argument("BNW mission quarantine differs from immutable imported journal");
        validate_bnw_unit_mission_continuation_checkpoint_shape(*pending);
    }
    saves::require_supported_standalone_services(state.standalone_services);
    state.city_strategy_runtime.clear(); // Generic restored DTO cannot mint this owner.
    if(source.source.live_bnw_population_totals&&!source.source.live_bnw_food_growth_journal)throw std::invalid_argument("Population totals checkpoint cannot mint a generic restored live session");
    if(source.source.live_bnw_food_growth_journal){
        if(!food||food->creator_!=std::this_thread::get_id()||food->retired_||food->attempted_||food->restored_||food->generation_||
            food->journal_.bytes!=*source.source.live_bnw_food_growth_journal)
            throw std::invalid_argument("Food47 generic restored state cannot mint actual import once history");
    }else if(food)throw std::invalid_argument("Food47 admission has no actual imported journal");
    if(source.source.live_bnw_unit_serial){
        if(!serial||serial->creator!=std::this_thread::get_id()||serial->retired.load()||
           serial->section.bytes!=*source.source.live_bnw_unit_serial||serial->generation||
           !state.world.all_players||state.world.all_players->players.size()!=64)
            throw std::invalid_argument("Serial45 generic restored state cannot mint allocator history");
        for(std::size_t i=0;i<64;++i)if(state.world.all_players->players[i].units.header.fields.at(4).value!=serial->starts[i])
            throw std::invalid_argument("Serial45 restored raw counter changed before publication");
    }else if(serial)throw std::invalid_argument("Serial45 issuer has no imported section");
    LiveSession result(std::move(source),std::move(state));
    if(food){food->source_=result.source_;food->state_=result.state_;food->generation_=result.state_generation_;result.bnw_food_growth_import_=std::move(food);}
    if(serial){serial->source=result.source_;serial->state=result.state_;serial->generation=result.state_generation_;result.bnw_unit_serial_restore_=std::move(serial);}
    return result;
}
void LiveSession::initialize_original_payload_mission_pathfinder(
 const BnwNativeUnitAllocatorCore& core,const ui::SelectedBnwPreviewHost& host) {
 if(plot_yield_refresh_thread_!=std::this_thread::get_id() ||
    host.saved_database_thread_!=std::this_thread::get_id() || host.session_.get()!=this)
    throw BnwCommonPathfinderUnavailable("Mission reset foreign or different unpublished Host/session");
 const auto& pending=state_->bnw_unit_mission_continuation_quarantine;
 if(!pending || !source_->source.live_bnw_unit_mission_continuation ||
    pending->bytes!=*source_->source.live_bnw_unit_mission_continuation ||
    source_->source.container!=saves::SaveContainerKind::civis || pathfinder_)
    throw BnwCommonPathfinderUnavailable("Mission reset needs one bound journal and absent fresh finder");
 core.require_native_common_pathfinder_binding(*this);
 const auto actual_source=source_;const auto actual_state=state_;const auto gen=state_generation_;
 auto controls=core.make_original_payload_mission_pathfinder_controls();
 auto paths=core.make_original_payload_mission_unit_paths();
 auto current=host.database_->immutable_preview_fingerprint_currentness();
 if(!current || !current() || !state_->world.map_plots || !state_->world.map_projection)
    throw BnwCommonPathfinderUnavailable("Mission reset actual immutable rules/map owner unavailable");
 auto staged=std::make_shared<BnwPathfinderOwnedStorage>(actual_source,actual_state,gen,
    std::move(current),std::move(controls));
 staged->native_unit_paths.emplace(std::move(paths));
 core.require_native_common_pathfinder_binding(*this);
 if(pathfinder_ || source_!=actual_source || state_!=actual_state || state_generation_!=gen ||
    !staged->rules_current() || staged->controls.identity.payload_sha256!=host.identity_.payload_sha256 ||
    staged->controls.identity.rules_sha256!=host.identity_.rules_sha256)
    throw BnwCommonPathfinderUnavailable("Mission reset final actual source changed during preparation");
 core.require_native_common_pathfinder_session_without_callbacks(*this);
 // Actual NEW source ctor/Initialize controls and native Reset paths, never
 // a reset of retained search/history. All allocations/providers are above.
 pathfinder_=std::move(staged);
}
void LiveSession::bind_restored_unit_mission_owner(
 const std::shared_ptr<const BnwNativeUnitAllocatorCore>& core,
 const ui::SelectedBnwPreviewHost& host) {
 if(plot_yield_refresh_thread_!=std::this_thread::get_id() ||
    host.saved_database_thread_!=std::this_thread::get_id() || host.session_.get()!=this ||
    !core || bnw_restored_mission_core_ || !host.bnw_restored_mission_core_ ||
    host.bnw_restored_mission_core_->owner_before(core) || core.owner_before(*host.bnw_restored_mission_core_))
    throw std::runtime_error("Mission resolution requires the one actual unpublished Host/Core issuer");
 const auto& pending=state_->bnw_unit_mission_continuation_quarantine;
 if(!pending || !source_->source.live_bnw_unit_mission_continuation ||
    pending->bytes!=*source_->source.live_bnw_unit_mission_continuation)
    throw std::runtime_error("Mission resolution lost immutable imported journal");
 core->require_restored_mission_checkpoint_current_without_callbacks(*pending);
 core->require_native_common_pathfinder_session_without_callbacks(*this);
 // Earned weak binding, not deletion of bytes or a completed DTO. Expiry or
 // source/generation changes again make the readiness observation refuse.
 bnw_restored_mission_core_.emplace(core);
 bnw_restored_mission_state_=state_.get();bnw_restored_mission_source_=source_.get();
 bnw_restored_mission_generation_=state_generation_;
}
bool LiveSession::has_bnw_unit_mission_continuation_quarantine() const {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("BNW mission quarantine belongs to another session thread");
    if(!source_||!state_)throw std::invalid_argument("BNW mission quarantine requires a live facade");
    // The immutable input is authoritative too: erasing a detached DTO field
    // cannot erase the imported once-operation journal/readiness frontier.
    const auto& pending=state_->bnw_unit_mission_continuation_quarantine;
    if(!source_->source.live_bnw_unit_mission_continuation && !pending)return false;
    if(!pending || !source_->source.live_bnw_unit_mission_continuation ||
       pending->bytes!=*source_->source.live_bnw_unit_mission_continuation ||
       !bnw_restored_mission_core_ || bnw_restored_mission_state_!=state_.get() ||
       bnw_restored_mission_source_!=source_.get() ||
       bnw_restored_mission_generation_!=state_generation_)return true;
    try {
        // The weak promotion is counted INSIDE the actual Core owner. It
        // cannot impersonate the independent external result/Preview holder.
        BnwNativeUnitAllocatorCore::require_restored_mission_weak_owner_continuity_current_without_callbacks(
            *bnw_restored_mission_core_,*pending);
    } catch(const std::exception&) {return true;}
    return false;
}
LiveSessionReadiness LiveSession::readiness() const {
    // Existing restored contexts are query/diagnostic state. They and the
    // mutable decoded world may diverge after a writer acts; no cache/update
    // graph or authoritative full-world save exists yet. Keep continuation
    // blocked even after a successful import or in-memory rollback.
    LiveSessionReadiness result;
    if(has_bnw_unit_mission_continuation_quarantine())
        result.blockers.push_back(LiveSessionBlocker::bnw_unit_mission_continuation_restore_required);
    if (!state_->nearby_cities.current && state_->nearby_cities.order_gap)
        result.blockers.push_back(
            LiveSessionBlocker::bnw_nearby_city_pointer_order_unavailable);
    if(!bnw_unhappiness_current_ || !state_->nearby_cities.current ||
       !state_->city_flavor_recipients.current ||
       !state_->player_flavor_recipients.current ||
       !state_->plot_impassability.current || !state_->adjacent_land.current ||
       !state_->plot_visibility.current ||
       !state_->notification_load_transients.current ||
       !state_->player_plot_lists.current ||
       !state_->player_read_tail.current ||
       !state_->game_plot_extras.current)
        result.blockers.push_back(LiveSessionBlocker::bnw_query_caches_dirty);
    result.blockers.insert(result.blockers.end(),{
             LiveSessionBlocker::load_hooks_unimplemented,
             LiveSessionBlocker::gamecore_services_unimplemented,
             LiveSessionBlocker::full_turn_unimplemented,
             LiveSessionBlocker::full_persistence_unimplemented,
             LiveSessionBlocker::lua_turn_entrypoints_unimplemented});
    return result;
}

bool LiveSession::bnw_game_debug_mode() const {
    // Constructor/import thread FIRST, before debug/source/state members.
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("BNW Game debug cache belongs to another session thread");
    if (!bnw_game_debug_owner_current())
        throw std::invalid_argument("BNW Game debug cache requires current actual reset/Read owner");
    if (debug_->raw_mode != debug_->cached_mode)
        throw std::invalid_argument("BNW Game debug cache has an unowned update transition");
    return debug_->cached_mode;
}

void LiveSession::require_bnw_pathfinder()const {
    // Constructor thread FIRST, before mutable cache flags or current storage.
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw BnwCommonPathfinderUnavailable("Pathfinder belongs to another session thread");
    const auto held=pathfinder_;
    const auto need=[](bool ok,const char* why){if(!ok)throw BnwCommonPathfinderUnavailable(why);};
    need(held&&held->thread==std::this_thread::get_id()&&!held->retired.load(),
        "Actual original-import common pathfinder service absent/retired");
    need(state_&&source_&&held->state==state_&&held->source==source_&&held->generation==state_generation_,
        "Common pathfinder SAME source/world/generation differs");
    need(held->rules_current&&held->rules_current()&&bnw_game_debug_owner_current(),
        "Common pathfinder actual rules/Game allocation retired");
    need(debug_->identity.payload_sha256==held->controls.identity.payload_sha256&&
        debug_->identity.rules_sha256==held->controls.identity.rules_sha256,
        "Common pathfinder actual source/rules identity differs");
    const auto& world=state_->world;const auto& map=world.map_header;
    need(map.layout==bnw_layout&&world.map_plots&&world.map_plots->layout==bnw_layout&&world.map_projection&&
        state_->setup.profile==bnw_profile&&map.guid==held->controls.map_guid&&
        map.grid.width==held->controls.grid.width&&map.grid.height==held->controls.grid.height&&
        map.wrap_x==held->controls.wrap_x&&map.wrap_y==held->controls.wrap_y&&
        world.map_plots->plots.data()==held->plot_storage&&world.map_projection->cells.data()==held->projection_storage&&
        world.map_plots->plots.size()==map.grid.size()&&world.map_projection->cells.size()==map.grid.size()&&
        world.map_projection->grid.width==map.grid.width&&world.map_projection->grid.height==map.grid.height&&
        world.map_projection->wrap_x==map.wrap_x&&world.map_projection->wrap_y==map.wrap_y,
        "Common pathfinder actual map allocation/shape differs");
    constexpr std::array<BnwActivationFinder,6> order{
        BnwActivationFinder::common,BnwActivationFinder::ignore_units,BnwActivationFinder::route,
        BnwActivationFinder::water_route,BnwActivationFinder::influence,BnwActivationFinder::tactical};
    need(held->controls.initialized&&held->controls.initialization_order.size()==order.size()&&
        std::equal(order.begin(),order.end(),held->controls.initialization_order.begin()),
        "Common pathfinder actual source Initialize/setup order differs");
}

std::shared_ptr<BnwTwoLayerUnitPathFinder> LiveSession::bnw_common_unit_pathfinder_for_native_unit(const BnwNativeUnitAllocatorCore&core)const{
 if(plot_yield_refresh_thread_!=std::this_thread::get_id())throw BnwCommonPathfinderUnavailable("Foreign native common path factory before source");
 core.require_native_common_pathfinder_session_without_callbacks(*this);
 core.require_native_common_pathfinder_binding(*this);require_bnw_pathfinder();
 core.require_native_common_pathfinder_session_without_callbacks(*this);
 const auto held=pathfinder_;
 if(!held->common_unit_tree){
  auto tree=std::unique_ptr<BnwTwoLayerUnitPathFinder>(new BnwTwoLayerUnitPathFinder(
   held->controls.at(BnwActivationFinder::common),held->controls.grid.width,held->controls.grid.height,held->controls.wrap_x,held->controls.wrap_y));
  core.require_native_common_pathfinder_session_without_callbacks(*this);
  if(pathfinder_.get()!=held.get()||held->retired.load())throw BnwCommonPathfinderUnavailable("Common tree source retired during allocation");
  held->common_unit_tree=std::move(tree);
 }
 // The alias pins this very controls allocation, never a detached control.
 auto result=std::shared_ptr<BnwTwoLayerUnitPathFinder>(held,held->common_unit_tree.get());
 require_bnw_common_unit_pathfinder_native_current_without_callbacks(*result,core);return result;
}
void LiveSession::require_bnw_common_unit_pathfinder_native_current_without_callbacks(
 const BnwTwoLayerUnitPathFinder&tree,const BnwNativeUnitAllocatorCore&core)const{
 if(plot_yield_refresh_thread_!=std::this_thread::get_id())throw BnwCommonPathfinderUnavailable("Foreign native common path fence before controls");
 core.require_native_common_pathfinder_session_without_callbacks(*this);
 const auto held=pathfinder_;
 const auto need=[](bool ok,const char*why){if(!ok)throw BnwCommonPathfinderUnavailable(why);};
 need(held&&held->thread==std::this_thread::get_id()&&!held->retired.load()&&
  held->state==state_&&held->source==source_&&held->generation==state_generation_&&
  held->common_unit_tree.get()==&tree&&tree.control_==&held->controls.at(BnwActivationFinder::common),
  "Native common path SAME tree/control/world/owner retired");
 tree.require_thread();
 need(bnw_game_debug_owner_current()&&debug_->raw_mode==debug_->cached_mode&&
  debug_->identity.payload_sha256==held->controls.identity.payload_sha256&&debug_->identity.rules_sha256==held->controls.identity.rules_sha256,
  "Native common path genuine Game/source identity changed");
 const auto&w=state_->world;const auto&m=w.map_header;
 need(w.map_plots&&w.map_projection&&m.layout==bnw_layout&&w.map_plots->layout==bnw_layout&&state_->setup.profile==bnw_profile&&
  m.guid==held->controls.map_guid&&m.grid.width==held->controls.grid.width&&m.grid.height==held->controls.grid.height&&
  m.wrap_x==held->controls.wrap_x&&m.wrap_y==held->controls.wrap_y&&
  w.map_plots->plots.data()==held->plot_storage&&w.map_projection->cells.data()==held->projection_storage&&
  w.map_plots->plots.size()==m.grid.size()&&w.map_projection->cells.size()==m.grid.size()&&
  w.map_projection->grid.width==m.grid.width&&w.map_projection->grid.height==m.grid.height&&
  w.map_projection->wrap_x==m.wrap_x&&w.map_projection->wrap_y==m.wrap_y&&
  tree.width_==static_cast<std::int64_t>(m.grid.width)&&tree.height_==static_cast<std::int64_t>(m.grid.height)&&tree.wrap_x_==m.wrap_x&&tree.wrap_y_==m.wrap_y,
  "Native common path map allocation/geometry changed");
 constexpr std::array<BnwActivationFinder,6>order{BnwActivationFinder::common,BnwActivationFinder::ignore_units,BnwActivationFinder::route,BnwActivationFinder::water_route,BnwActivationFinder::influence,BnwActivationFinder::tactical};
 need(held->controls.initialized&&held->controls.initialization_order.size()==order.size()&&
  std::equal(order.begin(),order.end(),held->controls.initialization_order.begin()),"Native common path actual setup order differs");
}

BnwUnitPathOwner& LiveSession::bnw_native_unit_paths_for_core(const BnwNativeUnitAllocatorCore&core,const BnwTwoLayerUnitPathFinder&tree)const{
 require_bnw_common_unit_pathfinder_native_current_without_callbacks(tree,core);
 core.require_native_common_pathfinder_binding(*this);require_bnw_pathfinder();const auto held=pathfinder_;
 if(!held->native_unit_paths){
  auto actual=bind_bnw_unit_paths_original_import(*source_,held->controls.identity);
  core.require_native_common_pathfinder_binding(*this);
  require_bnw_common_unit_pathfinder_native_current_without_callbacks(tree,core);
  if(pathfinder_.get()!=held.get()||held->retired.load())throw BnwCommonPathfinderUnavailable("Native retained paths retired during actual original Read factory");
  held->native_unit_paths.emplace(std::move(actual));
 }
 require_bnw_native_unit_paths_current_without_callbacks(*held->native_unit_paths,tree,core);return *held->native_unit_paths;
}
void LiveSession::require_bnw_native_unit_paths_current_without_callbacks(const BnwUnitPathOwner&paths,const BnwTwoLayerUnitPathFinder&tree,const BnwNativeUnitAllocatorCore&core)const{
 require_bnw_common_unit_pathfinder_native_current_without_callbacks(tree,core);
 const auto held=pathfinder_;const auto need=[](bool ok,const char*why){if(!ok)throw BnwCommonPathfinderUnavailable(why);};
 need(held->native_unit_paths&&&*held->native_unit_paths==&paths&&paths.identity_.payload_sha256==held->controls.identity.payload_sha256&&paths.identity_.rules_sha256==held->controls.identity.rules_sha256&&paths.map_guid_==state_->world.map_header.guid,"Native retained paths SAME common storage/source owner differs");
 need(state_->world.all_players&&state_->world.all_players->layout==bnw_layout,"Native retained paths actual roster unavailable");std::size_t count=0;
 for(const auto&player:state_->world.all_players->players)for(const auto&unit:player.units.units){const saves::UnitCoreField*id=nullptr,*owner=nullptr;
  for(const auto&field:unit.core.fields)if(field.name){if(std::string_view(field.name)=="m_iID"){need(!id,"Native retained path duplicate ID");id=&field;}else if(std::string_view(field.name)=="m_eOwner"){need(!owner,"Native retained path duplicate owner");owner=&field;}}
  need(id&&owner&&id->values.size()==1&&owner->values.size()==1&&owner->values[0]==player.saved_player_id&&paths.paths_.contains({player.saved_player_id,id->values[0]}),"Native retained path actual imported roster no longer matches Reset registration");++count;
 }
 need(count==paths.paths_.size(),"Native retained paths actual roster extent changed");
}

bool LiveSession::bnw_common_pathfinder_force_reset()const {
    require_bnw_pathfinder();const bool result=pathfinder_->controls.at(BnwActivationFinder::common).force_reset();
    require_bnw_pathfinder();return result;
}
void LiveSession::force_reset_bnw_common_pathfinder() {
    require_bnw_pathfinder();
    // Native CvAStar::ForceReset has no SP/alive/turn/scope predicates.
    pathfinder_->controls.at(BnwActivationFinder::common).force_reset_cache();
    // No callback, allocation or throwing source query after the native flag
    // store: Pending records it immediately, then performs its post fences.
}
void LiveSession::retire_bnw_pathfinder()noexcept {
    if(pathfinder_)pathfinder_->retired.store(true);
}
void LiveSession::advance_state_generation() {
    if (state_generation_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Live session state generation exhausted");
    retire_bnw_unit_move();retire_bnw_food_growth();
    retire_bnw_pathfinder();
    retire_plot_yield_refresh();
    retire_bnw_city_entry();
    retire_bnw_production_turn();
    retire_bnw_client_selection();
    state_->city_strategy_runtime.clear();
    ++state_generation_;
}

void LiveSession::retire_plot_yield_refresh() noexcept {
    if(city_yield_refresh_registry_)city_yield_refresh_registry_->retire();
    if (plot_yield_refresh_issuer_) plot_yield_refresh_issuer_->retire();
    for (const auto& [plot,pending] : plot_yield_refresh_pending_) {
        (void)plot;
        if (pending) pending->invalidate();
    }
    plot_yield_refresh_issuer_.reset();
    // Retain once/history slots. A replacement or mutable alias cannot erase
    // a mandatory publication by issuing a new clean cursor for the same plot.
}

void LiveSession::retire_bnw_production_turn()noexcept {
    for(const auto& [key,pending]:production_turn_pending_){(void)key;if(pending)pending->invalidate();}
}

void LiveSession::retire_bnw_city_entry()noexcept {
    for(const auto& [key,pending]:city_entry_pending_) {
        (void)key;if(pending)pending->invalidate();
    }
    // Keep every once/history slot across facade and allocation retirement.
}

void LiveSession::retire_bnw_client_selection()noexcept {
    if(const auto owner=city_selection_.lock())owner->retire();
    city_selection_.reset();
}

std::shared_ptr<LiveSession::RetainedFacadeLifetime>
LiveSession::retain_bnw_facade_lifetime() const {
    // The owner comes from the actual session constructor/move contract,
    // never from whichever thread happens to request the lazy first issue.
    if(plot_yield_refresh_thread_!=std::this_thread::get_id()){
        throw std::runtime_error("Retained facade foreign thread before live borrow");
    }
    if(!retained_facade_lifetime_)
        retained_facade_lifetime_=std::make_shared<RetainedFacadeLifetime>(this,plot_yield_refresh_thread_);
    const auto token=retained_facade_lifetime_;
    token->require_current();
    if(token->facade!=this)
        throw std::runtime_error("Retained facade inherited a different source lifetime");
    return token;
}
void LiveSession::retire_bnw_facade_lifetime(bool destroyed) noexcept {
    if(retained_facade_lifetime_){
        retained_facade_lifetime_->retired.store(true);
        if(destroyed)retained_facade_lifetime_->destroyed.store(true);
    }
}

LiveSession::~LiveSession() { if(bnw_food_growth_import_)bnw_food_growth_import_->retired_=true; retire_bnw_unit_serial_restore();retire_bnw_facade_lifetime(true);retire_bnw_pathfinder();retire_bnw_unit_move();retire_bnw_food_growth(); retire_bnw_production_turn();retire_bnw_client_selection();retire_plot_yield_refresh();retire_bnw_city_entry();retire_bnw_game_debug_owner(); }

LiveSession::LiveSession(LiveSession&& other) noexcept {
    // No allocation in noexcept move. Source keeps its tombstoned anchor;
    // destination shares the SAME retired token and cannot mint fresh history.
    other.retire_bnw_facade_lifetime();
    retained_facade_lifetime_=other.retained_facade_lifetime_;
    other.retire_bnw_unit_move();other.retire_bnw_food_growth();
    other.retire_plot_yield_refresh(); // Before facade ownership changes.
    other.retire_bnw_city_entry();
    other.retire_bnw_production_turn();
    other.retire_bnw_client_selection();
    // A moved/overwritten facade cannot discard unsaved retained operations.
    bnw_retained_turn_history_=bnw_retained_turn_history_||other.bnw_retained_turn_history_;
    retire_bnw_unit_serial_restore();bnw_unit_serial_restore_=std::move(other.bnw_unit_serial_restore_);
    source_=std::move(other.source_); state_=std::move(other.state_);
    bnw_imported_pregame_read_cache_=std::move(other.bnw_imported_pregame_read_cache_);
    owner_=other.owner_; bnw_unhappiness_current_=other.bnw_unhappiness_current_;
    bnw_game_read_final_qualified_=other.bnw_game_read_final_qualified_;
    bnw_game_read_final_initialized_=other.bnw_game_read_final_initialized_;
    bnw_game_read_final_state_=other.bnw_game_read_final_state_;
    bnw_game_read_final_generation_=other.bnw_game_read_final_generation_;
    other.bnw_game_read_final_qualified_=false;
    other.bnw_game_read_final_state_=nullptr;other.bnw_game_read_final_generation_=0;
    state_generation_=other.state_generation_; debug_=std::move(other.debug_);
    plot_yield_refresh_thread_=other.plot_yield_refresh_thread_;
    debug_state_=other.debug_state_; debug_generation_=other.debug_generation_;
    plot_yield_refresh_pending_=std::move(other.plot_yield_refresh_pending_);
    city_entry_pending_=std::move(other.city_entry_pending_);
    production_turn_pending_=std::move(other.production_turn_pending_);
    city_yield_refresh_registry_=std::move(other.city_yield_refresh_registry_);
    unit_move_registry_=std::move(other.unit_move_registry_);
    food_growth_registry_=std::move(other.food_growth_registry_);
    bnw_food_growth_import_=std::move(other.bnw_food_growth_import_);
    // Transfer the SAME allocation and actual flags; never reinitialize on move.
    // Existing hooks already retire all old Pending/UI/VM facade issuers.
    pathfinder_=std::move(other.pathfinder_);
}

LiveSession& LiveSession::operator=(LiveSession&& other) noexcept {
    if (this==&other) return *this;
    if(bnw_food_growth_import_)bnw_food_growth_import_->retired_=true;
    retire_bnw_facade_lifetime();other.retire_bnw_facade_lifetime();
    // An issued destination identity never becomes a new clean epoch.
    // Only an unissued destination inherits the source's retired anchor.
    if(!retained_facade_lifetime_)retained_facade_lifetime_=other.retained_facade_lifetime_;
    retire_bnw_unit_move();retire_bnw_food_growth();other.retire_bnw_unit_move();other.retire_bnw_food_growth();
    retire_bnw_pathfinder(); // Destination service retires; SAME source service transfers below.
    retire_plot_yield_refresh();retire_bnw_game_debug_owner();other.retire_plot_yield_refresh();
    retire_bnw_city_entry();other.retire_bnw_city_entry();
    retire_bnw_production_turn();other.retire_bnw_production_turn();
    retire_bnw_client_selection();other.retire_bnw_client_selection();
    // A moved/overwritten facade cannot discard unsaved retained operations.
    bnw_retained_turn_history_=bnw_retained_turn_history_||other.bnw_retained_turn_history_;
    retire_bnw_unit_serial_restore();bnw_unit_serial_restore_=std::move(other.bnw_unit_serial_restore_);
    source_=std::move(other.source_); state_=std::move(other.state_);
    bnw_imported_pregame_read_cache_=std::move(other.bnw_imported_pregame_read_cache_);
    owner_=other.owner_; bnw_unhappiness_current_=other.bnw_unhappiness_current_;
    bnw_game_read_final_qualified_=other.bnw_game_read_final_qualified_;
    bnw_game_read_final_initialized_=other.bnw_game_read_final_initialized_;
    bnw_game_read_final_state_=other.bnw_game_read_final_state_;
    bnw_game_read_final_generation_=other.bnw_game_read_final_generation_;
    other.bnw_game_read_final_qualified_=false;
    other.bnw_game_read_final_state_=nullptr;other.bnw_game_read_final_generation_=0;
    state_generation_=other.state_generation_; debug_=std::move(other.debug_);
    plot_yield_refresh_thread_=other.plot_yield_refresh_thread_;
    debug_state_=other.debug_state_; debug_generation_=other.debug_generation_;
    plot_yield_refresh_pending_=std::move(other.plot_yield_refresh_pending_);
    city_entry_pending_=std::move(other.city_entry_pending_);
    production_turn_pending_=std::move(other.production_turn_pending_);
    city_yield_refresh_registry_=std::move(other.city_yield_refresh_registry_);
    unit_move_registry_=std::move(other.unit_move_registry_);
    food_growth_registry_=std::move(other.food_growth_registry_);
    bnw_food_growth_import_=std::move(other.bnw_food_growth_import_);
    // Transfer the SAME allocation and actual flags; never reinitialize on move.
    // Existing hooks already retire all old Pending/UI/VM facade issuers.
    pathfinder_=std::move(other.pathfinder_);
    return *this;
}

std::shared_ptr<const BnwPlotYieldRefreshPending>
LiveSession::begin_bnw_plot_early_zero_yield_refresh(std::int64_t plot) {
    if (plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Plot yield refresh belongs to another session thread");
    if (!source_ || !state_ || !state_generation_)
        throw std::invalid_argument("Plot yield refresh requires a live session facade");
    if(city_yield_refresh_registry_&&city_yield_refresh_registry_->has_history())
        throw std::invalid_argument("Standalone plot call cannot replay/mix retained city child history");
    for (const auto& [issued_plot,pending] : plot_yield_refresh_pending_) {
        (void)issued_plot;
        if (!pending)
            throw std::invalid_argument("Plot yield refresh has an unissued session slot");
        const auto phase=pending->progress().phase;
        if (phase!=BnwPlotYieldRefreshPhase::complete &&
            phase!=BnwPlotYieldRefreshPhase::skipped_no_area)
            throw std::invalid_argument("Plot yield refresh must finish its required effects before another plot");
    }
    const auto [slot,inserted]=plot_yield_refresh_pending_.try_emplace(plot);
    if (!inserted)
        throw std::invalid_argument("Plot yield refresh already owns this plot's pending/history slot");
    try {
        if (!plot_yield_refresh_issuer_)
            plot_yield_refresh_issuer_=std::shared_ptr<BnwPlotYieldRefreshIssuer>(
                new BnwPlotYieldRefreshIssuer(state_,state_generation_));
        slot->second=BnwPlotYieldRefreshPending::prepare(plot_yield_refresh_issuer_,state_,plot);
    } catch (...) {
        plot_yield_refresh_pending_.erase(slot); // No cursor or source stores issued.
        throw;
    }
    // Ownership assignment above is noexcept. Keep that exact once slot if
    // any later store fails; required UI/symbol effects have no success stub.
    slot->second->advance_prefix();
    return slot->second;
}

std::shared_ptr<const BnwPlotYieldRefreshPending>
LiveSession::begin_bnw_plot_normal_yield_refresh(std::int64_t plot,
                                               const BnwNormalPlotYieldRules& rules) {
    // Do not inspect debug_, source_ or state_ on a foreign calling thread.
    if (plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Plot yield refresh belongs to another session thread");
    rules.require_current(); // Before current import/world/source binding reads.
    if (!source_ || !state_ || !state_generation_ || !bnw_game_debug_owner_current())
        throw std::invalid_argument("Normal plot refresh requires current private BNW import identity");
    rules.require_current(debug_->identity.rules_sha256);
    (void)bnw_game_debug_mode(); // Current real raw/cache, including no-area paths.
    if(city_yield_refresh_registry_&&city_yield_refresh_registry_->has_history())
        throw std::invalid_argument("Standalone plot call cannot replay/mix retained city child history");
    for (const auto& [issued_plot,pending] : plot_yield_refresh_pending_) {
        (void)issued_plot;
        if (!pending)
            throw std::invalid_argument("Plot yield refresh has an unissued session slot");
        const auto phase=pending->progress().phase;
        if (phase!=BnwPlotYieldRefreshPhase::complete &&
            phase!=BnwPlotYieldRefreshPhase::skipped_no_area)
            throw std::invalid_argument("Plot yield refresh must finish its required effects before another plot");
    }
    const auto [slot,inserted]=plot_yield_refresh_pending_.try_emplace(plot);
    if (!inserted)
        throw std::invalid_argument("Plot yield refresh already owns this plot's pending/history slot");
    try {
        if (!plot_yield_refresh_issuer_)
            plot_yield_refresh_issuer_=std::shared_ptr<BnwPlotYieldRefreshIssuer>(
                new BnwPlotYieldRefreshIssuer(state_,state_generation_));
        plot_yield_refresh_issuer_->bind_game_debug(debug_,source_);
        slot->second=BnwPlotYieldRefreshPending::prepare_normal(
            plot_yield_refresh_issuer_,state_,source_,debug_->identity,rules,plot);
    } catch (...) {
        plot_yield_refresh_pending_.erase(slot); // No issued stores or external effects.
        throw;
    }
    // Publish into the SAME preallocated once slot before the first source store.
    slot->second->advance_prefix();
    return slot->second;
}

std::shared_ptr<const BnwPlotYieldRefreshPending>
LiveSession::bnw_plot_yield_refresh_pending(std::int64_t plot) const {
    if (plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Plot yield pending belongs to another session thread");
    if (!source_ || !state_)
        throw std::invalid_argument("Plot yield pending requires a live session facade");
    const auto found=plot_yield_refresh_pending_.find(plot);
    if (found==plot_yield_refresh_pending_.end() || !found->second)
        throw std::invalid_argument("Plot yield refresh has no issued cursor for this plot");
    found->second->require_current();
    return found->second;
}

std::shared_ptr<const BnwCityEntryPending>
LiveSession::begin_bnw_city_entry_damage_zero(std::int64_t owner,std::int64_t city) {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("City entry belongs to another session thread");
    auto pending=BnwCityEntryPending::prepare(*this,owner,city); // No stores/effects.
    const auto [slot,inserted]=city_entry_pending_.try_emplace(pending->source_key(),pending);
    if(!inserted)
        throw std::invalid_argument("City entry already owns this native turn/city history slot");
    // Never erase a published slot on a later failure. Native resets are a
    // no-throw tail after the retained attempted marker and final preflight.
    slot->second->advance_damage_zero_prefix();return slot->second;
}

std::shared_ptr<const BnwCityEntryPending>
LiveSession::continue_bnw_city_entry_feature_surrounded(std::int64_t owner,std::int64_t city,
    const BnwCityEntryFeatureRules& rules) {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("City feature continuation belongs to another session thread");
    rules.require_current(); // Closed SAME DB scope BEFORE any world borrow.
    if(owner<0||owner>=64||city<0||city>INT32_MAX)
        throw std::invalid_argument("City feature continuation native handle unavailable");
    std::shared_ptr<BnwCityEntryPending> found;
    for(const auto& [key,pending]:city_entry_pending_) {
        const auto actual_owner=std::get<1>(key),actual_id=std::get<2>(key);
        if(actual_owner!=owner||(actual_id&8191)!=(city&8191)||
           ((city&~8191LL)!=0&&actual_id!=city))continue;
        if(found)throw std::invalid_argument("City feature continuation has ambiguous retained predecessor");
        found=pending;
    }
    if(!found)throw std::invalid_argument("City feature continuation requires retained city-entry predecessor");
    // Retired/moved/restored owners refuse before their world/rules operands.
    found->advance_feature_surrounded(rules);return found;
}

std::shared_ptr<const BnwCityEntryPending> LiveSession::continue_bnw_city_entry_strategy(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host,
    std::shared_ptr<const BnwCityEntryPending> supplied) {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("City strategy continuation belongs to another session thread");
    if(!host||!supplied)
        throw std::invalid_argument("City strategy requires SAME genuine selected Host and Pending");
    const auto key=supplied->source_key();const auto found=city_entry_pending_.find(key);
    if(found==city_entry_pending_.end()||!found->second||found->second.get()!=supplied.get())
        throw std::invalid_argument("City strategy Pending is not the exact registered owner");
    found->second->require_current(); // Quarantine BEFORE old DB/VM/Game/world.
    if(&host->session()!=this)
        throw std::invalid_argument("City strategy selected Host belongs to another session");
    (void)bnw_game_debug_mode();
    found->second->advance_strategy_one(std::move(host));return found->second;
}

std::shared_ptr<const BnwCityEntryPending> LiveSession::continue_bnw_city_citizens_verify(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host,
    std::shared_ptr<const BnwCityEntryPending> supplied) {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("City strategy continuation belongs to another session thread");
    if(!host||!supplied)
        throw std::invalid_argument("City strategy requires SAME genuine selected Host and Pending");
    const auto key=supplied->source_key();const auto found=city_entry_pending_.find(key);
    if(found==city_entry_pending_.end()||!found->second||found->second.get()!=supplied.get())
        throw std::invalid_argument("City strategy Pending is not the exact registered owner");
    found->second->require_current(); // Quarantine BEFORE old DB/VM/Game/world.
    if(&host->session()!=this)
        throw std::invalid_argument("City strategy selected Host belongs to another session");
    (void)bnw_game_debug_mode();
    found->second->advance_citizens_verify_one(*this,std::move(host));return found->second;
}

std::shared_ptr<const BnwCityProductionTurnPending>
LiveSession::begin_bnw_city_production_process_suppressed(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city) {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Production turn session foreign thread");
    auto pending=BnwCityProductionTurnPending::prepare(*this,std::move(host),owner,city);
    const auto [slot,inserted]=production_turn_pending_.try_emplace(pending->source_key(),pending);
    if(!inserted)throw std::invalid_argument("Production turn already owns this turn/city once history");
    slot->second->advance_one();return slot->second;
}
std::shared_ptr<const BnwCityProductionTurnPending>
LiveSession::begin_bnw_city_production_project(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){
 if(plot_yield_refresh_thread_!=std::this_thread::get_id())throw std::invalid_argument("Production turn session foreign thread");
 auto pending=BnwCityProductionTurnPending::prepare_project(*this,std::move(host),owner,city);
 const auto [slot,inserted]=production_turn_pending_.try_emplace(pending->source_key(),pending);
 if(!inserted)throw std::invalid_argument("Production turn already owns this turn/city once history");
 slot->second->advance_one();return slot->second;
}
std::shared_ptr<const BnwCityProductionTurnPending>
LiveSession::continue_bnw_city_production_process_suppressed(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::shared_ptr<const BnwCityProductionTurnPending> supplied) {
    if(plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Production turn session foreign thread");
    if(!supplied||!host||&host->session()!=this)
        throw std::invalid_argument("Production turn requires SAME actual Host/registered cursor");
    const auto found=production_turn_pending_.find(supplied->source_key());
    if(found==production_turn_pending_.end()||!found->second||found->second.get()!=supplied.get())
        throw std::invalid_argument("Production turn requires exact registered Pending");
    found->second->advance_one();return found->second;
}

LiveSession::Checkpoint LiveSession::checkpoint() const {
    if (plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Plot yield checkpoint belongs to another session thread");
    if (!source_ || !state_)
        throw std::invalid_argument("Checkpoint requires a live session facade");
    // Validate/export each genuine pending owner BEFORE copying world state.
    // Healthy normal cursors guard actual rules before their world reads;
    // retired cursors export only owned metadata and no old DB/source query.
    std::map<std::int64_t,std::shared_ptr<const BnwPlotYieldRefreshCheckpoint>> saved_pending;
    for (const auto& [plot,pending] : plot_yield_refresh_pending_) {
        if (!pending)
            throw std::invalid_argument("Plot yield checkpoint has an unissued slot");
        saved_pending.emplace(plot,pending->checkpoint());
    }
    std::map<BnwCityEntrySourceKey,std::shared_ptr<const BnwCityEntryCheckpoint>> saved_city_entry;
    for(const auto& [key,pending]:city_entry_pending_) {
        if(!pending)throw std::invalid_argument("City entry checkpoint has an unissued slot");
        saved_city_entry.emplace(key,pending->checkpoint());
    }
    std::map<BnwCityProductionTurnKey,std::shared_ptr<const BnwCityProductionTurnCheckpoint>> saved_production;
    for(const auto& [key,pending]:production_turn_pending_){
        if(!pending)throw std::invalid_argument("Production turn checkpoint unissued slot");
        saved_production.emplace(key,pending->checkpoint());
    }
    std::optional<BnwGameDebugReadState> snapshot;
    if(bnw_game_debug_owner_current()){
        (void)bnw_game_debug_mode();
        snapshot.emplace(static_cast<const BnwGameDebugReadState&>(*debug_));
    }
    auto city_yields=city_yield_refresh_registry_?city_yield_refresh_registry_->checkpoint():nullptr;
    auto checkpoint=Checkpoint(owner_,*state_,bnw_unhappiness_current_,snapshot);
    checkpoint.city_yield_refresh_=std::move(city_yields);
    checkpoint.plot_yield_refresh_pending_.swap(saved_pending);
    checkpoint.city_entry_pending_.swap(saved_city_entry);
    checkpoint.production_turn_pending_.swap(saved_production);
    return checkpoint;
}

void LiveSession::rebuild_bnw_unhappiness_cache(const BnwUnhappinessRules &rules,
                                                const content::GlobalDefineCache &defines) {
    if (plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("BNW unhappiness rebuild belongs to another session thread");
    if (!source_ || !state_)
        throw std::invalid_argument("BNW unhappiness rebuild requires a live session facade");
    // The real city population, player roster and imported BNW extra fields
    // jointly determine this cache. Rebuild on a private copy so a missing
    // Info or an overflow never publishes a partially updated query world.
    auto replacement=std::make_shared<LiveSessionState>(*state_);
    replacement->city_strategy_runtime.clear();
    const bool debug_current=bnw_game_debug_owner_current();
    (void)bind_bnw_unhappiness(replacement->unhappiness,replacement->setup,
                               replacement->cities,rules,defines);
    advance_state_generation();
    state_.swap(replacement);
    bnw_unhappiness_current_=true;
    if (debug_current) bind_bnw_game_debug_owner();
    else retire_bnw_game_debug_owner();
}

void LiveSession::restore(const Checkpoint &checkpoint) {
    if (plot_yield_refresh_thread_!=std::this_thread::get_id())
        throw std::invalid_argument("Plot yield restore belongs to another session thread");
    if (!source_ || !state_)
        throw std::invalid_argument("Restore requires a live session facade");
    if(checkpoint.owner_!=owner_)
        throw std::invalid_argument("Live session checkpoint belongs to another session");
    // Only owned metadata and genuine healthy normal rules are examined here.
    // Do not copy/borrow the checkpoint world before that private guard.
    // Stage city history before any OLD normal rules/debug/world query.
    const bool city_history=bool(checkpoint.city_yield_refresh_)||
        (city_yield_refresh_registry_&&city_yield_refresh_registry_->has_history());
    auto restored_city_yields=city_history?BnwCityYieldRefreshRegistry::restore_quarantined(
        checkpoint.city_yield_refresh_,city_yield_refresh_registry_):nullptr;
    std::map<std::int64_t,std::shared_ptr<const BnwPlotYieldRefreshCheckpoint>> plot_restore_history;
    for(const auto& [plot,saved]:checkpoint.plot_yield_refresh_pending_){
        if(!saved)throw std::invalid_argument("Plot checkpoint metadata absent");
        plot_restore_history.emplace(plot,city_history?BnwPlotYieldRefreshPending::quarantined_history(*saved):saved);
    }
    bool healthy_normal_game=false;
    for (const auto& [plot,saved] : plot_restore_history) {
        (void)plot;
        if (!saved) throw std::invalid_argument("Plot yield checkpoint cursor missing");
        healthy_normal_game=BnwPlotYieldRefreshPending::preflight_restore_rules(*saved,checkpoint.debug_)||healthy_normal_game;
    }
    // Restore only owned city-entry facts, never resumption of reset calls.
    // Prepare/union shared once history BEFORE copying the checkpoint world.
    std::map<BnwCityEntrySourceKey,std::shared_ptr<BnwCityEntryPending>> restored_city_entry;
    for(const auto& [key,saved]:checkpoint.city_entry_pending_) {
        if(!saved)throw std::invalid_argument("City entry checkpoint history missing");
        restored_city_entry.emplace(key,BnwCityEntryPending::restore_quarantined(*saved));
    }
    for(const auto& [key,pending]:city_entry_pending_) {
        if(!pending)throw std::invalid_argument("City entry current history missing");
        if(!restored_city_entry.contains(key))
            restored_city_entry.emplace(key,BnwCityEntryPending::restore_quarantined(*pending->checkpoint_history()));
    }
    // Pure metadata union before CP world copy; never query old source/DB.
    std::map<BnwCityProductionTurnKey,std::shared_ptr<BnwCityProductionTurnPending>> restored_production;
    for(const auto& [key,saved]:checkpoint.production_turn_pending_){
        if(!saved)throw std::invalid_argument("Production turn checkpoint missing history");
        restored_production.emplace(key,BnwCityProductionTurnPending::restore_quarantined(*saved));
    }
    for(const auto& [key,pending]:production_turn_pending_){
        if(!pending)throw std::invalid_argument("Production turn current history missing");
        if(!restored_production.contains(key))
            restored_production.emplace(key,BnwCityProductionTurnPending::restore_quarantined(*pending->checkpoint(true)));
    }
    auto replacement=std::make_shared<LiveSessionState>(checkpoint.state_);
    // Old checkpoint nodes belong to the old allocation/revision. No token
    // or Init/Read owner is resurrected by rollback.
    replacement->city_strategy_runtime.clear();
    const auto ui_revision=std::max(state_->ui_source_revision,replacement->ui_source_revision);
    if(ui_revision==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Live session UI source revision exhausted");
    replacement->ui_source_revision=ui_revision+1;
    replacement->city_dirty_next_occurrence=std::max(state_->city_dirty_next_occurrence,replacement->city_dirty_next_occurrence);
    // NEW actual field storage for the replacement Game. The checkpoint
    // value snapshot is never an old resumable read control; staging does not
    // mutate the old owner/token and no callback is invoked.
    std::shared_ptr<BnwGameDebugReadStorage> debug;
    if(checkpoint.debug_)debug=std::make_shared<BnwGameDebugReadStorage>(*checkpoint.debug_);
    if (state_generation_==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Live session state generation exhausted");
    std::shared_ptr<BnwPlotYieldRefreshIssuer> restored_issuer;
    std::map<std::int64_t,std::shared_ptr<BnwPlotYieldRefreshPending>> restored_pending;
    if (!checkpoint.plot_yield_refresh_pending_.empty() || !plot_yield_refresh_pending_.empty()) {
        restored_issuer=std::shared_ptr<BnwPlotYieldRefreshIssuer>(
            new BnwPlotYieldRefreshIssuer(replacement,state_generation_+1));
        if(healthy_normal_game)restored_issuer->bind_game_debug(debug,source_);
        for (const auto& [plot,saved] : plot_restore_history) {
            if (!saved) throw std::invalid_argument("Plot yield checkpoint cursor missing");
            restored_pending.emplace(plot,BnwPlotYieldRefreshPending::restore_checkpoint(
                restored_issuer,replacement,*saved));
        }
        for (const auto& [plot,pending] : plot_yield_refresh_pending_) {
            if (!restored_pending.contains(plot) && pending && pending->has_irreversible_history())
                restored_pending.emplace(plot,BnwPlotYieldRefreshPending::quarantine_for_new_allocation(
                    restored_issuer,replacement,*pending));
        }
    }
    // All throwing preparation is complete. Commit conservative irreversible
    // history markers together with the already prepared allocation/cursors.
    for (const auto& [plot,pending] : restored_pending) {
        (void)plot;
        pending->commit_quarantine();
    }
    for(const auto& [key,pending]:restored_city_entry) {
        (void)key;pending->commit_quarantine();
    }
    if(restored_city_yields)restored_city_yields->commit_quarantine();
    retire_bnw_game_debug_owner();
    advance_state_generation();
    state_.swap(replacement);
    plot_yield_refresh_issuer_.swap(restored_issuer);
    plot_yield_refresh_pending_.swap(restored_pending);
    city_entry_pending_.swap(restored_city_entry);
    production_turn_pending_.swap(restored_production);
    city_yield_refresh_registry_.swap(restored_city_yields);
    bnw_unhappiness_current_=checkpoint.happiness_current_;
    static_assert(std::is_nothrow_swappable_v<decltype(debug_)>);
    debug_.swap(debug);
    bind_bnw_game_debug_owner();
}

} // namespace civ5::runtime

namespace civ5::runtime {
std::shared_ptr<const BnwCityProductionTurnPending> LiveSession::begin_bnw_city_production_train(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){
 if(plot_yield_refresh_thread_!=std::this_thread::get_id())throw std::invalid_argument("Production TRAIN session foreign thread");
 auto pending=BnwCityProductionTurnPending::prepare_train(*this,std::move(host),owner,city);const auto [slot,inserted]=production_turn_pending_.try_emplace(pending->source_key(),pending);
 if(!inserted)throw std::invalid_argument("Production turn already owns this turn/city once history");slot->second->advance_one();return slot->second;
}
}
