#include <climits>
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/runtime/plot_religion.hpp"
#include "civ5/runtime/vp_improvement_yield_profile.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Improvement calculation ID exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){
    const T *found=nullptr;for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate improvement input");found=&x;}
    if(!found)throw std::runtime_error("Missing improvement input: "+std::string(name));return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,const char *name){const auto &a=named(v,name).values;
    if(a.size()!=1)throw std::runtime_error("Invalid improvement scalar");return a[0];}
std::int64_t value(const YieldValues &v,std::int64_t key){const auto it=v.find(key);return it==v.end()?0:it->second;}
std::int64_t value(const ImprovementYieldMatrix &m,std::int64_t a,std::int64_t b){const auto it=m.find(a);return it==m.end()?0:value(it->second,b);}
std::int64_t required(const std::optional<std::int64_t> &v){if(!v)throw std::runtime_error("Unresolved shared improvement input");return *v;}
std::int64_t resolve(const ImprovementYieldContext &c,const auto &id,const auto &hash){
    if(id&&hash)throw std::runtime_error("Ambiguous improvement content identity");if(id)return *id;
    if(hash){if(!*hash)return -1;if(const auto *t=c.water.types.find_hash(*hash))return t->id;}
    throw std::runtime_error("Unresolved improvement content identity");
}
void validate(const ImprovementYieldContext &c,bool vp){
    const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
    if(c.rules.profile!=profile||c.state.profile!=profile||c.water.rules.profile!=profile||c.water.cities.profile!=profile||
       c.resources.rules.profile!=profile||&c.resources.map!=&c.water.map||&c.resources.players!=&c.water.players||
       &c.resources.types!=&c.water.types||&c.resources.defines!=&c.water.defines)
        throw std::runtime_error("Improvement calculation must use one shared profiled world");
}
MapGeometry geometry(const ImprovementYieldContext &c){const auto &m=c.water.map;
    return {m.grid,m.wrap_x,m.wrap_y,c.rules.profile==TechnologyProfile::vp542?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1};}
