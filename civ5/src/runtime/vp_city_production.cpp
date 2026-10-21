#include "civ5/runtime/vp_city_production.hpp"
#include "civ5/runtime/vp_city_trait_yield_profile.hpp"
#include "civ5/runtime/vp_city_happiness_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Production ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate production field");found=&x;}
    if(!found)throw std::runtime_error("Missing production field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid production scalar");return a[0];}
std::int64_t define(const content::GlobalDefineCache &d,const char *name){const auto *v=d.find(name);
    if(!v||!v->lookup.error.empty())throw CityYieldUnavailable("Unresolved production define: "+std::string(name));return std::get<std::int64_t>(v->lookup.value);}
const VpGeneralProductionProfileState::Player &validate(const VpCityProductionContext &c,std::int64_t owner){const auto &s=c.supply;
    const auto *profile=find_vp_general_production_profile(s.cities);
    if(s.cities.profile!=vp||s.players.profile!=vp||s.units.profile!=vp||s.diplomacy.profile!=vp||s.defines.profile()!=content::GlobalDefineProfile::vp542||!s.options.all_succeeded()||
       &c.food.cities!=&s.cities||&c.food.players!=&s.players||&c.food.defines!=&s.defines||!s.cities.players.contains(owner)||
       !profile||!profile->players.contains(owner))
        throw CityYieldUnavailable("Production requires resolved shared VP world");
    return profile->players.at(owner);
}
const VpGeneralProductionProfileState::Player &validate(const VpCityProductionContext &c,const CityYieldState &city){
    const auto &p=validate(c,city.owner);
    const auto *profile=find_vp_general_production_profile(c.supply.cities);
    if(&c.supply.cities.players.at(city.owner).cities.at(city.id)!=&city||!profile->cities.contains({city.owner,city.id}))
        throw CityYieldUnavailable("Production requires a live resolved city");
    (void)vp_city_trait_yield_city(c.supply.cities,city.owner,city.id);
    (void)vp_city_happiness_city(c.supply.cities,city.owner,city.id);
    return p;
}
bool spaceship(const VpCityProductionContext &c,std::int64_t id){const auto i=c.rules.projects.find(id);
    if(i==c.rules.projects.end())throw CityYieldUnavailable("Missing project Info violates original production precondition");return i->second;}
}
VpCityProductionRules load_vp_city_production_rules(const content::RulesDatabase &db){
    VpCityProductionRules r;const auto projects=db.prefetch_info("Projects",[&](auto id,auto,const auto &row){r.projects[id64(id)]=row.boolean("Spaceship");return true;});
    const auto processes=db.prefetch_info("Processes",[&](auto id,auto,const auto &){r.processes.insert(id64(id));return true;});
    if(!projects.all_succeeded()||!processes.all_succeeded())throw CityYieldUnavailable("Unresolved production Info");return r;
}
void restore_vp_city_production(const saves::SavedSessionSnapshot &s,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw CityYieldUnavailable("Production import requires VP world");
    auto &profile=ensure_vp_general_production_profile(world);profile.players.clear();profile.cities.clear();
    std::set<std::int64_t> players;for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;auto &p=world.players.at(owner);
        if(!players.insert(owner).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner)
            throw std::runtime_error("Production player binding mismatch");
        VpGeneralProductionProfileState::Player a;a.puppet=scalar(sp.core.fields,"m_iPuppetProdMod");a.occupied=scalar(sp.core.fields,"m_iOccupiedProdMod");
        a.conquest_turns=scalar(sp.core.fields,"m_iProductionBonusTurnsConquest");a.space=scalar(sp.core.fields,"m_iSpaceProductionModifier");
        a.conquest=scalar(sp.traits.fields,"m_iProductionBonusModifierConquest");a.from_specialists=scalar(sp.traits.fields,"m_bProdModFromNumSpecialists")!=0;profile.players[owner]=a;
        std::set<std::int64_t> cities;for(const auto &sc:sp.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||city.owner!=owner||city.id!=id||scalar(sc.core.fields,"m_eOwner")!=owner||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})
                throw std::runtime_error("Production city binding mismatch");
            profile.cities[{owner,id}]={scalar(sc.core.fields,"m_iSpaceProductionModifier")};
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete production cities");
    }if(players.size()!=world.players.size())throw std::runtime_error("Incomplete production players");
}
VpProductionMaintenance vp_unit_production_maintenance(VpUnitSupplyContext &c,std::int64_t owner){
    VpProductionMaintenance q;q.over=vp_unit_supply(c,owner).over;if(q.over<=0)return q;
    const auto cap=std::max<std::int64_t>(0,define(c.defines,"MAX_UNIT_SUPPLY_PRODMOD"));const auto rate=std::max<std::int64_t>(0,define(c.defines,"PRODUCTION_PENALTY_PER_UNIT_OVER_SUPPLY"));
    if(rate)q.modifier=-(q.over>cap/rate?cap:wide::multiply_divide(q.over,rate,1));return q;
}
VpPlayerProductionQuote vp_player_general_production(const VpCityProductionContext &c,std::int64_t owner){
    const auto &p=validate(c,owner);const auto supply=vp_unit_production_maintenance(c.supply,owner);VpPlayerProductionQuote q;q.supply_over=supply.over;q.supply=supply.modifier;
    if(p.conquest_turns>0)q.conquest=p.conquest;q.total=add(q.supply,q.conquest);return q;
}
VpCityGeneralProductionQuote vp_city_general_production(const VpCityProductionContext &c,const CityYieldState &city){
    const auto &p=validate(c,city);VpCityGeneralProductionQuote q;
    if(vp_city_trait_yield_city(c.supply.cities,city.owner,city.id).industrial)q.industrial=define(c.supply.defines,"INDUSTRIAL_ROUTE_PRODUCTION_MOD");
    if(p.puppet>0&&city.puppet)q.puppet=p.puppet;
    const auto &happiness=vp_city_happiness_city(c.supply.cities,city.owner,city.id);
    if(p.occupied>0&&!city.puppet&&happiness.occupied&&happiness.no_occupied_unhappiness<=0)q.occupied=p.occupied;
    if(p.from_specialists)q.specialists=vp_total_specialists(c.food,city);
    q.total=add(add(add(q.industrial,q.puppet),q.occupied),q.specialists);return q;
}
VpPlayerProductionQuote vp_player_project_production(const VpCityProductionContext &c,std::int64_t owner,std::int64_t project){
    auto q=vp_player_general_production(c,owner);if(spaceship(c,project))q.space=find_vp_general_production_profile(c.supply.cities)->players.at(owner).space;
    q.total=add(q.total,q.space);return q;
}
VpPlayerProductionQuote vp_player_process_production(const VpCityProductionContext &c,std::int64_t owner,std::int64_t /*process*/){
    return vp_player_general_production(c,owner);
}
VpCityProductionQuote vp_city_project_production(const VpCityProductionContext &c,const CityYieldState &city,std::int64_t project){
    VpCityProductionQuote q;q.city=vp_city_general_production(c,city);q.player=vp_player_project_production(c,city.owner,project);
    if(spaceship(c,project))q.space=find_vp_general_production_profile(c.supply.cities)->cities.at({city.owner,city.id}).space;
    q.total=add(add(q.city.total,q.player.total),q.space);return q;
}
VpCityProductionQuote vp_city_process_production(const VpCityProductionContext &c,const CityYieldState &city,std::int64_t process){
    VpCityProductionQuote q;q.city=vp_city_general_production(c,city);q.player=vp_player_process_production(c,city.owner,process);
    const auto &p=find_vp_general_production_profile(c.supply.cities)->players.at(city.owner);if(p.conquest_turns>0)q.second_conquest=p.conquest;
    q.total=add(add(q.city.total,q.player.total),q.second_conquest);return q;
}
} // namespace civ5::runtime
