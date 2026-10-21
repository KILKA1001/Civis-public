#pragma once
#include "civ5/runtime/route_economy.hpp"
#include "civ5/runtime/vp_route_economy_profile.hpp"
#include <sstream>
#include <tuple>

namespace civ5::validation {
struct RouteEconomyProbeServices final : runtime::RouteEconomyServices {
    std::vector<std::int64_t> dirty;
    void game_data_dirty(std::int64_t player) override {dirty.push_back(player);}
    void test_over_resource_notification(std::int64_t,std::int64_t) override {
        throw runtime::MaintenanceUnavailable("BNW resource notification requires live availability and notifications");
    }
};
inline std::string route_economy_report(const saves::SavedSessionSnapshot &snapshot,
        const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
        const runtime::MaintenanceContext &maintenance,const content::CustomModOptions *options,std::int64_t active_player) {
    if(!snapshot.map_projection)throw std::runtime_error("Route economy diagnostic needs shared map");
    const auto rules=runtime::load_route_economy_rules(database,maintenance.profile);
    const auto source=runtime::restore_route_economy(snapshot,rules,maintenance,types);
    if(source.players.empty())throw std::runtime_error("Empty route economy roster");
    const auto &map=*snapshot.map_projection;
    std::ostringstream out;
    const auto pairs=[&](const auto &values){out<<'[';bool comma=false;for(const auto &[id,n]:values){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto player=[&](std::int64_t id,const auto &state,const auto &money){
        const auto &p=state.players.at(id);
        out<<"{\"id\":"<<id<<",\"no_hills_maintenance\":"<<(p.no_hills_maintenance?"true":"false")
           <<",\"base_improvement\":"<<money.players.at(id).base_improvement<<",\"resources_used\":";pairs(p.resources_used);
        out<<",\"responsible_routes\":";
        if(state.profile==content::TechnologyProfile::vp542)pairs(runtime::vp_route_counts(state,id));
        else pairs(std::map<std::int64_t,std::int64_t>{});
        out<<'}';
    };
    const auto route=[&](const saves::SavedRoutePlot &r){out<<'['<<r.type<<','<<r.responsible<<',';if(r.builder)out<<*r.builder;else out<<"null";out<<','<<(r.pillaged?"true":"false")<<']';};
    std::uint64_t routed=0,pillaged=0;
    for(const auto &cell:map.cells){
        if(!cell.route)throw std::runtime_error("Unresolved saved route projection");
        routed+=cell.route->type!=-1;pillaged+=cell.route->pillaged;
    }
    out<<"{\"disposable_copies\":true,\"full_turn\":false,\"route_type_changed\":false,\"active_player\":"<<active_player
       <<",\"cells\":"<<map.cells.size()<<",\"routed_cells\":"<<routed<<",\"pillaged_cells\":"<<pillaged
       <<",\"resource_slots\":"<<rules.resource_slots<<",\"route_slots\":"<<rules.route_slots
       <<",\"balance_vp\":"<<(options&&options->enabled("BALANCE_VP")?"true":"false")
       <<",\"improvements_extensions\":"<<(options&&options->enabled("IMPROVEMENTS_EXTENSIONS")?"true":"false")<<",\"rules\":[";
    bool comma=false;
    for(const auto &[id,r]:rules.routes){if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"gold\":"<<r.gold_maintenance<<",\"resources\":";pairs(r.resources);out<<'}';}
    out<<"],\"players\":[";comma=false;
    for(const auto &[id,p]:source.players){if(comma)out<<',';comma=true;player(id,source,maintenance);}
    out<<"],\"probes\":[";comma=false;
    // One real cell for each distinct responsibility/type/terrain/city/pillage
    // case. Every case starts from an independent imported copy, releases its
    // payer and then assigns another player without building/removing a road.
    std::set<std::tuple<std::int64_t,std::int64_t,std::int64_t,std::int64_t,bool,bool>> cases;
    for(const auto &original:map.cells){
        if(!original.plot_type||!original.terrain_type)throw std::runtime_error("Unresolved route maintenance terrain");
        const auto &r=*original.route;
        if(!cases.emplace(r.type,r.responsible,*original.plot_type,*original.terrain_type,original.is_city,r.pillaged).second)continue;
        auto state=source;auto money=maintenance;auto cell=original;
        auto next=source.players.begin()->first;
        for(const auto &[id,p]:source.players)if(id!=r.responsible){next=id;break;}
        RouteEconomyProbeServices services;
        runtime::RouteEconomyContext context{state,money,rules,options,services,active_player};
        if(comma)out<<',';comma=true;
        out<<"{\"plot\":"<<original.index<<",\"plot_type\":"<<*original.plot_type<<",\"terrain_type\":"<<*original.terrain_type
           <<",\"city\":"<<(original.is_city?"true":"false")<<",\"before\":";route(r);
        const auto phase=[&](const char *name,std::int64_t target){
            services.dirty.clear();runtime::set_route_responsible(context,cell,target);
            out<<",\""<<name<<"\":{\"route\":";route(*cell.route);
            out<<",\"players\":[";bool pc=false;
            for(const auto id:std::set<std::int64_t>{r.responsible,next})if(id!=-1){if(pc)out<<',';pc=true;player(id,state,money);}
            out<<"],\"dirty\":[";pc=false;for(const auto id:services.dirty){if(pc)out<<',';pc=true;out<<id;}out<<"]}";
        };
        phase("released",-1);phase("assigned",next);out<<'}';
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
