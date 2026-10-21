#include "civ5/runtime/vp_city_construction_intrigue.hpp"
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
std::int64_t city_id(const saves::SavedCityObservation &saved) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:saved.core.fields)
        if(field.name&&std::string_view(field.name)=="m_iID") {
            if(found)throw std::runtime_error("Duplicate VP intrigue city ID");
            found=&field;
        }
    if(!found||found->values.size()!=1)
        throw std::runtime_error("Missing VP intrigue city ID");
    return found->values.front();
}
std::int64_t property(const content::RulesDatabase &database,const char *sql,
                      std::int64_t item,const char *kind) {
    std::optional<std::int64_t> result;
    const auto query=database.query_rows({sql,{item}},[&](const auto &row) {
        if(result)throw std::runtime_error(std::string("Ambiguous VP intrigue ")+kind);
        result=row.integer_at(0);
    },1);
    if(!query.all_succeeded()||!result)
        throw std::runtime_error(std::string("Unresolved VP intrigue ")+kind+": "+query.error);
    return *result;
}
}
VpCityIntrigueOrderProfileState &ensure_vp_city_intrigue_order_profile(CityYieldContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP city intrigue order requires VP GameCore profile");
    if(auto *profile=world.extensions.find<VpCityIntrigueOrderProfileState>())return *profile;
    return world.extensions.emplace<VpCityIntrigueOrderProfileState>();
}
const VpCityIntrigueOrderProfileState *find_vp_city_intrigue_order_profile(const CityYieldContext &world) {
    return world.profile==vp?world.extensions.find<VpCityIntrigueOrderProfileState>():nullptr;
}
void restore_vp_city_intrigue_order_profile(const saves::SavedSessionSnapshot &snapshot,
                                             CityYieldContext &world) {
    if(world.profile!=vp||!snapshot.all_players||
       snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP city intrigue order requires VP saved players");
    VpCityIntrigueOrderProfileState imported;
    for(const auto &saved:snapshot.all_players->players) {
        const auto owner=saved.saved_player_id;
        if(saved.cities.layout!=saves::GameStateLayout::vp542_corpus||
           !world.players.contains(owner))
            throw std::runtime_error("VP city intrigue owner is not in one VP roster");
        const auto &live=world.players.at(owner).cities;
        std::vector<std::int64_t> order;
        std::set<std::int64_t> unique;
        for(const auto &source:saved.cities.cities) {
            if(source.core.layout!=saves::GameStateLayout::vp542_corpus)
                throw std::runtime_error("VP intrigue city layout mismatch");
            const auto id=city_id(source);
            if(!unique.insert(id).second||!live.contains(id))
                throw std::runtime_error("Duplicate or missing VP intrigue city");
            const auto &city=live.at(id);
            if(city.owner!=owner||city.id!=id||
               city.source_core!=std::optional{std::pair{source.core.start_offset,source.core.end_offset}})
                throw std::runtime_error("VP intrigue city source binding mismatch");
            order.push_back(id);
        }
        if(order.size()!=live.size()||!imported.players.emplace(owner,std::move(order)).second)
            throw std::runtime_error("Incomplete VP intrigue city roster");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP intrigue player roster");
    ensure_vp_city_intrigue_order_profile(world)=std::move(imported);
}
VpConstructionIntrigueScan vp_first_city_construction_intrigue(
    const CityYieldContext &world,const content::RulesDatabase &database,std::int64_t owner) {
    const auto *profile=find_vp_city_intrigue_order_profile(world);
    if(!profile||!profile->players.contains(owner)||!world.players.contains(owner))
        throw std::runtime_error("Unresolved VP intrigue city order");
    const auto &live=world.players.at(owner).cities;
    const auto &order=profile->players.at(owner);
    if(order.size()!=live.size())throw std::runtime_error("Stale VP intrigue city order");
    std::set<std::int64_t> visited;
    for(const auto id:order) {
        if(!visited.insert(id).second||!live.contains(id)||
           live.at(id).owner!=owner||live.at(id).id!=id)
            throw std::runtime_error("Invalid VP intrigue city order");
    }
    VpConstructionIntrigueScan result;
    for(const auto id:order) {
        const auto &city=live.at(id);
        if(!city.production_orders)
            throw std::runtime_error("Unresolved VP intrigue city production queue");
        ++result.scanned_cities;
        if(city.production_orders->empty())continue;
        const auto &current=city.production_orders->front();
        bool eligible=false;
        switch(current.type) {
        case 0: // ORDER_TRAIN, spaceship unit only.
            eligible=property(database,
                "SELECT EXISTS(SELECT 1 FROM Projects WHERE Projects.Type=Units.SpaceshipProject) "
                "FROM Units WHERE Units.ID=? LIMIT 1",current.item,"unit")!=0;
            break;
        case 1: // ORDER_CONSTRUCT, world wonder class only.
            eligible=property(database,
                "SELECT BuildingClasses.MaxGlobalInstances FROM Buildings JOIN BuildingClasses "
                "ON BuildingClasses.Type=Buildings.BuildingClass WHERE Buildings.ID=? LIMIT 1",
                current.item,"building")!=-1;
            break;
        case 2: // ORDER_CREATE, any nonrepeatable project.
            eligible=property(database,"SELECT IsRepeatable FROM Projects WHERE ID=? LIMIT 1",
                current.item,"project")==0;
            break;
        case 3: // ORDER_MAINTAIN is not an intrigue source.
            break;
        default:
            throw std::runtime_error("Unrecognized VP intrigue production order");
        }
        if(eligible) {
            result.candidate=VpConstructionIntrigueCandidate{id,city.x,city.y,
                current.type,current.item};
            break;
        }
    }
    return result;
}
} // namespace civ5::runtime
