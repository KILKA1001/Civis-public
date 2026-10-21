#include "civ5/runtime/bnw_unit_sight_detached.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
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
    throw std::runtime_error("Selected BNW detached sight unit field missing");
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
    throw std::runtime_error("Selected BNW save has no simple sight candidate");
}
const saves::PlotArrayObservation &array(
    const std::vector<saves::PlotArrayObservation> &arrays,std::string_view wanted) {
    for(const auto &item:arrays)
        if(item.name && wanted==item.name) return item;
    throw std::runtime_error("Selected BNW detached sight array missing");
}
std::int64_t area_count(const LiveSessionState &state,std::int64_t area,
                        std::int64_t team) {
    for(const auto &container:state.world.map_regions->containers)
        if(container.name && std::string_view(container.name)=="areas")
            for(const auto &record:container.records)
                for(const auto &field:record.fields)
                    if(field.name && std::string_view(field.name)=="id" &&
                       field.value==area)
                        return array(record.arrays,"revealed_tiles").values.at(
                            static_cast<std::size_t>(team));
    throw std::runtime_error("Selected BNW area missing");
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
    const auto result=apply_bnw_unit_sight_detached(
        state,request,database,types,defines,host.identity());
    check(result.calculated && result.detached && !result.can_commit() &&
          result.plan.newly_visible.size()==5,
          result.reason.c_str());
    const auto &copy=*result.detached;
    std::size_t changed=0,first=0;
    std::map<std::int64_t,std::int64_t> area_increments;
    for(const auto &delta:result.plan.counters.plots) {
        const auto &raw=array(copy.world.map_plots->plots[delta.plot].arrays,
                              "visibility").values;
        const auto &invisible=array(copy.world.map_plots->plots[delta.plot].arrays,
                                    "invisible_visibility").values;
        const auto &source_raw=array(state.world.map_plots->plots[delta.plot].arrays,
                                     "visibility").values;
        const auto slot=static_cast<std::size_t>(result.plan.team);
        check(raw[slot]==delta.after_new &&
              invisible[slot]==delta.invisible_after_new &&
              copy.plot_visibility.counts[delta.plot*80+slot]==delta.after_new &&
              copy.plot_visibility.source_raw[delta.plot*80+slot]==delta.after_new &&
              source_raw[slot]==state.plot_visibility.source_raw[delta.plot*80+slot],
              "Detached BNW sight raw/cache counts or source ownership disagree");
        changed+=delta.after_new!=delta.before;
    }
    for(const auto &edge:result.plan.newly_visible) {
        const auto &cell=copy.world.map_projection->cells[edge.plot];
        const auto &source_cell=state.world.map_projection->cells[edge.plot];
        check(cell.revealed_teams->contains(result.plan.team),
              "Detached BNW newly visible plot was not revealed");
        if(edge.first_reveal) {
            ++first;
            check(!source_cell.revealed_teams->contains(result.plan.team),
                  "Detached BNW reveal changed immutable source projection");
            const auto &words=array(copy.world.map_plots->plots[edge.plot].arrays,
                                    "revealed_words").values;
            check((static_cast<std::uint32_t>(words[result.plan.team/32]) &
                   (std::uint32_t{1}<<(result.plan.team%32)))!=0,
                  "Detached BNW raw first reveal bit was not set");
            if(edge.area>=0) ++area_increments[edge.area];
        }
    }
    for(const auto &[area,count]:area_increments)
        check(area_count(copy,area,result.plan.team)==
              area_count(state,area,result.plan.team)+count,
              "Detached BNW area reveal count disagrees with first reveals");
    check(changed>0 && first==5,
          "Selected BNW detached visibility/reveal corpus result changed");
    (void)bnw_live_plot_visibility_cache(copy);
    auto wrong=host.identity();wrong.rules_sha256=std::string(64,'0');
    const auto rejected=apply_bnw_unit_sight_detached(
        state,request,database,types,defines,wrong);
    check(!rejected.calculated && !rejected.detached && !rejected.can_commit(),
          "BNW detached sight did not discard failed transaction");
    check(identify_file(save).sha256==source.sha256 &&
          state.world.map_projection->cells[
              result.plan.newly_visible.front().plot].revealed_teams==
          host.session().state().world.map_projection->cells[
              result.plan.newly_visible.front().plot].revealed_teams,
          "BNW detached sight mutated source or live session");
    std::cout << "BNW detached sight plots=" << result.plan.counters.plots.size() <<
        " changed=" << changed << " first reveal=" << first << '\n';
}
}

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW detached sight partial transition\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW detached sight: " << error.what() << '\n';
        return 1;
    }
}
