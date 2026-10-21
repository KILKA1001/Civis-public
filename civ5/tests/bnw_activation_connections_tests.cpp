#include "civ5/runtime/bnw_activation_connections.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
template<class T> auto &named(std::vector<T>&rows,std::string_view name) {
    for(auto &r:rows)if(r.name && std::string_view(r.name)==name)return r;
    throw std::runtime_error("Missing test field");
}
std::int64_t &scalar(saves::PlayerCoreObservation &p,std::string_view name) {return named(p.fields,name).values.at(0);}
std::int64_t scalar(const saves::PlayerCoreObservation &p,std::string_view name) {
    for(const auto &f:p.fields)if(f.name && std::string_view(f.name)==name)return f.values.at(0);
    throw std::runtime_error("Missing const test scalar");
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected BNW host unavailable");
    auto &host=*selected.preview.host;const auto player=host.active_player_id();
    auto source=host.session().state();
    auto owner=make_bnw_activation_connections_import_owner(host.session().source(),host.identity());
    auto &raw=source.world.all_players->players.at(player);
    auto &matrix=named(named(raw.planning.blocks,"city_connections").arrays,"route_infos");
    const auto original=matrix.values;
    auto before=update_bnw_activation_city_connection_gold(source,owner,player);
    check(before.outcome==LiveActionOutcome::rejected,"Gold ran before source connection update");
    auto unknown=owner;unknown.players.at(player).plot_routes_known=false;
    check(update_bnw_activation_city_connections(source,unknown,host.database(),player).outcome==LiveActionOutcome::rejected &&
          matrix.values==original,"Unknown transient accepted or changed source matrix");
    unknown=owner;unknown.players.at(player).plot_routes={{0,1}};
    check(update_bnw_activation_city_connections(source,unknown,host.database(),player).outcome==LiveActionOutcome::rejected,
          "Effectful plot-route broadcast was silently skipped");
    matrix.values.assign(matrix.values.size(),7); // stale imported cache is recomputed
    auto applied=update_bnw_activation_city_connections(source,owner,host.database(),player);
    if(applied.outcome==LiveActionOutcome::rejected)throw std::runtime_error(applied.reason);
    auto &updated=named(named(raw.planning.blocks,"city_connections").arrays,"route_infos");
    for(std::size_t i=0;i<updated.values.size();i+=2)
        check(updated.values[i]==-1 && updated.values[i+1]==0,"Route matrix was not actually reset");
    auto &ids=named(named(raw.planning.blocks,"city_connections").arrays,"city_plot_ids");
    check(ids.values.size()==raw.cities.cities.size() && owner.players.at(player).updated_turn==source.setup.game_turn,
          "City IDs or source update receipt differs");
    auto &saved_gold=named(named(raw.administration.blocks,"treasury").fields,"m_iCityConnectionGoldTimes100").value;
    source.treasury.players.at(player).connections_times100=1234;saved_gold=1234;
    auto gold=update_bnw_activation_city_connection_gold(source,owner,player);
    check(gold.outcome==LiveActionOutcome::committed && saved_gold==0 &&
          source.treasury.players.at(player).connections_times100==0,"Actual gold cache was not recomputed");
    saved_gold=1;
    check(update_bnw_activation_city_connection_gold(source,owner,player).outcome==LiveActionOutcome::rejected &&
          saved_gold==1 && source.treasury.players.at(player).connections_times100==0,
          "Raw/owned treasury mismatch was accepted or partially published");
    saved_gold=0;
    auto malformed=source;malformed.world.all_players->players.at(player).cities.header.wire_city_count+=1;
    check(update_bnw_activation_city_connections(malformed,owner,host.database(),player).outcome==LiveActionOutcome::rejected,
          "Undecoded required city was inferred absent");
    auto ai=source;ai.setup.players.at(player).slot_status=1;
    check(update_bnw_activation_city_connections(ai,owner,host.database(),player).outcome==LiveActionOutcome::rejected,
          "AI desire/quest branch was guessed");
    auto route=source;
    const auto team=source.setup.players.at(player).team;
    std::int64_t road=-1;
    host.database().query_rows({"SELECT t.ID FROM Builds b JOIN Routes r ON r.Type=b.RouteType JOIN Technologies t ON t.Type=b.PrereqTech WHERE r.Value>0 ORDER BY b.ID LIMIT 1",{}},
        [&](const auto &r){road=r.integer_at(0);});
    check(road>=0,"Usable route rejection fixture changed");
    auto tech=std::find_if(route.technologies.begin(),route.technologies.end(),[&](const auto &t){return t.team()==team;});
    tech->set_has_tech(road,true);
    check(update_bnw_activation_city_connections(route,owner,host.database(),player).outcome==LiveActionOutcome::rejected,
          "Route pathfinder effects were silently skipped");
    // Integrate native economy behind an explicitly synthetic finder service.
    // This verifies scheduler binding/rollback only, never native finder parity.
    auto activation=host.session().state();
    for(auto &p:activation.world.all_players->players)scalar(p.core,"m_bTurnActive")=0;
    BnwGameActivationOwner activation_owner{host.identity()};activation_owner.city_connections=owner;
    const auto phases=make_bnw_game_activation_source_player_handlers(host.database(),host.identity(),player);
    std::vector<BnwTurnPhase> prefix;
    for(const auto phase:bnw_game_turn_order()){if(phase==BnwTurnPhase::game_activation)break;prefix.push_back(phase);}
    const BnwGameActivationService fixtures[]={
        {BnwGameActivationStep::active_player_deferred_fog,[](LiveSessionState&,std::int64_t){return LiveActionResult{LiveActionOutcome::committed,"synthetic fog seam"};}},
        {BnwGameActivationStep::pathfinder_cache_enter,[](LiveSessionState&,std::int64_t){return LiveActionResult{LiveActionOutcome::committed,"synthetic finder seam"};}}
    };
    const auto executed=execute_bnw_game_activation_draft(activation,activation_owner,host.database(),host.identity(),prefix,phases,fixtures);
    check(!executed.draft && executed.completed_steps.size()==11 &&
          executed.completed_steps.back()==BnwGameActivationStep::city_connection_gold &&
          executed.blocker.find("happiness")!=std::string::npos &&
          activation_owner.pending.empty() && scalar(activation.world.all_players->players.at(player).core,"m_bTurnActive")==0,
          "Economy execution order or rejected activation rollback differs");
    check(scalar(host.session().state().world.all_players->players.at(player).core,"m_bTurnActive")==1,
          "Original imported active player was changed");
    std::cout<<"PASS selected BNW native city connections/gold, unknown/effectful guards and synthetic scheduler rollback\n";
}
