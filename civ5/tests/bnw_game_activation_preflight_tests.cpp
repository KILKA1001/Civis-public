#include "civ5/runtime/bnw_game_activation_preflight.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::vector<BnwTurnPhase> prefix() {
    const auto order=bnw_game_turn_order();
    const auto end=std::find(order.begin(),order.end(),
                             BnwTurnPhase::game_activation);
    return {order.begin(),end};
}
std::vector<BnwTurnHandler> player_handlers() {
    std::vector<BnwTurnHandler> result;
    for(auto phase:bnw_player_turn_order())
        if(phase!=BnwTurnPhase::player_achievements)
            result.push_back({phase,[](LiveSessionState &,std::int64_t) {
                return LiveActionResult{LiveActionOutcome::rejected,
                                        "test handler never executes"};
            }});
    return result;
}
std::int64_t &field(saves::PlayerCoreObservation &core,
                    std::string_view name) {
    for(auto &field:core.fields)
        if(field.name && std::string_view(field.name)==name &&
           field.values.size()==1) return field.values[0];
    throw std::runtime_error("Missing selected player core scalar");
}
std::vector<BnwGameActivationService> activation_services(bool active_player) {
    std::vector<BnwGameActivationService> result;
    for(std::uint32_t id=0;
        id<=static_cast<std::uint32_t>(BnwGameActivationStep::active_player_turn_start);
        ++id) {
        const auto step=static_cast<BnwGameActivationStep>(id);
        if(step==BnwGameActivationStep::player_do_turn) continue;
        if(!active_player &&
           (step==BnwGameActivationStep::active_player_deferred_fog ||
            step>=BnwGameActivationStep::active_player_unit_cycler)) continue;
        result.push_back({step,[](LiveSessionState &,std::int64_t)->LiveActionResult {
            throw std::runtime_error("Preflight must not execute a service");
        }});
    }
    return result;
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.profile.can_activate &&
          bootstrap.profile.enabled_dlc.size()==14 &&
          bootstrap.profile.enabled_mods.empty(),
          "Selected Gandhi profile changed");
    const auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    auto completed=prefix();
    auto handlers=player_handlers();
    const auto map_rng=state.random.map.state();
    const auto gameplay_rng=state.random.gameplay.state();

    const auto selected=preflight_bnw_game_activation(
        state,host.database(),completed,handlers);
    check(!selected.phase_committed &&
          state.random.map.state()==map_rng &&
          state.random.gameplay.state()==gameplay_rng,
          "Activation preflight committed a phase or spent RNG");

    auto ready=state;
    std::int64_t first_alive=-1;
    std::size_t active_before=0;
    for(std::int64_t id=0;id<64;++id) {
        auto &core=ready.world.all_players->players[id].core;
        const auto alive=field(core,"m_bAlive");
        auto &active=field(core,"m_bTurnActive");
        if(alive && first_alive<0) first_alive=id;
        if(active) ++active_before;
        active=0; // Detached state after the previous player's turn ends.
    }
    check(first_alive>=0 && ready.setup.game_type==0,
          "Selected Gandhi sequential roster changed");
    if(active_before)
        check(!selected.structurally_composable &&
              selected.blocker.find("already active")!=std::string::npos,
              "Selected active player was scheduled twice");
    const auto active_player=ready.world.pregame->archive.integers.at("active_player");
    auto services=activation_services(first_alive==active_player);
    const auto planned=preflight_bnw_game_activation(
        ready,host.database(),completed,handlers,services);
    check(planned.structurally_composable && !planned.phase_committed &&
          planned.ticket.mode==
              BnwGameActivationMode::sequential_single_player &&
          planned.ticket.player==first_alive &&
          planned.ticket.active_player==active_player &&
          planned.ticket.activation_order.size()==services.size()+1 &&
          planned.ticket.required_player_turn_calls==1 &&
          planned.ticket.required_unit_turn_calls==1 &&
          planned.ticket.activation_order_rng_draws==0 &&
          planned.ticket.player_phase_order.size()==
              bnw_player_turn_order().size()-1 &&
          ready.random.map.state()==map_rng &&
          ready.random.gameplay.state()==gameplay_rng,
          "Selected sequential activation ticket changed or spent RNG");

    auto incomplete=completed;incomplete.pop_back();
    check(!preflight_bnw_game_activation(ready,host.database(),
          incomplete,handlers,services).structurally_composable,
          "Missing prior game phase was accepted");
    auto reordered=completed;std::swap(reordered[0],reordered[1]);
    check(!preflight_bnw_game_activation(ready,host.database(),
          reordered,handlers,services).structurally_composable,
          "Reordered source game prefix was accepted");
    auto repeated=handlers;repeated.push_back(handlers[0]);
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,repeated,services).structurally_composable,
          "Duplicate player phase was accepted");
    repeated=handlers;repeated[0].run={};
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,repeated,services).structurally_composable,
          "Empty player phase was accepted");
    repeated=handlers;repeated[0].after_commit=[](LiveSession &) {
        return LiveActionResult{LiveActionOutcome::committed,{}};
    };
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,repeated,services).structurally_composable,
          "Nested player external publication was accepted");

    auto simultaneous=ready;
    simultaneous.setup.game_options.insert(
        simultaneous.setup.game_options.begin(),
        {"GAMEOPTION_SIMULTANEOUS_TURNS",1});
    check(!preflight_bnw_game_activation(simultaneous,host.database(),
          completed,handlers,services).structurally_composable &&
          simultaneous.random.gameplay.state()==gameplay_rng,
          "Simultaneous shuffle branch spent or ignored RNG");
    auto pitboss=ready;
    pitboss.setup.game_options.insert(pitboss.setup.game_options.begin(),
        {"GAMEOPTION_PITBOSS",1});
    check(!preflight_bnw_game_activation(pitboss,host.database(),
          completed,handlers,services).structurally_composable,
          "PitBoss reminder branch was skipped");
    auto autoplay=ready;
    std::get<saves::Expansion2PrefixObservation>(
        autoplay.world.game_state.prefix).scalars[21]=1;
    check(!preflight_bnw_game_activation(autoplay,host.database(),
          completed,handlers,services).structurally_composable,
          "Autoplay revive branch was skipped");
    auto again=ready;
    field(again.world.all_players->players[first_alive].core,
          "m_bTurnActive")=1;
    check(!preflight_bnw_game_activation(again,host.database(),
          completed,handlers,services).structurally_composable,
          "Already activated player was scheduled again");

    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,handlers).structurally_composable,
          "Mandatory setTurnActive prefix and suffix services were skipped");
    for(std::size_t i=0;i<services.size();++i) {
        auto missing=services;missing.erase(missing.begin()+i);
        const auto checked=preflight_bnw_game_activation(ready,host.database(),
            completed,handlers,missing);
        check(!checked.structurally_composable &&
              checked.blocker.find("missing setTurnActive service")!=std::string::npos,
              "Missing mandatory activation service was accepted");
    }
    auto repeated_services=services;repeated_services.push_back(services[0]);
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,handlers,repeated_services).structurally_composable,
          "Repeated activation service was accepted");
    repeated_services=services;repeated_services[0].run={};
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,handlers,repeated_services).structurally_composable,
          "Unbound activation service was accepted");
    repeated_services=services;
    repeated_services.push_back({BnwGameActivationStep::player_do_turn,services[0].run});
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,handlers,repeated_services).structurally_composable,
          "A second player doTurn owner was accepted");
    repeated_services=services;
    repeated_services[0].step=static_cast<BnwGameActivationStep>(999);
    check(!preflight_bnw_game_activation(ready,host.database(),
          completed,handlers,repeated_services).structurally_composable,
          "Unknown activation service was accepted");
    auto stale_clock=ready;++stale_clock.setup.game_turn;
    check(!preflight_bnw_game_activation(stale_clock,host.database(),
          completed,handlers,services).structurally_composable,
          "Stale decoded game clock was accepted");
    auto stale_roster=ready;
    field(stale_roster.world.all_players->players[0].core,"m_eID")=10;
    check(!preflight_bnw_game_activation(stale_roster,host.database(),
          completed,handlers,services).structurally_composable,
          "Raw player identity mismatch was accepted");
    auto synthetic=ready;
    for(std::int64_t id=0;id<64;++id) {
        field(synthetic.world.all_players->players[id].core,"m_bAlive")=(id==9);
        synthetic.setup.players.at(id).alive=(id==9);
    }
    synthetic.world.pregame->archive.integers.at("active_player")=9;
    const auto active_services=activation_services(true);
    const auto ninth=preflight_bnw_game_activation(synthetic,host.database(),
        completed,handlers,active_services);
    check(ninth.structurally_composable && ninth.ticket.player==9 &&
          ninth.ticket.activation_order.size()==22 &&
          ninth.ticket.activation_order[14]==BnwGameActivationStep::player_do_turn &&
          ninth.ticket.activation_order[15]==BnwGameActivationStep::player_do_turn_units &&
          !ninth.phase_committed && ninth.ticket.activation_order_rng_draws==0,
          "Sequential synthetic activation order or first alive slot changed");
    synthetic.world.pregame->archive.integers.at("active_player")=0;
    const auto inactive_services=activation_services(false);
    const auto other=preflight_bnw_game_activation(synthetic,host.database(),
        completed,handlers,inactive_services);
    check(other.structurally_composable && other.ticket.player==9 &&
          other.ticket.active_player==0 && other.ticket.activation_order.size()==17 &&
          other.ticket.activation_order[13]==BnwGameActivationStep::player_do_turn &&
          other.ticket.activation_order[14]==BnwGameActivationStep::player_do_turn_units,
          "Nonactive player activation included active-player UI operations");
    check(!preflight_bnw_game_activation(synthetic,host.database(),
          completed,handlers,active_services).structurally_composable,
          "Inapplicable active-player service was accepted");
    synthetic.setup.players.at(9).alive=false;
    check(!preflight_bnw_game_activation(synthetic,host.database(),
          completed,handlers,inactive_services).structurally_composable,
          "Owned/raw alive mismatch was accepted");
    field(synthetic.world.all_players->players[9].core,"m_bAlive")=0;
    check(!preflight_bnw_game_activation(synthetic,host.database(),
          completed,handlers,inactive_services).structurally_composable,
          "No living sequential player was accepted");
    check(ready.random.map.state()==map_rng && ready.random.gameplay.state()==gameplay_rng &&
          synthetic.random.map.state()==map_rng && synthetic.random.gameplay.state()==gameplay_rng,
          "Synthetic or selected activation consumed RNG");
    std::cout << "PASS selected BNW activation preflight and synthetic service/roster guards\n";
}
