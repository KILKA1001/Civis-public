#include "civ5/runtime/vp_city_unit_production.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/vp_corporation_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City unit production ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t value(const YieldValues &a,std::int64_t id){const auto i=a.find(id);return i==a.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate city unit production field");found=&x;}
    if(!found)throw std::runtime_error("Missing city unit production field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid city unit production scalar");return a[0];}
YieldValues sparse(const std::vector<std::int64_t> &v,std::uint64_t slots,const char *name){
    if(v.size()!=slots)throw std::runtime_error(std::string(name)+" wire span mismatch");YieldValues a;
    for(std::size_t i=0;i<v.size();++i)if(v[i])a[id64(i)]=v[i];return a;
}
std::int64_t integer(const content::GlobalDefineCache &d,const char *name){const auto *v=d.find(name);
    if(!v||!v->lookup.error.empty()||!std::holds_alternative<std::int64_t>(v->lookup.value))throw CityYieldUnavailable("Unresolved city unit production define: "+std::string(name));
    return std::get<std::int64_t>(v->lookup.value);
}
std::int64_t global_settler_penalty(const content::GlobalDefineCache &d,std::int64_t happiness){
    const auto *v=d.find("GLOBAL_SETTLER_PRODUCTION_PENALTY_PER_UNHAPPY");
    if(!v||!v->lookup.error.empty()||!std::holds_alternative<double>(v->lookup.value))throw CityYieldUnavailable("Unresolved global settler production rule");
    const float rate=static_cast<float>(std::get<double>(v->lookup.value));if(!std::isfinite(rate))throw CityYieldUnavailable("Nonfinite settler production rule");
    const auto floor=integer(d,"UNHAPPY_MAX_UNIT_PRODUCTION_PENALTY");
    const float product=rate*static_cast<float>(wide::subtract(happiness,integer(d,"UNHAPPY_THRESHOLD")));
    return product<=static_cast<float>(floor)?floor:product>=0.0f?0:static_cast<std::int64_t>(product);
}
const VpCityUnitProductionRules::Unit *find_unit(const VpCityUnitProductionRules &r,std::int64_t id){const auto i=r.units.find(id);return i==r.units.end()?nullptr:&i->second;}
void validate(const VpCityUnitProductionContext &c,const CityYieldState &city){
    const auto &world=c.player.production.supply.cities;
    const auto *profile=find_vp_unit_production_profile(world);
    if(world.profile!=vp||&c.buildings.cities!=&world||&c.growth.religion.water.cities!=&world||
       &c.buildings.religion.water.map!=&c.growth.religion.water.map||
       !profile||!profile->cities.contains({city.owner,city.id})||!profile->city_players.contains(city.owner)||
       !city.production_orders||
       &world.players.at(city.owner).cities.at(city.id)!=&city)throw CityYieldUnavailable("City unit production requires a resolved shared VP city");
    (void)vp_city_growth_city(world,city.owner,city.id);
}
}
VpCityUnitProductionRules load_vp_city_unit_production_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpCityUnitProductionRules r;const auto type=[&](const auto &v,const char *field){const auto s=v.text(field);if(s)if(const auto *x=types.find(*s))return x->id;return std::int64_t{-1};};
    const auto load=[&](const char *table,const auto &fn){const auto q=db.prefetch_info(table,fn);if(!q.all_succeeded())throw CityYieldUnavailable(std::string("Unresolved city unit production Info: ")+table+": "+q.error);return q.slots;};
    r.class_slots=load("UnitClasses",[](auto,auto,const auto &){return true;});
    r.combat_slots=load("UnitCombatInfos",[](auto,auto,const auto &){return true;});
    r.resource_slots=load("Resources",[](auto,auto,const auto &){return true;});
    r.building_slots=load("Buildings",[&](auto id,auto,const auto &){r.buildings.insert(id64(id));return true;});
    load("Beliefs",[&](auto id,auto,const auto &v){r.beliefs[id64(id)].unit=v.integer("UnitProductionModifier");return true;});
    load("Corporations",[&](auto id,auto,const auto &){r.corporations[id64(id)];return true;});
    load("Units",[&](auto id,auto,const auto &v){auto &u=r.units[id64(id)];
        u.unit_class=type(v,"Class");u.domain=type(v,"Domain");u.combat=type(v,"CombatClass");u.spaceship=type(v,"SpaceshipProject");
        u.strength=v.integer("Combat");u.ranged=v.integer("RangedCombat");u.nuke=v.integer("NukeDamageLevel");
        u.military=v.boolean("MilitaryProduction");u.found=v.boolean("Found");u.found_mid=v.boolean("FoundMid");u.found_late=v.boolean("FoundLate");u.found_abroad=v.boolean("FoundAbroad");return true;});
    const auto relation=[&](const char *sql,auto fn){const auto q=db.query_rows({sql,{}},fn);if(!q.all_succeeded())throw CityYieldUnavailable(std::string("Unresolved city unit production relation: ")+q.error);};
    relation("select Units.ID,Resources.ID,Unit_ResourceQuantityRequirements.Cost from Unit_ResourceQuantityRequirements inner join Units on Units.Type=Unit_ResourceQuantityRequirements.UnitType inner join Resources on Resources.Type=Unit_ResourceQuantityRequirements.ResourceType",[&](const auto &v){r.units.at(v.integer_at(0)).required_resources[v.integer_at(1)]=v.integer_at(2);});
    relation("select Units.ID,Resources.ID,Unit_ResourceQuantityTotals.Amount from Unit_ResourceQuantityTotals inner join Units on Units.Type=Unit_ResourceQuantityTotals.UnitType inner join Resources on Resources.Type=Unit_ResourceQuantityTotals.ResourceType",[&](const auto &v){r.units.at(v.integer_at(0)).total_resources[v.integer_at(1)]=v.integer_at(2);});
    relation("select Units.ID,Buildings.ID,Unit_ProductionModifierBuildings.ProductionModifier from Unit_ProductionModifierBuildings inner join Units on Units.Type=Unit_ProductionModifierBuildings.UnitType inner join Buildings on Buildings.Type=Unit_ProductionModifierBuildings.BuildingType",[&](const auto &v){r.units.at(v.integer_at(0)).buildings[v.integer_at(1)]=v.integer_at(2);});
    relation("select Corporations.ID,Resources.ID,Corporation_UnitResourceProductionModifier.Modifier from Corporation_UnitResourceProductionModifier inner join Corporations on Corporations.Type=Corporation_UnitResourceProductionModifier.CorporationType inner join Resources on Resources.Type=Corporation_UnitResourceProductionModifier.ResourceType",[&](const auto &v){r.corporations.at(v.integer_at(0))[v.integer_at(1)]=v.integer_at(2);});
    relation("select Beliefs.ID,UnitCombatInfos.ID,Belief_UnitCombatProductionModifiers.Modifier from Belief_UnitCombatProductionModifiers inner join Beliefs on Beliefs.Type=Belief_UnitCombatProductionModifiers.BeliefType inner join UnitCombatInfos on UnitCombatInfos.Type=Belief_UnitCombatProductionModifiers.UnitCombatType",[&](const auto &v){r.beliefs.at(v.integer_at(0)).combats[v.integer_at(1)]=v.integer_at(2);});
    return r;
}
void restore_vp_city_unit_production(const saves::SavedSessionSnapshot &s,const VpCityUnitProductionRules &r,CityYieldContext &world,MaintenanceContext &units){
    if(world.profile!=vp||units.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw CityYieldUnavailable("City unit production import requires VP state");
    auto &profile=ensure_vp_unit_production_profile(world);
    profile.city_players.clear();profile.cities.clear();profile.military_units.clear();
    std::set<std::int64_t> seen;
    for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;auto &p=world.players.at(owner);auto &up=units.players.at(owner);
        if(!seen.insert(owner).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.planning.layout!=layout||sp.cities.layout!=layout||sp.units.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner)
            throw std::runtime_error("City unit production player binding mismatch");
        VpUnitProductionProfileState::CityPlayer a;a.capital_settler=scalar(sp.core.fields,"m_iCapitalSettlerProductionModifier");
        a.classes=sparse(named(sp.core.fields,"m_paiUnitClassProductionModifiers").values,r.class_slots,"Unit class production");
        const auto &pledges=named(sp.traits.fields,"m_aiNumPledgesDomainProdMod").values;
        for(std::size_t i=0;i<pledges.size();++i)if(pledges[i])a.pledge_domains[id64(i)]=pledges[i];
        const auto &flags=named(named(sp.planning.blocks,"minor_civ_ai").arrays,"m_abPledgeToProtect");
        if(flags.columns!=1||flags.values.size()!=sp.planning.dimensions.major)throw std::runtime_error("Minor protection wire span mismatch");
        for(std::size_t i=0;i<flags.values.size();++i)if(flags.values[i])a.protected_by.insert(id64(i));profile.city_players[owner]=std::move(a);
        std::set<std::int64_t> cities;for(const auto &sc:sp.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||city.id!=id||city.owner!=owner||scalar(sc.core.fields,"m_eOwner")!=owner||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})
                throw std::runtime_error("City unit production city binding mismatch");
            VpUnitProductionProfileState::City b;b.military=scalar(sc.core.fields,"m_iMilitaryProductionModifier");
            const auto &domains=named(sc.core.fields,"m_aiDomainProductionModifier").values;
            for(std::size_t i=0;i<domains.size();++i)if(domains[i])b.domains[id64(i)]=domains[i];
            b.combats=sparse(named(sc.core.fields,"m_paiUnitCombatProductionModifier").values,r.combat_slots,"City combat production");
            profile.cities[{owner,id}]=std::move(b);
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete city unit production cities");
        std::set<std::int64_t> ids;for(const auto &su:sp.units.units){const auto id=scalar(su.core.fields,"m_iID");auto &u=up.units.at(id);
            if(!ids.insert(id).second||su.core.layout!=layout||scalar(su.core.fields,"m_eOwner")!=owner||u.source_core!=std::optional{std::pair{su.core.start_offset,su.core.end_offset}})
                throw std::runtime_error("Military production unit binding mismatch");
            profile.military_units[{owner,id}]=scalar(su.core.fields,"m_iUnitProductionModifier");
        }if(ids.size()!=up.units.size())throw std::runtime_error("Incomplete military production units");
    }if(seen.size()!=world.players.size()||seen.size()!=units.players.size())throw std::runtime_error("Incomplete city unit production players");
}
VpCityUnitProductionQuote vp_city_unit_production(const VpCityUnitProductionContext &c,const CityYieldState &city,std::int64_t id,bool ignore_happiness){
    VpCityUnitProductionQuote q;const auto *u=find_unit(c.rules,id);if(!u)return q;validate(c,city);
    const auto &supply=c.player.production.supply;const auto &world=supply.cities;const auto &p=world.players.at(city.owner);
    const auto &profile=*find_vp_unit_production_profile(world);
    const auto &ps=profile.city_players.at(city.owner);const auto &cs=profile.cities.at({city.owner,city.id});
    q.city=vp_city_general_production(c.player.production,city);q.player=vp_player_unit_production(c.player,city.owner,id);
    if(city.id==p.capital&&u->found)q.capital_settler=ps.capital_settler;
    if(u->unit_class!=-1)q.unit_class=value(ps.classes,u->unit_class);
    if(u->domain!=-1){q.domain=value(cs.domains,u->domain);const auto per_pledge=value(ps.pledge_domains,u->domain);
        if(per_pledge){std::int64_t n=0;for(const auto &[other,minor]:world.players)if(minor.minor&&profile.city_players.contains(other)&&profile.city_players.at(other).protected_by.contains(city.owner))n=wide::add(n,1);
            q.pledges=wide::multiply_divide(per_pledge,n,1);}}
    if(u->combat!=-1)q.combat=value(cs.combats,u->combat);
    const auto *corporations=find_vp_corporation_profile(world);
    if(corporations){const auto corporation=corporations->players.find(city.owner);
        if(corporation!=corporations->players.end()&&corporation->second.founded!=-1){const auto found=c.rules.corporations.find(corporation->second.founded);
            if(found!=c.rules.corporations.end())for(const auto &[resource,bonus]:found->second)
                if(value(u->required_resources,resource)>0||(supply.options.enabled("UNITS_RESOURCE_QUANTITY_TOTALS")&&value(u->total_resources,resource)>0))q.corporation=wide::add(q.corporation,bonus);}}
    if(supply.options.enabled("BALANCE_VP")&&!ignore_happiness&&!city.production_orders->empty()&&city.production_orders->front().type==0){
        const auto *queued=find_unit(c.rules,city.production_orders->front().item);
        if(!queued)throw CityYieldUnavailable("Missing queued Unit Info violates original happiness precondition");
        const bool combat=queued->strength>0||queued->ranged>0||queued->nuke>0;
        const bool settler=queued->found||queued->found_mid||queued->found_late||queued->found_abroad;
        const auto local=vp_city_growth_city(world,city.owner,city.id).happiness_delta;std::int64_t penalty=0;
        if(combat)penalty=std::min<std::int64_t>(0,wide::multiply_divide(wide::multiply_divide(local,integer(supply.defines,"LOCAL_UNHAPPY_COMBAT_UNIT_PRODUCTION_PENALTY"),1),-1,1));
        else if(settler){penalty=std::min<std::int64_t>(0,wide::multiply_divide(wide::multiply_divide(local,integer(supply.defines,"LOCAL_UNHAPPY_SETTLER_PRODUCTION_PENALTY"),1),-1,1));
            const bool neutral=p.minor||c.growth.science.barbarian_players.contains(city.owner)||p.cities.empty()||world.no_happiness;
            const auto happiness=neutral?integer(supply.defines,"UNHAPPY_THRESHOLD"):
                vp_science_profile(c.growth.science.players.at(city.owner)).cached_happiness_total;
            penalty=wide::add(penalty,global_settler_penalty(supply.defines,happiness));}
        q.unhappiness=std::max(penalty,integer(supply.defines,"UNHAPPY_MAX_UNIT_PRODUCTION_PENALTY"));
    }
    if(u->military){q.military_city=cs.military;const auto &map=c.growth.religion.water.map;
        MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,layout};const auto plot=geometry.plot(city.x,city.y);
        if(!plot||map.cells.size()!=map.grid.size()||!map.cells.at(*plot).units)throw CityYieldUnavailable("Unresolved military production city plot");
        for(const auto &ref:*map.cells.at(*plot).units){
            if(!supply.units.players.at(ref.owner).units.contains(ref.id))throw CityYieldUnavailable("Unresolved plot unit military production");
            const auto it=profile.military_units.find({ref.owner,ref.id});
            if(it==profile.military_units.end())throw CityYieldUnavailable("Unresolved plot unit military production");
            q.military_units=wide::add(q.military_units,it->second);}}
    if(u->spaceship!=-1){const auto *production=find_vp_general_production_profile(world);
        if(!production||!production->cities.contains({city.owner,city.id})||!production->players.contains(city.owner))throw CityYieldUnavailable("Unresolved VP space production");
        q.space_city=production->cities.at({city.owner,city.id}).space;q.space_player=production->players.at(city.owner).space;
        if(profile.league_space_ship_production.size()!=world.leagues.size())throw CityYieldUnavailable("Unresolved VP league production");
        for(std::size_t i=0;i<world.leagues.size();++i)if(const auto &league=world.leagues[i];
            std::find(league.members.begin(),league.members.end(),city.owner)!=league.members.end())
            for(const auto mod:profile.league_space_ship_production[i])q.space_leagues=wide::add(q.space_leagues,mod);}
    const auto religion=vp_plot_religion_selection(world,c.growth.religion.rules,city).majority;
    if(religion){const auto &religion_profile=vp_religion_beliefs(*religion);
        for(auto belief:religion->beliefs){const auto &b=c.rules.beliefs.at(belief);
            if(u->strength>0||u->ranged>0)q.religion_unit=wide::add(q.religion_unit,b.unit);
            if(u->combat!=-1&&vp_plot_belief_valid(c.growth.religion,belief,religion_profile.belief_religion,city.owner,&city))
                q.religion_combat=wide::add(q.religion_combat,value(b.combats,u->combat));}}
    for(const auto &[building,bonus]:u->buildings)if(bonus&&c.rules.buildings.contains(building)){
        const auto n=vp_city_building_count(c.buildings,city,building);if(n>0)q.buildings=wide::add(q.buildings,wide::multiply_divide(bonus,n,1));}
    for(auto n:{q.city.total,q.player.total,q.capital_settler,q.unit_class,q.domain,q.pledges,q.combat,q.corporation,q.unhappiness,
        q.military_city,q.military_units,q.space_city,q.space_player,q.space_leagues,q.religion_unit,q.religion_combat,q.buildings})q.total=wide::add(q.total,n);
    return q;
}
} // namespace civ5::runtime