const saves::SavedMapCellProjection &cell(const ImprovementYieldContext &c,std::int64_t id){
    const auto &m=c.water.map;if(id<0||m.cells.size()!=m.grid.size())throw std::runtime_error("Invalid improvement calculation map");
    const auto &p=m.cells.at(static_cast<std::size_t>(id));const auto xy=m.grid.coordinates(static_cast<std::uint64_t>(id));
    if(p.index!=static_cast<std::uint64_t>(id)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("Improvement calculation cell identity mismatch");return p;
}
std::vector<std::int64_t> neighbors(const ImprovementYieldContext &c,std::int64_t id){const auto &p=cell(c,id);const auto g=geometry(c);
    std::vector<std::int64_t> r;for(int d=0;d<6;++d)if(const auto n=g.direction(i64(p.x),i64(p.y),d))r.push_back(*n);return r;}
const CityYieldState *city(const ImprovementYieldContext &c,const std::optional<saves::SavedPlotCityReference> &ref){
    if(!ref)throw std::runtime_error("Unresolved improvement city reference");
    if(c.current_city_registry&&c.rules.profile==TechnologyProfile::bnw){
        if(ref->owner<INT32_MIN||ref->owner>INT32_MAX||ref->id<INT32_MIN||ref->id>INT32_MAX)
            throw std::runtime_error("Improvement city handle exceeds native width");
        return resolve_bnw_city_yield_registry(*c.current_city_registry,c.water.cities,
            static_cast<std::int32_t>(ref->owner),static_cast<std::int32_t>(ref->id));
    }
    if(ref->owner<0)return nullptr;
    const auto player=c.water.cities.players.find(ref->owner);if(player==c.water.cities.players.end())return nullptr;
    const auto found=player->second.cities.find(ref->id);return found==player->second.cities.end()?nullptr:&found->second;
}
bool riverside(const ImprovementYieldContext &c,std::int64_t id){
    const auto &p=cell(c,id);const auto bits=static_cast<std::uint64_t>(required(p.river_crossing_bits));const auto &m=c.water.map;
    const auto wrap=[](std::int64_t n,std::int64_t size,bool enabled){if(enabled){if(n>size/2)return wide::subtract(n,size);if(n<-size/2)return wide::add(n,size);}return n;};
    // BNW directionXY uses wrapped hex-space differences, even on tiny maps
    // where multiple direction slots may resolve to the same neighbor.
    for(const auto n:neighbors(c,id)){const auto &q=cell(c,n);
        const auto dx=wrap(wide::subtract(wide::subtract(i64(q.x),i64(q.y)/2),wide::subtract(i64(p.x),i64(p.y)/2)),i64(m.grid.width),m.wrap_x);
        const auto dy=wrap(wide::subtract(i64(q.y),i64(p.y)),i64(m.grid.height),m.wrap_y);
        const auto d=dy>0?(dx>=0?0:5):dy<0?(dx>0?2:3):dx>0?1:dx<0?4:-1;
        if(d!=-1&&(bits&(std::uint64_t{1}<<d)))return true;
    }return false;
}
std::int64_t era(const ImprovementYieldContext &c,std::int64_t player){return c.team_eras.at(c.water.players.players.at(player).team);}
void era_yield(ImprovementYieldQuote &q,const ImprovementYieldContext &c,const ImprovementYieldInfo &info,std::int64_t plot,std::int64_t player,std::int64_t yield){
    const auto per=value(info.yields[static_cast<std::size_t>(ImprovementYieldKind::per_era)],yield);
    if(player!=-1&&per>0){const auto ancient=required(cell(c,plot).artifact_era);const auto difference=wide::subtract(era(c,player),ancient);
        if(ancient!=-1&&difference>0)q.era=wide::multiply_divide(difference,per,1);}
}
void geography(ImprovementYieldQuote &q,ImprovementYieldContext &c,const ImprovementYieldInfo &info,std::int64_t plot,std::int64_t yield,bool vp){
    const auto &p=cell(c,plot);const auto get=[&](ImprovementYieldKind k){return value(info.yields[static_cast<std::size_t>(k)],yield);};
    bool river=false;if(vp){if(!p.rivers)throw std::runtime_error("Unresolved improvement river count");river=p.rivers->is_river;}else river=riverside(c,plot);
    if(river)q.river=get(ImprovementYieldKind::river);
    if(plot_is_coastal_land(c.water,plot))q.coast=get(ImprovementYieldKind::coastal);
    if(required(p.plot_type)==1)q.hills=get(ImprovementYieldKind::hills);
    const auto adjacent_city=get(ImprovementYieldKind::adjacent_city),mountain=get(ImprovementYieldKind::adjacent_mountain);
    if(adjacent_city>0||mountain>0)for(const auto n:neighbors(c,plot)){const auto &other=cell(c,n);
        // BNW isCity() resolves getPlotCity(); only VP serializes a city flag.
        const auto *neighbor=(!vp||other.is_city)?city(c,other.plot_city):nullptr;
        const bool is_city=vp?other.is_city:neighbor!=nullptr;
        if(is_city){if(!neighbor)throw std::runtime_error("City plot has no live city");
            if(neighbor->owner==required(p.owner))q.adjacent_city=wide::add(q.adjacent_city,adjacent_city);}
        if((vp||!is_city)&&(required(other.plot_type)==0||(vp&&required(other.terrain_type)==7)))q.adjacent_mountain=wide::add(q.adjacent_mountain,mountain);
    }
}
void all_tech_policies(ImprovementYieldQuote &q,const ImprovementYieldContext &c,const ImprovementYieldInfo &info,std::int64_t improvement,std::int64_t yield,bool fresh){
    std::set<std::int64_t> ids;for(const auto &[id,row]:info.technologies)ids.insert(id);
    const auto &water=fresh?info.fresh_technologies:info.dry_technologies;for(const auto &[id,row]:water)ids.insert(id);
    for(const auto id:ids)if(id>=0&&static_cast<std::uint64_t>(id)<c.rules.technology_slots){q.technologies=wide::add(q.technologies,value(info.technologies,id,yield));q.technologies=wide::add(q.technologies,value(water,id,yield));}
    for(const auto &[id,policy]:c.rules.policies)if(id>=0&&static_cast<std::uint64_t>(id)<c.rules.policy_slots)q.policies=wide::add(q.policies,value(policy,improvement,yield));
}
std::int64_t best_route(const ImprovementYieldContext &c,const ImprovementYieldInfo &info,std::int64_t yield){
    std::int64_t best=0;for(const auto &[id,route]:info.routes)if(id>=0&&static_cast<std::uint64_t>(id)<c.rules.route_slots)best=std::max(best,value(route,yield));return best;
}
std::int64_t religious_yield(const ImprovementYieldContext &c,const CityYieldState &city,std::int64_t improvement,std::int64_t yield){
    if(!c.religion)throw std::runtime_error("BNW improvement formula needs belief rules");
    std::int64_t result=0;
    for(const auto *belief:bnw_plot_beliefs(c.water.cities,city,c.religion->beliefs))
        result=wide::add(result,value(belief->improvements,improvement,yield));
    return result;
}
}
ImprovementReligionRules load_improvement_religion_rules(const content::RulesDatabase &db){
    ImprovementReligionRules rules;
    const auto r=db.prefetch_info("Beliefs",[&](auto slot,auto,const auto &row){auto &b=rules.beliefs[i64(slot)];b.pantheon=row.integer("Pantheon")!=0;
        const auto type=row.text("Type");const content::DatabaseValue param=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
        const auto result=db.query_rows({"select Improvements.ID, Yields.ID, Yield from Belief_ImprovementYieldChanges inner join Improvements on Improvements.Type = ImprovementType inner join Yields on Yields.Type = YieldType where BeliefType = ?",{param}},[&](const auto &linked){
            const auto improvement=linked.integer_at(0),yield=linked.integer_at(1),amount=linked.integer_at(2);
            if(improvement<0||yield<0)throw std::runtime_error("Negative belief yield relation ID");b.improvements[improvement][yield]=amount;});
        if(!result.all_succeeded())throw std::runtime_error("Belief improvement yields: "+result.error);return true;});
    if(!r.all_succeeded())throw std::runtime_error("Belief Info: "+r.error);return rules;
}
ImprovementYieldState restore_improvement_yield_state(const saves::SavedSessionSnapshot &snapshot,const ImprovementYieldRules &rules,const content::InfoTypeCatalog &types,CityYieldContext &cities){
    const bool vp=rules.profile==TechnologyProfile::vp542;const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snapshot.teams||!snapshot.all_players||snapshot.teams->layout!=layout||snapshot.all_players->layout!=layout||cities.profile!=rules.profile||!types.report().all_succeeded())throw std::runtime_error("Improvement state profile mismatch");
    ImprovementYieldState state;state.profile=rules.profile;const auto width=cities.yield_slots;
    const auto matrix=[&](const auto &values,std::uint64_t columns){
        if(!width||columns!=width||values.size()%width||values.size()/width!=rules.improvement_slots)throw std::runtime_error("Improvement matrix differs from active rules");
        ImprovementYieldMatrix result;for(std::size_t i=0;i<values.size();++i)if(values[i])result[i64(i/width)][i64(i%width)]=values[i];return result;};
    for(const auto &source:snapshot.teams->teams){const auto id=named(source.fields,"id").value;if(state.teams.contains(id))throw std::runtime_error("Duplicate improvement team");auto &t=state.teams[id];
        if(vp){const auto &a=named(source.arrays,"improvement_yields");
            if(!width||width>UINT64_MAX/3||a.columns!=3*width||a.values.size()%a.columns||a.values.size()/a.columns!=rules.improvement_slots)throw std::runtime_error("Invalid VP team improvement matrices");
            for(std::size_t i=0;i<a.values.size();++i)if(a.values[i])t[(i/width)%3][i64(i/a.columns)][i64(i%width)]=a.values[i];
        }else{const char *names[]={"improvement_yields","improvement_dry_yields","improvement_fresh_yields"};
            for(std::size_t kind=0;kind<3;++kind)for(const auto &row:named(source.hashes,names[kind]).rows){if(!row.type_hash){if(!row.values.empty())throw std::runtime_error("Improvement hash sentinel has values");continue;}
                const auto *type=types.find_hash(row.type_hash);if(!type||type->id<0||static_cast<std::uint64_t>(type->id)>=rules.improvement_slots||row.values.size()!=width)throw std::runtime_error("Unresolved team improvement hash/row");
                auto &yields=t[kind][type->id];yields.clear();for(std::size_t y=0;y<row.values.size();++y)if(row.values[y])yields[i64(y)]=row.values[y];}
        }
    }
    for(const auto &source:snapshot.all_players->players){const auto id=source.saved_player_id;
        if(!cities.players.contains(id)||state.players.contains(id)||source.traits.layout!=layout||source.core.layout!=layout)throw std::runtime_error("Improvement saved player mismatch");
        auto &p=state.players[id];const auto &traits=named(source.traits.fields,"m_ppaaiImprovementYieldChange");p.traits=matrix(traits.values,traits.columns);
        if(vp){const auto &own=named(source.core.fields,"m_ppaaiImprovementYieldChange");p.own=matrix(own.values,source.core.yield_slots);
            if(source.planning.layout!=layout)throw std::runtime_error("Improvement planning profile mismatch");const auto &connections=named(source.planning.blocks,"city_connections");
            const auto &a=named(connections.arrays,"plots_with_connection_to_capital"),&b=named(connections.arrays,"plots_with_industrial_connection_to_capital");
            if(a.columns!=1||b.columns!=1)throw std::runtime_error("Invalid saved connection vectors");
            auto &player_connections=ensure_vp_improvement_yield_profile(state).players[id];
            player_connections.connections=a.values;player_connections.industrial_connections=b.values;
        }else{if(source.administration.layout!=layout)throw std::runtime_error("Improvement administration profile mismatch");
            const auto &a=named(named(source.administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange");p.own=matrix(a.values,a.columns);}
        auto &player=cities.players.at(id);player.second_pantheon_count=scalar(source.core.fields,"m_iSecondReligionPantheonCount");
        for(const auto &source_city:source.cities.cities){const auto city_id=scalar(source_city.core.fields,"m_iID");
            player.cities.at(city_id).we_love_king_turns=scalar(source_city.core.fields,"m_iWeLoveTheKingDayCounter");}
    }
    if(state.players.size()!=cities.players.size())throw std::runtime_error("Incomplete improvement player state");return state;
}
std::int64_t ImprovementYieldQuote::total()const{
    std::int64_t sum=0;for(const auto v:{base,era,adjacent_features,adjacent_resources,resource,golden_age,wltkd,river,coast,hills,feature,adjacent_city,adjacent_mountain,fresh,
        route,trade_route,connection,route_intrinsic,technologies,policies,player,traits,team,team_water,religion,fractional})sum=wide::add(sum,v);
    return sum;
}
ImprovementYieldQuote bnw_improvement_yield(ImprovementYieldContext &c,std::int64_t plot,std::int64_t improvement,std::int64_t yield,std::int64_t player,bool optimal,std::optional<std::int64_t> assumed){
    validate(c,false);ImprovementYieldQuote q;const auto found=c.rules.improvements.find(improvement);if(found==c.rules.improvements.end())return q;
    if(yield<0)throw std::runtime_error("Invalid improvement yield ID");const auto &info=found->second;const auto &p=cell(c,plot);
    q.base=value(info.yields[0],yield);era_yield(q,c,info,plot,player,yield);geography(q,c,info,plot,yield,false);
    if(optimal){q.fresh=value(info.yields[static_cast<std::size_t>(ImprovementYieldKind::fresh_water)],yield);q.route=best_route(c,info,yield);}
    else{if(!assumed||*assumed==c.automatic_route){if(!p.route)throw std::runtime_error("Unresolved improvement route");assumed=p.route->type;}
        if(*assumed!=-1)q.route=value(info.routes,*assumed,yield);}
    const bool fresh=plot_is_fresh_water(c.water,plot);
    if(optimal||player==-1)all_tech_policies(q,c,info,improvement,yield,fresh);
    else{const auto &bonus=c.state.players.at(player);const auto &team=c.state.teams.at(c.water.players.players.at(player).team);
        q.player=value(bonus.own,improvement,yield);q.traits=value(bonus.traits,improvement,yield);q.team=value(team[0],improvement,yield);q.team_water=value(team[fresh?2:1],improvement,yield);}
    if(player!=-1){const auto resource=plot_resource_type(c.resources,plot,c.water.players.players.at(player).team);if(resource!=-1)q.resource=value(info.resources,resource,yield);}
    if(const auto *working=city(c,p.yield_city))q.religion=religious_yield(c,*working,improvement,yield);
    q.total();return q;
}
ImprovementYieldQuote vp_improvement_yield(ImprovementYieldContext &c,std::int64_t plot,std::int64_t improvement,std::int64_t yield,std::int64_t player,std::int64_t route,
    std::int64_t feature,std::int64_t resource,const CityYieldState *owning,bool optimal,std::optional<std::int64_t> forced){
    validate(c,true);ImprovementYieldQuote q;if(improvement==-1)return q;const auto found=c.rules.improvements.find(improvement);if(found==c.rules.improvements.end())return q;
    if(yield<0)throw std::runtime_error("Invalid improvement yield ID");const auto &info=found->second;const auto &p=cell(c,plot);const bool fresh=plot_is_fresh_water(c.water,plot);
    q.base=value(info.yields[0],yield);const auto force=forced.value_or(c.automatic_route);
    if(player!=-1){q.fractional=improvement_fractional_adjacency(c.water.map,c.rules,c.water.types,plot,improvement,yield).combined();era_yield(q,c,info,plot,player,yield);
        if(required(p.owner)==player)for(const auto n:neighbors(c,plot)){const auto &other=cell(c,n);const auto f=resolve(c,other.feature_type,other.feature_type_hash);
            if(f!=-1)q.adjacent_features=wide::add(q.adjacent_features,value(info.adjacent_features,f,yield));
            if(plot_resource_type(c.resources,n,c.water.players.players.at(player).team)!=-1)q.adjacent_resources=wide::add(q.adjacent_resources,value(info.adjacent_resources,plot_resource_type(c.resources,n),yield));}
        if(resource!=-1)q.resource=value(info.resources,resource,yield);
        if(c.water.cities.players.at(player).golden_age)q.golden_age=value(info.yields[static_cast<std::size_t>(ImprovementYieldKind::golden_age)],yield);
    }
    if(owning&&required(owning->we_love_king_turns)>0)q.wltkd=value(info.yields[static_cast<std::size_t>(ImprovementYieldKind::wltkd)],yield);
    geography(q,c,info,plot,yield,true);if(feature!=-1)q.feature=value(info.features,feature,yield);
    if(fresh||optimal)q.fresh=value(info.yields[static_cast<std::size_t>(ImprovementYieldKind::fresh_water)],yield);
    if(optimal)q.route=best_route(c,info,yield);
    else if(player!=-1){if(required(p.trade_route_units)>0&&force==c.automatic_route)q.trade_route=value(info.routes,era(c,player)>=4?c.railroad:c.road,yield);
        if(route!=-1||(force!=c.automatic_route&&force!=-1)){
            const auto &connections=vp_improvement_yield_player(c.state,player);
            const auto connected=[&](bool industrial){const auto &v=industrial?connections.industrial_connections:connections.connections;
                // A manual lower_bound keeps the native algorithm defined even
                // for unsorted saved vectors; do not silently sort/rebuild them.
                std::size_t first=0,count=v.size();while(count){const auto step=count/2,mid=first+step;if(v[mid]<plot){first=mid+1;count-=step+1;}else count=step;}
                return first<v.size()&&!(plot<v[first]);};
            if(!c.water.options)throw std::runtime_error("VP route yields need custom mod options");
            if((force==c.railroad||(force==c.automatic_route&&connected(true)))&&c.water.options->enabled("BALANCE_VP"))q.connection=value(info.routes,c.railroad,yield);
            else if(force==c.road||force==c.railroad||(force==c.automatic_route&&connected(false)))q.connection=value(info.routes,c.road,yield);
            if(route!=-1){const auto route_info=c.rules.routes.find(route);if(route_info!=c.rules.routes.end())q.route_intrinsic=value(route_info->second,yield);}
        }
    }
    if(optimal||player==-1)all_tech_policies(q,c,info,improvement,yield,fresh);
    q.total();return q;
}
} // namespace civ5::runtime
