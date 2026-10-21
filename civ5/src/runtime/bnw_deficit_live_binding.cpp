#include "civ5/runtime/bnw_deficit_live_binding.hpp"
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
using Handle=std::pair<std::int64_t,std::int64_t>;
void require(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
std::int64_t core_field(const saves::UnitCoreObservation &core,
                        std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for (const auto &field:core.fields)
        if (field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW fighting unit core field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW fighting unit core field unavailable");
    return found->values.front();
}
std::int64_t tail_field(const saves::UnitTailObservation &tail,
                        std::string_view name) {
    const saves::UnitTailNode *state=nullptr,*found=nullptr;
    for (const auto &block:tail.blocks)
        if (block.name && std::string_view(block.name)=="state") {
            require(!state,"Duplicate BNW fighting unit tail state");
            state=&block;
        }
    require(state,"BNW fighting unit tail state unavailable");
    for (const auto &field:state->children)
        if (field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW fighting unit tail field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW fighting unit tail field unavailable");
    return found->values.front();
}
const saves::SavedUnitObservation *unit(const LiveSessionState &state,
                                       Handle handle) {
    const saves::SavedUnitObservation *found=nullptr;
    for (const auto &player:state.world.all_players->players) {
        require(player.units.layout==bnw,
                "BNW fighting player units profile mismatch");
        if (player.saved_player_id!=handle.first) continue;
        for (const auto &candidate:player.units.units)
            if (core_field(candidate.core,"m_iID")==handle.second) {
                require(!found,"Duplicate BNW fighting unit identity");
                require(candidate.core.layout==bnw && candidate.tail.layout==bnw &&
                        core_field(candidate.core,"m_eOwner")==handle.first,
                        "BNW fighting unit identity/profile mismatch");
                found=&candidate;
            }
    }
    return found;
}
} // namespace

bool bind_bnw_plot_unit_fighting(
    const LiveSessionState &state,std::int64_t x,std::int64_t y,
    const BnwDeficitLiveEvidence &evidence) {
    require(state.world.game_state.layout==bnw &&
            state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.world.map_projection &&
            evidence.plot_unit_roster_current && x>=0 && y>=0,
            "BNW plot fighting requires current live plot unit roster");
    const auto &projection=*state.world.map_projection;
    require(static_cast<std::uint64_t>(x)<projection.grid.width &&
            static_cast<std::uint64_t>(y)<projection.grid.height &&
            projection.cells.size()==projection.grid.size(),
            "BNW plot fighting coordinates/grid unresolved");
    const auto index=projection.grid.index(static_cast<std::uint64_t>(x),
                                           static_cast<std::uint64_t>(y));
    const auto &cell=projection.cells.at(static_cast<std::size_t>(index));
    require(cell.index==index && cell.x==static_cast<std::uint64_t>(x) &&
            cell.y==static_cast<std::uint64_t>(y) && cell.units,
            "BNW plot unit list is unavailable or mismatched");
    std::set<Handle> roster;
    for (const auto &handle:*cell.units) {
        require(handle.owner>=0 && handle.id>=0 &&
                roster.insert({handle.owner,handle.id}).second,
                "BNW plot unit handle is invalid or duplicated");
        const auto *resident=unit(state,{handle.owner,handle.id});
        require(resident && core_field(resident->core,"m_iX")==x &&
                core_field(resident->core,"m_iY")==y,
                "BNW plot unit roster disagrees with owned units");
    }
    // Catch a unit placed at these coordinates but absent from the plot list.
    for (const auto &player:state.world.all_players->players)
        for (const auto &candidate:player.units.units) {
            require(candidate.core.layout==bnw && candidate.tail.layout==bnw,
                    "BNW owned unit profile mismatch in plot roster");
            if (core_field(candidate.core,"m_iX")==x &&
                core_field(candidate.core,"m_iY")==y)
                require(roster.contains({player.saved_player_id,
                                         core_field(candidate.core,"m_iID")}),
                        "BNW owned unit is missing from current plot roster");
        }

    // CvPlot::isUnitFighting loops its roster; CvUnit::isFighting is combat
    // unit OR combat city. The latter is an unserialized IDInfo in BNW.
    bool unknown_city=false;
    for (const auto &handle:roster) {
        const auto *resident=unit(state,handle);
        const auto combat_owner=tail_field(resident->tail,"m_combatUnit.eOwner");
        const auto combat_id=tail_field(resident->tail,"m_combatUnit.iID");
        require((combat_owner==-1 && combat_id==-1) ||
                (combat_owner>=0 && combat_id>=0),
                "BNW combat unit handle is malformed");
        if (combat_owner>=0) {
            require(unit(state,{combat_owner,combat_id}),
                    "BNW combat unit handle is unresolved");
            return true;
        }
        const auto city=evidence.combat_city_current.find(handle);
        if (city==evidence.combat_city_current.end()) unknown_city=true;
        else if (city->second) return true;
    }
    require(!unknown_city,
            "BNW combat city is not serialized; live combat-city state required");
    return false;
}

BnwDeficitMilitaryContext bind_bnw_deficit_military_context(
    const LiveSessionState &state,std::int64_t player,
    std::span<const std::pair<std::int64_t,std::int64_t>> plots,
    const BnwDeficitLiveEvidence &evidence) {
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw && player>=0 &&
            evidence.military_ai_current && evidence.land_defense &&
            evidence.naval_defense && *evidence.land_defense>=-1 &&
            *evidence.land_defense<=3 && *evidence.naval_defense>=-1 &&
            *evidence.naval_defense<=3,
            "BNW military defense states require current CvMilitaryAI");
    bool owner=false;
    for (const auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!owner,"Duplicate BNW military player");owner=true;
        }
    require(owner,"BNW military player unavailable");
    BnwDeficitMilitaryContext result;
    result.land_defense=evidence.land_defense;
    result.naval_defense=evidence.naval_defense;
    for (const auto &plot:plots)
        result.plot_unit_fighting.emplace(plot,
            bind_bnw_plot_unit_fighting(state,plot.first,plot.second,evidence));
    return result;
}

} // namespace civ5::runtime
