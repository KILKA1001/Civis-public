#include "civ5/runtime/bnw_unit_sight_preflight.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/live_session.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted,
                  saves::UnitCoreEncoding encoding=saves::UnitCoreEncoding::i32) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW sight unit core field");found=&field;
        }
    require(found && found->encoding==encoding && found->values.size()==1,
            "Missing BNW sight unit core field");
    return found->values.front();
}
const saves::PlotArrayObservation &array(const saves::PlotObservation &plot,
                                         std::string_view wanted,std::size_t width) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:plot.arrays)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW sight plot array");found=&item;
        }
    require(found && found->columns==1 && found->values.size()==width,
            "Missing BNW sight plot array or wrong wire width");
    return *found;
}
bool revealed(const saves::SavedSessionSnapshot &world,std::uint64_t index,
              std::int64_t team) {
    const auto &plot=world.map_plots->plots.at(index);
    const auto &cell=world.map_projection->cells.at(index);
    require(plot.index==index && cell.index==index && cell.revealed_teams,
            "BNW sight raw/projected plot identity or reveal is unavailable");
    const auto &words=array(plot,"revealed_words",4).values;
    const auto word=static_cast<std::size_t>(team/32);
    require(words[word]>=0 && words[word]<=UINT32_MAX,
            "BNW sight reveal word is outside u32 wire range");
    const bool raw=(static_cast<std::uint32_t>(words[word]) &
                    (std::uint32_t{1}<<(team%32)))!=0;
    require(raw==cell.revealed_teams->contains(team),
            "BNW sight raw/projected reveal state disagrees");
    return raw;
}
} // namespace

BnwUnitSightPreflight preflight_bnw_unit_sight(
    const LiveSessionState &state,const BnwSetXYRequest &request) {
    BnwUnitSightPreflight result;
    try {
        const auto movement=preflight_bnw_set_xy(state.world,request);
        require(movement.simple_candidate,
                "BNW sight requires a simple setXY candidate");
        const auto &world=state.world;
        require(world.game_state.layout==bnw && world.pregame &&
                world.pregame->layout==bnw && world.map_plots &&
                world.map_projection && world.teams && world.teams->layout==bnw &&
                world.all_players && world.all_players->layout==bnw,
                "BNW sight requires decoded setup, teams, map and units");
        const auto mapping=world.pregame->archive.integer_arrays.find("team_types");
        require(mapping!=world.pregame->archive.integer_arrays.end() &&
                request.owner>=0 &&
                static_cast<std::uint64_t>(request.owner)<mapping->second.size(),
                "BNW sight owner-to-team mapping is unavailable");
        const auto team=mapping->second[static_cast<std::size_t>(request.owner)];
        require(team>=0 && team<64 &&
                static_cast<std::size_t>(team)<world.teams->teams.size() &&
                static_cast<std::uint64_t>(team)<world.map_plots->dimensions.teams,
                "BNW sight team is outside active SDK team range");
        const saves::SavedUnitObservation *selected=nullptr;
        for(const auto &player:world.all_players->players)
            for(const auto &unit:player.units.units)
                if(core(unit,"m_eOwner")==request.owner &&
                   core(unit,"m_iID")==request.unit_id) {
                    require(!selected && player.saved_player_id==request.owner,
                            "BNW sight unit owner or ID is duplicated");
                    selected=&unit;
                }
        require(selected,"BNW sight unit is absent");
        // CvUnit::canChangeVisibility is true on DEFAULT_UNIT_MAP_LAYER;
        // getFacingDirection(true) is always NO_DIRECTION in Expansion2.
        // The embarked branch needs m_iEmbarkExtraVisibility, which BNW's
        // decoded unit core does not currently supply.
        require(core(*selected,"m_bEmbarked",saves::UnitCoreEncoding::boolean)==0,
                "BNW embarked sight range needs undecoded extra visibility");
        const auto unit_type=core(*selected,"m_eUnitType");
        const auto extra=core(*selected,"m_iExtraVisibilityRange");
        const auto see_invisible=core(*selected,"m_eSeeInvisibleType");
        require(unit_type>=0 && extra>=INT32_MIN && extra<=INT32_MAX &&
                see_invisible>=-1 && see_invisible<INT32_MAX,
                "BNW sight unit range or invisibility inputs are invalid");
        const auto &grid=world.map_header.grid;
        const auto from=grid.index(movement.from_x,movement.from_y);
        const auto to=grid.index(movement.to_x,movement.to_y);
        (void)bnw_live_plot_visibility_cache(state);
        result.from_center_visibility=bnw_plot_visibility_count(state,from,team);
        result.to_center_visibility=bnw_plot_visibility_count(state,to,team);
        result.from_center_revealed=revealed(world,from,team);
        result.to_center_revealed=revealed(world,to,team);
        result.candidate=true;
        result.owner=request.owner;result.unit_id=request.unit_id;
        result.team=team;result.unit_type=unit_type;
        result.extra_visibility=extra;result.see_invisible=see_invisible;
        result.from_plot=from;result.to_plot=to;
        result.blockers={BnwSightBlocker::unit_sight_range_rules,
                         BnwSightBlocker::terrain_and_feature_line_of_sight,
                         BnwSightBlocker::reveal_meeting_and_exploration_effects,
                         BnwSightBlocker::ui_and_derived_cache_effects,
                         BnwSightBlocker::visibility_checkpoint};
        if(see_invisible!=-1)
            result.blockers.push_back(BnwSightBlocker::invisible_visibility_effects);
        result.reason="BNW sight radius, all affected plots and reveal/team/cache effects remain unimplemented";
    } catch(const std::exception &error) {
        result.candidate=false;
        result.blockers={BnwSightBlocker::unsupported_candidate};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
