#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t plot_field(const saves::PlotObservation &plot,
                        std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:plot.fields) if(name(field.name)==wanted) {
        require(!found,"Duplicate BNW map preflight plot field");
        found=&field;
    }
    require(found,"Missing BNW map preflight plot field");
    return found->value;
}
std::int64_t core(const saves::SavedUnitObservation &unit,
                  std::string_view wanted,saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields) if(name(field.name)==wanted) {
        require(!found,"Duplicate BNW map preflight unit core field");
        found=&field;
    }
    require(found && found->encoding==encoding && found->values.size()==1,
            "Missing BNW map preflight unit core field");
    return found->values.front();
}
std::int64_t core(const saves::SavedUnitObservation &unit,
                  std::string_view wanted) {
    return core(unit,wanted,saves::UnitCoreEncoding::i32);
}
const saves::UnitTailNode *tail_field(const saves::UnitTailNode &node,
                                      std::string_view wanted) {
    if(name(node.name)==wanted) return &node;
    for(const auto &child:node.children)
        if(const auto *found=tail_field(child,wanted)) return found;
    return nullptr;
}
std::int64_t tail(const saves::SavedUnitObservation &unit,
                  std::string_view wanted) {
    const saves::UnitTailNode *found=nullptr;
    for(const auto &block:unit.tail.blocks)
        if(const auto *candidate=tail_field(block,wanted)) {
            require(!found,"Duplicate BNW map preflight unit tail field");
            found=candidate;
        }
    require(found && found->values.size()==1,
            "Missing BNW map preflight unit tail field");
    return found->values.front();
}
std::vector<saves::SavedPlotUnitReference> raw_handles(
    const saves::PlotObservation &plot) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays) if(name(array.name)=="units") {
        require(!found,"Duplicate BNW map preflight raw unit list");
        found=&array;
    }
    require(found && found->columns==2 && found->values.size()%2==0,
            "Missing BNW map preflight raw unit list");
    std::vector<saves::SavedPlotUnitReference> result;
    result.reserve(found->values.size()/2);
    for(std::size_t i=0;i<found->values.size();i+=2)
        result.push_back({found->values[i],found->values[i+1]});
    return result;
}
} // namespace

BnwMapOccupiedPreflight preflight_bnw_map_occupied(
    const saves::SavedSessionSnapshot &world) {
    BnwMapOccupiedPreflight report;
    try {
        require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
                world.map_plots && world.map_plots->layout==bnw &&
                world.map_projection && world.all_players &&
                world.all_players->layout==bnw &&
                world.map_plots->plots.size()==world.map_header.grid.size() &&
                world.map_projection->cells.size()==world.map_header.grid.size(),
                "BNW map preflight requires complete decoded map and players");
        std::map<std::pair<std::int64_t,std::int64_t>,
                 const saves::SavedUnitObservation*> units;
        for(const auto &player:world.all_players->players)
            for(const auto &unit:player.units.units) {
                require(unit.core.layout==bnw && unit.tail.layout==bnw,
                        "BNW map preflight unit layout changed");
                const auto owner=core(unit,"m_eOwner"),id=core(unit,"m_iID");
                require(owner==player.saved_player_id && owner>=0 && id>=0 &&
                        units.emplace(std::pair{owner,id},&unit).second,
                        "BNW map preflight unit identity is inconsistent");
            }
        std::set<std::pair<std::int64_t,std::int64_t>> plotted;
        const auto &grid=world.map_header.grid;
        for(std::uint64_t index=0;index<grid.size();++index) {
            const auto &plot=world.map_plots->plots[index];
            const auto &cell=world.map_projection->cells[index];
            const auto xy=grid.coordinates(index);
            require(plot.index==index && plot.x==xy[0] && plot.y==xy[1] &&
                    cell.index==index && cell.x==xy[0] && cell.y==xy[1] &&
                    cell.units && cell.owner &&
                    *cell.owner==plot_field(plot,"owner"),
                    "BNW map preflight plot identity or owner changed");
            const auto raw=raw_handles(plot);
            require(raw==*cell.units,
                    "BNW map preflight raw/projected unit rosters differ");
            const auto anchor=plot_field(plot,"world_anchor");
            const auto data=plot_field(plot,"world_anchor_data");
            require(anchor>=-1 && anchor<=127 && data>=-128 && data<=127,
                    "BNW map preflight anchor exceeds source wire range");
            if(anchor!=-1) report.anchors.push_back(
                {index,static_cast<std::int64_t>(xy[0]),
                 static_cast<std::int64_t>(xy[1]),anchor,data});
            for(const auto handle:raw) {
                require(handle.owner>=0 && handle.id>=0 &&
                        plotted.insert({handle.owner,handle.id}).second,
                        "BNW map preflight duplicate or invalid plot unit handle");
                BnwOccupiedPlotUnit item;
                item.plot=index;
                item.x=static_cast<std::int64_t>(xy[0]);
                item.y=static_cast<std::int64_t>(xy[1]);
                item.plot_owner=*cell.owner;
                item.handle=handle;
                const auto found=units.find({handle.owner,handle.id});
                if(found!=units.end()) {
                    const auto &unit=*found->second;
                    item.saved_unit_found=true;
                    item.at_saved_plot=core(unit,"m_iX")==item.x &&
                                       core(unit,"m_iY")==item.y;
                    const auto death=core(unit,"m_bDeathDelay",
                        saves::UnitCoreEncoding::boolean);
                    require(death==0 || death==1,
                            "BNW map preflight invalid death-delay boolean");
                    item.delayed_death=death!=0;
                    item.transport_link=tail(unit,"m_transportUnit.eOwner")!=-1;
                    item.combat_link=tail(unit,"m_combatUnit.eOwner")!=-1 ||
                                     core(unit,"m_iCombatTimer")!=0;
                }
                report.units.push_back(item);
            }
        }
        report.shape_valid=true;
        report.requires_unit_services=!report.units.empty();
        report.requires_anchor_service=!report.anchors.empty();
        if(report.requires_unit_services || report.requires_anchor_service)
            report.reason="CvPlot::verifyUnitValidPlot predicates/actions and/or "
                "SetWorldAnchor engine removal must be resolved before map turn";
    } catch(const std::exception &error) {
        report.shape_valid=false;
        report.reason=error.what();
    }
    return report;
}

} // namespace civ5::runtime
