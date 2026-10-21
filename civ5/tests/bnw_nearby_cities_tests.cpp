#include "civ5/runtime/bnw_nearby_cities.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch (const std::exception &) {return;}
    throw std::runtime_error("Missing BNW nearby-city blocker");
}
void field(saves::SavedCityObservation &city,const char *name,
           std::int64_t value) {
    saves::CityCoreField row{};row.name=name;row.values={value};
    city.core.fields.push_back(std::move(row));
}
std::int64_t saved_field(const saves::CityCoreObservation &city,
                         std::string_view name) {
    for (const auto &field:city.fields)
        if (field.name && std::string_view(field.name)==name &&
            field.values.size()==1) return field.values.front();
    throw std::runtime_error("Selected BNW city core field unavailable");
}
void add_city(LiveSessionState &state,std::int64_t id,
              std::int64_t x,std::int64_t y) {
    auto &player=state.world.all_players->players.front();
    auto &saved=player.cities.cities.emplace_back();
    saved.core.layout=bnw;
    saved.core.start_offset=static_cast<std::uint64_t>(id*10);
    saved.core.end_offset=static_cast<std::uint64_t>(id*10+9);
    field(saved,"m_iID",id);field(saved,"m_eOwner",5);
    field(saved,"m_iX",x);field(saved,"m_iY",y);
    auto &live=state.cities.players[5].cities[id];
    live.owner=5;live.id=id;live.x=x;live.y=y;
    live.source_core=std::pair{saved.core.start_offset,
                               saved.core.end_offset};
    auto &map=*state.world.map_projection;
    auto &cell=map.cells.at(map.grid.index(x,y));
    cell.is_city=true;cell.plot_city=saves::SavedPlotCityReference{5,id};
}
LiveSessionState world() {
    LiveSessionState state{
        saves::SavedSessionSnapshot{},ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt};
    state.world.game_state.layout=state.world.map_header.layout=bnw;
    auto &all=state.world.all_players.emplace();all.layout=bnw;
    auto &player=all.players.emplace_back();
    player.saved_player_id=5;player.cities.layout=bnw;
    state.world.map_header.grid.width=7;
    state.world.map_header.grid.height=7;
    auto &map=state.world.map_projection.emplace();
    map.grid=state.world.map_header.grid;
    for (std::uint64_t y=0;y<7;++y)
        for (std::uint64_t x=0;x<7;++x) {
            auto &cell=map.cells.emplace_back();
            cell.index=y*7+x;cell.x=x;cell.y=y;
        }
    state.cities.players[5];
    add_city(state,10,0,0);
    add_city(state,11,2,0);
    add_city(state,12,5,5);
    return state;
}
void selected_save(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;
    context.decode_teams=true;
    context.decode_all_players=true;
    context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto session=saves::read_saved_session(path,context);
    check(session.snapshot.game_state.layout==bnw &&
          session.snapshot.all_players &&
          session.snapshot.map_projection,
          "Selected save is not a complete decoded BNW city/map source");
    auto state=world();
    state.world=std::move(session.snapshot);
    state.cities.players.clear();
    std::size_t city_count=0;
    for (const auto &player:state.world.all_players->players) {
        auto &live=state.cities.players[player.saved_player_id];
        for (const auto &saved:player.cities.cities) {
            const auto id=saved_field(saved.core,"m_iID");
            auto &city=live.cities[id];
            city.id=id;
            city.owner=saved_field(saved.core,"m_eOwner");
            city.x=saved_field(saved.core,"m_iX");
            city.y=saved_field(saved.core,"m_iY");
            city.source_core=std::pair{saved.core.start_offset,
                                       saved.core.end_offset};
        }
        city_count+=player.cities.cities.size();
    }
    // The selected Gandhi BNW save contains a founded city. Build its minimal
    // owned city roster from the complete decoded source, then exercise the
    // post-read CvCityManager hook against its actual map plot links.
    check(city_count>0,"Selected BNW save has no city to exercise the hook");
    bool unresolved_before=false;
    for (const auto &[owner,player]:state.cities.players)
        for (const auto &[id,city]:player.cities) {
            const auto &map=*state.world.map_projection;
            const auto &cell=map.cells.at(map.grid.index(city.x,city.y));
            check(cell.plot_city && cell.plot_city->owner==owner &&
                  cell.plot_city->id==id,
                  "Selected BNW city plot handle is unresolved");
            unresolved_before |= !cell.is_city;
        }
    check(unresolved_before,
          "Selected BNW save did not expose the pre-hook city flag gap");
    // Read-only decoded fixture; not a genuine Game issuer/full import proof.
    resolve_bnw_city_plot_links(state,true);
    try {
        rebuild_bnw_nearby_cities(state,true);
        check(state.nearby_cities.current &&
              state.nearby_cities.nearby.size()==city_count &&
              !state.nearby_cities.order_gap,
              "Selected BNW save did not restore every non-tied proximity list");
    } catch (const BnwNearbyCityOrderUnavailable &e) {
        check(!state.nearby_cities.current && state.nearby_cities.order_gap &&
              *state.nearby_cities.order_gap==e.gap() && e.gap().tied.size()>=2,
              "Selected tied source lost the actual typed readiness operands");
        std::cout << "Selected proximity pointer order UNAVAILABLE: owner="
                  << e.gap().origin.owner << " city=" << e.gap().origin.id
                  << " distance=" << e.gap().distance
                  << " tied=" << e.gap().tied.size() << '\n';
    }
    for (const auto &[owner,player]:state.cities.players)
        for (const auto &[id,city]:player.cities) {
            (void)owner;(void)id;
            const auto &map=*state.world.map_projection;
            check(map.cells.at(map.grid.index(city.x,city.y)).is_city,
                  "Selected BNW city plot did not become live after load");
        }
}
} // namespace

