#include "civ5/runtime/vp_corporations.hpp"
#include "civ5/runtime/vp_corporation_profile.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/saves/vp_player_knowledge_profile.hpp"

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Corporation ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate corporation field");found=&x;}
    if(!found)throw std::runtime_error("Missing corporation field: "+std::string(name));return *found;}
const VpCorporationProfileState::Player &validate(const VpCorporationContext &c,const CityYieldState &city){
    const auto &g=c.buildings;const auto &w=g.religion.water;const auto &p=g.cities.players.at(city.owner);
    const auto *profile=find_vp_corporation_profile(g.cities);
    if(g.cities.profile!=vp||g.city_rules.profile!=vp||w.players.profile!=vp||&w.cities!=&g.cities||!w.options||!w.options->all_succeeded()||
       w.defines.profile()!=content::GlobalDefineProfile::vp542||&p.cities.at(city.id)!=&city||!profile||!profile->players.contains(city.owner))
        throw CityYieldUnavailable("Corporations require resolved VP profile state");
    return profile->players.at(city.owner);
}
const VpCorporationRules::Corporation *info(const VpCorporationRules &r,std::int64_t id){
    if(id==-1)return nullptr;const auto i=r.corporations.find(id);return i==r.corporations.end()?nullptr:&i->second;
}
}
VpCorporationRules load_vp_corporation_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpCorporationRules r;const auto yields=db.prefetch_info("Yields",[&](auto n,auto,const auto &){r.yields.insert(id64(n));return true;});
    const auto corporations=db.prefetch_info("Corporations",[&](auto n,auto,const auto &row){auto &c=r.corporations[id64(n)];
        const auto type_id=[&](const char *name){const auto text=row.text(name);if(text)if(const auto *t=types.find(*text))return t->id;return std::int64_t{-1};};
        c.headquarters=type_id("HeadquartersBuildingClass");c.office=type_id("OfficeBuildingClass");c.franchise=type_id("FranchiseBuildingClass");
        const auto type=row.text("Type");const auto q=db.query_rows({"select Yields.ID, Yield from Corporation_TradeRouteCityYield inner join Yields on YieldType = Yields.Type where CorporationType = ?",
            {type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{}}},[&](const auto &v){const auto id=v.integer_at(0),amount=v.integer_at(1);
                if(id<0)throw std::runtime_error("Negative corporation yield ID");if(amount)c.trade_city[id]=amount;else c.trade_city.erase(id);});
        if(!q.all_succeeded())throw CityYieldUnavailable("Unresolved corporation city yields: "+q.error);return true;});
    if(!yields.all_succeeded()||!corporations.all_succeeded())throw CityYieldUnavailable("Unresolved corporation Info");return r;
}
void restore_vp_corporations(const saves::SavedSessionSnapshot &s,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout||s.final_systems.layout!=layout||!s.final_systems.vp)
        throw CityYieldUnavailable("Corporation import requires VP state");
    std::map<std::int64_t,VpCorporationProfileState::Player> restored;
    for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;const auto &id=named(sp.core.fields,"m_eID").values;
        if(!world.players.contains(owner)||sp.core.layout!=layout||sp.knowledge.layout!=layout||!saves::vp_player_knowledge_dimensions(sp.knowledge)||id.size()!=1||id[0]!=owner||
           sp.knowledge.start_offset>=sp.knowledge.end_offset)throw std::runtime_error("Corporation player binding mismatch");
        const auto &block=named(sp.knowledge.blocks,"player_corporations");
        if(block.start_offset<sp.knowledge.start_offset||block.end_offset>sp.knowledge.end_offset||block.start_offset>=block.end_offset)
            throw std::runtime_error("Corporation block outside player knowledge span");
        VpCorporationProfileState::Player c;
        const char *names[]={"founded_corporation","offices","franchises","additional_franchises","additional_franchises_modifier","offices_as_franchises",
            "random_foreign_franchise_modifier","free_franchise_above_popular","no_foreign_corporations_in_cities","no_franchises_in_foreign_cities"};
        std::int64_t *values[]={&c.founded,&c.offices,&c.franchises,&c.additional_franchises,&c.additional_franchises_modifier,&c.offices_as_franchises,
            &c.random_foreign_franchise_modifier,&c.free_franchise_above_popular,&c.no_foreign_corporations_in_cities,&c.no_franchises_in_foreign_cities};
        for(std::size_t i=0;i<10;++i)*values[i]=named(block.fields,names[i]).value;
        const auto &a=named(block.arrays,"franchises_per_improvement");const auto count=named(block.fields,"franchises_per_improvement_wire_count").value;
        const auto length=count==0xffffffffLL?0:count; // Original STL empty-vector sentinel, not a host capacity.
        if(a.columns!=1||length<0||static_cast<std::uint64_t>(length)!=a.values.size())throw std::runtime_error("Corporation improvement span mismatch");c.franchises_per_improvement=a.values;
        if(!restored.emplace(owner,std::move(c)).second)throw std::runtime_error("Duplicate corporation player");
    }
    if(restored.size()!=world.players.size())throw std::runtime_error("Incomplete corporation players");
    auto &profile=ensure_vp_corporation_profile(world);
    profile.players=std::move(restored);profile.active=s.final_systems.vp->corporations;
}
const saves::CorporationObservation *vp_active_corporation(const CityYieldContext &world,std::int64_t id){
    const auto *profile=find_vp_corporation_profile(world);
    if(!profile||!profile->active)throw CityYieldUnavailable("Unresolved global corporations");
    for(const auto &c:*profile->active)if(c.type==id)return &c;return nullptr;
}
const CityYieldState *vp_city_at(const CityYieldContext &world,const saves::SavedMapProjection &map,std::int64_t x,std::int64_t y){
    if(world.profile!=vp)throw CityYieldUnavailable("VP city lookup requires a VP world");const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,layout};
    const auto index=geometry.plot(x,y);if(!index)return nullptr;const auto &plot=map.cells.at(static_cast<std::size_t>(*index));
    if(plot.index!=static_cast<std::uint64_t>(*index))throw CityYieldUnavailable("Shared city plot index mismatch");if(!plot.is_city)return nullptr;
    if(!plot.plot_city)throw CityYieldUnavailable("Unresolved shared plot city handle");const auto &ref=*plot.plot_city;
    if(ref.owner==-1&&ref.id==-1)return nullptr;const auto &p=world.players.at(ref.owner);const auto city=p.cities.find(ref.id);
    return city==p.cities.end()?nullptr:&city->second;
}
bool vp_city_is_headquarters(const VpCorporationContext &c,const CityYieldState &city){
    const auto *r=info(c.rules,validate(c,city).founded);return r&&r->headquarters!=-1&&vp_city_has_building_class(c.buildings,city,r->headquarters);
}
bool vp_city_has_office(const VpCorporationContext &c,const CityYieldState &city){
    const auto *r=info(c.rules,validate(c,city).founded);return r&&r->office!=-1&&vp_city_has_building_class(c.buildings,city,r->office);
}
bool vp_city_has_franchise(const VpCorporationContext &c,const CityYieldState &city,std::int64_t id){
    const auto &p=validate(c,city);const auto *r=info(c.rules,id);if(!r)return false;const bool nationalized=p.offices_as_franchises>0;
    const auto cls=nationalized?r->office:r->franchise;if(cls==-1)return false;
    if(nationalized){const auto *active=vp_active_corporation(c.buildings.cities,id);if(active&&city.owner!=active->founder)return false;}
    return vp_city_has_building_class(c.buildings,city,cls);
}
std::int64_t vp_city_corporation_trade_modifier(const VpCorporationContext &c,const CityYieldState &city,std::int64_t y){
    const auto &p=validate(c,city);if(!c.rules.yields.contains(y))throw std::out_of_range("Invalid corporation yield");const auto *r=info(c.rules,p.founded);
    if(!r)return 0;const auto amount=value(r->trade_city,y);if(!amount||!vp_city_has_office(c,city))return 0;
    const auto &world=c.buildings.cities;const auto &map=c.buildings.religion.water.map;std::int64_t result=0;
    for(const auto &route:world.trade_routes){
        if(route.origin_x==-1&&route.origin_y==-1&&route.destination_x==-1&&route.destination_y==-1)continue;
        if(route.origin_owner!=city.owner||vp_city_at(world,map,route.origin_x,route.origin_y)!=&city)continue;
        const auto *dest=vp_city_at(world,map,route.destination_x,route.destination_y);
        if(dest&&vp_city_has_office(c,city)&&vp_city_has_franchise(c,*dest,p.founded))result=wide::add(result,amount);
    }return result;
}
} // namespace civ5::runtime
