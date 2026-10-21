#include "civ5/runtime/vp_city_building_yields.hpp"
#include "civ5/runtime/vp_player_accomplishments_profile.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City building yield ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate city building yield field");found=&x;}
    if(!found)throw std::runtime_error("Missing city building yield field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid city building yield scalar");return a[0];}
std::int64_t row_sum(const YieldModifierRows &rows,std::int64_t yield){
    std::int64_t n=0;if(const auto i=rows.find(yield);i!=rows.end())for(const auto &[id,v]:i->second)n=add(n,v);return n;
}
}
VpCityBuildingYieldRules load_vp_city_building_yield_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpCityBuildingYieldRules r;
    const auto load=[&](const char *table,const auto &consume){const auto q=db.prefetch_info(table,consume);if(!q.all_succeeded())throw std::runtime_error("City building yield Info: "+q.error);return q.slots;};
    load("Yields",[&](auto id,auto,const auto &){r.yields.insert(id64(id));return true;});
    load("Beliefs",[&](auto id,auto,const auto &row){r.faith_building_tourism[id64(id)]=row.integer("FaithBuildingTourism");return true;});
    r.building_slots=load("Buildings",[&](auto id,auto,const auto &row){auto &b=r.buildings[id64(id)];const auto policy=row.text("PolicyType");const auto *info=policy?types.find(*policy):nullptr;
        b.faith_only=row.integer("FaithCost")>0&&row.integer("Cost")<=-1&&(row.boolean("UnlockedByBelief")||(info&&info->id!=-1));
        const auto type=row.text("Type");const content::DatabaseValue param=type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{};
        const auto q=db.query_rows({"select Accomplishments.ID, Yields.ID as YieldID, Yield from Building_YieldChangesFromAccomplishments left join Accomplishments on Accomplishments.Type = AccomplishmentType inner join Yields on Yields.Type = YieldType where BuildingType = ?",{param}},[&](const auto &v){
            // GetInt(NULL) is zero in the original LEFT JOIN. Duplicate rows add,
            // including cancellations to zero; retain the resulting map entry.
            auto &n=b.accomplishments[v.integer_at(0)][v.integer_at(1)];n=add(n,v.integer_at(2));});
        if(!q.all_succeeded())throw std::runtime_error("Building accomplishment relation: "+q.error);return true;});return r;
}
void restore_vp_city_building_yields(const saves::SavedSessionSnapshot &s,const VpCityBuildingYieldRules &r,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw std::runtime_error("City building yield import requires VP world");
    const auto vector=[&](const auto &fields,const char *name){const auto &v=named(fields,name).values;YieldValues result;
        if(v.size()!=world.yield_slots)throw std::runtime_error("City building yield wire span mismatch");for(std::size_t i=0;i<v.size();++i)if(v[i])result[id64(i)]=v[i];return result;};
    std::set<std::int64_t> players;auto &extension=ensure_vp_city_building_yield_profile(world);
    for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;auto &p=world.players.at(owner);
        if(!players.insert(owner).second||sp.core.layout!=layout||sp.tail.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner)throw std::runtime_error("City building yield player binding mismatch");
        auto &pg=extension.players[owner];pg.strategic_from_minors=scalar(sp.core.fields,"m_iNumStrategicResourcesFromMinors");pg.per_religion_times100=vector(sp.tail.fields,"m_piYieldChangesPerReligion");
        std::set<std::int64_t> cities;
        for(const auto &sc:sp.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||sc.production.layout!=layout||sc.tail.layout!=layout||city.owner!=owner||city.id!=id||scalar(sc.core.fields,"m_eOwner")!=owner||
               city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}}||saves::vp_city_production_building_slots(sc.production.context)!=r.building_slots)throw std::runtime_error("City building yield city binding mismatch");
            auto &a=extension.cities[{owner,id}];a.buildings=named(named(sc.production.blocks,"buildings").fields,"m_iNumBuildings").value;
            const auto &plots=named(sc.core.fields,"m_siPlots").values;a.plots.insert(plots.begin(),plots.end());
            a.era_scaling_times100=vector(sc.core.fields,"m_aiYieldRateFromBuildingsEraScalingTimes100");a.passing_trade=vector(sc.core.fields,"m_aiYieldFromPassingTR");
            const char *names[]={"m_afYieldPerBuilding","m_afYieldPerTile","m_afYieldPerCityStateStrategicResource"};
            for(std::size_t k=0;k<3;++k){const auto &field=named(sc.core.fields,names[k]);const auto &v=field.values;
                if(field.encoding!=saves::CityCoreEncoding::vector_fraction||v.size()%2||v.size()/2!=world.yield_slots)throw std::runtime_error("Invalid city building yield fractions");
                for(std::size_t y=0;y<world.yield_slots;++y){YieldFraction f{v[y*2],v[y*2+1]};if(f.numerator())a.fractions[k].emplace(id64(y),f);}}
            const auto base=vector(sc.core.fields,"m_aiBaseYieldRateFromBuildings"),religions=vector(sc.core.fields,"m_aiYieldPerReligion");
            for(std::uint64_t y=0;y<world.yield_slots;++y){auto &v=city.yields[id64(y)];v.buildings=value(base,id64(y));v.per_religion=value(religions,id64(y));}
            const auto &rows=named(named(sc.tail.blocks,"remainder").children,"m_yieldChanges").children;
            if(rows.size()!=world.yield_slots)throw std::runtime_error("City building yield row span mismatch");
            for(std::size_t y=0;y<rows.size();++y){const char *fields[]={"forXTerrain","forXFeature"};YieldModifierRows *outputs[]={&a.per_x_terrain,&a.per_x_feature};
                for(std::size_t k=0;k<2;++k){const auto &v=named(rows[y].children,fields[k]).values;if(v.size()%2)throw std::runtime_error("Invalid city building yield pairs");
                    auto &dest=(*outputs[k])[id64(y)];for(std::size_t i=0;i<v.size();i+=2)dest.emplace_back(v[i],v[i+1]);}}
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete city building yield cities");
    }if(players.size()!=world.players.size())throw std::runtime_error("Incomplete city building yield players");
}
VpCityBuildingYieldQuote vp_city_building_yield(const VpCityBuildingYieldContext &c,const CityYieldState &city,std::int64_t yield){
    const auto &g=c.great_works;const auto &world=g.cities;const auto &water=g.religion.water;const auto &p=world.players.at(city.owner);
    if(world.profile!=vp||&water.cities!=&world||&p.cities.at(city.id)!=&city||!vp_city_majority_religion(city))
        throw CityYieldUnavailable("Building income requires shared resolved VP state");
    if(!c.rules.yields.contains(yield))throw std::out_of_range("Invalid city building yield");
    const auto &accomplishments=vp_player_accomplishments(world,city.owner);
    const auto &a=vp_city_building_yield_city(world,city.owner,city.id);const auto &pg=vp_city_building_yield_player(world,city.owner);VpCityBuildingYieldQuote q;const auto it=city.yields.find(yield);
    const auto base=it==city.yields.end()?CityYieldComponents{}:it->second;
    q.base=mul(base.buildings,100);const auto era=c.team_eras.at(water.players.players.at(city.owner).team);q.era=mul(value(a.era_scaling_times100,yield),std::max<std::int64_t>(1,era));
    const auto fraction=[&](std::size_t k,std::int64_t count){const auto i=a.fractions[k].find(yield);return i==a.fractions[k].end()?0:i->second.scaled_truncate(count,100);};
    q.per_building=fraction(0,a.buildings);q.per_tile=fraction(1,id64(a.plots.size()));q.minor_resources=fraction(2,pg.strategic_from_minors);
    MapGeometry geometry{water.map.grid,water.map.wrap_x,water.map.wrap_y,layout};const auto index=geometry.plot(city.x,city.y);if(!index)throw CityYieldUnavailable("Invalid building-income city plot");
    const auto &plot=water.map.cells.at(*index);if(!plot.trade_route_units)throw CityYieldUnavailable("Unresolved city trade-unit route count");
    if(*plot.trade_route_units>0)q.passing_trade=mul(value(a.passing_trade,yield),100);
    q.terrain=row_sum(a.per_x_terrain,yield);q.features=row_sum(a.per_x_feature,yield);
    for(const auto &[bid,b]:c.rules.buildings)if(!b.accomplishments.empty()){
        const auto num=vp_city_building_count(g,city,bid);if(num<=0)continue;
        for(const auto &[accomplishment,values]:b.accomplishments){const auto n=value(accomplishments,accomplishment),bonus=value(values,yield);
            if(n>0&&bonus)q.accomplishments=add(q.accomplishments,mul(mul(mul(num,n),bonus),100));}
    }
    if(yield==g.rules.tourism)if(const auto *religion=vp_find_religion(world,*vp_city_majority_religion(city),city.owner)){
        const auto &profile=vp_religion_beliefs(*religion);std::int64_t bonus=0;
        for(auto bid:religion->beliefs){const auto n=value(c.rules.faith_building_tourism,bid);if(n&&vp_plot_belief_valid(g.religion,bid,profile.belief_religion,city.owner,&city))bonus=add(bonus,n);}
        if(bonus){std::int64_t buildings=0;for(auto bid:vp_great_work_city(world,city.owner,city.id).existing_buildings)if(const auto b=c.rules.buildings.find(bid);b!=c.rules.buildings.end()&&b->second.faith_only)buildings=add(buildings,1);q.faith=mul(mul(buildings,bonus),100);}
    }
    const auto per_religion=add(base.per_religion,value(pg.per_religion_times100,yield));
    if(per_religion){std::int64_t count=0;for(const auto &r:city.religions)if(r.followers>0&&r.religion>world.pantheon)count=add(count,1);q.religions=mul(per_religion,count);}
    const auto local=value(vp_great_work_city(world,city.owner,city.id).per_local_theme,yield);if(local>0)q.local_themes=mul(mul(local,vp_themed_buildings(g,city)),100);
    for(auto n:{q.base,q.era,q.per_building,q.per_tile,q.passing_trade,q.terrain,q.features,q.minor_resources,q.accomplishments,q.faith,q.religions,q.local_themes})q.total_times100=add(q.total_times100,n);
    return q;
}
} // namespace civ5::runtime
