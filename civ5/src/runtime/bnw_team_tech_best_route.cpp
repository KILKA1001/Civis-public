#include "civ5/runtime/bnw_team_tech_best_route.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::vector<BnwBuildRouteRule> active_builds(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types) {
    require(types.report().all_succeeded() &&
            !types.report().hash_collisions && !types.report().name_overwrites,
            "BNW best-route type catalog is ambiguous");
    std::vector<BnwBuildRouteRule> builds;
    const auto report=database.query_rows(
        {"SELECT ID,Type,RouteType,PrereqTech FROM Builds ORDER BY ID",{}},
        [&](const content::RulesRowView &row) {
            const auto id=row.integer("ID");
            const auto name=row.text("Type");
            require(id>=0 && id<=INT32_MAX &&
                    id==static_cast<std::int64_t>(builds.size()) &&
                    name && !name->empty(),
                    "BNW Builds do not occupy dense source slots");
            const auto *build=types.find(*name);
            require(build && build->table=="Builds" && build->id==id,
                    "BNW Build type cache disagrees with active rules");
            BnwBuildRouteRule item;
            item.build=id;
            if(const auto route_name=row.text("RouteType");
               route_name && !route_name->empty()) {
                const auto *route=types.find(*route_name);
                require(route && route->table=="Routes" &&
                        route->id>=0 && route->id<=INT32_MAX,
                        "BNW Build route type is unavailable");
                item.route=route->id;
            }
            if(const auto tech_name=row.text("PrereqTech");
               tech_name && !tech_name->empty()) {
                const auto *tech=types.find(*tech_name);
                require(tech && tech->table=="Technologies" &&
                        tech->id>=0 && tech->id<=INT32_MAX,
                        "BNW Build prerequisite technology is unavailable");
                item.prerequisite_technology=tech->id;
            }
            builds.push_back(item);
        });
    require(report.all_succeeded(),"BNW best-route Builds query failed");
    std::size_t cached_builds=0;
    for(const auto &[name,entry]:types.entries())
        if(entry.table=="Builds") ++cached_builds;
    require(builds.size()==cached_builds,
            "BNW Builds count disagrees with active type cache");
    return builds;
}
} // namespace

BnwBestRouteSelection select_bnw_best_route(
    std::span<const BnwBuildRouteRule> builds,
    const TeamTechnologies &owned,
    const content::TechnologyRules &technologies) {
    require(technologies.profile==content::TechnologyProfile::bnw &&
            technologies.all_succeeded() &&
            owned.profile()==content::TechnologyProfile::bnw,
            "BNW best-route technology rules are incomplete");
    BnwBestRouteSelection best;
    for(std::size_t index=0;index<builds.size();++index) {
        const auto &build=builds[index];
        require(build.build==static_cast<std::int64_t>(index) &&
                build.route>=-1 && build.route<=INT32_MAX &&
                build.prerequisite_technology>=-1 &&
                build.prerequisite_technology<=INT32_MAX,
                "BNW best-route Build slot or reference is invalid");
        if(build.route==-1) continue;
        std::int64_t value=-1;
        if(build.prerequisite_technology==-1) value=1;
        else {
            const auto *tech=technologies.find(
                static_cast<std::uint64_t>(build.prerequisite_technology));
            require(tech && !tech->base.type.empty(),
                    "BNW best-route prerequisite technology rule is missing");
            const auto cost=tech->integers.find("Cost");
            require(cost!=tech->integers.end() &&
                    cost->second>=INT32_MIN && cost->second<=INT32_MAX,
                    "BNW best-route raw technology Cost is unavailable");
            if(owned.has_tech(build.prerequisite_technology)) value=cost->second;
        }
        // Source uses >=, so the last route Build wins an equal-value tie.
        if(value>=best.value) {
            best.value=value;
            best.route=build.route;
            best.winning_build=build.build;
        }
    }
    return best;
}

BnwTeamTechBestRouteResult run_bnw_team_tech_best_route(
    LiveSessionState &state,const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,std::int64_t technology) {
    BnwTeamTechBestRouteResult result;
    const auto prefix=plan_bnw_team_tech_after_lua(
        state,lua,technologies,database,types,team,technology);
    if(prefix.first_pending!=BnwTeamTechAfterLuaStep::best_route ||
       prefix.proven_no_effect.size()!=4) {
        result.blocker=prefix.blocker.empty() ?
            "BNW best-route source-order predecessor is incomplete" : prefix.blocker;
        return result;
    }
    if(state.setup.extensions.find<BnwTeamTechBestRouteMarker>()) {
        result.blocker="BNW best-route Agriculture effect already applied";
        return result;
    }
    try {
        auto &saved=state.world.teams->teams[static_cast<std::size_t>(team)];
        saves::NamedSaveInteger *best_field=nullptr;
        for(auto &field:saved.fields)
            if(field.name && std::string_view(field.name)=="best_route") {
                require(!best_field,"Duplicate BNW best_route field");
                best_field=&field;
            }
        require(best_field && best_field->value>=-1 &&
                best_field->value<=INT32_MAX,
                "BNW best_route saved field is unavailable");
        const auto builds=active_builds(database,types);
        const auto selected=select_bnw_best_route(
            builds,state.technologies[static_cast<std::size_t>(team)],technologies);
        if(best_field->value!=-1) {
            bool found=false;
            for(const auto &[name,entry]:types.entries())
                if(entry.table=="Routes" && entry.id==best_field->value)
                    found=true;
            require(found,"BNW saved best_route does not name an active route");
        }
        result.before=best_field->value;
        result.after=selected.route;
        result.winning_build=selected.winning_build;
        result.route_value=selected.value;
        result.inspected_builds=builds.size();
        // CvTeam::SetBestPossibleRoute performs only this scalar write when
        // the value changes. No route unlock cache is updated by this setter.
        state.setup.extensions.emplace<BnwTeamTechBestRouteMarker>(
            BnwTeamTechBestRouteMarker{team,technology,result.before,result.after});
        best_field->value=selected.route;
        result.applied=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}
} // namespace civ5::runtime
