#pragma once
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::runtime {
// Inputs not already owned by the shared roster/diplomacy/passability models.
// This is player-level plot eligibility, not a unit pathfinder or move action.
struct PlotMovementState {
    struct Player {
        bool embarked_all_water=false,mountain_pass=false,mountains_after_general=false,crosses_ice=false;
        std::int64_t generals_created=0; // includes free generals, as source getter(false)
    };
    struct Team {
        std::int64_t embark_count=0,all_water_count=0;
        std::set<std::int64_t> open_borders;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::int64_t,Team> teams;
    ProfileExtensions extensions;
};
PlotMovementState restore_plot_movement(const saves::SavedSessionSnapshot&,
    const ResearchContext&,const PlayerResearchContext&);
struct PlotMovementContext {
    const saves::SavedMapProjection &map;
    const PlotPassability &passability;
    const ResearchContext &players;
    const PlayerResearchContext &diplomacy;
    PlotMovementState &state;
    const content::InfoTypeCatalog &types;
    const content::GlobalDefineCache &defines;
};
// Explicit portion of VP updateTeamStatus. Changing pregame flags alone does
// not silently refresh the cached minor-team result used by movement.
void refresh_plot_movement_team_status(PlotMovementContext&,std::int64_t team);
bool movement_open_borders(const PlotMovementContext&,std::int64_t from,std::int64_t to);
bool player_can_embark(const PlotMovementContext&,std::int64_t player);
bool player_can_cross_ocean(const PlotMovementContext&,std::int64_t player);
bool player_can_cross_mountain(const PlotMovementContext&,std::int64_t player);
bool player_can_cross_ice(const PlotMovementContext&,std::int64_t player);
bool plot_is_valid_move(const PlotMovementContext&,std::int64_t plot,
    std::int64_t player,bool check_territory=true);
} // namespace civ5::runtime
