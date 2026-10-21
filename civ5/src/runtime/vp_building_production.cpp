#include "civ5/runtime/vp_building_production.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/vp_city_happiness_profile.hpp"
#include "civ5/runtime/vp_policy_branch_profile.hpp"
#include "civ5/runtime/vp_city_building_yield_profile.hpp"
#include "civ5/runtime/vp_corporations.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Building production ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate building production field");found=&x;}
    if(!found)throw std::runtime_error("Missing building production field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid building production scalar");return a[0];}
const VpBuildingProductionProfileState::Player &validate(const VpBuildingProductionContext &c,std::int64_t owner){const auto &s=c.production.supply;const auto &w=c.buildings.religion.water;
    const auto *profile=find_vp_building_production_profile(s.cities);
    if(s.cities.profile!=vp||s.players.profile!=vp||s.units.profile!=vp||&s.cities!=&c.buildings.cities||&w.cities!=&s.cities||&w.players!=&s.players||&w.defines!=&s.defines||!w.options||w.options!=&s.options||!w.options->all_succeeded()||!s.cities.players.contains(owner)||!profile||!profile->players.contains(owner))
        throw CityYieldUnavailable("Building production requires resolved shared VP world");
    return profile->players.at(owner);
}
void validate(const VpBuildingProductionContext &c,const CityYieldState &city){validate(c,city.owner);
    const auto *profile=find_vp_building_production_profile(c.buildings.cities);
    if(&c.buildings.cities.players.at(city.owner).cities.at(city.id)!=&city||!profile->cities.contains({city.owner,city.id}))throw CityYieldUnavailable("Building production requires a live resolved city");}
const VpBuildingProductionRules::Building &building(const VpBuildingProductionContext &c,std::int64_t id){const auto i=c.rules.buildings.find(id);
    if(i==c.rules.buildings.end())throw CityYieldUnavailable("Missing Building Info violates original production precondition");return i->second;}
const VpBuildingProductionRules::Class &building_class(const VpBuildingProductionContext &c,std::int64_t id){const auto i=c.rules.classes.find(id);
    if(i==c.rules.classes.end())throw CityYieldUnavailable("Missing BuildingClass Info");return i->second;}
