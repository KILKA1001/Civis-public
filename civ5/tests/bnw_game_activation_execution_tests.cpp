#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
std::int64_t &scalar(saves::PlayerCoreObservation &core,std::string_view name) {
    for(auto &f:core.fields) if(f.name && std::string_view(f.name)==name && f.values.size()==1) return f.values[0];
    throw std::runtime_error("Missing test player scalar");
}
std::int64_t &scalar(saves::UnitCoreObservation &core,std::string_view name) {
    for(auto &f:core.fields) if(f.name && std::string_view(f.name)==name && f.values.size()==1) return f.values[0];
    throw std::runtime_error("Missing test unit scalar");
}
void unchanged(const LiveSessionState &a,const LiveSessionState &b) {
    check(a.random.map.state()==b.random.map.state() && a.random.gameplay.state()==b.random.gameplay.state(),"Activation changed source RNG");
    check(a.setup.game_turn==b.setup.game_turn && a.player_read_tail.active_player_ids==b.player_read_tail.active_player_ids,"Activation changed source clocks or active owner");
    for(std::size_t i=0;i<64;++i) {
        const auto &x=a.world.all_players->players[i],&y=b.world.all_players->players[i];
        check(x.core.fields.size()==y.core.fields.size() && x.units.units.size()==y.units.units.size(),"Activation changed source object shape");
        for(std::size_t j=0;j<x.core.fields.size();++j) check(x.core.fields[j].values==y.core.fields[j].values,"Activation changed source player flags/timers");
        for(std::size_t j=0;j<x.units.units.size();++j)
            for(std::size_t k=0;k<x.units.units[j].core.fields.size();++k)
                check(x.units.units[j].core.fields[k].values==y.units.units[j].core.fields[k].values,"Activation changed source unit flags/moves");
    }
}
std::int64_t &tail_scalar(std::vector<saves::UnitTailNode> &nodes,std::string_view name) {
    for(auto &node:nodes) {
        if(node.name && std::string_view(node.name)==name && node.values.size()==1) return node.values[0];
        if(!node.children.empty()) {
            try {return tail_scalar(node.children,name);} catch(const std::runtime_error &) {}
        }
    }
    throw std::runtime_error("Missing test unit tail scalar");
}
bool native(BnwGameActivationStep s) {
    using S=BnwGameActivationStep;
    return s==S::set_turn_active || s==S::publish_end_turn_dirty || s==S::clear_end_turn ||
        s==S::unit_attrition || s==S::active_turn_count_log || s==S::publish_player_turn_start || s==S::all_units_unprocessed;
}
// These are control-flow fixtures, not GameCore implementations or parity evidence.
std::vector<BnwGameActivationService> fixture_services(std::uint32_t &units) {
    std::vector<BnwGameActivationService> result;
    for(std::uint32_t id=0;id<=static_cast<std::uint32_t>(BnwGameActivationStep::active_player_turn_start);++id) {
        const auto step=static_cast<BnwGameActivationStep>(id);
        if(native(step) || step==BnwGameActivationStep::player_do_turn) continue;
        result.push_back({step,[step,&units](LiveSessionState &s,std::int64_t player) {
            if(step==BnwGameActivationStep::player_do_turn_units) {
                ++units;
                for(auto &u:s.world.all_players->players[player].units.units) {
                    check(scalar(u.core,"m_bAITurnProcessed")==0,"Nested unit turn ran before SetAllUnitsUnprocessed");
                    scalar(u.core,"m_iMoves")=0;
                }
            }
            return LiveActionResult{LiveActionOutcome::committed,{}};
        }});
    }
    return result;
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,"Selected BNW host unavailable");
    auto &host=*bootstrap.preview.host;
    auto source=host.session().state();
    for(auto &p:source.world.all_players->players) scalar(p.core,"m_bTurnActive")=0;
    const auto active=source.world.pregame->archive.integers.at("active_player");
    std::int64_t first=-1;
    for(const auto &[id,p]:source.setup.players) if(p.alive) {first=id;break;}
    check(first==active && source.setup.is_human(source.setup.players.at(first)),"Selected first alive active-human branch changed");
    auto &core=source.world.all_players->players[first].core;
    scalar(core,"m_bEndTurn")=1;scalar(core,"m_bAutoMoves")=1;
    for(auto &u:source.world.all_players->players[first].units.units) scalar(u.core,"m_bAITurnProcessed")=1;
    const auto before=source;
    const auto order=bnw_game_turn_order();
    std::vector<BnwTurnPhase> prefix(order.begin(),std::find(order.begin(),order.end(),BnwTurnPhase::game_activation));
    BnwGameActivationOwner owner{host.identity()};
    auto real=make_bnw_game_activation_source_player_handlers(host.database(),host.identity(),active);
    auto selected=execute_bnw_game_activation_draft(source,owner,host.database(),host.identity(),prefix,real);
    check(!selected.draft && selected.completed_steps.size()==4 &&
          selected.completed_steps.back()==BnwGameActivationStep::unit_attrition &&
          selected.blocker.find("active_player_deferred_fog")!=std::string::npos &&
          selected.player_turn_calls==0 && selected.unit_turn_calls==0,
          "Selected owned activation prefix did not stop at unresolved deferred fog");
    unchanged(source,before);check(owner.completed.empty() && owner.pending.empty(),"Rejected activation changed input ledger");

    auto damaging=source;
    auto &damaging_unit=damaging.world.all_players->players[first].units.units.front();
    tail_scalar(damaging_unit.tail.blocks,"m_iEnemyDamageChance")=100;
    tail_scalar(damaging_unit.tail.blocks,"m_iEnemyDamage")=5;
    const auto damage_before=damaging;
    const auto damage=execute_bnw_game_activation_draft(damaging,owner,host.database(),host.identity(),prefix,real);
    check(!damage.draft && damage.completed_steps.size()==3 &&
          damage.blocker.find("territory RNG/damage")!=std::string::npos,
          "Effectful attrition was replaced with a successful no-effect branch");
    unchanged(damaging,damage_before);
    check(tail_scalar(damaging_unit.tail.blocks,"m_iEnemyDamageChance")==100 &&
          tail_scalar(damaging_unit.tail.blocks,"m_iEnemyDamage")==5,"Rejected attrition changed source tail fields");
    auto stale_map=source;
    auto &stale_unit=stale_map.world.all_players->players[first].units.units.front();
    const auto plot=stale_map.world.map_header.grid.index(scalar(stale_unit.core,"m_iX"),scalar(stale_unit.core,"m_iY"));
    stale_map.world.map_projection->cells.at(plot).plot_type=0;
    const auto stale=execute_bnw_game_activation_draft(stale_map,owner,host.database(),host.identity(),prefix,real);
    check(!stale.draft && stale.blocker.find("raw/projected plot type differs")!=std::string::npos,
          "Stale projected plot was used to prove attrition behavior");

    std::uint32_t units=0;
    auto services=fixture_services(units);
    auto nested=execute_bnw_game_activation_draft(source,owner,host.database(),host.identity(),prefix,real,services);
    check(!nested.draft && nested.player_turn_calls==1 && nested.unit_turn_calls==0 &&
          nested.completed_player_phases.size()==3 &&
          nested.blocker.find("player_trade_strategy_diplomacy")!=std::string::npos,
          "Source-backed nested human player prefix was skipped or duplicated");
    unchanged(source,before);
    std::uint32_t player_calls=0,publications=0;
    std::vector<BnwTurnHandler> fixture;
    for(auto phase:bnw_player_turn_order()) if(phase!=BnwTurnPhase::player_achievements)
        fixture.push_back({phase,[&](LiveSessionState &state,std::int64_t player) {
            ++player_calls;++scalar(state.world.all_players->players[player].core,"m_iConversionTimer");
            return LiveActionResult{LiveActionOutcome::committed,{}};
        }});
    fixture[0].after_commit=[&](LiveSession &)->LiveActionResult {
        ++publications;return {LiveActionOutcome::committed,{}};
    };
    auto complete=execute_bnw_game_activation_draft(source,owner,host.database(),host.identity(),prefix,fixture,services);
    check(complete.draft && complete.blocker.empty() && complete.completed_steps.size()==22 &&
          complete.player_turn_calls==1 && complete.unit_turn_calls==1 && units==1 &&
          player_calls==bnw_player_turn_order().size()-1 && publications==0 &&
          complete.draft->player_publications.size()==1 && complete.draft->owner.pending.size()==3,
          "Synthetic activation draft or deferred player publication changed");
    auto &draft=*complete.draft;
    auto &changed=draft.state.world.all_players->players[first].core;
    check(scalar(changed,"m_bTurnActive")==1 && scalar(changed,"m_bEndTurn")==0 &&
          scalar(changed,"m_bAutoMoves")==0 && !draft.owner.processed_auto_moves.at(first) &&
          draft.state.player_read_tail.active_player_ids==std::vector<std::int64_t>{first},
          "Native flags/automoves/recomputed active count did not change the draft");
    for(auto &u:draft.state.world.all_players->players[first].units.units)
        check(scalar(u.core,"m_bAITurnProcessed")==0 && scalar(u.core,"m_iMoves")==0,"Unit processed or nested turn mutation missing");
    unchanged(source,before);
    // Simulate delivery and deactivation. The same epoch still cannot re-enter.
    draft.owner.pending.clear();scalar(changed,"m_bTurnActive")=0;
    const auto repeated=execute_bnw_game_activation_draft(draft.state,draft.owner,host.database(),host.identity(),prefix,fixture,services);
    check(!repeated.draft && repeated.blocker.find("already completed")!=std::string::npos &&
          repeated.completed_steps.empty(),"Deactivation allowed a second player turn for the same epoch");
    auto failing=services;
    for(auto &s:failing) if(s.step==BnwGameActivationStep::warnings)
        s.run=[](LiveSessionState &,std::int64_t)->LiveActionResult {throw std::runtime_error("late warning failure");};
    auto late=execute_bnw_game_activation_draft(source,owner,host.database(),host.identity(),prefix,fixture,failing);
    check(!late.draft && late.player_turn_calls==1 && late.unit_turn_calls==1 &&
          late.blocker.find("late warning failure")!=std::string::npos,"Late failure published a partial activation");
    unchanged(source,before);
    auto wrong=owner;wrong.identity.rules_sha256[0]^=1;
    check(!execute_bnw_game_activation_draft(source,wrong,host.database(),host.identity(),prefix,real).draft,
          "Activation epoch from another rules profile was accepted");
    auto incomplete=source;incomplete.world.pregame.reset();
    check(!execute_bnw_game_activation_draft(incomplete,owner,host.database(),host.identity(),prefix,real).draft,
          "Incomplete activation source was accepted");
    std::cout << "PASS selected native activation prefix + source-backed nested human prefix; synthetic atomicity, publication and epoch guards\n";
}
