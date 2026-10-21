#include "civ5/runtime/vp_nature_yield.hpp"
#include "civ5/runtime/vp_unit_yield_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("VP nature ID exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){
    const T *found=nullptr;for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate VP nature input");found=&x;}
    if(!found)throw std::runtime_error("Missing VP nature input: "+std::string(name));return *found;
}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid VP nature scalar");return a[0];}
std::int64_t value(const YieldValues &v,std::int64_t y){const auto it=v.find(y);return it==v.end()?0:it->second;}
std::int64_t required(const std::optional<std::int64_t> &v){if(!v)throw std::runtime_error("Unresolved VP nature input");return *v;}
const auto &unit_state(const MaintenanceUnit &u){const auto &profile=vp_unit_yield_profile(u);if(!profile.plot_yields)throw std::runtime_error("Unresolved VP unit plot yields");return *profile.plot_yields;}
std::int64_t divide(std::int64_t a,std::int64_t b){
    if(!b)throw std::invalid_argument("Zero nature divisor");if(a==INT64_MIN&&b==-1)throw std::overflow_error("Nature quotient exceeds int64");return a/b;
}
void assign(YieldValues &v,std::int64_t y,std::int64_t n){if(y<0)throw std::runtime_error("Negative VP nature relation ID");if(n)v[y]=n;else v.erase(y);}
std::int64_t define(const VpNatureYieldContext &c,const char *name){const auto *d=c.water.defines.find(name);
    if(!d||!d->lookup.error.empty()||!std::holds_alternative<std::int64_t>(d->lookup.value))throw std::runtime_error("Unresolved VP nature define");return std::get<std::int64_t>(d->lookup.value);}
