#include <climits>
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_nature_yield.hpp"
#include "civ5/runtime/plot_religion.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Nature yield ID exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){
    const T *found=nullptr;for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate nature yield input");found=&x;}
    if(!found)throw std::runtime_error("Missing nature yield input: "+std::string(name));return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,const char *name){const auto &a=named(v,name).values;
    if(a.size()!=1)throw std::runtime_error("Invalid nature yield scalar");return a[0];}
std::int64_t value(const YieldValues &v,std::int64_t key){const auto it=v.find(key);return it==v.end()?0:it->second;}
std::int64_t value(const NatureYieldMatrix &v,std::int64_t a,std::int64_t b){const auto it=v.find(a);return it==v.end()?0:value(it->second,b);}
std::int64_t required(const std::optional<std::int64_t> &v){if(!v)throw std::runtime_error("Unresolved nature yield input");return *v;}
void assign(YieldValues &v,std::int64_t y,std::int64_t amount){if(y<0)throw std::runtime_error("Negative nature yield relation ID");if(amount)v[y]=amount;else v.erase(y);}
std::int64_t resolve(const BnwNatureYieldContext &c,const auto &id,const auto &hash){
    if(id&&hash)throw std::runtime_error("Ambiguous nature content identity");if(id)return *id;
    if(hash){if(!*hash)return -1;if(const auto *t=c.water.types.find_hash(*hash))return t->id;}
    throw std::runtime_error("Unresolved nature content identity");
}
void validate(const BnwNatureYieldContext &c){
    using content::TechnologyProfile;const auto &w=c.water;
    if(w.rules.profile!=TechnologyProfile::bnw||w.players.profile!=TechnologyProfile::bnw||w.cities.profile!=TechnologyProfile::bnw||
       w.state.layout!=saves::GameStateLayout::expansion2_version1||w.passability.layout!=w.state.layout||
       c.resources.rules.profile!=TechnologyProfile::bnw||&c.resources.map!=&w.map||&c.resources.players!=&w.players||
       &c.resources.types!=&w.types||&c.resources.defines!=&w.defines||w.map.cells.size()!=w.map.grid.size()||w.passability.plots.size()!=w.map.cells.size())
        throw std::runtime_error("BNW nature yield requires one shared profiled world");
}
const CityYieldState *working_city(const BnwNatureYieldContext &c,const saves::SavedMapCellProjection &p){
    if(!p.yield_city)throw std::runtime_error("Unresolved nature working-city reference");
    if(c.current_city_registry){
        const auto owner=p.yield_city->owner,id=p.yield_city->id;
        if(owner<INT32_MIN||owner>INT32_MAX||id<INT32_MIN||id>INT32_MAX)
            throw std::runtime_error("Nature city handle exceeds native width");
        return resolve_bnw_city_yield_registry(*c.current_city_registry,c.water.cities,
            static_cast<std::int32_t>(owner),static_cast<std::int32_t>(id));
    }
    const auto player=c.water.cities.players.find(p.yield_city->owner);if(player==c.water.cities.players.end())return nullptr;
    const auto city=player->second.cities.find(p.yield_city->id);return city==player->second.cities.end()?nullptr:&city->second;
}
}
BnwNatureYieldRules load_bnw_nature_yield_rules(const content::RulesDatabase &db){
    BnwNatureYieldRules rules;
    const auto load=[&](const char *table,const auto &consume){const auto r=db.prefetch_info(table,consume);
        if(!r.all_succeeded())throw std::runtime_error(std::string("Nature Info ")+table+": "+r.error);return r.slots;};
    const auto yields=[&](const char *table,const char *key,const auto &type,YieldValues &out){
        const auto sql=std::string("select Yields.ID, Yield from ")+table+" inner join Yields on YieldType = Yields.Type where "+key+" = ?";
        const content::DatabaseValue param=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
        const auto r=db.query_rows({sql,{param}},[&](const auto &row){assign(out,row.integer_at(0),row.integer_at(1));});
        if(!r.all_succeeded())throw std::runtime_error("Nature yield relation: "+r.error);
    };
    const auto matrix=[&](const char *table,const char *target,const char *column,const auto &type,NatureYieldMatrix &out){
        const auto sql=std::string("select ")+target+".ID, Yields.ID, Yield from "+table+" inner join "+target+" on "+target+".Type = "+column+" inner join Yields on Yields.Type = YieldType where BeliefType = ?";
        const content::DatabaseValue param=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
        const auto r=db.query_rows({sql,{param}},[&](const auto &row){const auto id=row.integer_at(0);if(id<0)throw std::runtime_error("Negative belief nature relation ID");assign(out[id],row.integer_at(1),row.integer_at(2));});
        if(!r.all_succeeded())throw std::runtime_error("Nature belief relation: "+r.error);
    };
    rules.yield_slots=load("Yields",[&](auto slot,auto,const auto &row){rules.yields[i64(slot)]={row.integer("HillsChange"),row.integer("MountainChange"),row.integer("LakeChange")};return true;});
    load("Terrains",[&](auto slot,auto,const auto &row){auto &t=rules.terrains[i64(slot)];const auto type=row.text("Type");
        yields("Terrain_Yields","TerrainType",type,t.base);yields("Terrain_RiverYieldChanges","TerrainType",type,t.river);yields("Terrain_HillsYieldChanges","TerrainType",type,t.hills);return true;});
    rules.feature_slots=load("Features",[&](auto slot,auto,const auto &row){auto &f=rules.features[i64(slot)];const auto type=row.text("Type");
        f.nonadditive=row.boolean("YieldNotAdditive");f.natural_wonder=row.boolean("NaturalWonder");
        yields("Feature_YieldChanges","FeatureType",type,f.base);yields("Feature_RiverYieldChanges","FeatureType",type,f.river);yields("Feature_HillsYieldChanges","FeatureType",type,f.hills);return true;});
    load("Resources",[&](auto slot,auto,const auto &row){yields("Resource_YieldChanges","ResourceType",row.text("Type"),rules.resources[i64(slot)]);return true;});
    load("Beliefs",[&](auto slot,auto,const auto &row){auto &b=rules.beliefs[i64(slot)];const auto type=row.text("Type");b.pantheon=row.integer("Pantheon")!=0;
        matrix("Belief_TerrainYieldChanges","Terrains","TerrainType",type,b.terrains);
        matrix("Belief_FeatureYieldChanges","Features","FeatureType",type,b.features);
        matrix("Belief_ResourceYieldChanges","Resources","ResourceType",type,b.resources);
        yields("Belief_YieldChangeNaturalWonder","BeliefType",type,b.natural_wonder);
        yields("Belief_YieldModifierNaturalWonder","BeliefType",type,b.natural_wonder_modifier);return true;});
    return rules;
}
BnwNatureYieldState restore_bnw_nature_yields(const saves::SavedSessionSnapshot &s,const BnwNatureYieldRules &rules,CityYieldContext &cities){
    constexpr auto layout=saves::GameStateLayout::expansion2_version1;
    if(!s.all_players||s.all_players->layout!=layout||s.leagues.layout!=layout||cities.profile!=content::TechnologyProfile::bnw||
       !rules.yield_slots||rules.yield_slots!=cities.yield_slots)throw std::runtime_error("BNW nature import profile/width mismatch");
    BnwNatureYieldState state;
    for(const auto &source:s.all_players->players){const auto id=source.saved_player_id;
        if(!cities.players.contains(id)||state.players.contains(id)||source.traits.layout!=layout||source.core.layout!=layout)throw std::runtime_error("BNW nature player mismatch");
        auto &p=state.players[id];const auto &unimproved=named(source.traits.fields,"m_ppaaiUnimprovedFeatureYieldChange");
        if(unimproved.columns!=rules.yield_slots||unimproved.values.size()%rules.yield_slots||unimproved.values.size()/rules.yield_slots!=rules.feature_slots)
            throw std::runtime_error("Nature trait matrix differs from active rules");
        for(std::size_t i=0;i<unimproved.values.size();++i)if(unimproved.values[i])p.unimproved_features[i64(i/rules.yield_slots)][i64(i%rules.yield_slots)]=unimproved.values[i];
        const auto &wonder=named(source.traits.fields,"m_iYieldChangeNaturalWonder").values;
        if(wonder.size()!=rules.yield_slots)throw std::runtime_error("Nature trait yield width differs from rules");
        for(std::size_t i=0;i<wonder.size();++i)if(wonder[i])p.natural_wonder[i64(i)]=wonder[i];
        p.natural_wonder_modifier=scalar(source.traits.fields,"m_iNaturalWonderYieldModifier");
        cities.players.at(id).second_pantheon_count=scalar(source.core.fields,"m_iSecondReligionPantheonCount");
    }
    if(state.players.size()!=cities.players.size())throw std::runtime_error("Incomplete nature player roster");
    for(const auto &source:s.leagues.leagues){BnwNatureYieldState::League league;
        for(const auto &member:source.members)league.members.push_back(member.player);
        for(const auto &resolution:source.active_resolutions)league.culture_per_natural_wonder.push_back(named(resolution.effects.integers,"culture_per_natural_wonder").value);
        state.leagues.push_back(std::move(league));
    }
    return state;
}
BnwNatureYieldQuote bnw_nature_yield(BnwNatureYieldContext &c,std::int64_t plot,std::int64_t yield,std::int64_t team,bool ignore){
    validate(c);if(plot<0)throw std::out_of_range("Negative nature plot ID");const auto &p=c.water.map.cells.at(static_cast<std::size_t>(plot));const auto xy=c.water.map.grid.coordinates(static_cast<std::uint64_t>(plot));
    if(p.index!=static_cast<std::uint64_t>(plot)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("Nature plot identity mismatch");
    BnwNatureYieldQuote q;const auto feature_id=resolve(c,p.feature_type,p.feature_type_hash);
    const auto *actual_feature=feature_id==-1?nullptr:&c.rules.features.at(feature_id);
    const auto plot_type=required(p.plot_type);
    if((c.water.passability.plots.at(static_cast<std::size_t>(plot)).impassable||plot_type==0)&&(!actual_feature||!actual_feature->natural_wonder)){q.suppressed=true;return q;}
    const auto &y=c.rules.yields.at(yield);const auto terrain=required(p.terrain_type);const auto &t=c.rules.terrains.at(terrain);
    const auto *city=working_city(c,p);
    const auto beliefs=city?bnw_plot_beliefs(c.water.cities,*city,c.rules.beliefs):std::vector<const BnwNatureYieldRules::Belief*>{};
    const auto religion=[&](const auto &get){std::int64_t result=0;for(const auto *b:beliefs)result=wide::add(result,get(*b));return result;};
    q.terrain=value(t.base,yield);q.religion_terrain=religion([&](const auto &b){return value(b.terrains,terrain,yield);});
    q.total=wide::add(q.terrain,q.religion_terrain);
    if(plot_type==1)q.plot_hills=y.hills;if(plot_type==0)q.mountain=y.mountain;if(plot_is_lake(c.water,plot))q.lake=y.lake;
    for(const auto n:{q.plot_hills,q.mountain,q.lake})q.total=wide::add(q.total,n);
    const auto *f=ignore?nullptr:actual_feature;
    if(f){q.feature=value(f->base,yield);const auto owner=required(p.owner);
        if(owner!=-1&&resolve(c,p.improvement_type,p.improvement_type_hash)==-1)q.trait_feature=value(c.state.players.at(owner).unimproved_features,feature_id,yield);
        if(city&&f->natural_wonder&&yield==c.culture)for(const auto &league:c.state.leagues)
            if(std::find(league.members.begin(),league.members.end(),city->owner)!=league.members.end()){
                std::int64_t sum=0;for(const auto n:league.culture_per_natural_wonder)sum=wide::add(sum,n);q.league=wide::add(q.league,sum);}
        q.religion_feature=religion([&](const auto &b){return value(b.features,feature_id,yield);});
        q.feature_total=wide::add(wide::add(wide::add(q.feature,q.trait_feature),q.league),q.religion_feature);
        if(owner!=-1&&f->natural_wonder){
            q.religion_wonder=religion([&](const auto &b){return value(b.natural_wonder,yield);});
            q.feature_total=wide::add(q.feature_total,q.religion_wonder);
            q.wonder_modifier=religion([&](const auto &b){return value(b.natural_wonder_modifier,yield);});
            const auto &traits=c.state.players.at(owner);q.trait_wonder=value(traits.natural_wonder,yield);q.feature_total=wide::add(q.feature_total,q.trait_wonder);
            q.wonder_modifier=wide::add(q.wonder_modifier,traits.natural_wonder_modifier);
            if(q.wonder_modifier>0)q.feature_total=wide::multiply_divide(q.feature_total,wide::add(100,q.wonder_modifier),100);
        }
        q.replaced=f->nonadditive;q.total=q.replaced?q.feature_total:wide::add(q.total,q.feature_total);
    }
    if(team!=-1){const auto resource=plot_resource_type(c.resources,plot,team);if(resource!=-1){q.resource=value(c.rules.resources.at(resource),yield);
        q.total=wide::add(q.total,q.resource);q.religion_resource=religion([&](const auto &b){return value(b.resources,resource,yield);});q.total=wide::add(q.total,q.religion_resource);}}
    if(!p.rivers)throw std::runtime_error("Unresolved nature river count");
    if(p.rivers->is_river)q.river=value(f?f->river:t.river,yield);
    if(plot_type==1)q.hills=value(f?f->hills:t.hills,yield);
    q.total=std::max<std::int64_t>(0,wide::add(wide::add(q.total,q.river),q.hills));return q;
}
std::int64_t bnw_best_nature_yield(BnwNatureYieldContext &c,std::int64_t plot,std::int64_t yield,std::int64_t team){
    return std::max(bnw_nature_yield(c,plot,yield,team,false).total,bnw_nature_yield(c,plot,yield,team,true).total);
}
} // namespace civ5::runtime
