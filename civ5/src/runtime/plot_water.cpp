#include "civ5/runtime/plot_water.hpp"
#include <algorithm>
#include <set>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
template<class T> const T &named(const std::vector<T> &fields,std::string_view name) {
    const T *r=nullptr;for(const auto &f:fields)if(f.name&&std::string_view(f.name)==name){
        if(r)throw std::runtime_error("Duplicate plot water field: "+std::string(name));r=&f;
    }
    if(!r)throw std::runtime_error("Missing plot water field: "+std::string(name));return *r;
}
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Plot water ID exceeds int64");return static_cast<std::int64_t>(n);}
bool vp(const PlotWaterContext &c){return c.rules.profile==TechnologyProfile::vp542;}
MapGeometry geometry(const PlotWaterContext &c){return {c.map.grid,c.map.wrap_x,c.map.wrap_y,c.state.layout};}
void validate(const PlotWaterContext &c){
    geometry(c).validate();
    if(c.state.layout!=c.passability.layout||c.rules.profile!=c.players.profile||c.rules.profile!=c.cities.profile||
       c.state.layout!=(vp(c)?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1)||
       c.defines.profile()!=(vp(c)?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw)||
       c.map.cells.size()!=c.map.grid.size()||c.passability.plots.size()!=c.map.cells.size()||!c.types.report().all_succeeded()||
       (vp(c)?(!c.options||!c.options->all_succeeded()):c.options!=nullptr))throw std::runtime_error("Plot water context mismatch");
}
const saves::SavedMapCellProjection &cell(const PlotWaterContext &c,std::int64_t index){
    if(index<0)throw std::out_of_range("Negative plot water ID");const auto &p=c.map.cells.at(index);
    const auto xy=c.map.grid.coordinates(index);
    if(p.index!=static_cast<std::uint64_t>(index)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("Plot water identity mismatch");return p;
}
std::int64_t required(const std::optional<std::int64_t> &v,const char *name){if(!v)throw std::runtime_error(std::string("Unresolved plot ")+name);return *v;}
bool water(const saves::SavedMapCellProjection &p){return required(p.plot_type,"category")==3;}
bool river(const saves::SavedMapCellProjection &p){if(!p.rivers)throw std::runtime_error("Unresolved plot river");return p.rivers->is_river;}
std::int64_t type(const PlotWaterContext &c,const std::optional<std::int64_t> &id,const std::optional<std::uint64_t> &hash){
    if(id&&hash)throw std::runtime_error("Ambiguous plot type identity");if(id)return *id;
    if(hash){if(!*hash)return -1;const auto *t=c.types.find_hash(*hash);if(t)return t->id;}
    throw std::runtime_error("Unresolved plot water content type");
}
std::int64_t feature(const PlotWaterContext &c,const saves::SavedMapCellProjection &p){return type(c,p.feature_type,p.feature_type_hash);}
std::vector<std::int64_t> neighbors(const PlotWaterContext &c,std::int64_t id){
    const auto &p=cell(c,id);std::vector<std::int64_t> result;
    for(int d=0;d<6;++d)if(const auto n=geometry(c).direction(i64(p.x),i64(p.y),d))result.push_back(*n);return result;
}
std::int64_t define(const PlotWaterContext &c,const char *name){
    const auto *d=c.defines.find(name);
    if(!d||!d->lookup.error.empty()||!std::holds_alternative<std::int64_t>(d->lookup.value))
        throw std::runtime_error(std::string("Unresolved water define: ")+name);
    return std::get<std::int64_t>(d->lookup.value);
}
const PlotWaterLandmass *landmass(const PlotWaterContext &c,const saves::SavedMapCellProjection &p){
    const auto it=c.state.landmasses.find(required(p.landmass,"landmass"));return it==c.state.landmasses.end()?nullptr:&it->second;
}
bool lake(const PlotWaterContext &c,const saves::SavedMapCellProjection &p){
    const auto *m=landmass(c,p);if(!m||!m->water)return false;
    return vp(c)?m->tiles<define(c,"MIN_WATER_SIZE_FOR_OCEAN"):m->tiles<=define(c,"LAKE_MAX_AREA_SIZE");
}
const saves::PlotWaterCache &cached(const PlotWaterContext &c,std::int64_t id){
    const auto &p=cell(c,id);if(!p.water_cache)throw std::runtime_error("Plot water cache has not been initialized");return *p.water_cache;
}
bool adds(const std::map<std::int64_t,bool> &infos,std::int64_t id){
    if(id==-1)return false;const auto i=infos.find(id);if(i==infos.end())throw std::runtime_error("Unresolved freshwater Info");return i->second;
}
bool city_adds_water(const PlotWaterContext &c,const saves::SavedMapCellProjection &p){
    const auto owner=required(p.owner,"city owner");const auto &cities=c.cities.players.at(owner).cities;
    for(const auto &[id,city]:cities)if(city.x==i64(p.x)&&city.y==i64(p.y)){
        if(!city.building_counts_resolved)throw std::runtime_error("Freshwater city needs live building counts");
        // The original buildings-here cache uses real>0 OR free>0, not their sum.
        for(const auto *counts:{&city.real_buildings,&city.free_buildings})for(const auto &[building,n]:*counts)if(n>0){
            const auto info=c.rules.buildings.find(building);if(info!=c.rules.buildings.end()&&info->second)return true;
        }
        return false;
    }
    throw std::runtime_error("City plot has no matching live city");
}
std::int64_t team(const PlotWaterContext &c,const saves::SavedMapCellProjection &p){
    const auto owner=required(p.owner,"owner");return owner==-1?-1:c.players.players.at(owner).team;
}
}
PlotWaterRules load_plot_water_rules(const content::RulesDatabase &db,TechnologyProfile profile){
    PlotWaterRules r;r.profile=profile;
    const auto load=[&](const char *table,auto &values){
        const auto q=db.prefetch_info(table,[&](auto slot,auto,const auto &row){values.emplace(i64(slot),row.boolean("AddsFreshWater"));return true;});
        if(!q.all_succeeded())throw std::runtime_error("Water Info: "+q.error);return q.slots;
    };
    load("Features",r.features);
    if(profile==TechnologyProfile::vp542){load("Improvements",r.improvements);r.building_slots=load("Buildings",r.buildings);}
    return r;
}
PlotWaterState restore_plot_water(const saves::SavedSessionSnapshot &snapshot){
    if(!snapshot.map_regions||snapshot.map_regions->layout!=snapshot.map_header.layout)throw std::runtime_error("Plot water requires matching regions");
    PlotWaterState r;r.layout=snapshot.map_regions->layout;
    for(const auto &record:named(snapshot.map_regions->containers,"landmasses").records){
        const auto id=named(record.fields,"id").value,n=named(record.fields,"tiles").value,water=named(record.fields,"water").value;
        if(id<0||n<0||(water!=0&&water!=1)||!r.landmasses.emplace(id,PlotWaterLandmass{n,water!=0}).second)
            throw std::runtime_error("Invalid or duplicate saved landmass");
    }
    return r;
}
void restore_vp_water_city_buildings(const saves::SavedSessionSnapshot &snapshot,CityYieldContext &cities,const PlotWaterRules &rules){
    if(cities.profile!=TechnologyProfile::vp542||rules.profile!=cities.profile||!snapshot.all_players||
       snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)throw std::runtime_error("VP water building profile mismatch");
    std::set<std::pair<std::int64_t,std::int64_t>> seen;
    for(const auto &player:snapshot.all_players->players)for(const auto &source:player.cities.cities){
        if(source.production.layout!=saves::GameStateLayout::vp542_corpus)throw std::runtime_error("VP water city layout mismatch");
        const auto &ids=named(source.core.fields,"m_iID").values;
        if(ids.size()!=1||!seen.emplace(player.saved_player_id,ids[0]).second)throw std::runtime_error("Duplicate/invalid water city");
        auto &city=cities.players.at(player.saved_player_id).cities.at(ids[0]);
        const auto &b=named(source.production.blocks,"buildings");
        const auto load=[&](const char *name){
            const auto &v=named(b.arrays,name);
            if(v.columns!=1||v.values.size()!=rules.building_slots)throw std::runtime_error("Water building dimensions differ from rules");
            YieldValues result;for(std::size_t i=0;i<v.values.size();++i)if(v.values[i])result[i64(i)]=v.values[i];return result;
        };
        auto real=load("m_paiNumRealBuilding"),free=load("m_paiNumFreeBuilding");
        city.real_buildings=std::move(real);city.free_buildings=std::move(free);city.building_counts_resolved=true;
    }
    std::size_t count=0;for(const auto &[id,p]:cities.players)count+=p.cities.size();
    if(count!=seen.size())throw std::runtime_error("Incomplete water city roster");
}
void reset_plot_water_cache(PlotWaterContext &c){validate(c);for(auto &p:c.map.cells)p.water_cache=saves::PlotWaterCache{};}
bool plot_is_adjacent_to_water(PlotWaterContext &c,std::int64_t id){
    validate(c);for(const auto n:neighbors(c,id))if(water(cell(c,n)))return true;return false;
}
void update_plot_water_flags(PlotWaterContext &c,std::int64_t id){
    validate(c);const auto &p=cell(c,id);
    if(!vp(c))throw std::runtime_error("BNW evaluates water predicates directly, without VP caches");
    (void)cached(c,id);auto &v=*c.map.cells.at(id).water_cache;
    const auto adjacent=neighbors(c,id);
    v.lake=water(p)&&lake(c,p);v.adjacent_land=false;v.adjacent_water=false;
    for(const auto n:adjacent){const auto &other=cell(c,n);
        if(water(p)){if(!water(other)){v.adjacent_land=true;break;}}
        else if(water(other)&&feature(c,other)!=0){v.adjacent_water=true;break;} // VP FEATURE_ICE enum
    }
    v.fresh=river(p);if(water(p)||v.fresh)return;
    auto around=adjacent;around.push_back(id);
    for(const auto n:around){const auto &other=cell(c,n);
        // Deliberately read neighbours' CURRENT lake caches. A later-index
        // lake not yet updated can affect the first pass, as in the source.
        if(cached(c,n).lake||adds(c.rules.features,feature(c,other))||
           adds(c.rules.improvements,type(c,other.improvement_type,other.improvement_type_hash))||
           (other.is_city&&city_adds_water(c,other))){v.fresh=true;return;}
    }
}
bool plot_is_lake(PlotWaterContext &c,std::int64_t id,bool use_cache){
    validate(c);if(!vp(c))return lake(c,cell(c,id));if(!use_cache)update_plot_water_flags(c,id);return cached(c,id).lake;
}
bool plot_is_fresh_water(PlotWaterContext &c,std::int64_t id,bool use_cache){
    validate(c);const auto &p=cell(c,id);
    if(vp(c)){if(!use_cache)update_plot_water_flags(c,id);return cached(c,id).fresh;}
    if(water(p)||c.passability.plots.at(id).impassable||required(p.plot_type,"category")==0)return false;
    if(river(p))return true;
    auto around=neighbors(c,id);around.push_back(id);
    for(const auto n:around){const auto &other=cell(c,n);if(lake(c,other)||adds(c.rules.features,feature(c,other)))return true;}
    return false;
}
bool plot_is_adjacent_to_land(PlotWaterContext &c,std::int64_t id,bool use_cache){
    validate(c);if(vp(c)){if(!use_cache)update_plot_water_flags(c,id);return cached(c,id).adjacent_land;}
    for(const auto n:neighbors(c,id))if(!water(cell(c,n)))return true;return false;
}
bool plot_is_coastal_land(PlotWaterContext &c,std::int64_t id,std::int64_t minimum,bool use_cache,bool canals){
    validate(c);const auto &p=cell(c,id);const auto adjacent=neighbors(c,id);
    if(!vp(c)){
        if(water(p))return false;if(minimum==-1)minimum=define(c,"MIN_WATER_SIZE_FOR_OCEAN");
        for(const auto n:adjacent){const auto &other=cell(c,n);if(water(other)){
            if(minimum<=0)return true;const auto *m=landmass(c,other);if(m&&m->tiles>=minimum)return true;
        }}return false;
    }
    if(!use_cache)update_plot_water_flags(c,id);
    if(!cached(c,id).adjacent_water)return false;
    if(minimum<2)return true; // VP -1 does NOT substitute the ocean define
    for(const auto n:adjacent){const auto &other=cell(c,n);if(water(other)){
        if(feature(c,other)==0&&required(other.owner,"owner")==-1)continue;
        const auto *m=landmass(c,other);if(m&&m->tiles>=minimum)return true;
    }}
    if(!canals||!c.options->enabled("GLOBAL_PASSABLE_FORTS")||water(p))return false;
    std::vector<std::int64_t> reachable{id};std::set<std::int64_t> visited{id};const auto own_team=team(c,p);
    for(std::size_t i=0;i<reachable.size();++i)for(const auto n:neighbors(c,reachable[i])){
        const auto &other=cell(c,n);
        if(water(other)){if(feature(c,other)==0&&required(other.owner,"owner")==-1)continue;}
        else if(team(c,other)==own_team){
            if(!other.is_city){
                if(!other.improvement_passable||!other.improvement_pillaged)throw std::runtime_error("Unresolved canal improvement flags");
                if(!*other.improvement_passable||*other.improvement_pillaged)continue;
            }
            if(!plot_is_adjacent_to_water(c,n))continue;
        }else continue;
        if(visited.insert(n).second){reachable.push_back(n);if(i64(reachable.size()-1)>=minimum)return true;}
    }
    return false;
}
} // namespace civ5::runtime
