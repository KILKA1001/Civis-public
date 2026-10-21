#include "civ5/runtime/vp_city_base_yields.hpp"
#include "civ5/runtime/vp_unit_yield_profile.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City base yield ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate base yield field");found=&x;}
    if(!found)throw std::runtime_error("Missing base yield field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid base yield scalar");return a[0];}
void validate(const VpCityBaseYieldContext &c,const CityYieldState &city){const auto &g=c.buildings.great_works;const auto &w=g.religion.water;
    const auto *profile=find_vp_city_base_yield_profile(g.cities);
    if(g.cities.profile!=vp||w.players.profile!=vp||&w.cities!=&g.cities||&c.happiness.great_works!=&g||&c.religion.religion!=&g.religion||
       &c.traits.cities!=&g.cities||&c.traits.players!=&w.players||&c.religion.treasury!=&c.traits.treasury||&c.traits.team_eras!=&c.buildings.team_eras||
       &c.traits.options!=w.options||&c.traits.defines!=&w.defines||&c.happiness.units.team_eras!=&c.traits.team_eras||
       &g.cities.players.at(city.owner).cities.at(city.id)!=&city||!profile||!profile->players.contains(city.owner)||
       !profile->cities.contains({city.owner,city.id})||!w.options||!w.options->all_succeeded())
        throw CityYieldUnavailable("Base city yield requires one resolved shared VP world");
}
const saves::SavedMapCellProjection &cell(const saves::SavedMapProjection &map,std::int64_t index){
    if(index<0||map.cells.size()!=map.grid.size())throw CityYieldUnavailable("Invalid base city yield map");const auto &p=map.cells.at(index);const auto xy=map.grid.coordinates(index);
    if(p.index!=static_cast<std::uint64_t>(index)||p.x!=xy[0]||p.y!=xy[1])throw CityYieldUnavailable("Base city yield plot binding mismatch");return p;
}
CityYieldComponents components(const CityYieldState &city,std::int64_t y){const auto i=city.yields.find(y);return i==city.yields.end()?CityYieldComponents{}:i->second;}
}
VpCityBaseYieldRules load_vp_city_base_yield_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpCityBaseYieldRules r;const auto load=[&](const char *table,const auto &fn){if(!db.prefetch_info(table,fn).all_succeeded())throw CityYieldUnavailable("Unresolved base yield Info: "+std::string(table));};
    const auto identity=[&](const char *name){const auto *t=types.find(name);if(!t)throw CityYieldUnavailable("Missing base city yield identity");return t->id;};
    r.culture=identity("YIELD_CULTURE");r.tourism=identity("YIELD_TOURISM");r.culture_local=identity("YIELD_CULTURE_LOCAL");
    load("Yields",[&](auto id,auto,const auto &){r.yields.insert(id64(id));return true;});
    load("BuildingClasses",[&](auto id,auto,const auto &row){r.world_wonder_classes[id64(id)]=row.integer("MaxGlobalInstances")!=-1;return true;});
    load("Features",[&](auto id,auto,const auto &row){r.natural_wonders[id64(id)]=row.integer("NaturalWonder")!=0||row.integer("PseudoNaturalWonder")!=0;return true;});
    load("Buildings",[&](auto id,auto,const auto &row){auto &b=r.buildings[id64(id)];const auto cls=row.text("BuildingClass");if(cls)if(const auto *t=types.find(*cls))b.building_class=t->id;
        const auto type=row.text("Type");const content::DatabaseValue name=type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{};
        const auto q=db.query_rows({"select Yields.ID, Yield from Building_YieldChanges inner join Yields on YieldType = Yields.Type where BuildingType = ?",{name}},[&](const auto &v){if(v.integer_at(0)==r.culture)b.culture=v.integer_at(1);});
        if(!q.all_succeeded())throw CityYieldUnavailable("Unresolved building culture yield");return true;});return r;
}
std::optional<std::int64_t> vp_city_ring_plot(const saves::SavedMapProjection &map,std::int64_t x,std::int64_t y,std::int64_t index){
    if(index<0)throw std::out_of_range("Negative city ring index");MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,layout};if(!index)return geometry.plot(x,y);
    // Find the first ring whose last index 3*r*(r+1) reaches index. Division
    // avoids overflowing that comparison, including for an INT64_MAX index.
    const auto reaches=[&](std::int64_t r){return r>(index-1)/mul(3,add(r,1));};
    std::int64_t low=0,high=1;while(!reaches(high))high=mul(high,2);
    while(high-low>1){const auto mid=low+(high-low)/2;if(reaches(mid))high=mid;else low=mid;}
    const auto ring=high,offset=index-add(mul(mul(3,ring-1),ring),1),side=offset/ring,step=offset%ring;std::int64_t dx=0,dy=0;
    switch(side){case 0:dx=step;dy=ring-step;break;case 1:dx=ring;dy=-step;break;case 2:dx=ring-step;dy=-ring;break;
        case 3:dx=-step;dy=-ring+step;break;case 4:dx=-ring;dy=step;break;case 5:dx=-ring+step;dy=ring;break;default:throw std::logic_error("Invalid city ring side");}
    const auto half=[](std::int64_t n){return n/2-(n<0&&n%2!=0?1:0);};const auto hx=add(wide::subtract(x,half(y)),dx);y=add(y,dy);return geometry.plot(add(hx,half(y)),y);
}
void vp_rebuild_city_worked_plots(const saves::SavedMapProjection &map,CityYieldContext &world,CityYieldState &city){
    auto &a=vp_city_base_yield_city(world,city.owner,city.id);a.worked_plots.clear();
    for(std::size_t i=0;i<a.working.size();++i)if(a.working[i])if(const auto plot=vp_city_ring_plot(map,city.x,city.y,id64(i)))a.worked_plots.push_back(*plot);
}
void restore_vp_city_base_yields(const saves::SavedSessionSnapshot &s,CityYieldContext &world,MaintenanceContext &units){
    if(world.profile!=vp||units.profile!=vp||!s.map_projection||!s.all_players||s.all_players->layout!=layout)throw CityYieldUnavailable("Base city yield import requires VP world");
    const auto vector=[&](const auto &fields,const char *name){const auto &a=named(fields,name).values;if(a.size()!=world.yield_slots)throw std::runtime_error("Base yield span mismatch");YieldValues v;for(std::size_t i=0;i<a.size();++i)if(a[i])v[id64(i)]=a[i];return v;};
    auto &profile=ensure_vp_city_base_yield_profile(world);std::set<std::int64_t> players;
    for(const auto &sp:s.all_players->players){const auto id=sp.saved_player_id;auto &p=world.players.at(id);
        if(!players.insert(id).second||sp.core.layout!=layout||sp.cities.layout!=layout||sp.units.layout!=layout||scalar(sp.core.fields,"m_eID")!=id||p.total_population!=scalar(sp.core.fields,"m_iTotalPopulation"))throw std::runtime_error("Base yield player binding mismatch");
        VpCityBaseYieldProfileState::Player a;a.landmarks_tourism=scalar(sp.core.fields,"m_iLandmarksTourismPercentGlobal");a.state_religion=vector(sp.core.fields,"m_aiReligionYieldRateModifier");
        a.non_specialists=vector(sp.core.fields,"m_aiYieldFromNonSpecialistCitizensTimes100");a.capital_tiles=vector(sp.core.fields,"m_aiYieldFromExpendTileCapital");profile.players[id]=std::move(a);
        std::set<std::int64_t> cities;for(const auto &sc:sp.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(cid);
            if(!cities.insert(cid).second||sc.core.layout!=layout||sc.production.layout!=layout||sc.management.layout!=layout||sc.tail.layout!=layout||city.owner!=id||city.id!=cid||scalar(sc.core.fields,"m_eOwner")!=id||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("Base yield city binding mismatch");
            VpCityBaseYieldProfileState::City b;b.world_wonders=scalar(sc.core.fields,"m_iNumWorldWonders");b.landmarks_tourism=named(named(sc.production.blocks,"buildings").fields,"m_iLandmarksTourismPercent").value;
            const char *names[]={"m_aiYieldChangeFromCorporationFranchises","m_aiEventCityYield","m_aiBaseYieldRateFromLeague","m_aiYieldFromHappiness","m_aiYieldFromHealth","m_aiYieldFromPreviousGoldenAges"};
            YieldValues *dest[]={&b.corporations,&b.events,&b.league,&b.happiness,&b.health,&b.previous_golden_ages};for(std::size_t k=0;k<6;++k)*dest[k]=vector(sc.core.fields,names[k]);
            const auto &empire=named(named(sc.tail.blocks,"remainder").children,"m_aiYieldPerPopInEmpire").values;if(empire.size()%2)throw std::runtime_error("Invalid city empire-population yield map");
            for(std::size_t k=0;k<empire.size();k+=2)b.empire_population.emplace(empire[k],empire[k+1]);
            const auto &working=named(named(sc.management.blocks,"citizens").arrays,"m_pabWorkingPlot").values;if(working.size()!=91)throw std::runtime_error("Unexpected VP working-plot wire span");
            for(auto n:working){if(n!=0&&n!=1)throw std::runtime_error("Invalid working-plot flag");b.working.push_back(n!=0);}profile.cities[{id,cid}]=std::move(b);vp_rebuild_city_worked_plots(*s.map_projection,world,city);
            const auto fill=[&](const char *name,auto member){const auto v=vector(sc.core.fields,name);for(std::uint64_t y=0;y<world.yield_slots;++y)city.yields[id64(y)].*member=value(v,id64(y));};
            fill("m_aiBaseYieldRateFromTerrain",&CityYieldComponents::terrain);fill("m_aiBaseYieldRateFromSpecialists",&CityYieldComponents::specialists);fill("m_aiBaseYieldRateFromMisc",&CityYieldComponents::misc);fill("m_aiYieldPerPop",&CityYieldComponents::per_population);
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete base yield cities");
        std::set<std::int64_t> seen;for(const auto &su:sp.units.units){const auto uid=scalar(su.core.fields,"m_iID");auto &u=units.players.at(id).units.at(uid);
            if(!seen.insert(uid).second||su.core.layout!=layout||su.tail.layout!=layout||scalar(su.core.fields,"m_eOwner")!=id||u.source_core!=std::optional{std::pair{su.core.start_offset,su.core.end_offset}})throw std::runtime_error("Base yield unit binding mismatch");
            ensure_vp_unit_yield_profile(u,units.profile).city_yield_change=vector(named(su.tail.blocks,"state").children,"m_YieldChange");
        }if(seen.size()!=units.players.at(id).units.size())throw std::runtime_error("Incomplete base yield units");
    }if(players.size()!=world.players.size()||players.size()!=units.players.size())throw std::runtime_error("Incomplete base yield players");
}
std::int64_t vp_league_world_wonder_yield(const CityYieldContext &world,std::int64_t player,std::int64_t yield,std::int64_t culture){
    if(world.profile!=vp||!world.players.contains(player))throw CityYieldUnavailable("League wonder yield requires a VP player");std::int64_t n=0;if(yield!=culture)return n;
    for(const auto &l:world.leagues)if(std::find(l.members.begin(),l.members.end(),player)!=l.members.end())for(auto amount:l.culture_per_wonder)n=add(n,amount);return n;
}
VpCityLandmarkTourismQuote vp_city_landmark_tourism(const VpCityBaseYieldContext &c,const CityYieldState &city){
    validate(c,city);const auto &g=c.buildings.great_works;const auto &world=g.cities;const auto &map=g.religion.water.map;VpCityLandmarkTourismQuote q;
    const auto &base=vp_city_base_yield_city(world,city.owner,city.id);const auto &player=vp_city_base_yield_player(world,city.owner);
    q.percent=add(base.landmarks_tourism,player.landmarks_tourism);if(!q.percent)return q;
    const auto &great_work_state=vp_great_work_city(world,city.owner,city.id);
    for(auto id:great_work_state.existing_buildings){const auto it=c.rules.buildings.find(id);if(it==c.rules.buildings.end())continue;const auto &b=it->second;
        if(c.rules.world_wonder_classes.at(b.building_class))q.wonders=add(q.wonders,add(b.culture,vp_league_world_wonder_yield(world,city.owner,c.rules.culture,c.rules.culture)));}
    for(auto id:base.worked_plots){const auto &plot=cell(map,id);if(!plot.feature_type)throw CityYieldUnavailable("Unresolved worked plot feature");
        if(*plot.feature_type!=-1&&c.rules.natural_wonders.at(*plot.feature_type)){if(!plot.cached_yields)throw CityYieldUnavailable("Unresolved worked plot yield cache");q.natural_wonders=add(q.natural_wonders,plot.cached_yields->at(c.rules.culture));}}
    q.terrain=components(city,c.rules.culture).terrain;q.total_times100=mul(add(add(q.wonders,q.natural_wonders),q.terrain),q.percent);return q;
}
VpCityBaseYieldQuote vp_city_base_yield(const VpCityBaseYieldContext &c,CityYieldState &city,std::int64_t y){
    validate(c,city);if(!c.rules.yields.contains(y))throw std::out_of_range("Invalid city base yield");const auto &g=c.buildings.great_works;const auto &world=g.cities;const auto &p=world.players.at(city.owner);const auto &a=vp_city_base_yield_city(world,city.owner,city.id);const auto &b=vp_city_base_yield_player(world,city.owner);
    const auto &map=g.religion.water.map;const auto v=components(city,y);VpCityBaseYieldQuote q;
    if(y==c.rules.culture_local)q.local_culture=vp_city_cached_yield(world,city.owner,city.id,c.rules.culture);
    q.great_works=vp_cached_great_work_yield(g,city,y);q.terrain=mul(v.terrain,100);q.buildings=vp_city_building_yield(c.buildings,city,y).total_times100;
    q.policies=vp_city_policy_yield(c.happiness,city,y).total_times100;q.specialists=mul(v.specialists,100);q.religion=vp_city_religion_yield(c.religion,city,y).total_times100;q.minors=vp_city_minor_yield(c.religion,city,y).total_times100;
    q.traits=vp_city_trait_yield(c.traits,city,y).total_times100;q.corporations=mul(value(a.corporations,y),100);q.events=mul(value(a.events,y),100);
    const auto connection=vp_city_connection_yield(c.traits,city,y);q.connection=connection.route;
    q.league=mul(add(mul(a.world_wonders,vp_league_world_wonder_yield(world,city.owner,y,c.rules.culture)),value(a.league,y)),100);
    if(y==c.rules.tourism)q.landmarks=vp_city_landmark_tourism(c,city).total_times100;
    if(c.traits.options.enabled("YIELD_MODIFIER_FROM_UNITS")){MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,layout};const auto id=geometry.plot(city.x,city.y);if(!id)throw CityYieldUnavailable("Invalid unit-yield city plot");const auto &plot=cell(map,*id);
        if(!plot.units)throw CityYieldUnavailable("Unresolved city plot units");for(const auto &ref:*plot.units){const auto &u=c.happiness.units.players.at(ref.owner).units.at(ref.id);const auto &profile=vp_unit_yield_profile(u);if(!profile.city_yield_change)throw CityYieldUnavailable("Unresolved unit city yield");q.units=add(q.units,mul(value(*profile.city_yield_change,y),100));}}
    if(!vp_city_majority_religion(city)||!vp_player_state_religion(p))throw CityYieldUnavailable("Unresolved state-religion city income");if(*vp_city_majority_religion(city)>world.pantheon&&vp_player_state_religion(p)==vp_city_majority_religion(city))q.state_religion=mul(value(b.state_religion,y),100);
    q.population=mul(v.per_population,city.population);q.empire_population=mul(value(a.empire_population,y),p.total_population);q.industrial=connection.industrial;
    if(const auto bonus=value(b.non_specialists,y))q.non_specialists=mul(bonus,wide::subtract(city.population,vp_total_specialists(c.happiness.food,city)));
    const auto &happiness=vp_city_happiness_city(c.happiness.great_works.cities,city.owner,city.id);std::int64_t misc=v.misc;for(auto n:{value(a.happiness,y),value(a.health,y),value(happiness.crime,y),value(happiness.development,y)})misc=add(misc,n);q.misc=mul(misc,100);
    q.previous_golden_ages=mul(value(a.previous_golden_ages,y),100);if(p.capital==city.id)q.capital_tiles=mul(value(b.capital_tiles,y),100);
    for(auto n:{q.local_culture,q.great_works,q.terrain,q.buildings,q.policies,q.specialists,q.religion,q.minors,q.traits,q.corporations,q.events,q.connection,q.league,q.landmarks,q.units,q.state_religion,q.population,q.empire_population,q.industrial,q.non_specialists,q.misc,q.previous_golden_ages,q.capital_tiles})q.total_times100=add(q.total_times100,n);return q;
}
} // namespace civ5::runtime