void validate(const VpNatureYieldContext &c){const auto &w=c.water;const auto &m=c.movement;
    if(w.rules.profile!=vp||w.players.profile!=vp||w.cities.profile!=vp||c.cities.profile!=vp||c.units.profile!=vp||c.unit_rules.profile!=vp||
       &m.map!=&w.map||&m.passability!=&w.passability||&m.players!=&w.players||&m.types!=&w.types||&m.defines!=&w.defines||&c.cities!=&w.cities||
       w.state.layout!=layout||!w.options||!w.options->all_succeeded())throw std::runtime_error("VP nature requires one shared profiled world");
}
MapGeometry geometry(const VpNatureYieldContext &c){const auto &m=c.water.map;return {m.grid,m.wrap_x,m.wrap_y,layout};}
const MaintenanceUnit *unit(const VpNatureYieldContext &c,std::int64_t owner,std::int64_t id){
    const auto p=c.units.players.find(owner);if(p==c.units.players.end())return nullptr;const auto u=p->second.units.find(id);return u==p->second.units.end()?nullptr:&u->second;
}
std::int64_t actual_feature(const VpNatureYieldContext &c,const saves::SavedMapCellProjection &p){
    if(p.feature_type&&p.feature_type_hash)throw std::runtime_error("Ambiguous VP feature identity");if(p.feature_type)return *p.feature_type;
    if(p.feature_type_hash){if(!*p.feature_type_hash)return -1;if(const auto *t=c.water.types.find_hash(*p.feature_type_hash))return t->id;}
    throw std::runtime_error("Unresolved VP actual feature");
}
std::int64_t adjacency(const VpNatureYieldContext &c,const saves::SavedMapCellProjection &p,const VpNatureYieldRules::Plot &info,std::int64_t yield){
    if(!c.water.options->enabled("PLOTS_EXTENSIONS")||info.adjacent_features.empty())return 0;
    bool wonder=false;const auto feature=actual_feature(c,p);if(feature!=-1){const auto &f=c.rules.features.at(feature);wonder=f.natural_wonder||f.pseudo_natural_wonder;}
    std::int64_t result=0;const auto g=geometry(c);
    for(int d=0;d<6;++d)if(const auto index=g.direction(i64(p.x),i64(p.y),d)){
        const auto f=actual_feature(c,c.water.map.cells.at(*index));if(f==-1)continue;const auto a=info.adjacent_features.find(f);
        if(a!=info.adjacent_features.end()){std::int64_t n=value(a->second[0],yield);if(!wonder)n=wide::add(n,value(a->second[1],yield));result=wide::add(result,n);}
    }
    return result;
}
std::int64_t military_yield(const VpNatureYieldContext &c,std::int64_t player,std::int64_t yield){
    const auto &p=c.state.players.at(player);const auto traits=value(p.yields[static_cast<std::size_t>(VpNaturePlayerYield::military_trait)],yield);
    const auto policy=value(p.yields[static_cast<std::size_t>(VpNaturePlayerYield::military_policy)],yield);
    if(traits<=0&&policy<=0)return 0;std::int64_t count=0;
    for(const auto &[id,u]:c.units.players.at(player).units)if(!u.civilian){const auto &s=unit_state(u);
        if(c.unit_rules.units.contains(u.type)&&s.position&&!s.death_delay)count=wide::add(count,1);}
    return wide::add(policy>0?count/policy:0,traits>0?count/traits:0);
}
}
const MaintenanceUnit *vp_city_garrison(CityYieldContext &cities,const MaintenanceContext &units,CityYieldState &city){
    if(cities.profile!=vp||units.profile!=vp||&cities.players.at(city.owner).cities.at(city.id)!=&city)
        throw CityYieldUnavailable("Garrison lookup requires the shared VP city");
    auto &plot_state=vp_city_plot_profile_city(cities,city.owner,city.id).yields;if(!plot_state)throw CityYieldUnavailable("Garrison lookup requires saved VP city plot state");
    auto &s=*plot_state;if(s.garrison<0)return nullptr;
    const auto find=[&](std::int64_t id)->const MaintenanceUnit*{const auto p=units.players.find(city.owner);if(p==units.players.end())return nullptr;
        const auto u=p->second.units.find(id);return u==p->second.units.end()?nullptr:&u->second;};
    if(s.garrison_override!=-1)if(const auto *u=find(s.garrison_override))return u;
    if(const auto *u=find(s.garrison))return u;s.garrison=-1;return nullptr;
}
VpNatureYieldRules load_vp_nature_yield_rules(const content::RulesDatabase &db){
    VpNatureYieldRules r;
    const auto load=[&](const char *table,const auto &consume){const auto q=db.prefetch_info(table,consume);if(!q.all_succeeded())throw std::runtime_error(std::string("VP nature Info ")+table+": "+q.error);return q.slots;};
    const auto yields=[&](const char *table,const char *key,const auto &type,YieldValues &out){
        const content::DatabaseValue param=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
        const auto q=db.query_rows({std::string("select Yields.ID, Yield from ")+table+" inner join Yields on YieldType = Yields.Type where "+key+" = ?",{param}},
            [&](const auto &row){assign(out,row.integer_at(0),row.integer_at(1));});if(!q.all_succeeded())throw std::runtime_error("VP nature yield relation: "+q.error);
    };
    r.yield_slots=load("Yields",[&](auto slot,auto,const auto &row){auto &y=r.yields[i64(slot)];
        y.hills=row.integer("HillsChange");y.mountain=row.integer("MountainChange");y.lake=row.integer("LakeChange");y.city=row.integer("CityChange");
        y.population_offset=row.integer("PopulationChangeOffset");y.population_divisor=row.integer("PopulationChangeDivisor");y.min_city=row.integer("MinCity");
        const char *names[]={"MinCityFlatFreshWater","MinCityFlatNoFreshWater","MinCityHillFreshWater","MinCityHillNoFreshWater","MinCityMountainFreshWater","MinCityMountainNoFreshWater"};
        for(std::size_t i=0;i<6;++i)y.city_geography[i]=row.integer(names[i]);return true;});
    load("Terrains",[&](auto slot,auto,const auto &row){auto &s=r.terrains[i64(slot)];const auto type=row.text("Type");
        const char *tables[]={"Terrain_Yields","Terrain_RiverYieldChanges","Terrain_HillsYieldChanges","Terrain_FreshWaterYields","Terrain_CoastalLandYields"};
        for(std::size_t i=0;i<5;++i)yields(tables[i],"TerrainType",type,s.yields[i]);return true;});
    load("Features",[&](auto slot,auto,const auto &row){auto &s=r.features[i64(slot)];const auto type=row.text("Type");
        s.nonadditive=row.boolean("YieldNotAdditive");s.natural_wonder=row.boolean("NaturalWonder");s.pseudo_natural_wonder=row.boolean("PseudoNaturalWonder");
        const char *tables[]={"Feature_YieldChanges","Feature_RiverYieldChanges","Feature_HillsYieldChanges","Feature_FreshWaterYields","Feature_CoastalLandYields"};
        for(std::size_t i=0;i<5;++i)yields(tables[i],"FeatureType",type,s.yields[i]);return true;});
    r.resource_slots=load("Resources",[&](auto slot,auto,const auto &row){yields("Resource_YieldChanges","ResourceType",row.text("Type"),r.resources[i64(slot)]);return true;});
    load("Improvements",[&](auto slot,auto,const auto &row){r.improvements[i64(slot)]={row.boolean("ConnectsAllResources"),row.boolean("NoFollowUp")};return true;});
    load("Plots",[&](auto slot,auto,const auto &row){auto &p=r.plots[i64(slot)];const auto type=row.text("Type");yields("Plot_Yields","PlotType",type,p.base);
        const content::DatabaseValue param=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
        const auto q=db.query_rows({"select Features.ID, Yields.ID, Yield, IgnoreNaturalWonderPlots from Plot_AdjacentFeatureYieldChanges inner join Features on FeatureType = Features.Type inner join Yields on YieldType = Yields.Type where PlotType = ?",{param}},[&](const auto &a){
            const auto feature=a.integer_at(0),yield=a.integer_at(1),amount=a.integer_at(2);
            if(feature<0||yield<0)throw std::runtime_error("Negative VP adjacency relation ID");
            if(amount){auto &n=p.adjacent_features[feature][a.integer_at(3)!=0?1:0][yield];n=wide::add(n,amount);}
        });if(!q.all_succeeded())throw std::runtime_error("VP plot adjacency relation: "+q.error);return true;});
    return r;
}
VpNatureYieldState restore_vp_nature_yields(const saves::SavedSessionSnapshot &s,const VpNatureYieldRules &r,CityYieldContext &cities,MaintenanceContext &units){
    if(!s.all_players||s.all_players->layout!=layout||s.history.layout!=layout||cities.profile!=vp||units.profile!=vp||!r.yield_slots||r.yield_slots!=cities.yield_slots)
        throw std::runtime_error("VP nature import profile/width mismatch");
    const auto sparse=[&](const auto &a){if(a.size()!=r.yield_slots)throw std::runtime_error("VP nature yield width differs from active rules");
        YieldValues out;for(std::size_t i=0;i<a.size();++i)if(a[i])out[i64(i)]=a[i];return out;};
    VpNatureYieldState state;
    const char *core_names[]={"m_aiCityYieldChange","m_aiCoastalCityYieldChange","m_aiMonopolyCityYieldChange","m_aiCapitalYieldChange","m_aiCapitalYieldPerPopChange","m_aiCapitalYieldPerPopChangeEmpire","m_aiYieldPerCityOverStrengthThreshold","m_aiYieldFromXMilitaryUnits"};
    const char *trait_names[]={"m_iYieldFromXMilitaryUnits","m_iCityYieldChanges","m_iCoastalCityYieldChanges","m_iCapitalYieldChanges"};
    for(const auto &source:s.all_players->players){const auto id=source.saved_player_id;
        if(!cities.players.contains(id)||!units.players.contains(id)||state.players.contains(id)||source.core.layout!=layout||source.traits.layout!=layout||source.tail.layout!=layout||source.cities.layout!=layout||source.units.layout!=layout)
            throw std::runtime_error("VP nature player/profile mismatch");
        auto &p=state.players[id];cities.players.at(id).total_population=scalar(source.core.fields,"m_iTotalPopulation");
        for(std::size_t i=0;i<8;++i)p.yields[i]=sparse(named(source.core.fields,core_names[i]).values);
        for(std::size_t i=0;i<4;++i)p.yields[i+8]=sparse(named(source.traits.fields,trait_names[i]).values);
        // The source serializes this member twice. The later tail replaces core.
        const auto &monopolies=named(source.tail.fields,"m_pabHasGlobalMonopoly").values;
        if(monopolies.size()!=r.resource_slots)throw std::runtime_error("VP monopoly span differs from active resources");
        for(std::size_t i=0;i<monopolies.size();++i){if(monopolies[i]!=0&&monopolies[i]!=1)throw std::runtime_error("Invalid monopoly flag");if(monopolies[i])p.has_global_monopoly.insert(i64(i));}
        std::set<std::int64_t> seen_cities,seen_units;
        for(const auto &source_city:source.cities.cities){const auto cid=scalar(source_city.core.fields,"m_iID");auto &city=cities.players.at(id).cities.at(cid);
            if(!seen_cities.insert(cid).second||source_city.core.layout!=layout||city.owner!=id||city.id!=cid||scalar(source_city.core.fields,"m_eOwner")!=id||
               city.source_core!=std::optional{std::pair{source_city.core.start_offset,source_city.core.end_offset}})throw std::runtime_error("VP nature city binding mismatch");
            VpCityPlotYieldState a;a.strength=scalar(source_city.core.fields,"m_iStrengthValue");a.garrison=scalar(source_city.core.fields,"m_hGarrison");
            a.per_strength_times100=sparse(named(source_city.core.fields,"m_aiYieldChangesPerCityStrengthTimes100").values);ensure_vp_city_plot_profile_city(cities,id,cid).yields=std::move(a);
        }
        for(const auto &source_unit:source.units.units){const auto uid=scalar(source_unit.core.fields,"m_iID");auto &u=units.players.at(id).units.at(uid);
            if(!seen_units.insert(uid).second||source_unit.core.layout!=layout||source_unit.tail.layout!=layout||scalar(source_unit.core.fields,"m_eOwner")!=id||
               u.source_core!=std::optional{std::pair{source_unit.core.start_offset,source_unit.core.end_offset}})throw std::runtime_error("VP nature unit binding mismatch");
            VpUnitYieldProfileState::PlotYieldState a;const auto x=scalar(source_unit.core.fields,"m_iX"),y=scalar(source_unit.core.fields,"m_iY");
            if(x!=-1&&y!=-1&&x!=-2147483647LL&&y!=-2147483647LL)a.position={{x,y}};
            a.base_combat=scalar(source_unit.core.fields,"m_iBaseCombat");const auto death=scalar(source_unit.core.fields,"m_bDeathDelay");
            if(death!=0&&death!=1)throw std::runtime_error("Invalid saved unit death-delay flag");a.death_delay=death!=0;
            const auto &fields=named(source_unit.tail.blocks,"state").children;
            a.garrison=sparse(named(fields,"m_iGarrisonYieldChange").values);a.fortification=sparse(named(fields,"m_iFortificationYieldChange").values);
            ensure_vp_unit_yield_profile(u,units.profile).plot_yields=std::move(a);
        }
        if(seen_cities.size()!=cities.players.at(id).cities.size()||seen_units.size()!=units.players.at(id).units.size())throw std::runtime_error("Incomplete VP nature city/unit roster");
    }
    if(state.players.size()!=cities.players.size()||state.players.size()!=units.players.size())throw std::runtime_error("Incomplete VP nature players");
    for(const auto &extra:s.history.extra_yields)state.extra_yields.push_back({extra.plot.x,extra.plot.y,sparse(extra.yields)});
    return state;
}
void refresh_vp_global_monopoly_cache(VpNatureYieldState &s,const VpNatureYieldRules &r,const content::CustomModOptions &options,std::int64_t player){
    if(!options.all_succeeded())throw std::runtime_error("Unresolved VP monopoly options");auto &p=s.players.at(player);p.global_monopoly_cache.clear();
    if(options.enabled("BALANCE_RESOURCE_MONOPOLIES"))for(const auto &[id,info]:r.resources)if(p.has_global_monopoly.contains(id))p.global_monopoly_cache.push_back(id);
}
VpNatureYieldQuote vp_nature_yield(VpNatureYieldContext &c,std::int64_t plot,std::int64_t yield,std::int64_t player,std::int64_t feature,std::int64_t resource,std::int64_t improvement,CityYieldState *city,bool display){
    validate(c);if(plot<0)throw std::out_of_range("Negative VP nature plot ID");const auto &p=c.water.map.cells.at(plot);const auto g=geometry(c);const auto xy=g.grid.coordinates(plot);
    if(p.index!=static_cast<std::uint64_t>(plot)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("VP nature plot identity mismatch");
    const auto team=player==-1?-1:c.water.players.players.at(player).team;
    const auto terrain=required(p.terrain_type),kind=required(p.plot_type);const bool hills=kind==1,mountain=kind==0||terrain==7;
    const auto &y=c.rules.yields.at(yield);const auto &t=c.rules.terrains.at(terrain);const auto &pi=c.rules.plots.at(kind);
    VpNatureYieldQuote q;q.terrain_suppressed=!plot_is_valid_move(c.movement,plot,player)&&required(p.owner)!=player&&!(kind==3&&terrain==define(c,"SHALLOW_WATER_TERRAIN"));
    if(!q.terrain_suppressed){q.terrain=value(t.yields[0],yield);
        if(yield==c.production&&terrain==c.desert&&!hills&&feature==-1&&resource!=-1&&improvement!=-1)q.desert=define(c,"BALANCE_PRODUCTION_DESERT_IMPROVEMENT");}
    q.total=wide::add(q.terrain,q.desert);q.plot=value(pi.base,yield);q.total=wide::add(q.total,q.plot);
    for(const auto &extra:c.state.extra_yields)if(extra.x==i64(p.x)&&extra.y==i64(p.y)){q.extra=value(extra.yields,yield);break;}q.total=wide::add(q.total,q.extra);
    if(hills)q.hills=y.hills;if(mountain)q.mountain=y.mountain;if(plot_is_lake(c.water,plot))q.lake=y.lake;
    for(const auto n:{q.hills,q.mountain,q.lake})q.total=wide::add(q.total,n);
    const auto &surface=feature==-1?t:c.rules.features.at(feature);
    if(feature!=-1){q.feature=value(surface.yields[0],yield);q.feature_replaced=surface.nonadditive;q.total=q.feature_replaced?q.feature:wide::add(q.total,q.feature);}
    if(!p.rivers)throw std::runtime_error("Unresolved VP nature rivers");const bool fresh=plot_is_fresh_water(c.water,plot),coastal=plot_is_coastal_land(c.water,plot);
    if(p.rivers->is_river)q.river=value(surface.yields[1],yield);if(hills)q.surface_hills=value(surface.yields[2],yield);
    if(fresh)q.fresh=value(surface.yields[3],yield);if(coastal)q.coastal=value(surface.yields[4],yield);
    for(const auto n:{q.river,q.surface_hills,q.fresh,q.coastal})q.total=wide::add(q.total,n);
    const auto ii=c.rules.improvements.find(improvement);
    if(team!=-1&&resource!=-1){const auto ri=c.rules.resources.find(resource);
        if(ri!=c.rules.resources.end()&&(!c.water.options->enabled("BALANCE_NO_RESOURCE_YIELDS_FROM_GP_IMPROVEMENT")||ii==c.rules.improvements.end()||!ii->second.connects_all_resources))q.resource=value(ri->second,yield);}
    q.total=wide::add(q.total,q.resource);q.adjacent=adjacency(c,p,pi,yield);q.total=wide::add(q.total,q.adjacent);
    if(city&&g.plot(city->x,city->y)==plot){
        if(&c.cities.players.at(city->owner).cities.at(city->id)!=city)throw std::runtime_error("VP nature must use the live city object");
        const auto &plot_yields=vp_city_plot_profile_city(c.cities,city->owner,city->id).yields;if(!plot_yields)throw std::runtime_error("Unresolved VP nature city fields");
        if(c.water.options->enabled("BALANCE_VP")){
            q.city_replaced=true;q.city_base=y.min_city;q.city_resource=resource==-1?0:value(c.rules.resources.at(resource),yield);
            q.city_geography=y.city_geography[(mountain?4:hills?2:0)+(fresh?0:1)];
            q.total=wide::add(wide::add(q.city_base,q.city_resource),q.city_geography);q.total=wide::add(q.total,q.adjacent);
        }else{q.city_base=y.min_city;q.total=std::max(q.total,y.min_city);}
        if(const auto *u=vp_city_garrison(c.cities,c.units,*city)){const auto &s=unit_state(*u);q.garrison=wide::multiply_divide(value(s.garrison,yield),s.base_combat,8);}q.total=wide::add(q.total,q.garrison);
        bool visible=!display;if(display){if(c.active_team<0||!c.movement.diplomacy.teams.contains(c.active_team)||!p.revealed_teams)throw std::runtime_error("Unresolved VP display city reveal");visible=p.revealed_teams->contains(c.active_team);}
        if(visible){q.city_change=y.city;if(y.population_divisor)q.population=divide(wide::add(city->population,y.population_offset),y.population_divisor);}
        q.total=wide::add(wide::add(q.total,q.city_change),q.population);
        if(player!=-1){const auto &a=c.state.players.at(player);const auto bonus=[&](VpNaturePlayerYield k){return value(a.yields[static_cast<std::size_t>(k)],yield);};
            q.military=military_yield(c,player,yield);q.trait_city=bonus(VpNaturePlayerYield::trait_city);
            if(coastal){q.player_coastal=bonus(VpNaturePlayerYield::coastal);q.trait_coastal=bonus(VpNaturePlayerYield::trait_coastal);}
            q.monopoly=wide::multiply_divide(bonus(VpNaturePlayerYield::monopoly),i64(a.global_monopoly_cache.size()),1);
            const auto strength_bonus=value(plot_yields->per_strength_times100,yield);
            if(strength_bonus>0)q.strength=wide::multiply_divide(strength_bonus,plot_yields->strength,10000);
            if(plot_yields->strength>=wide::multiply_divide(define(c,"CITY_STRENGTH_THRESHOLD_FOR_BONUSES"),100,1))q.strength_threshold=bonus(VpNaturePlayerYield::strength_threshold);
            auto times100=bonus(VpNaturePlayerYield::city_times100);
            if(c.cities.players.at(city->owner).capital==city->id){
                times100=wide::add(times100,bonus(VpNaturePlayerYield::capital_times100));q.trait_capital=bonus(VpNaturePlayerYield::trait_capital);
                q.capital_population=wide::multiply_divide(city->population,bonus(VpNaturePlayerYield::capital_per_population),100);
                q.capital_empire=wide::multiply_divide(c.cities.players.at(city->owner).total_population,bonus(VpNaturePlayerYield::capital_per_empire),100);
            }
            q.city_and_capital=times100/100;
            for(const auto n:{q.military,q.trait_city,q.player_coastal,q.trait_coastal,q.monopoly,q.strength,q.strength_threshold,q.trait_capital,q.capital_population,q.capital_empire,q.city_and_capital})q.total=wide::add(q.total,n);
        }
    }
    if(improvement!=-1){if(ii==c.rules.improvements.end())throw std::runtime_error("Unresolved VP fortification improvement Info");
        if(ii->second.no_followup){if(!p.improvement_pillaged)throw std::runtime_error("Unresolved VP improvement pillage");if(!*p.improvement_pillaged){
            if(!p.units)throw std::runtime_error("Unresolved VP plot unit handles");
            for(const auto &ref:*p.units)if(const auto *u=unit(c,ref.owner,ref.id)){const auto &s=unit_state(*u);const auto n=wide::multiply_divide(value(s.fortification,yield),s.base_combat,8);
                q.fortification=wide::add(q.fortification,n);q.total=wide::add(q.total,n);}
        }}
    }
    q.total=std::max<std::int64_t>(0,q.total);return q;
}
} // namespace civ5::runtime
