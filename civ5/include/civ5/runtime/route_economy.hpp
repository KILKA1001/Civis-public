#pragma once
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::runtime {
// The financial part of CvRouteInfo. Other route properties are loaded by
// their consumers; this is not a replacement for the complete plot updater.
struct RouteCostInfo {
    std::int64_t gold_maintenance=0;
    std::map<std::int64_t,std::int64_t> resources;
};
struct RouteEconomyRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::uint64_t resource_slots=0, route_slots=0;
    std::map<std::int64_t,RouteCostInfo> routes;
};
RouteEconomyRules load_route_economy_rules(const content::RulesDatabase &database,content::TechnologyProfile profile);
struct RouteEconomyPlayer {
    bool no_hills_maintenance=false;
    // All resource usage, not just roads. Other consumers must share these
    // counters; a route transfer changes the original imported totals.
    std::map<std::int64_t,std::int64_t> resources_used;
};
struct RouteEconomyState {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,RouteEconomyPlayer> players;
    ProfileExtensions profile_extensions;
};
RouteEconomyState restore_route_economy(const saves::SavedSessionSnapshot &snapshot,
    const RouteEconomyRules &rules,const MaintenanceContext &maintenance,const content::InfoTypeCatalog &types);
class RouteEconomyServices {
  public:
    virtual ~RouteEconomyServices()=default;
    virtual void game_data_dirty(std::int64_t player)=0;
    // BNW only, before its resource-use UI invalidation. Must run the actual
    // resource-shortage notification query; an unavailable service must throw.
    virtual void test_over_resource_notification(std::int64_t player,std::int64_t resource)=0;
};
struct RouteEconomyContext {
    RouteEconomyState &state;
    MaintenanceContext &maintenance;
    const RouteEconomyRules &rules;
    const content::CustomModOptions *options;
    RouteEconomyServices &services;
    std::int64_t active_player=-1;
};
bool must_pay_plot_maintenance(const RouteEconomyContext &context,const saves::SavedMapCellProjection &plot,std::int64_t player);
void change_base_improvement_maintenance(RouteEconomyContext &context,std::int64_t player,std::int64_t change);
void change_resource_used(RouteEconomyContext &context,std::int64_t player,std::int64_t resource,std::int64_t change);
// Exact responsibility setter: BNW assigns only; VP transfers maintenance,
// route counts and resources before assigning the shared plot field. Does not
// build/remove a road, recalculate yields or execute SetRouteType load hooks.
// Required service failures preserve prior mutations, as in the source; a
// caller must restore its checkpoint before retrying.
void set_route_responsible(RouteEconomyContext &context,saves::SavedMapCellProjection &plot,std::int64_t player);
} // namespace civ5::runtime
