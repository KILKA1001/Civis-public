#include "civ5/runtime/bnw_unit_sight_reveal_details.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/map_geometry.hpp"
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
    throw std::runtime_error("Selected BNW unit field missing");
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
    throw std::runtime_error("Selected Gandhi save lacks simple sight candidate");
}
const std::vector<std::int64_t> &array(
    const std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted) {
    for(const auto &item:arrays)
        if(item.name && wanted==item.name) return item.values;
    throw std::runtime_error("Selected BNW sight detail array missing");
}
std::vector<std::int64_t> &mutable_array(
    std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted) {
    for(auto &item:arrays)
        if(item.name && wanted==item.name) return item.values;
    throw std::runtime_error("Selected BNW sight detail array missing");
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view wanted) {
    for(const auto &item:plot.fields)
        if(item.name && wanted==item.name) return item.value;
    throw std::runtime_error("Selected BNW plot field missing");
}
void set_field(saves::PlotObservation &plot,std::string_view wanted,
               std::int64_t value) {
    for(auto &item:plot.fields)
        if(item.name && wanted==item.name) {item.value=value;return;}
    throw std::runtime_error("Selected BNW plot field missing for synthetic effect");
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
    auto result=apply_bnw_unit_sight_reveal_details(
        state,request,database,types,defines,host.identity());
    check(result.calculated && result.sight.detached && !result.can_commit() &&
          result.sight.plan.newly_visible.size()==5,
          result.reason.c_str());
    const auto &copy=*result.sight.detached;
    const auto team=static_cast<std::size_t>(result.sight.plan.team);
    const MapGeometry geometry{copy.world.map_header.grid,
        copy.world.map_header.wrap_x,copy.world.map_header.wrap_y,
        saves::GameStateLayout::expansion2_version1};
    const auto visible=[&](std::uint64_t index) {
        return copy.plot_visibility.counts.at(index*80+team)>0;
    };
    std::size_t changed_owners=0;
    for(const auto &edge:result.sight.plan.newly_visible) {
        const auto &plot=copy.world.map_plots->plots[edge.plot];
        const auto &before=state.world.map_plots->plots[edge.plot];
        check(array(plot.arrays,"revealed_owners")[team]==edge.plot_owner &&
              array(plot.arrays,"revealed_improvement_hashes")[team]==
                  field(plot,"improvement_type_hash") &&
              array(plot.arrays,"revealed_routes")[team]==
                  field(plot,"route_type") &&
              field(plot,"owner")==field(before,"owner"),
              "BNW setRevealed known owner/improvement/route mismatch");
        for(const auto neighbor:edge.adjacent_owner_refresh) {
            const auto &changed=copy.world.map_plots->plots[neighbor];
            bool owner_visible=visible(neighbor);
            const auto xy=copy.world.map_header.grid.coordinates(neighbor);
            for(std::int64_t direction=0;direction<6 && !owner_visible;++direction) {
                const auto around=geometry.direction(xy[0],xy[1],direction);
                owner_visible=around && visible(*around);
            }
            check(array(changed.arrays,"revealed_owners")[team]==
                  (owner_visible?field(changed,"owner"):-1),
                  "BNW adjacent owner refresh disagrees with final visible neighborhood");
            changed_owners+=array(changed.arrays,"revealed_owners")[team]!=
                array(state.world.map_plots->plots[neighbor].arrays,
                      "revealed_owners")[team];
        }
    }
    check(request.owner==0 && request.unit_id==16385,
          "Selected Gandhi adjacent owner refresh candidate changed");
    // Exercise positive known-state writes on a detached corpus variation.
    // Keep source/destination untouched so the simple setXY preflight remains
    // valid; owner, improvement and route are independent sight inputs.
    auto synthetic=state;
    std::uint64_t from=state.world.map_header.grid.size();
    for(const auto &player:state.world.all_players->players)
        for(const auto &unit:player.units.units)
            if(core(unit,"m_eOwner")==request.owner &&
               core(unit,"m_iID")==request.unit_id)
                from=state.world.map_header.grid.index(
                    core(unit,"m_iX"),core(unit,"m_iY"));
    check(from<state.world.map_header.grid.size(),
          "Selected BNW moved unit disappeared");
    const auto to=state.world.map_header.grid.index(request.to_x,request.to_y);
    const auto it=std::find_if(result.sight.plan.newly_visible.begin(),
        result.sight.plan.newly_visible.end(),[&](const auto &edge) {
            return edge.plot!=from && edge.plot!=to;
        });
    check(it!=result.sight.plan.newly_visible.end(),
          "Selected Gandhi lacks independent reveal-effect plot");
    const auto synthetic_plot=it->plot;
    const auto *farm=types.find("IMPROVEMENT_FARM");
    const auto *road=types.find("ROUTE_ROAD");
    check(farm && farm->table=="Improvements" && road &&
          road->table=="Routes","Selected BNW reveal rules lack farm/road");
    auto &raw=synthetic.world.map_plots->plots[synthetic_plot];
    auto &projected=synthetic.world.map_projection->cells[synthetic_plot];
    set_field(raw,"owner",request.owner);projected.owner=request.owner;
    set_field(raw,"improvement_type_hash",farm->hash);
    projected.improvement_type_hash=farm->hash;
    set_field(raw,"route_type",road->id);
    check(projected.route.has_value(),"Selected BNW route projection unavailable");
    projected.route->type=road->id;
    constexpr std::int64_t synthetic_city_id=900001;
    set_field(raw,"plot_city_owner",request.owner);
    set_field(raw,"plot_city_id",synthetic_city_id);
    projected.plot_city=saves::SavedPlotCityReference{
        request.owner,synthetic_city_id};
    projected.is_city=true;
    const auto xy=synthetic.world.map_header.grid.coordinates(synthetic_plot);
    saves::SavedCityObservation fake_city;
    fake_city.core.layout=saves::GameStateLayout::expansion2_version1;
    const auto city_scalar=[&](const char *name,std::int64_t value) {
        saves::CityCoreField field;
        field.name=name;field.encoding=saves::CityCoreEncoding::i32;
        field.values={value};fake_city.core.fields.push_back(std::move(field));
    };
    city_scalar("m_iID",synthetic_city_id);
    city_scalar("m_eOwner",request.owner);
    city_scalar("m_iX",xy[0]);city_scalar("m_iY",xy[1]);
    saves::CityCoreField reveal_vector;
    reveal_vector.name="m_abRevealed";
    reveal_vector.encoding=saves::CityCoreEncoding::vector_bool;
    reveal_vector.values.assign(80,0);
    fake_city.core.fields.push_back(std::move(reveal_vector));
    bool city_added=false;
    for(auto &player:synthetic.world.all_players->players)
        if(player.saved_player_id==request.owner) {
            player.cities.cities.push_back(std::move(fake_city));
            city_added=true;break;
        }
    check(city_added,"Selected BNW synthetic city owner absent");
    const auto enriched=apply_bnw_unit_sight_reveal_details(
        synthetic,request,database,types,defines,host.identity());
    check(enriched.calculated && enriched.sight.detached,
          enriched.reason.c_str());
    const auto &enriched_plot=enriched.sight.detached->world.map_plots->plots[
        synthetic_plot];
    check(array(enriched_plot.arrays,"revealed_owners")[team]==request.owner &&
          array(enriched_plot.arrays,"revealed_improvement_hashes")[team]==
              farm->hash &&
          array(enriched_plot.arrays,"revealed_routes")[team]==road->id &&
          array(enriched.sight.detached->world.teams->teams[team].arrays,
                "found_player_territory")[request.owner]==1,
          "BNW known territory/improvement/route detached effects missing");
    bool city_revealed=false;
    for(const auto &player:enriched.sight.detached->world.all_players->players)
        if(player.saved_player_id==request.owner)
            for(const auto &city:player.cities.cities)
                for(const auto &field:city.core.fields)
                    if(field.name && std::string_view(field.name)=="m_iID" &&
                       field.values==std::vector<std::int64_t>{synthetic_city_id})
                        for(const auto &state:city.core.fields)
                            if(state.name && std::string_view(state.name)=="m_abRevealed")
                                city_revealed=state.values.at(team)==1;
    check(city_revealed,"BNW detached city reveal bit was not updated");
    auto corrupt=state;
    const auto later=result.sight.plan.newly_visible.at(1).plot;
    mutable_array(corrupt.world.map_plots->plots[later].arrays,
                  "revealed_improvement_hashes").pop_back();
    const auto rejected=apply_bnw_unit_sight_reveal_details(
        corrupt,request,database,types,defines,host.identity());
    check(!rejected.calculated && !rejected.sight.detached &&
          !rejected.can_commit(),
          "BNW sight details published a partially updated detached world");
    check(identify_file(save).sha256==source.sha256 &&
          array(state.world.map_plots->plots[later].arrays,
                "revealed_improvement_hashes").size()==80,
          "BNW sight details changed immutable source");
    std::cout << "BNW sight known reveal edges=" <<
        result.sight.plan.newly_visible.size() <<
        " adjacent owner differences=" << changed_owners << '\n';
}
}

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW detached known-reveal details\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW sight reveal details: " << error.what() << '\n';
        return 1;
    }
}
