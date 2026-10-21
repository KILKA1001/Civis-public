#pragma once

#include "civ5/runtime/bnw_map_turn.hpp"
#include "civ5/content/rules_database.hpp"
#include <optional>
#include <string>

namespace civ5::runtime {

enum class BnwTerritoryBranch {
    no_team,friendly,enemy,rival,unmet_minor,human_minor,
    anger_free_minor,open_borders,blocked,unresolved
};
struct BnwTerritoryDecision {
    std::optional<bool> allowed;
    BnwTerritoryBranch branch=BnwTerritoryBranch::unresolved;
    std::int64_t unit_owner=-1,plot_owner=-1,unit_team=-1,plot_team=-1;
    std::string reason;
};

// CvPlot::verifyUnitValidPlot calls canEnterTerritory(plot.getTeam(), false,
// plot.isCity(), false). The query proves source branches from the current
// owned unit/map/team roster. Active BNW rules are needed if execution reaches
// CvUnitInfo::IsRivalTerritory. Unresolved minor AI intrusion/friendship is
// reported as nullopt, never silently read as false.
BnwTerritoryDecision quote_bnw_unit_can_enter_territory(
    const LiveSessionState &state,std::uint64_t plot,
    saves::SavedPlotUnitReference handle,
    const content::RulesDatabase *active_rules=nullptr);

// Assign directly to BnwPlotUnitReadServices::can_enter_territory. The rules
// database, if supplied, must outlive the synchronous GameCore preflight.
BnwUnitValidityProbe make_bnw_can_enter_territory_probe(
    const content::RulesDatabase *active_rules=nullptr);

} // namespace civ5::runtime
