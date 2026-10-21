#include "civ5/runtime/bnw_unit_sight_event_order.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(const auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW sight event unit field missing");
}
BnwSetXYRequest candidate(const LiveSessionState &state) {
    const auto &grid=state.world.map_header.grid;
    for(const auto &player:state.world.all_players->players)
        for(const auto &unit:player.units.units) {
            const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const BnwSetXYRequest request{core(unit,"m_eOwner"),
                    core(unit,"m_iID"),x+dx,y+dy};
                if(preflight_bnw_unit_sight(state,request).candidate) return request;
            }
        }
    throw std::runtime_error("Selected Gandhi save has no sight candidate");
}
std::vector<std::int64_t> &array(saves::PlotObservation &plot,
                                 std::string_view wanted) {
    for(auto &item:plot.arrays)
        if(item.name && wanted==item.name) return item.values;
    throw std::runtime_error("Selected BNW sight event plot array missing");
}
void set_field(saves::PlotObservation &plot,std::string_view wanted,
               std::int64_t value) {
    for(auto &field:plot.fields)
        if(field.name && wanted==field.name) {field.value=value;return;}
    throw std::runtime_error("Selected BNW sight event plot field missing");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    const auto source=identify_file(save);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(static_cast<bool>(bootstrap.preview.host),
          "Selected BNW live preview did not bootstrap");
    auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    const auto input=host.preview_input();
    check(static_cast<bool>(input.frame_sources.database),
          "Selected BNW rules database is absent");
    const auto &database=*input.frame_sources.database;
    const auto types=database.cache_info_types();
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto request=candidate(state);
    const auto trace=trace_bnw_unit_sight_event_order(
        state,request,database,types,defines,host.identity());
    check(trace.calculated && trace.exploration.sight.detached &&
          !trace.can_commit() && trace.unit_meet_candidates==0 &&
          trace.city_meet_candidates==0,trace.reason.c_str());
    const auto &calls=trace.exploration.sight.plan.counters.new_call_order;
    check(calls.size()==16,"Selected BNW sight new ring call count changed");
    for(std::size_t i=1;i<calls.size();++i)
        check(calls[i-1].ring<=calls[i].ring,
              "BNW sight call order differs from SDK ring traversal");
    std::size_t revealed=0,exploration=0,adjacent=0;
    for(const auto &event:trace.events) {
        switch(event.effect) {
        case BnwSightOrderedEffect::set_revealed:++revealed;break;
        case BnwSightOrderedEffect::exploration_player_dirty:
            ++exploration;
            check(event.player==0 && event.first_reveal,
                  "BNW discovery event recipient/order changed");
            break;
        case BnwSightOrderedEffect::adjacent_owner_refresh:++adjacent;break;
        default:throw std::runtime_error("Selected Gandhi has unexpected meet event");
        }
    }
    check(revealed==5 && exploration==5 && adjacent>0 &&
          request.owner==0 && request.unit_id==16385,
          "Selected Gandhi sight event distribution changed");
    auto synthetic=state;
    const auto to=state.world.map_header.grid.index(
        request.to_x,request.to_y);
    std::uint64_t from=state.world.map_header.grid.size();
    for(const auto &player:state.world.all_players->players)
        for(const auto &unit:player.units.units)
            if(core(unit,"m_eOwner")==request.owner &&
               core(unit,"m_iID")==request.unit_id)
                from=state.world.map_header.grid.index(
                    core(unit,"m_iX"),core(unit,"m_iY"));
    check(from<state.world.map_header.grid.size(),
          "Selected BNW moved unit disappeared");
    const auto it=std::find_if(
        trace.exploration.sight.plan.newly_visible.begin(),
        trace.exploration.sight.plan.newly_visible.end(),
        [&](const auto &edge){return edge.plot!=from && edge.plot!=to;});
    check(it!=trace.exploration.sight.plan.newly_visible.end(),
          "Selected Gandhi lacks independent event plot");
    const auto plot=it->plot;
    auto &raw=synthetic.world.map_plots->plots[plot];
    auto &cell=synthetic.world.map_projection->cells[plot];
    array(raw,"units").insert(array(raw,"units").end(),{1,222});
    cell.units->push_back({1,222});
    set_field(raw,"plot_city_owner",2);
    set_field(raw,"plot_city_id",333);
    cell.plot_city=saves::SavedPlotCityReference{2,333};
    const auto candidates=trace_bnw_unit_sight_event_order(
        synthetic,request,database,types,defines,host.identity());
    check(candidates.calculated && candidates.unit_meet_candidates==1 &&
          candidates.city_meet_candidates==1 && !candidates.can_commit(),
          candidates.reason.c_str());
    std::size_t last_adjacent=0,unit_at=0,city_at=0;
    for(std::size_t i=0;i<candidates.events.size();++i) {
        const auto &event=candidates.events[i];
        if(event.plot!=plot) continue;
        if(event.effect==BnwSightOrderedEffect::adjacent_owner_refresh)
            last_adjacent=i+1;
        if(event.effect==BnwSightOrderedEffect::unit_meet_candidate)
            unit_at=i+1;
        if(event.effect==BnwSightOrderedEffect::city_meet_candidate)
            city_at=i+1;
    }
    check(last_adjacent>0 && last_adjacent<unit_at && unit_at<city_at,
          "BNW sight unit/city meet candidates precede adjacent-owner SDK phase");
    auto wrong=host.identity();wrong.rules_sha256=std::string(64,'0');
    check(!trace_bnw_unit_sight_event_order(
        state,request,database,types,defines,wrong).calculated,
        "BNW ordered sight accepted a different rules identity");
    std::uint32_t wonder_hash=0;
    const auto wonder_report=database.query_rows(
        {"SELECT Type FROM Features WHERE NaturalWonder=1",{}},
        [&](const auto &row) {
            if(wonder_hash) return;
            const auto type=row.text("Type");
            if(type) wonder_hash=content::saved_info_type_hash(*type);
        });
    check(wonder_report.all_succeeded() && wonder_hash,
          "Selected BNW natural wonder rule is absent");
    auto wonder=state;
    const auto first=std::find_if(
        trace.exploration.sight.plan.newly_visible.begin(),
        trace.exploration.sight.plan.newly_visible.end(),
        [](const auto &edge){return edge.first_reveal;});
    check(first!=trace.exploration.sight.plan.newly_visible.end(),
          "Selected BNW sight plan has no first reveal");
    set_field(wonder.world.map_plots->plots[first->plot],
              "feature_type_hash",wonder_hash);
    wonder.world.map_projection->cells[first->plot].feature_type_hash=
        wonder_hash;
    const auto blocked=trace_bnw_unit_sight_event_order(
        wonder,request,database,types,defines,host.identity());
    check(!blocked.calculated && !blocked.exploration.sight.detached &&
          blocked.reason.find("natural wonder")!=std::string::npos,
          "BNW first-revealed natural wonder did not fail closed");
    check(identify_file(save).sha256==source.sha256,
          "BNW read-only event trace changed original save");
    std::cout << "BNW ring calls=" << calls.size() <<
        " revealed=" << revealed << " exploration=" << exploration <<
        " adjacent=" << adjacent << '\n';
}
}

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW sight event order trace\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW sight event order: " << error.what() << '\n';
        return 1;
    }
}