bool limited(const VpBuildingProductionRules::Class &v){return v.world!=-1||v.team!=-1||v.national!=-1;}
std::int64_t era(const VpBuildingProductionContext &c,const VpBuildingProductionRules::Building &b){return b.technology==-1?-1:c.rules.technology_eras.at(b.technology);}
const saves::SavedMapCellProjection &cell(const VpBuildingProductionContext &c,std::int64_t id){const auto &m=c.buildings.religion.water.map;
    if(id<0||m.cells.size()!=m.grid.size())throw CityYieldUnavailable("Invalid building production map");const auto &p=m.cells.at(id);if(p.index!=static_cast<std::uint64_t>(id))throw CityYieldUnavailable("Building production plot binding mismatch");return p;}
}
VpBuildingProductionRules load_vp_building_production_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpBuildingProductionRules r;const auto type=[&](const auto &row,const char *name){const auto v=row.text(name);if(v)if(const auto *t=types.find(*v))return t->id;return std::int64_t{-1};};
    const auto load=[&](const char *table,const auto &f){auto q=db.prefetch_info(table,f);if(!q.all_succeeded())throw CityYieldUnavailable("Unresolved building production Info: "+q.error);return q.slots;};
    load("BuildingClasses",[&](auto id,auto,const auto &v){r.classes[id64(id)]={v.integer("MaxGlobalInstances"),v.integer("MaxTeamInstances"),v.integer("MaxPlayerInstances")};return true;});
    r.building_slots=load("Buildings",[&](auto id,auto,const auto &v){r.buildings[id64(id)]={type(v,"BuildingClass"),type(v,"PrereqTech")};return true;});
    load("Technologies",[&](auto id,auto,const auto &v){r.technology_eras[id64(id)]=type(v,"Era");return true;});
    r.resource_slots=load("Resources",[&](auto id,auto,const auto &v){r.resources[id64(id)]={v.integer("WonderProductionMod"),type(v,"WonderProductionModObsoleteEra")};return true;});
    load("Beliefs",[&](auto id,auto,const auto &v){r.beliefs[id64(id)]={v.integer("WonderProductionModifier"),type(v,"ObsoleteEra")};return true;});
    load("Improvements",[&](auto id,auto,const auto &v){r.improvements[id64(id)]=v.integer("WonderProductionModifier");return true;});
    r.branch_slots=load("PolicyBranchTypes",[&](auto id,auto,const auto &v){r.free_policies[id64(id)]=type(v,"FreePolicy");return true;});
    r.policy_slots=load("Policies",[&](auto id,auto,const auto &v){auto &p=r.policies[id64(id)];p.branch=type(v,"PolicyBranchType");
        if(p.branch==-1)for(const auto &[branch,free]:r.free_policies)if(free==id64(id)){p.branch=branch;break;}
        const auto t=v.text("Type");const auto q=db.query_rows({"select BuildingClasses.ID, ProductionModifier from Policy_BuildingClassProductionModifiers inner join BuildingClasses on BuildingClassType = BuildingClasses.Type where PolicyType = ?",{t?content::DatabaseValue(std::string(*t)):content::DatabaseValue{}}},[&](const auto &row){auto id=row.integer_at(0),n=row.integer_at(1);if(id<0)throw CityYieldUnavailable("Negative policy building class ID");if(n)p.classes[id]=n;else p.classes.erase(id);});
        if(!q.all_succeeded())throw CityYieldUnavailable("Unresolved building production policy relation: "+q.error);return true;});return r;
}
void restore_vp_building_production(const saves::SavedSessionSnapshot &s,const VpBuildingProductionRules &r,CityYieldContext &world,ResearchContext &players,MaintenanceContext &units){
    if(world.profile!=vp||players.profile!=vp||units.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw CityYieldUnavailable("Building production import requires VP state");
    auto &profile=ensure_vp_building_production_profile(world);
    profile.players.clear();profile.cities.clear();profile.wonder_units.clear();
    auto &branches=ensure_vp_policy_branch_profile(players);branches.players.clear();
    const auto vector=[&](const auto &fields,const char *name){const auto &v=named(fields,name).values;if(v.size()!=r.resource_slots)throw std::runtime_error("Local resource wire span mismatch");YieldValues a;for(std::size_t i=0;i<v.size();++i)if(v[i])a[id64(i)]=v[i];return a;};
    std::set<std::int64_t> seen;
    for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;auto &p=world.players.at(owner);auto &rp=players.players.at(owner);auto &up=units.players.at(owner);
        const auto *strategy=saves::vp_player_strategy_dimensions(sp.strategy.dimensions);
        if(!seen.insert(owner).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.strategy.layout!=layout||sp.cities.layout!=layout||sp.units.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner||!strategy||strategy->policies!=r.policy_slots||strategy->branches!=r.branch_slots)
            throw std::runtime_error("Building production player binding mismatch");
        VpBuildingProductionProfileState::Player a;
        const char *core[]={"m_iWonderProductionModifier","m_iMaxGlobalBuildingProductionModifier","m_iMaxTeamBuildingProductionModifier","m_iMaxPlayerBuildingProductionModifier","m_iConquestPerEraBuildingProductionMod","m_iPerPastEraBuildingProductionMod"};
        std::int64_t *dest[]={&a.wonder,&a.world,&a.team,&a.national,&a.conquest_era,&a.past_era};for(std::size_t k=0;k<6;++k)*dest[k]=scalar(sp.core.fields,core[k]);
        a.golden_age=scalar(sp.traits.fields,"m_iWonderProductionModGA");a.to_building=scalar(sp.traits.fields,"m_iWonderProductionModifierToBuilding");a.capital_discount=scalar(sp.traits.fields,"m_iCapitalBuildingModifier");
        const auto &policies=named(sp.strategy.blocks,"policies");const auto &mods=named(policies.arrays,"policy_modifiers").values;if(mods.size()!=60)throw std::runtime_error("Invalid VP policy modifier wire layout");a.policy_wonder=mods[8];a.policy_building=mods[9];profile.players[owner]=a;
        const auto &has=named(policies.arrays,"has_policy").values,&blocked=named(policies.arrays,"branch_blocked").values;
        if(has.size()!=r.policy_slots||blocked.size()!=r.branch_slots||!rp.policies_resolved)throw std::runtime_error("Unresolved policy branch ownership");
        std::set<std::int64_t> owned;for(std::size_t i=0;i<has.size();++i)if(has[i])owned.insert(id64(i));if(owned!=rp.policies)throw std::runtime_error("Shared policy ownership mismatch");
        auto &player_branches=branches.players[owner];for(std::size_t i=0;i<blocked.size();++i)if(blocked[i])player_branches.insert(id64(i));
        std::set<std::int64_t> cities;for(const auto &sc:sp.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||sc.production.layout!=layout||saves::vp_city_production_building_slots(sc.production.context)!=r.building_slots||city.owner!=owner||city.id!=id||scalar(sc.core.fields,"m_eOwner")!=owner||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("Building production city binding mismatch");
            VpBuildingProductionProfileState::City b;b.wonder=scalar(sc.core.fields,"m_iWonderProductionModifier");b.building=named(named(sc.production.blocks,"buildings").fields,"m_iBuildingProductionModifier").value;
            b.local_resources=vector(sc.core.fields,"m_paiNumResourcesLocal");b.unimproved_resources=vector(sc.core.fields,"m_paiNumUnimprovedResourcesLocal");profile.cities[{owner,id}]=std::move(b);
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete building production cities");
        std::set<std::int64_t> ids;for(const auto &su:sp.units.units){const auto id=scalar(su.core.fields,"m_iID");auto &u=up.units.at(id);
            if(!ids.insert(id).second||su.core.layout!=layout||scalar(su.core.fields,"m_eOwner")!=owner||u.source_core!=std::optional{std::pair{su.core.start_offset,su.core.end_offset}})throw std::runtime_error("Wonder production unit binding mismatch");
            profile.wonder_units[{owner,id}]=scalar(su.core.fields,"m_iWonderProductionModifier");
        }if(ids.size()!=up.units.size())throw std::runtime_error("Incomplete wonder production units");
    }if(seen.size()!=world.players.size()||seen.size()!=players.players.size()||seen.size()!=units.players.size())throw std::runtime_error("Incomplete building production players");
}
bool vp_production_policy_blocked(const VpBuildingProductionContext &c,std::int64_t owner,std::int64_t policy){
    validate(c,owner);const auto &world=c.production.supply.players;const auto &p=world.players.at(owner);
    const auto *branches=find_vp_policy_branch_profile(world);
    if(!p.policies_resolved||!branches||!branches->players.contains(owner))throw CityYieldUnavailable("Unresolved policy branch state");
    const auto b=c.rules.policies.at(policy).branch;if(b!=-1&&!c.rules.free_policies.contains(b))throw CityYieldUnavailable("Unresolved production policy branch Info");return b!=-1&&vp_blocked_policy_branches(world,owner).contains(b);
}
std::int64_t vp_building_class_policy_production(const VpBuildingProductionContext &c,std::int64_t owner,std::int64_t cls){
    validate(c,owner);const auto &p=c.production.supply.players.players.at(owner);if(!p.policies_resolved)throw CityYieldUnavailable("Unresolved production policy ownership");
    std::int64_t n=0;for(const auto &[id,rule]:c.rules.policies)if(p.policies.contains(id)&&!vp_production_policy_blocked(c,owner,id))n=add(n,value(rule.classes,cls));return n;
}
std::int64_t vp_capital_building_discount(const VpBuildingProductionContext &c,std::int64_t owner,std::int64_t id){const auto &p=validate(c,owner);if(p.capital_discount<=0)return 0;
    const auto &player=c.buildings.cities.players.at(owner);const auto city=player.cities.find(player.capital);return city!=player.cities.end()&&vp_city_building_count(c.buildings,city->second,id)>0?p.capital_discount:0;
}
std::int64_t vp_wonder_to_building_discount(const VpBuildingProductionContext &c,std::int64_t owner,std::int64_t id){const auto &p=validate(c,owner);const auto b=c.rules.buildings.find(id);
    return b!=c.rules.buildings.end()&&p.to_building>0&&!limited(building_class(c,b->second.building_class))?p.to_building:0;
}
VpWonderResourceQuote vp_wonder_resource_production(const VpBuildingProductionContext &c,const CityYieldState &city,std::int64_t era){validate(c,city);VpWonderResourceQuote q;const auto &world=c.buildings.cities;const auto &water=c.buildings.religion.water;const bool routes=water.options->enabled("TRADE_WONDER_RESOURCE_ROUTES");
    const auto &profile=*find_vp_building_production_profile(world);
    if(routes)for(const auto &r:world.trade_routes){if(r.origin_x==-1&&r.origin_y==-1&&r.destination_x==-1&&r.destination_y==-1)continue;if(r.connection_type!=3)continue;
        const auto *from=vp_city_at(world,water.map,r.origin_x,r.origin_y),*to=vp_city_at(world,water.map,r.destination_x,r.destination_y);
        if(!from||!to)throw CityYieldUnavailable("Wonder resource route requires live endpoint cities");
        if(to->x==city.x&&to->y==city.y){std::int64_t resource=-1;for(const auto &[id,v]:c.rules.resources)if(v.modifier){resource=id;break;}
            if(resource!=-1){const auto it=profile.cities.find({from->owner,from->id});if(it==profile.cities.end())throw CityYieldUnavailable("Unresolved wonder resource origin");q.incoming=value(it->second.unimproved_resources,resource)>0;}}
        else if(from->x==city.x&&from->y==city.y)q.outgoing=true;break;
    }
    for(const auto &[id,r]:c.rules.resources)if(r.modifier){const auto n=value(profile.cities.at({city.owner,city.id}).local_resources,id);const bool present=routes?(q.incoming||(n==1&&!q.outgoing)||n>1):n>0;
        if(present&&(r.obsolete==-1||era<r.obsolete))q.total=add(q.total,r.modifier);}
    return q;
}
std::int64_t vp_building_resource_production(const VpBuildingProductionContext &c,const CityYieldState &city,std::int64_t id){validate(c,city);const auto &b=building(c,id);
    return limited(building_class(c,b.building_class))?vp_wonder_resource_production(c,city,era(c,b)).total:0;
}
VpWonderReligionQuote vp_wonder_religion_production(const VpBuildingProductionContext &c,const CityYieldState &city,std::int64_t era){validate(c,city);VpWonderReligionQuote q;if(era==-1)return q;
    const auto &rel=c.buildings.religion;const auto &world=c.buildings.cities;auto selection=vp_plot_religion_selection(world,rel.rules,city);
    if(*vp_city_majority_religion(city)!=-1){if(!selection.majority)throw CityYieldUnavailable("Wonder production requires the majority religion");
        const auto &religion_profile=vp_religion_beliefs(*selection.majority);
        for(auto id:selection.majority->beliefs){const auto &b=c.rules.beliefs.at(id);if(b.modifier&&(b.obsolete==-1||era<b.obsolete)&&vp_plot_belief_valid(rel,id,religion_profile.belief_religion,city.owner,&city))q.majority=add(q.majority,b.modifier);}
        if(selection.secondary!=-1){const auto &b=c.rules.beliefs.at(selection.secondary);if(era<b.obsolete)q.secondary=b.modifier;}
    }else selection.secondary=-1; // Original only resolves the secondary inside the majority branch.
    const auto permanent=vp_permanent_pantheon(rel.water,city,selection);if(permanent.belief!=-1){const auto &b=c.rules.beliefs.at(permanent.belief);if(era<b.obsolete)q.pantheon=b.modifier;}
    q.total=add(add(q.majority,q.secondary),q.pantheon);return q;
}
std::int64_t vp_wonder_unit_production(const VpBuildingProductionContext &c,const CityYieldState &city){validate(c,city);const auto &map=c.buildings.religion.water.map;
    const auto &profile=*find_vp_building_production_profile(c.buildings.cities);
    MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,layout};const auto id=geometry.plot(city.x,city.y);if(!id)throw CityYieldUnavailable("Invalid wonder city plot");const auto &p=cell(c,*id);
    if(!p.units)throw CityYieldUnavailable("Unresolved wonder city plot units");std::int64_t n=0;for(const auto &ref:*p.units){
        if(!c.production.supply.units.players.at(ref.owner).units.contains(ref.id))throw CityYieldUnavailable("Unresolved unit wonder modifier");
        const auto it=profile.wonder_units.find({ref.owner,ref.id});if(it==profile.wonder_units.end())throw CityYieldUnavailable("Unresolved unit wonder modifier");
        n=add(n,it->second);}return n;
}
std::int64_t vp_wonder_improvement_production(const VpBuildingProductionContext &c,const CityYieldState &city){validate(c,city);const auto &building=vp_city_building_yield_city(c.production.supply.cities,city.owner,city.id);std::int64_t n=0;
    for(auto id:building.plots){const auto &p=cell(c,id);if(!p.improvement_type)throw CityYieldUnavailable("Unresolved wonder improvement");if(*p.improvement_type==-1)continue;
        if(!p.improvement_pillaged)throw CityYieldUnavailable("Unresolved wonder improvement pillage");if(!*p.improvement_pillaged)n=add(n,c.rules.improvements.at(*p.improvement_type));}return n;
}
VpPlayerBuildingProductionQuote vp_player_building_production(const VpBuildingProductionContext &c,std::int64_t owner,std::int64_t id){const auto &p=validate(c,owner);const auto &b=building(c,id);const auto &cls=building_class(c,b.building_class);VpPlayerBuildingProductionQuote q;
    q.general=vp_player_general_production(c.production,owner);const auto wonder=[&]{return add(p.wonder,c.buildings.cities.players.at(owner).golden_age?p.golden_age:0);};
    if(limited(cls)){q.policy=p.policy_wonder;q.wonder=wonder();q.limited=cls.world!=-1?p.world:cls.team!=-1?p.team:p.national;}
    else{q.policy=p.policy_building;if(p.to_building>0){const auto mod=vp_wonder_to_building_discount(c,owner,id);q.converted_wonder=wide::multiply_divide(wonder(),mod,100);q.converted_policy=wide::multiply_divide(p.policy_wonder,mod,100);}}
    q.building_class=vp_building_class_policy_production(c,owner,b.building_class);q.capital=vp_capital_building_discount(c,owner,id);
    for(auto n:{q.general.total,q.policy,q.wonder,q.limited,q.converted_wonder,q.converted_policy,q.building_class,q.capital})q.total=add(q.total,n);return q;
}
VpCityBuildingProductionQuote vp_city_building_production(const VpBuildingProductionContext &c,const CityYieldState &city,std::int64_t id){validate(c,city);const auto &b=building(c,id);const auto &cls=building_class(c,b.building_class);
    const auto &profile=*find_vp_building_production_profile(c.buildings.cities);
    const auto &p=profile.players.at(city.owner);const auto &state=profile.cities.at({city.owner,city.id});VpCityBuildingProductionQuote q;
    q.general=vp_city_general_production(c.production,city);q.player=vp_player_building_production(c,city.owner,id);q.era=era(c,b);
    if(limited(cls)||p.to_building>0){q.conversion=limited(cls)?100:vp_wonder_to_building_discount(c,city.owner,id);
        const auto converted=[&](auto n){return wide::multiply_divide(n,q.conversion,100);};q.wonder=converted(state.wonder);q.resources=converted(vp_wonder_resource_production(c,city,q.era).total);
        q.religion=converted(vp_wonder_religion_production(c,city,q.era).total);q.units=converted(vp_wonder_unit_production(c,city));q.improvements=converted(vp_wonder_improvement_production(c,city));}
    if(!limited(cls))q.building=state.building;
    if(q.era!=-1){const auto &happiness=vp_city_happiness_city(c.production.supply.cities,city.owner,city.id);const auto delta=wide::subtract(c.production.supply.units.team_eras.at(c.production.supply.players.players.at(city.owner).team),q.era);
        if(city.puppet||happiness.no_occupied_unhappiness>0){q.past_eras=p.conquest_era;if(delta>0)q.past_eras=add(q.past_eras,wide::multiply_divide(p.conquest_era,delta,1));}
        if(delta>0)q.past_eras=add(q.past_eras,wide::multiply_divide(p.past_era,delta,1));}
    for(auto n:{q.general.total,q.player.total,q.wonder,q.resources,q.religion,q.units,q.improvements,q.building,q.past_eras})q.total=add(q.total,n);return q;
}
} // namespace civ5::runtime