int main(int argc,char **argv) {
    try {
        auto state=world();
        const auto rng=state.random;
        rejects([&]{rebuild_bnw_nearby_cities(state,false);});
        rebuild_bnw_nearby_cities(state,true);
        const auto &near=bnw_nearby_cities(state,{5,10});
        check(near.size()==2 && near[0]==BnwCityHandle{5,11} &&
              near[1]==BnwCityHandle{5,12},
              "CvCityManager nearby-city distance order changed");
        auto changed=state;
        changed.cities.players.at(5).cities.at(11).x=3;
        rejects([&]{(void)bnw_nearby_cities(changed,{5,10});});
        auto tie=state;
        add_city(tie,13,0,2);
        for (auto &cell:tie.world.map_projection->cells)
            cell.is_city=false;
        bool typed_tie=false;
        try { rebuild_bnw_nearby_cities(tie,true); }
        catch (const BnwNearbyCityOrderUnavailable &e) {
            typed_tie=true;
            check(e.gap().origin==BnwCityHandle{5,10} && e.gap().distance==2 &&
                  e.gap().tied==std::vector<BnwCityHandle>{{5,11},{5,13}},
                  "Typed pointer-order frontier did not retain real source operands");
        }
        check(typed_tie && tie.nearby_cities.order_gap &&
              tie.nearby_cities.nearby.empty(),
              "Tied rebuild published partial lists or lost typed gap");
        check(!tie.nearby_cities.current,
              "Failed rebuild exposed a partial nearby-city cache");
        check(!tie.world.map_projection->cells.at(0).is_city &&
              !tie.world.map_projection->cells.at(14).is_city,
              "Failed rebuild partially updated live city plot flags");
        resolve_bnw_city_plot_links(tie,true);
        for (const auto &[owner,player]:tie.cities.players)
            for (const auto &[id,city]:player.cities) {
                (void)owner; (void)id;
                check(tie.world.map_projection->cells.at(
                    tie.world.map_projection->grid.index(city.x,city.y)).is_city,
                    "Independent city-link hook depended on pointer order");
            }
        rejects([&]{(void)bnw_nearby_cities(tie,{5,10});});
        saves::SavedSession tied_source;
        tied_source.snapshot=tie.world;
        auto tied_session=LiveSession::from_restored_bnw(std::move(tied_source),tie);
        const auto readiness=tied_session.readiness();
        check(!readiness.can_continue() &&
              std::find(readiness.blockers.begin(),readiness.blockers.end(),
                LiveSessionBlocker::bnw_nearby_city_pointer_order_unavailable)!=
                readiness.blockers.end(),
              "Diagnostic tied DTO claimed continuation or lost precise blocker");
        const auto tied_cp=tied_session.checkpoint();
        (void)apply_live_action(tied_session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::committed,{}};
        });
        check(!tied_session.state().nearby_cities.order_gap,
              "Generic action retained stale pointer-order diagnostics");
        tied_session.restore(tied_cp);
        check(tied_session.state().nearby_cities.order_gap==tie.nearby_cities.order_gap &&
              !tied_session.state().nearby_cities.current &&
              !tied_session.readiness().can_continue(),
              "Checkpoint metadata invented a proximity order");
        (void)tied_session.mutable_state();
        check(!tied_session.state().nearby_cities.order_gap,
              "Mutable alias retained stale pointer-order diagnostics");
        auto bad_link=state;
        bad_link.world.map_projection->cells[0].plot_city->id=99;
        rejects([&]{rebuild_bnw_nearby_cities(bad_link,true);});
        for (auto &cell:bad_link.world.map_projection->cells) cell.is_city=false;
        rejects([&]{resolve_bnw_city_plot_links(bad_link,true);});
        check(!bad_link.world.map_projection->cells.at(0).is_city &&
              !bad_link.world.map_projection->cells.at(14).is_city,
              "Malformed link hook partially published plot flags");
        auto vp=state;
        vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        rejects([&]{rebuild_bnw_nearby_cities(vp,true);});
        check(state.random.gameplay.state()==rng.gameplay.state() &&
              state.random.map.state()==rng.map.state(),
              "BNW nearby-city load advanced RNG");
        saves::SavedSession imported;
        imported.snapshot=state.world;
        imported.source.bytes={1,2,3};
        auto session=LiveSession::from_restored_bnw(
            std::move(imported),std::move(state));
        const auto checkpoint=session.checkpoint();
        const auto declined=apply_live_action(session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::rejected,"declined"};
        });
        check(declined.outcome==LiveActionOutcome::rejected &&
              session.state().nearby_cities.current,
              "Rejected action invalidated the unchanged city index");
        (void)apply_live_action(session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::committed,{}};
        });
        check(!session.state().nearby_cities.current,
              "Committed action retained a potentially stale city index");
        session.restore(checkpoint);
        check(session.state().nearby_cities.current &&
              bnw_nearby_cities(session.state(),{5,10}).size()==2,
              "Checkpoint failed to restore city index and live plot flags");
        (void)session.mutable_state();
        check(!session.state().nearby_cities.current,
              "Arbitrary live state write retained the city index");
        if (argc==2) selected_save(std::filesystem::path(argv[1]));
        std::cout << "PASS BNW CvCityManager load index"
                  << (argc==2?" and selected save":"") << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW CvCityManager load index: "
                  << error.what() << '\n';
        return 1;
    }
}
