#include "civ5/runtime/bnw_game_activation_preflight.hpp"
#include "civ5/runtime/live_session.hpp"
#include <algorithm>
#include <array>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::int64_t scalar(const saves::PlayerCoreObservation &core,
                    std::string_view name) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW activation player scalar");
            found=&field;
        }
    require(found && found->values.size()==1,
            "Missing BNW activation player scalar");
    return found->values[0];
}
std::optional<bool> game_flag(const saves::GameStateObservation &game,
                              std::string_view name) {
    std::optional<bool> value;
    for(const auto &flag:game.flags)
        if(flag.name && std::string_view(flag.name)==name) {
            require(!value,"Duplicate BNW activation game flag");
            value=flag.value;
        }
    return value;
}
constexpr std::array activation_order{
    BnwGameActivationStep::set_turn_active,
    BnwGameActivationStep::publish_end_turn_dirty,
    BnwGameActivationStep::clear_end_turn,
    BnwGameActivationStep::unit_attrition,
    BnwGameActivationStep::active_player_deferred_fog,
    BnwGameActivationStep::active_turn_count_log,
    BnwGameActivationStep::publish_player_turn_start,
    BnwGameActivationStep::all_units_unprocessed,
    BnwGameActivationStep::pathfinder_cache_enter,
    BnwGameActivationStep::city_connections,
    BnwGameActivationStep::city_connection_gold,
    BnwGameActivationStep::happiness,
    BnwGameActivationStep::builder_tasking,
    BnwGameActivationStep::diplomacy_requests_begin,
    BnwGameActivationStep::player_do_turn,
    BnwGameActivationStep::player_do_turn_units,
    BnwGameActivationStep::pathfinder_cache_restore,
    BnwGameActivationStep::warnings,
    BnwGameActivationStep::active_player_unit_cycler,
    BnwGameActivationStep::active_player_selection_counter,
    BnwGameActivationStep::active_player_selection_camera,
    BnwGameActivationStep::active_player_turn_start,
};
constexpr std::array<std::string_view,activation_order.size()> step_names{
    "set_turn_active", "publish_end_turn_dirty", "clear_end_turn",
    "unit_attrition", "active_player_deferred_fog", "active_turn_count_log",
    "publish_player_turn_start", "all_units_unprocessed", "pathfinder_cache_enter",
    "city_connections", "city_connection_gold", "happiness", "builder_tasking",
    "diplomacy_requests_begin", "player_do_turn", "player_do_turn_units",
    "pathfinder_cache_restore", "warnings", "active_player_unit_cycler",
    "active_player_selection_counter", "active_player_selection_camera",
    "active_player_turn_start"};
static_assert(static_cast<std::size_t>(BnwGameActivationStep::active_player_turn_start)+1==step_names.size());
bool only_active_player(BnwGameActivationStep step) {
    return step==BnwGameActivationStep::active_player_deferred_fog ||
           step>=BnwGameActivationStep::active_player_unit_cycler;
}
std::vector<BnwGameActivationStep> check_services(
    std::span<const BnwGameActivationService> services,bool active_player) {
    std::vector<BnwGameActivationStep> order;
    std::set<BnwGameActivationStep> expected,seen;
    for(const auto step:activation_order) {
        if(only_active_player(step) && !active_player) continue;
        order.push_back(step);
        if(step!=BnwGameActivationStep::player_do_turn) expected.insert(step);
    }
    for(const auto &service:services) {
        require(expected.contains(service.step),
                "BNW activation unexpected service or duplicate player doTurn owner");
        require(seen.insert(service.step).second,
                "BNW activation duplicate setTurnActive service");
        require(static_cast<bool>(service.run),
                "BNW activation unbound setTurnActive service");
    }
    for(const auto step:expected)
        if(!seen.contains(step))
            throw std::runtime_error("BNW activation missing setTurnActive service: "+
                                     std::string(bnw_game_activation_step_name(step)));
    return order;
}
} // namespace

std::string_view bnw_game_activation_step_name(BnwGameActivationStep step) {
    const auto id=static_cast<std::size_t>(step);
    return id<step_names.size()?step_names[id]:"unknown";
}

