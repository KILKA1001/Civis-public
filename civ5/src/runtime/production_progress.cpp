#include "civ5/runtime/production_progress.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
using Profile=content::TechnologyProfile;
using Layout=saves::GameStateLayout;
using Progress=CityYieldState::ProductionProgress;
using Order=CityYieldState::ProductionOrder;
std::int64_t id64(std::size_t value){
    if(value>static_cast<std::size_t>(INT64_MAX))throw std::overflow_error("Production Info ID exceeds int64");
    return static_cast<std::int64_t>(value);
}
template<class T> const T &named(const std::vector<T> &rows,std::string_view name){
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name){
        if(found)throw std::runtime_error("Duplicate production progress field: "+std::string(name));found=&row;}
    if(!found)throw std::runtime_error("Missing production progress field: "+std::string(name));
    return *found;
}
std::int64_t scalar(const std::vector<saves::CityCoreField> &fields,const char *name){
    const auto &values=named(fields,name).values;
    if(values.size()!=1)throw std::runtime_error("Invalid production progress scalar");
    return values.front();
}
void indexed(const std::vector<std::int64_t> &source,std::uint64_t slots,
    const std::set<std::int64_t> &active,YieldValues &target,Progress &progress){
    if(source.size()!=slots)throw std::runtime_error("Production progress Info span mismatch");
    for(std::size_t i=0;i<source.size();++i)if(source[i]){
        const auto id=id64(i);target[id]=source[i];
        if(!active.contains(id))progress.fully_resolved=false;
    }
}
void hashed(const saves::CityProductionRecord &source,const char *name,const char *table,
    const std::set<std::int64_t> &active,const content::InfoTypeCatalog &types,
    YieldValues &target,Progress::HashedRows &unresolved,Progress &progress){
    const auto &records=named(source.children,name).children;
    for(const auto &row:records){
        const auto hash=named(row.fields,"hash").value;
        if(hash<0||hash>UINT32_MAX)throw std::runtime_error("Invalid production progress hash");
        if(!hash)continue; // Source null hash has no value.
        const auto &values=named(row.arrays,"values").values;
        if(values.size()!=1)throw std::runtime_error("Invalid hashed production progress");
        const auto *entry=types.find_hash(static_cast<std::uint32_t>(hash));
        if(entry&&entry->table==table&&active.contains(entry->id))target[entry->id]=values.front();
        else{unresolved.emplace_back(static_cast<std::uint32_t>(hash),values.front());progress.fully_resolved=false;}
    }
}
const std::set<std::int64_t> *order_ids(const ProductionProgressRules &rules,std::int64_t type){
    switch(type){
    case 0:return &rules.units;
    case 1:return &rules.buildings;
    case 2:return &rules.projects;
    case 3:return rules.profile==Profile::vp542?&rules.processes:&rules.specialists;
    case 4:return rules.profile==Profile::bnw?&rules.processes:nullptr;
    default:return nullptr;
    }
}
const char *order_table(const ProductionProgressRules &rules,std::int64_t type){
    switch(type){
    case 0:return "Units";
    case 1:return "Buildings";
    case 2:return "Projects";
    case 3:return rules.profile==Profile::vp542?"Processes":"Specialists";
    case 4:return rules.profile==Profile::bnw?"Processes":nullptr;
    default:return nullptr;
    }
}
std::vector<Order> queue(const saves::CityProductionObservation &source,
    const ProductionProgressRules &rules,const content::InfoTypeCatalog &types,Progress &progress){
    std::vector<Order> result;
    for(const auto &saved:named(source.blocks,"queue").children){
        const auto field=[&](const char *name){return named(saved.fields,name).value;};
        Order order;order.type=field("eOrderType");order.secondary=field("iData2");
        order.save=field("bSave")!=0;order.rush=field("bRush")!=0;
        const auto *ids=order_ids(rules,order.type);
        const auto *table=order_table(rules,order.type);
        const bool hashed_id=rules.profile==Profile::bnw&&source.context.bnw_city_version&&
            *source.context.bnw_city_version>=5&&order.type>=0&&order.type<=4;
        if(hashed_id){
            const auto raw=field("iData1_hash");
            if(raw<0||raw>UINT32_MAX)throw std::runtime_error("Invalid production order hash");
            order.source_hash=static_cast<std::uint32_t>(raw);
            const auto *entry=types.find_hash(*order.source_hash);
            if(entry&&table&&entry->table==table&&ids&&ids->contains(entry->id))order.item=entry->id;
            else progress.fully_resolved=false;
        }else{
            order.item=field("iData1");
            if(!ids||!ids->contains(order.item))progress.fully_resolved=false;
        }
        result.push_back(order);
    }
    return result;
}
std::int64_t value(const YieldValues &values,std::int64_t id){
    const auto it=values.find(id);return it==values.end()?0:it->second;
}
} // namespace
ProductionProgressRules load_production_progress_rules(const content::RulesDatabase &db,Profile profile){
    if(profile!=Profile::bnw&&profile!=Profile::vp542)throw std::runtime_error("Unsupported production progress profile");
    ProductionProgressRules rules;rules.profile=profile;
    const auto load=[&](const char *table,std::set<std::int64_t> &ids){
        const auto result=db.prefetch_info(table,[&](auto id,auto,const auto &row){
            const auto wide=id64(id);ids.insert(wide);
            const auto type=row.text("Type");
            rules.info_identity[table][wide]=type?std::optional{std::string(*type)}:std::nullopt;
            return true;});
        if(!result.all_succeeded())throw std::runtime_error(std::string("Unresolved production Info: ")+table);
        return result.slots;
    };
    rules.unit_slots=load("Units",rules.units);
    rules.building_slots=load("Buildings",rules.buildings);
    rules.project_slots=load("Projects",rules.projects);
    load("Processes",rules.processes);load("Specialists",rules.specialists);
    return rules;
}
void restore_production_progress(const saves::SavedSessionSnapshot &snapshot,
    const ProductionProgressRules &rules,const content::InfoTypeCatalog &types,CityYieldContext &world){
    const auto layout=rules.profile==Profile::vp542?Layout::vp542_corpus:Layout::expansion2_version1;
    if(world.profile!=rules.profile||!snapshot.all_players||snapshot.all_players->layout!=layout||!types.report().all_succeeded())
        throw CityYieldUnavailable("Production progress requires matching saved GameCore state");
    std::set<std::int64_t> seen_players;
    for(const auto &player:snapshot.all_players->players){
        const auto owner=player.saved_player_id;
        if(!seen_players.insert(owner).second||player.cities.layout!=layout)throw std::runtime_error("Duplicate production progress player");
        auto &live_player=world.players.at(owner);
        std::set<std::int64_t> seen_cities;
        for(const auto &saved:player.cities.cities){
            const auto id=scalar(saved.core.fields,"m_iID");
            if(!seen_cities.insert(id).second||saved.core.layout!=layout||saved.production.layout!=layout||
               scalar(saved.core.fields,"m_eOwner")!=owner)throw std::runtime_error("Production progress city binding mismatch");
            auto &city=live_player.cities.at(id);
            if(city.id!=id||city.owner!=owner||city.source_core!=std::optional{std::pair{saved.core.start_offset,saved.core.end_offset}})
                throw std::runtime_error("Production progress live city mismatch");
            Progress progress;progress.source_profile=rules.profile;
            progress.overflow_times100=scalar(saved.core.fields,"m_iOverflowProduction");
            progress.feature_times100=scalar(saved.core.fields,"m_iFeatureProduction");
            indexed(named(saved.core.fields,"m_paiProjectProduction").values,rules.project_slots,rules.projects,progress.projects,progress);
            const auto &buildings=named(saved.production.blocks,"buildings");
            if(rules.profile==Profile::vp542){
                indexed(named(saved.core.fields,"m_paiUnitProduction").values,rules.unit_slots,rules.units,progress.units,progress);
                indexed(named(saved.core.fields,"m_paiUnitProductionTime").values,rules.unit_slots,rules.units,progress.unit_times,progress);
                indexed(named(buildings.arrays,"m_paiBuildingProduction").values,rules.building_slots,rules.buildings,progress.buildings,progress);
                indexed(named(buildings.arrays,"m_paiBuildingProductionTime").values,rules.building_slots,rules.buildings,progress.building_times,progress);
            }else{
                const auto &before=named(saved.production.blocks,"before_queue");
                hashed(before,"m_paiUnitProduction","Units",rules.units,types,progress.units,progress.unresolved_units,progress);
                hashed(before,"m_paiUnitProductionTime","Units",rules.units,types,progress.unit_times,progress.unresolved_unit_times,progress);
                hashed(buildings,"m_paiBuildingProduction","Buildings",rules.buildings,types,progress.buildings,progress.unresolved_buildings,progress);
                hashed(buildings,"m_paiBuildingProductionTime","Buildings",rules.buildings,types,progress.building_times,progress.unresolved_building_times,progress);
            }
            auto orders=queue(saved.production,rules,types,progress);
            if(city.production_orders&&*city.production_orders!=orders)throw std::runtime_error("Production queue disagrees with live city");
            city.production_orders=std::move(orders);city.production_progress=std::move(progress);
        }
        if(seen_cities.size()!=live_player.cities.size())throw std::runtime_error("Incomplete production progress cities");
    }
    if(seen_players.size()!=world.players.size())throw std::runtime_error("Incomplete production progress players");
}
std::int64_t current_production_progress_times100(const CityYieldState &city,const ProductionProgressRules &rules){
    if(!city.production_progress||!city.production_orders)throw CityYieldUnavailable("Unresolved current city production");
    const auto &progress=*city.production_progress;
    if(progress.source_profile!=rules.profile)throw CityYieldUnavailable("Mismatched current production profile");
    if(city.production_orders->empty())return 0;
    const auto &order=city.production_orders->front();
    const auto *ids=order_ids(rules,order.type);
    if(!ids||!ids->contains(order.item))throw CityYieldUnavailable("Unresolved current production Info");
    switch(order.type){
    case 0:return value(progress.units,order.item);
    case 1:return value(progress.buildings,order.item);
    case 2:return value(progress.projects,order.item);
    default:return 0; // Process/prepare do not have a per-item production cache.
    }
}
} // namespace civ5::runtime
