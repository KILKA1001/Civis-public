#include "civ5/runtime/bnw_plot_domain_binding.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool valid,const char *message) {
    if(!valid) throw std::runtime_error(message);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
BnwUnitDomain domain(std::string_view value) {
    if(value=="DOMAIN_LAND") return BnwUnitDomain::land;
    if(value=="DOMAIN_SEA") return BnwUnitDomain::sea;
    if(value=="DOMAIN_AIR") return BnwUnitDomain::air;
    if(value=="DOMAIN_IMMOBILE") return BnwUnitDomain::immobile;
    if(value=="DOMAIN_HOVER") return BnwUnitDomain::hover;
    throw std::runtime_error("Unknown BNW unit domain");
}
std::int64_t core(const saves::SavedUnitObservation &unit,
                  std::string_view wanted,saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields) if(name(field.name)==wanted) {
        require(!found,"Duplicate BNW domain unit core field");
        found=&field;
    }
    require(found && found->encoding==encoding && found->values.size()==1,
            "Missing BNW domain unit core field");
    return found->values.front();
}
std::int64_t core(const saves::SavedUnitObservation &unit,
                  std::string_view wanted) {
    return core(unit,wanted,saves::UnitCoreEncoding::i32);
}
std::int64_t plot_field(const saves::PlotObservation &plot,
                        std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:plot.fields) if(name(field.name)==wanted) {
        require(!found,"Duplicate BNW domain plot field");
        found=&field;
    }
    require(found,"Missing BNW domain plot field");
    return found->value;
}
std::int64_t core(const saves::CityCoreObservation &city,
                  std::string_view wanted,saves::CityCoreEncoding encoding);
bool city(const saves::SavedSessionSnapshot &world,
          const saves::SavedMapCellProjection &cell,
          const saves::PlotObservation &plot) {
    const auto owner=plot_field(plot,"plot_city_owner");
    const auto id=plot_field(plot,"plot_city_id");
    require(cell.plot_city && cell.plot_city->owner==owner &&
            cell.plot_city->id==id,
            "BNW plot city raw/projected handle disagrees");
    if(owner==-1 && id==-1) return false;
    require(owner>=0 && owner<64 && id>=0 && world.all_players,
            "BNW plot city handle exceeds player roster");
    bool matched=false;
    for(const auto &player:world.all_players->players)
        if(player.saved_player_id==owner)
            for(const auto &candidate:player.cities.cities)
                if(core(candidate.core,"m_eOwner",saves::CityCoreEncoding::i32)==owner &&
                   core(candidate.core,"m_iID",saves::CityCoreEncoding::i32)==id) {
                    require(!matched,"Duplicate BNW plot city object");
                    matched=true;
                }
    return matched;
}
std::int64_t core(const saves::CityCoreObservation &city,
                  std::string_view wanted,saves::CityCoreEncoding encoding) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:city.fields) if(name(field.name)==wanted) {
        require(!found,"Duplicate BNW domain city core field");
        found=&field;
    }
    require(found && found->encoding==encoding && found->values.size()==1,
            "Missing BNW domain city core field");
    return found->values.front();
}
bool walk_water(const saves::SavedMapCellProjection &cell,
                const saves::PlotObservation &plot,
                const BnwPlotDomainRules &rules) {
    require(plot.version && *plot.version>=1 && *plot.version<=7,
            "Unsupported BNW plot version for improvement lookup");
    if(*plot.version>=5) {
        const auto hash=plot_field(plot,"improvement_type_hash");
        require(hash>=0 && hash<=UINT32_MAX,
                "BNW improvement hash exceeds source range");
        if(hash==0) {
            require(!cell.improvement_type || *cell.improvement_type==-1,
                    "BNW raw/projected no-improvement disagrees");
            return false;
        }
        require(cell.improvement_type && *cell.improvement_type>=0,
                "BNW improvement hash is unresolved");
        const auto found=rules.improvements.find(*cell.improvement_type);
        require(found!=rules.improvements.end() && found->second.hash==hash,
                "BNW improvement hash/ID disagrees with active rules");
        return found->second.allows_walk_water;
    }
    const auto id=plot_field(plot,"improvement_type");
    require(id>=-1 && cell.improvement_type && *cell.improvement_type==id,
            "BNW legacy improvement ID disagrees with projection");
    if(id==-1) return false;
    const auto found=rules.improvements.find(id);
    require(found!=rules.improvements.end(),
            "BNW legacy improvement ID is absent from active rules");
    return found->second.allows_walk_water;
}
} // namespace

