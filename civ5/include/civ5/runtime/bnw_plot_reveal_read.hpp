#pragma once
#include "civ5/saves/map_state.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime::detail {
// Native CvPlot::isRevealed's reached bit read, AFTER the caller's genuine
// Game-debug bypass. This source projection is not a Game/VM issuer.
inline bool bnw_plot_revealed_read(const saves::SavedSessionSnapshot& world,
                                 std::uint64_t slot,std::int32_t team){
 const auto need=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(world.game_state.layout==bnw&&world.map_header.layout==bnw&&world.map_plots&&
      world.map_plots->layout==bnw&&world.map_projection,"Reveal native BNW source unavailable");
 const auto& grid=world.map_header.grid;const auto& projection=*world.map_projection;
 need(grid.width&&grid.height&&projection.grid.width==grid.width&&projection.grid.height==grid.height&&
      world.map_plots->plots.size()==grid.size()&&projection.cells.size()==grid.size()&&slot<grid.size(),
      "Reveal current native map shape differs");
 const auto& raw=world.map_plots->plots.at(slot);const auto& cell=projection.cells.at(slot);
 const auto xy=grid.coordinates(slot);
 need(raw.index==slot&&raw.x==xy[0]&&raw.y==xy[1]&&cell.index==slot&&cell.x==xy[0]&&cell.y==xy[1],
      "Reveal current raw/owned plot identity differs");
 need(team>=0&&team<64,"Reveal NO_TEAM/outside native bit domain");
 const saves::PlotArrayObservation* words=nullptr;
 for(const auto& a:raw.arrays)if(a.name&&std::string_view(a.name)=="revealed_words"){
  need(!words,"Reveal duplicate native words");words=&a;
 }
 need(words&&words->columns==1&&words->values.size()==4&&cell.revealed_teams,
      "Reveal actual four-DWORD wire shape unavailable");
 const auto bit=static_cast<std::size_t>(team);const auto word=words->values.at(bit/32);
 need(word>=0&&word<=UINT32_MAX,"Reveal reached word outside uint32");
 const bool value=(static_cast<std::uint32_t>(word)&(std::uint32_t{1}<<(bit%32)))!=0;
 need(value==cell.revealed_teams->contains(team),"Reveal raw/owned bit differs");
 return value;
}
}
