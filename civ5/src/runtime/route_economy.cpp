#include "civ5/runtime/route_economy.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/runtime/vp_route_economy_profile.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
std::int64_t i64(std::uint64_t value) {
    if(value>INT64_MAX)throw std::overflow_error("Route economy index exceeds int64");
    return static_cast<std::int64_t>(value);
}
template<class T> const T &named(const std::vector<T> &fields,std::string_view name) {
    const T *result=nullptr;
    for(const auto &f:fields)if(f.name&&std::string_view(f.name)==name){
        if(result)throw std::runtime_error("Duplicate route economy field: "+std::string(name));result=&f;
    }
    if(!result)throw std::runtime_error("Missing route economy field: "+std::string(name));return *result;
}
std::int64_t value(const std::map<std::int64_t,std::int64_t> &values,std::int64_t id) {
    const auto i=values.find(id);return i==values.end()?0:i->second;
}
void validate(const RouteEconomyContext &c) {
    const bool vp=c.state.profile==TechnologyProfile::vp542;
    if(c.rules.profile!=c.state.profile||c.maintenance.profile!=c.state.profile||
       (vp?(!c.options||!c.options->all_succeeded()):c.options!=nullptr))
        throw std::runtime_error("Route economy profile/rules mismatch");
}
void validate_player(const RouteEconomyContext &c,std::int64_t player) {
    if(!c.state.players.contains(player)||!c.maintenance.players.contains(player))
        throw std::runtime_error("Unknown route economy player");
}
void change_count(RouteEconomyContext &c,std::int64_t player,std::int64_t route,std::int64_t change) {
    auto &counts=vp_route_counts(c.state,player);
    const auto n=wide::add(value(counts,route),change);
    // Source asserts on negative counters but does not clamp in release.
    // Keep that numeric behavior; never copy the source's integer overflow.
    if(n)counts[route]=n;else counts.erase(route);
}
}
RouteEconomyRules load_route_economy_rules(const content::RulesDatabase &db,TechnologyProfile profile) {
    RouteEconomyRules result;result.profile=profile;
    const auto resources=db.prefetch_info("Resources");
    if(!resources.all_succeeded())throw std::runtime_error("Route Resources collection: "+resources.error);
    result.resource_slots=resources.slots;i64(result.resource_slots);
    const auto routes=db.prefetch_info("Routes",[&](auto slot,auto,const auto &row){
        RouteCostInfo info;info.gold_maintenance=row.integer("GoldMaintenance");
        // The native bind takes a C string (NULL Type binds NULL). Duplicate
        // relation rows overwrite in SQLite iteration order, not by summing.
        const auto type=row.text("Type");
        const content::DatabaseValue parameter=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
        const auto q=db.query_rows({"select Resources.ID, Cost from Route_ResourceQuantityRequirements inner join Resources on ResourceType = Resources.Type where RouteType = ?",{parameter}},
            [&](const auto &linked){
                const auto id=linked.integer_at(0),cost=linked.integer_at(1);
                if(id<0||static_cast<std::uint64_t>(id)>=result.resource_slots)
                    throw std::runtime_error("Route resource relation outside active collection");
                if(cost)info.resources[id]=cost;else info.resources.erase(id);
            });
        if(!q.all_succeeded())throw std::runtime_error("Route resource costs: "+q.error);
        result.routes.emplace(i64(slot),std::move(info));return true;
    });
    if(!routes.all_succeeded())throw std::runtime_error("Routes collection: "+routes.error);
    result.route_slots=routes.slots;i64(result.route_slots);return result;
}
RouteEconomyState restore_route_economy(const saves::SavedSessionSnapshot &snap,const RouteEconomyRules &rules,
    const MaintenanceContext &maintenance,const content::InfoTypeCatalog &types) {
    const bool vp=rules.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snap.all_players||snap.game_state.layout!=layout||snap.all_players->layout!=layout||
       maintenance.profile!=rules.profile||!types.report().all_succeeded())
        throw std::runtime_error("Route economy needs matching saved state and rules");
    RouteEconomyState result;result.profile=rules.profile;
    for(const auto &saved:snap.all_players->players){
        const auto id=saved.saved_player_id;
        if(id<0||saved.core.layout!=layout||saved.traits.layout!=layout||(vp&&saved.tail.layout!=layout)||
           !maintenance.players.contains(id)||result.players.contains(id))throw std::runtime_error("Route economy player mismatch");
        RouteEconomyPlayer p;
        const auto &trait=named(saved.traits.fields,"m_bNoHillsImprovementMaintenance").values;
        if(trait.size()!=1)throw std::runtime_error("Invalid route maintenance trait");
        p.no_hills_maintenance=trait.front()!=0;
        const auto &used=named(saved.core.fields,"m_paiNumResourceUsed");
        if(vp){
            if(used.encoding!=saves::PlayerCoreEncoding::vector_i32||used.values.size()!=rules.resource_slots)
                throw std::runtime_error("VP resource-use dimensions differ from active rules");
            for(std::size_t i=0;i<used.values.size();++i)if(used.values[i])p.resources_used.emplace(i64(i),used.values[i]);
            const auto &counts=named(saved.tail.fields,"m_piResponsibleForRouteCount");
            if(!counts.encoding||std::string_view(counts.encoding)!="stl_pairs"||counts.values.size()%2)
                throw std::runtime_error("Invalid saved route counts");
            // FStlContainerSerialization::container.insert: FIRST duplicate wins.
            auto &responsible=ensure_vp_route_economy_profile(result).responsible_routes[id];
            for(std::size_t i=0;i<counts.values.size();i+=2)responsible.emplace(counts.values[i],counts.values[i+1]);
        }else{
            if(used.encoding!=saves::PlayerCoreEncoding::hashed_i32)throw std::runtime_error("Invalid BNW resource-use encoding");
            std::size_t n=0;
            for(const auto hash:used.hashes)if(hash){
                if(n==used.values.size())throw std::runtime_error("Truncated BNW resource-use values");
                const auto *type=types.find_hash(hash);
                if(!type||type->id<0||static_cast<std::uint64_t>(type->id)>=rules.resource_slots)
                    throw std::runtime_error("Unresolved saved resource-use type");
                const auto v=used.values[n++];
                if(v)p.resources_used[type->id]=v;else p.resources_used.erase(type->id);
            }
            if(n!=used.values.size())throw std::runtime_error("Extra BNW resource-use values");
        }
        result.players.emplace(id,std::move(p));
    }
    if(result.players.size()!=maintenance.players.size())throw std::runtime_error("Incomplete route economy roster");
    return result;
}
bool must_pay_plot_maintenance(const RouteEconomyContext &c,const saves::SavedMapCellProjection &plot,std::int64_t player) {
    validate(c);validate_player(c,player);
    if(!plot.plot_type)throw std::runtime_error("Unresolved maintenance plot type");
    bool free_terrain=*plot.plot_type==1;
    if(c.state.profile==TechnologyProfile::vp542&&c.options->enabled("BALANCE_VP")){
        if(!plot.terrain_type)throw std::runtime_error("Unresolved maintenance terrain type");
        free_terrain=free_terrain||*plot.plot_type==0||*plot.terrain_type==7;
    }
    return !(free_terrain&&c.state.players.at(player).no_hills_maintenance);
}
void change_base_improvement_maintenance(RouteEconomyContext &c,std::int64_t player,std::int64_t change) {
    validate(c);validate_player(c,player);
    auto &base=c.maintenance.players.at(player).base_improvement;
    const auto requested=wide::add(base,change);
    if(base==requested)return; // compare BEFORE the source's zero clamp
    base=std::max<std::int64_t>(0,requested);
    if(player==c.active_player)c.services.game_data_dirty(player);
}
void change_resource_used(RouteEconomyContext &c,std::int64_t player,std::int64_t resource,std::int64_t change) {
    validate(c);validate_player(c,player);
    if(resource<0||static_cast<std::uint64_t>(resource)>=c.rules.resource_slots)throw std::out_of_range("Resource-use index");
    auto &used=c.state.players.at(player).resources_used;
    if(change){
        const auto n=wide::add(value(used,resource),change);
        if(n)used[resource]=n;else used.erase(resource);
        if(c.state.profile==TechnologyProfile::bnw&&change>0)c.services.test_over_resource_notification(player,resource);
    }
    // Both source profiles invalidate even for zero change and non-active players.
    c.services.game_data_dirty(player);
}
void set_route_responsible(RouteEconomyContext &c,saves::SavedMapCellProjection &plot,std::int64_t player) {
    validate(c);
    if(!plot.route)throw std::runtime_error("Unresolved saved route state");
    auto &route=*plot.route;const auto old=route.responsible;
    if(old==player)return;
    if(player!=-1)validate_player(c,player);
    if(c.state.profile==TechnologyProfile::vp542&&route.type!=-1){
        const auto entry=c.rules.routes.find(route.type);
        if(entry!=c.rules.routes.end()){
            const auto &info=entry->second;
            const bool extensions=c.options->enabled("IMPROVEMENTS_EXTENSIONS");
            if(old!=-1){
                if(must_pay_plot_maintenance(c,plot,old))change_base_improvement_maintenance(c,old,wide::subtract(0,info.gold_maintenance));
                if(extensions)change_count(c,old,route.type,-1);
            }
            if(player!=-1){
                if(must_pay_plot_maintenance(c,plot,player))change_base_improvement_maintenance(c,player,info.gold_maintenance);
                if(extensions)change_count(c,player,route.type,1);
            }
            if(extensions)for(const auto &[resource,cost]:info.resources)if(cost>0){
                if(old!=-1)change_resource_used(c,old,resource,wide::subtract(0,cost));
                if(player!=-1)change_resource_used(c,player,resource,cost);
            }
        }
    }
    route.responsible=player;
}
} // namespace civ5::runtime