BnwPlotDomainRules load_bnw_plot_domain_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types) {
    require(types.report().all_succeeded(),
            "BNW domain binding needs complete active InfoTypeCatalog");
    BnwPlotDomainRules result;
    const auto units=database.prefetch_info("Units",[&](auto slot,auto id,
                                                          const auto &row) {
        require(id>=0 && id<=INT32_MAX &&
                static_cast<std::uint64_t>(id)==slot &&
                row.has_column("Type") && row.has_column("Domain"),
                "BNW Units ID or Domain schema changed");
        const auto type=row.text("Type"),kind=row.text("Domain");
        require(type && !type->empty() && kind && !kind->empty(),
                "BNW Unit Type/Domain is missing");
        const auto *unit=types.find(*type),*domain_type=types.find(*kind);
        require(unit && unit->table=="Units" && unit->id==id &&
                unit->hash!=0 && unit->hash==content::saved_info_type_hash(*type) &&
                types.find_hash(unit->hash) &&
                types.find_hash(unit->hash)->type==*type &&
                domain_type && domain_type->table=="Domains" &&
                types.find_hash(domain_type->hash) &&
                types.find_hash(domain_type->hash)->type==*kind,
                "BNW Unit/Domain ID, hash or active SQL disagree");
        require(result.units.emplace(id,BnwPlotDomainUnitRule{
                    *type,unit->hash,domain(*kind)}).second,
                "Duplicate BNW Unit ID");
        return true;
    });
    require(units.all_succeeded() && units.rows>0 &&
            units.overlapping_ids==0,
            "BNW Units rules collection is incomplete");
    const auto improvements=database.prefetch_info("Improvements",
        [&](auto slot,auto id,const auto &row) {
            require(id>=0 && id<=INT32_MAX &&
                    static_cast<std::uint64_t>(id)==slot &&
                    row.has_column("Type") && row.has_column("AllowsWalkWater"),
                    "BNW Improvements ID or AllowsWalkWater schema changed");
            const auto type=row.text("Type");
            const auto flag=row.integer("AllowsWalkWater");
            require(type && !type->empty() && (flag==0 || flag==1),
                    "Invalid BNW Improvement Type/AllowsWalkWater");
            const auto *entry=types.find(*type);
            require(entry && entry->table=="Improvements" && entry->id==id &&
                    entry->hash!=0 &&
                    entry->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(entry->hash) &&
                    types.find_hash(entry->hash)->type==*type &&
                    result.improvements.emplace(id,BnwPlotDomainImprovementRule{
                        *type,entry->hash,flag!=0}).second,
                    "BNW Improvement ID/hash disagrees with active SQL");
            return true;
        });
    require(improvements.all_succeeded() &&
            improvements.overlapping_ids==0,
            "BNW Improvements rules collection is incomplete");
    return result;
}

BnwPlotDomainVerdict evaluate_bnw_plot_valid_domain(
    const LiveSessionState &state,std::uint64_t plot_index,
    saves::SavedPlotUnitReference handle,
    const BnwPlotDomainRules &rules) {
    BnwPlotDomainVerdict result;
    try {
        const auto &world=state.world;
        const auto shape=preflight_bnw_map_occupied(world);
        require(shape.shape_valid && world.game_state.layout==bnw &&
                world.map_projection && world.map_plots && world.all_players &&
                plot_index<world.map_projection->cells.size(),
                "BNW plot domain needs a consistent owned map and unit roster");
        const auto &cell=world.map_projection->cells[plot_index];
        const auto &plot=world.map_plots->plots[plot_index];
        require(cell.plot_type && *cell.plot_type>=0 && *cell.plot_type<=3 &&
                plot_field(plot,"plot_type")==*cell.plot_type,
                "BNW plot type is unresolved or disagrees with raw plot");
        bool plotted=false;
        for(const auto &candidate:shape.units)
            if(candidate.plot==plot_index && candidate.handle==handle) {
                require(!plotted && candidate.saved_unit_found &&
                        candidate.at_saved_plot,
                        "BNW domain unit is absent or not at plot");
                plotted=true;
            }
        require(plotted,"BNW domain unit handle is absent from plot");
        const saves::SavedUnitObservation *unit=nullptr;
        for(const auto &player:world.all_players->players)
            for(const auto &candidate:player.units.units)
                if(core(candidate,"m_eOwner")==handle.owner &&
                   core(candidate,"m_iID")==handle.id) {
                    require(!unit && player.saved_player_id==handle.owner,
                            "Duplicate BNW domain unit identity");
                    unit=&candidate;
                }
        require(unit && unit->core.layout==bnw,
                "BNW domain unit core is unavailable");
        const auto type=core(*unit,"m_eUnitType");
        const auto rule=rules.units.find(type);
        require(rule!=rules.units.end(),
                "BNW unit type is unresolved in active rules");
        result.water=*cell.plot_type==3;
        result.effective_domain=rule->second.domain;
        if(*result.effective_domain==BnwUnitDomain::hover)
            result.effective_domain=result.water?
                BnwUnitDomain::sea:BnwUnitDomain::land;
        const auto hover=core(*unit,"m_iHoveringUnitCount");
        const auto all=core(*unit,"m_iCanMoveAllTerrainCount");
        const auto embarked=core(*unit,"m_bEmbarked",
            saves::UnitCoreEncoding::boolean);
        require(hover>=0 && all>=0 && (embarked==0 || embarked==1),
                "Invalid BNW unit hovering, all-terrain or embarked source flag");
        result.hovering=hover>0;
        result.all_terrain=all>0;
        result.embarked=embarked!=0;
        bool action=false;
        switch(*result.effective_domain) {
        case BnwUnitDomain::sea:
            action=result.water || result.all_terrain;
            break;
        case BnwUnitDomain::air:
            action=false;
            break;
        case BnwUnitDomain::land:
        case BnwUnitDomain::immobile:
            action=!result.water || result.hovering || result.all_terrain ||
                result.embarked;
            if(!action) {
                result.walk_water=walk_water(cell,plot,rules);
                action=result.walk_water;
            }
            break;
        case BnwUnitDomain::hover:
            throw std::runtime_error("BNW hover domain was not resolved");
        }
        if(action) {
            result.valid=true;
            return result;
        }
        result.city=city(world,cell,plot);
        result.valid=result.city;
    } catch(const std::exception &error) {
        result.valid.reset();
        result.reason=error.what();
    }
    return result;
}

BnwUnitValidityProbe bind_bnw_plot_valid_domain(BnwPlotDomainRules rules) {
    return [rules=std::move(rules)](const LiveSessionState &state,
                                   std::uint64_t plot_index,
                                   saves::SavedPlotUnitReference handle) {
        return evaluate_bnw_plot_valid_domain(
            state,plot_index,handle,rules).valid;
    };
}

} // namespace civ5::runtime