BnwGameActivationPreflight preflight_bnw_game_activation(
    const LiveSessionState &state,const content::RulesDatabase &database,
    std::span<const BnwTurnPhase> completed_game_prefix,
    std::span<const BnwTurnHandler> player_handlers,
    std::span<const BnwGameActivationService> services) {
    BnwGameActivationPreflight result;
    try {
        const auto order=bnw_game_turn_order();
        const auto activation=std::find(order.begin(),order.end(),
                                        BnwTurnPhase::game_activation);
        require(activation!=order.end() &&
                completed_game_prefix.size()==
                    static_cast<std::size_t>(activation-order.begin()) &&
                std::equal(completed_game_prefix.begin(),
                           completed_game_prefix.end(),order.begin()),
                "BNW activation requires all earlier game phases exactly once");
        const auto player_check=preflight_bnw_player_turn(player_handlers);
        require(player_check.ready(),
                "BNW activation requires each real player phase exactly once");
        // The player scheduler's after_commit hooks need the enclosing game
        // transaction's publisher. Until it exists, they cannot be nested.
        for(const auto &handler:player_handlers)
            require(!handler.after_commit,
                    "BNW player publication cannot be nested in game activation");
        require(state.setup.profile==content::TechnologyProfile::bnw &&
                state.world.game_state.layout==
                    saves::GameStateLayout::expansion2_version1 &&
                std::holds_alternative<saves::Expansion2PrefixObservation>(
                    state.world.game_state.prefix) &&
                state.world.pregame && state.world.all_players &&
                state.world.pregame->layout==
                    saves::GameStateLayout::expansion2_version1 &&
                state.world.all_players->layout==
                    saves::GameStateLayout::expansion2_version1 &&
                state.world.all_players->players.size()==64 &&
                state.world.all_players->context.player_count==64 &&
                state.setup.players.size()==64,
                "BNW activation needs complete live single-profile roster");
        const auto game_type=state.world.pregame->archive.integers.find(
            "game_type");
        require(game_type!=state.world.pregame->archive.integers.end() &&
                game_type->second==0 && state.setup.game_type==0,
                "BNW activation network/hotseat/PBEM branch is unsupported");
        auto setup=state.setup;
        require(setup.game_option_value(database,"GAMEOPTION_DYNAMIC_TURNS")==0 &&
                setup.game_option_value(database,"GAMEOPTION_SIMULTANEOUS_TURNS")==0,
                "BNW activation simultaneous branch requires shuffle RNG and team scheduling");
        require(setup.game_option_value(database,"GAMEOPTION_PITBOSS")==0 &&
                state.setup.pitboss!=true,
                "BNW activation PitBoss reminder branch is unsupported");
        const auto &prefix=std::get<saves::Expansion2PrefixObservation>(
            state.world.game_state.prefix);
        require(prefix.version==1, "BNW activation prefix version is unsupported");
        require(prefix.scalars[21]==0,
                "BNW activation AI autoplay decrement/revive is pending");
        require(game_flag(state.world.game_state,"final_initialized")==true,
                "BNW activation needs final-initialized player doTurn path");
        const auto game_turn=state.world.pregame->archive.integers.find(
            "game_turn");
        require(game_turn!=state.world.pregame->archive.integers.end() &&
                game_turn->second>=0 && game_turn->second==state.setup.game_turn &&
                prefix.scalars[1]>=0 &&
                prefix.scalars[1]==state.maintenance.elapsed_turns,
                "BNW activation game clock owner and decoded mirrors disagree");
        const auto active_player=state.world.pregame->archive.integers.find(
            "active_player");
        require(active_player!=state.world.pregame->archive.integers.end() &&
                active_player->second>=0 && active_player->second<64,
                "BNW activation active player is unavailable");
        require(game_flag(state.world.game_state,"hot_pbem_between_turns")==false,
                "BNW activation hotseat/PBEM between-turn state is unsupported");

        std::int64_t first_alive=-1;
        for(std::int64_t id=0;id<64;++id) {
            const auto &saved=state.world.all_players->players[id];
            const auto owned=state.setup.players.find(id);
            require(saved.index==static_cast<std::uint64_t>(id) &&
                    saved.saved_player_id==id &&
                    owned!=state.setup.players.end() &&
                    saved.core.layout==
                        saves::GameStateLayout::expansion2_version1 &&
                    scalar(saved.core,"m_eID")==id,
                    "BNW activation roster identity changed");
            const auto alive=scalar(saved.core,"m_bAlive");
            const auto active=scalar(saved.core,"m_bTurnActive");
            require((alive==0 || alive==1) &&
                    (active==0 || active==1) &&
                    owned->second.alive==(alive!=0),
                    "BNW activation raw and owned player flags diverged");
            require(active==0,
                    "BNW activation would repeat an already active player turn");
            if(alive && first_alive<0) first_alive=id;
        }
        require(first_alive>=0,
                "BNW activation has no living sequential player");
        const auto service_order=check_services(services,
            first_alive==active_player->second);
        result.ticket.mode=BnwGameActivationMode::sequential_single_player;
        result.ticket.active_player=active_player->second;
        result.ticket.activation_order=service_order;
        result.ticket.player=first_alive;
        result.ticket.game_turn=game_turn->second;
        result.ticket.required_player_turn_calls=1;
        result.ticket.required_unit_turn_calls=1;
        for(const auto phase:bnw_player_turn_order())
            if(phase!=BnwTurnPhase::player_achievements)
                result.ticket.player_phase_order.push_back(phase);
        result.structurally_composable=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}
} // namespace civ5::runtime
