#include "civ5/runtime/bnw_activation_connections.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const std::string &why) {if(!value) throw std::runtime_error(why);}
template<class T> auto &named(std::vector<T> &rows,std::string_view name) {
    T *found=nullptr;
    for(auto &row:rows) if(row.name && std::string_view(row.name)==name) {
        need(!found,"Duplicate connection field: "+std::string(name));found=&row;
    }
    need(found,"Missing connection field: "+std::string(name));return *found;
}
template<class T> const auto &named(const std::vector<T> &rows,std::string_view name) {
    return named(const_cast<std::vector<T>&>(rows),name);
}
std::int64_t core(const saves::PlayerCoreObservation &saved,std::string_view name) {
    const auto &f=named(saved.fields,name);
    need(f.values.size()==1,"Connection player scalar is not singular");return f.values[0];
}
saves::SavedPlayerObservation &checked_player(LiveSessionState &state,
    const BnwActivationConnectionsOwner &owner,std::int64_t player) {
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.all_players &&
         state.world.all_players->layout==bnw && state.world.pregame &&
         state.world.pregame->layout==bnw && player>=0 && player<63 &&
         state.world.all_players->players.size()==64,
         "City connection BNW roster is incomplete");
    need(state.world.map_header.guid==owner.map_guid,"City connection owner belongs to another map");
    auto &raw=state.world.all_players->players.at(player);
    const auto &setup=state.setup.players.at(player);
    need(raw.saved_player_id==player && raw.core.layout==bnw && raw.planning.layout==bnw &&
         raw.administration.layout==bnw && core(raw.core,"m_eID")==player &&
         state.world.pregame->archive.integer_arrays.at("team_types").at(player)==setup.team,
         "City connection player identity differs");
    need(core(raw.core,"m_bAlive")==setup.alive && setup.alive &&
         state.setup.is_human(setup) && !state.cities.players.at(player).minor,
         "City connection branch requires an alive human major");
    const auto &transient=owner.players.at(player);
    need(transient.plot_routes_known,"City connection plot-route transient is unknown");
    need(transient.plot_routes.empty(),"City connection plot-route broadcast needs actual SetTradeRoute owner");
    need(raw.cities.layout==bnw && raw.cities.header.wire_city_count>=0 &&
         static_cast<std::uint64_t>(raw.cities.header.wire_city_count)==raw.cities.cities.size() &&
         raw.cities.cities.size()==state.cities.players.at(player).cities.size(),
         "City connection city roster is incomplete");
    need(state.setup.game_turn==state.world.pregame->archive.integers.at("game_turn"),
         "City connection clocks differ");
    return raw;
}
void no_usable_route(const LiveSessionState &state,const content::RulesDatabase &rules,std::int64_t player) {
    const auto team=state.setup.players.at(player).team;
    const auto found=std::find_if(state.technologies.begin(),state.technologies.end(),
        [&](const auto &t){return t.team()==team;});
    need(found!=state.technologies.end() && found->profile()==content::TechnologyProfile::bnw &&
         found->remap().all_mapped(),"City connection team technology owner is unresolved");
    bool usable=false;
    const auto query=rules.query_rows({
        "SELECT r.Value, COALESCE(t.ID,-1), b.PrereqTech IS NULL FROM Builds b JOIN Routes r ON r.Type=b.RouteType LEFT JOIN Technologies t ON t.Type=b.PrereqTech",{}},
        [&](const auto &r){
            need(r.integer_at(2)!=0 || r.integer_at(1)>=0,"City connection route prerequisite is unresolved");
            if(r.integer_at(0)>0 && found->has_tech(r.integer_at(1))) usable=true;
        });
    need(query.all_succeeded(),"City connection route rules are unavailable: "+query.error);
    // getBestRoute(NULL) uses HasTech and strictly positive Route.Value. The
    // no-route/no-water branch returns after ResetRouteInfo, before finders.
    need(!usable,"City connection route update needs route/water pathfinder owner");
}
}
BnwActivationConnectionsOwner make_bnw_activation_connections_import_owner(
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity) {
    need(source.source.container==saves::SaveContainerKind::civ5 &&
         identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
         identity.rules_sha256.size()==64 && source.snapshot.all_players &&
         source.snapshot.all_players->layout==bnw && source.snapshot.all_players->players.size()==64,
         "City connection transient initialization needs the original complete BNW Civ5 import");
    BnwActivationConnectionsOwner owner;owner.identity=identity;owner.map_guid=source.snapshot.map_header.guid;
    for(const auto &p:source.snapshot.all_players->players) {
        need(p.planning.layout==bnw && p.index==static_cast<std::uint64_t>(p.saved_player_id),
             "City connection Init/Read import roster differs");
        auto &native=owner.players[p.saved_player_id];
        native.plot_routes.clear();native.plot_routes_known=true; // actual Init effect, not serialized inference
    }
    return owner;
}
LiveActionResult update_bnw_activation_city_connections(LiveSessionState &state,
    BnwActivationConnectionsOwner &owner,const content::RulesDatabase &rules,std::int64_t player) {
    try {
        need(identify_bytes(rules.gameplay_fingerprint_data()).sha256==owner.identity.rules_sha256,
             "City connection rules fingerprint differs");
        auto &raw=checked_player(state,owner,player);
        const auto team=state.setup.players.at(player).team;
        need(state.world.map_plots && state.world.map_projection,"City connection map owner is incomplete");
        std::vector<std::int64_t> city_plots;
        for(std::int64_t id=0;id<63;++id) {
            const auto &p=state.world.all_players->players.at(id);
            need(p.saved_player_id==id && state.world.pregame->archive.integer_arrays.at("team_types").at(id)==state.setup.players.at(id).team,
                 "Connectable city roster/team differs");
            // Human major ShouldConnectToOtherPlayer is false after the
            // major/minor predicate, so only same-team cities can enter IDs.
            if(state.setup.players.at(id).team==team) {
                need(p.cities.layout==bnw && p.cities.header.wire_city_count>=0 &&
                     static_cast<std::uint64_t>(p.cities.header.wire_city_count)==p.cities.cities.size() &&
                     p.cities.cities.size()==state.cities.players.at(id).cities.size(),
                     "City connection same-team city roster is incomplete");
                for(const auto &city:p.cities.cities) {
                    const auto city_id=named(city.core.fields,"m_iID").values.at(0);
                    const auto &owned=state.cities.players.at(id).cities.at(city_id);
                    need(city.core.layout==bnw && owned.owner==id && owned.id==city_id &&
                         owned.source_core==std::pair{city.core.start_offset,city.core.end_offset} &&
                         named(city.core.fields,"m_eOwner").values==std::vector<std::int64_t>{id} &&
                         named(city.core.fields,"m_iX").values==std::vector<std::int64_t>{owned.x} &&
                         named(city.core.fields,"m_iY").values==std::vector<std::int64_t>{owned.y},
                         "City connection raw/owned city identity differs");
                    const auto plot=state.world.map_header.grid.index(owned.x,owned.y);
                    const auto &saved_plot=state.world.map_plots->plots.at(plot);
                    need(named(saved_plot.fields,"plot_city_owner").value==id &&
                         named(saved_plot.fields,"plot_city_id").value==city_id &&
                         state.world.map_projection->cells.at(plot).is_city,
                         "City connection city/plot registration differs");
                    city_plots.push_back(plot);
                }
            }
        }
        const auto types=rules.cache_info_types();
        need(types.report().all_succeeded(),"City connection water building Info cache is unavailable");
        std::set<std::int64_t> water_buildings;
        const auto water_query=rules.query_rows({"SELECT ID FROM Buildings WHERE AllowsWaterRoutes<>0",{}},
            [&](const auto &r){water_buildings.insert(r.integer_at(0));});
        need(water_query.all_succeeded(),"City connection water building rules are unavailable");
        for(const auto &city:raw.cities.cities) {
            const auto city_id=named(city.core.fields,"m_iID").values.at(0);
            const auto &owned=state.cities.players.at(player).cities.at(city_id);
            need(city.production.layout==bnw && owned.building_counts_resolved,
                 "City connection water building counts are unresolved");
            // SetRouteToCapitalConnected(false) calls UpdateReligion when the
            // flag changes. That effect is required and has no successful stub.
            need(named(named(city.production.blocks,"before_strategy").fields,
                       "m_bRouteToCapitalConnectedThisTurn").value==0,
                 "City connection route flag transition needs UpdateReligion owner");
            for(const auto name:{"m_paiNumRealBuilding","m_paiNumFreeBuilding"}) {
                YieldValues counts;
                const auto &rows=named(named(city.production.blocks,"buildings").children,name).children;
                for(const auto &r:rows) {
                    const auto hash=named(r.fields,"hash").value;
                    const auto *entry=hash?types.find_hash(hash):nullptr;
                    if(!entry || !water_buildings.contains(entry->id))continue;
                    const auto &values=named(r.arrays,"values").values;
                    need(values.size()==1,"City connection saved building count is invalid");
                    counts[entry->id]=values[0]; // source last remapped duplicate wins
                }
                const auto &live=std::string_view(name)=="m_paiNumRealBuilding"?owned.real_buildings:owned.free_buildings;
                for(const auto building:water_buildings) {
                    const auto saved=counts.contains(building)?counts.at(building):0;
                    const auto current=live.contains(building)?live.at(building):0;
                    need(saved==current && current==0,
                         "City connection water routes need active-building/route pathfinder owner");
                }
            }
        }
        no_usable_route(state,rules,player);
        auto next=raw.planning;
        auto &record=named(next.blocks,"city_connections");
        const auto dimension=named(record.fields,"route_infos_dimension").value;
        need(dimension>0 && dimension<=UINT32_MAX,"City connection route matrix dimension is invalid");
        auto &routes=named(record.arrays,"route_infos");
        need(routes.columns==2 && routes.values.size()%2==0 &&
             (routes.values.size()/2)%dimension==0 && (routes.values.size()/2)/dimension==static_cast<std::uint64_t>(dimension),
             "City connection route matrix shape differs");
        auto &ids=named(record.arrays,"city_plot_ids");
        need(ids.columns==1,"Connectable city IDs shape differs");
        // UpdatePlotRouteStates/Broadcast iterate the owned empty vector;
        // UpdateCityPlotIDs replaces its actual serialized collection; active
        // water buildings are absent and route setters are validated unchanged.
        need(raw.cities.cities.size()<=static_cast<std::uint64_t>(dimension),
             "City connection route matrix resize needs source resize owner");
        ids.values=std::move(city_plots);named(record.fields,"city_ids_wire_count").value=ids.values.size();
        for(std::size_t i=0;i<routes.values.size();i+=2) {
            routes.values[i]=-1;routes.values[i+1]=0; // cPassEval, cRouteState
        }
        raw.planning=std::move(next);owner.players.at(player).updated_turn=state.setup.game_turn;
        return {LiveActionOutcome::committed,"CvCityConnections::Update no-route/no-water human branch executed"};
    } catch(const std::exception &e) {return {LiveActionOutcome::rejected,e.what()};}
}
LiveActionResult update_bnw_activation_city_connection_gold(LiveSessionState &state,
    const BnwActivationConnectionsOwner &owner,std::int64_t player) {
    try {
        auto &raw=checked_player(state,owner,player);
        need(owner.players.at(player).updated_turn==state.setup.game_turn,
             "City connection gold requires the preceding completed connection update");
        const auto &connections=named(raw.planning.blocks,"city_connections");
        const auto &routes=named(connections.arrays,"route_infos");
        const auto dimension=named(connections.fields,"route_infos_dimension").value;
        need(dimension>0 && dimension<=UINT32_MAX && routes.columns==2 && routes.values.size()%2==0 &&
             (routes.values.size()/2)%dimension==0 &&
             (routes.values.size()/2)/dimension==static_cast<std::uint64_t>(dimension),
             "City connection gold route matrix shape differs");
        const auto &ids=named(connections.arrays,"city_plot_ids");
        need(ids.columns==1 && named(connections.fields,"city_ids_wire_count").value==static_cast<std::int64_t>(ids.values.size()),
             "City connection gold connectable city IDs are incomplete");
        for(std::size_t i=0;i<routes.values.size();i+=2)
            need(routes.values[i]==-1 && routes.values[i+1]==0,
                 "City connection gold requires the completed no-route matrix");
        auto &gold=state.treasury.players.at(player);
        need(state.treasury.profile==content::TechnologyProfile::bnw &&
             gold.source_administration==std::pair{raw.administration.start_offset,raw.administration.end_offset},
             "City connection gold treasury provenance differs");
        auto &saved=named(named(raw.administration.blocks,"treasury").fields,"m_iCityConnectionGoldTimes100").value;
        need(saved==gold.connections_times100,"City connection gold raw/owned cache differs");
        // DoUpdateCityConnectionGold initializes zero and only traverses routes
        // when capital exists AND getNumCities()>1. Every subsequent source
        // HasCityConnectionRouteBetweenCities reads a proven empty matrix,
        // so no route-gold formula is entered; zero overwrites the cache.
        saved=0;gold.connections_times100=0;
        return {LiveActionOutcome::committed,"CvTreasury::DoUpdateCityConnectionGold no-route branch executed"};
    } catch(const std::exception &e) {return {LiveActionOutcome::rejected,e.what()};}
}
} // namespace civ5::runtime
