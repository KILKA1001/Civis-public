#include "civ5/runtime/bnw_team_tech_route_build.hpp"
#include "civ5/content/info_types.hpp"
#include <climits>
#include <map>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
struct ActiveInfo {
    std::int64_t id=0;
    std::string type;
    std::size_t saved_row=0;
};
saves::TeamHashedArray &hashed(saves::TeamObservation &team,
                               std::string_view name) {
    saves::TeamHashedArray *found=nullptr;
    for(auto &array:team.hashes)
        if(array.name && name==array.name) {
            require(!found,"Duplicate BNW team hashed array");
            found=&array;
        }
    require(found && found->wire_count>=0 &&
            static_cast<std::uint64_t>(found->wire_count)==found->rows.size(),
            "Missing or malformed BNW team hashed array");
    return *found;
}
const saves::TeamHashedArray& hashed_read(const saves::TeamObservation& team,std::string_view name) {
    const saves::TeamHashedArray* result=nullptr;
    for(const auto& row:team.hashes)if(row.name&&std::string_view(row.name)==name){require(!result,"Duplicate BNW team hashed array");result=&row;}
    require(result&&result->wire_count>=0&&static_cast<std::uint64_t>(result->wire_count)==result->rows.size(),"Missing or malformed BNW team hashed array");
    return *result;
}
std::vector<ActiveInfo> infos(const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,const char *table,
    const saves::TeamHashedArray &saved) {
    std::vector<ActiveInfo> result;
    const auto report=database.query_rows({std::string("SELECT ID,Type FROM ")+table+
                                           " ORDER BY ID",{}},
        [&](const content::RulesRowView &row) {
            const auto id=row.integer("ID");
            const auto type=row.text("Type");
            require(id>=0 && id<=INT32_MAX &&
                    id==static_cast<std::int64_t>(result.size()) &&
                    type && !type->empty(),
                    "BNW route/build info IDs are not dense source slots");
            const auto *mapped=types.find(*type);
            require(mapped && mapped->table==table && mapped->id==id &&
                    mapped->hash==content::saved_info_type_hash(*type),
                    "BNW route/build active type cache diverges from rules");
            std::size_t matched=0,row_index=0;
            for(std::size_t index=0;index<saved.rows.size();++index) {
                if(saved.rows[index].type_hash!=mapped->hash) continue;
                ++matched;
                row_index=index;
            }
            require(matched==1 && saved.rows[row_index].values.size()==1,
                    "BNW route/build saved hash does not map to exactly one active row");
            result.push_back({id,*type,row_index});
        });
    require(report.all_succeeded(),"BNW route/build active info query failed");
    require(result.size()==saved.rows.size(),
            "BNW route/build saved and active object counts differ");
    return result;
}
std::map<std::int64_t,std::int64_t> changes(
    const content::RulesDatabase &database,const char *table,
    const char *relation,const char *item_column,const char *value_column,
    const std::string &technology_type,std::size_t count) {
    std::map<std::int64_t,std::int64_t> result;
    const auto sql=std::string("SELECT ")+table+".ID,"+relation+"."+
        value_column+" FROM "+relation+" INNER JOIN "+table+" ON "+
        relation+"."+item_column+"="+table+".Type WHERE "+relation+
        ".TechType=?";
    const auto report=database.query_rows({sql,{technology_type}},
        [&](const content::RulesRowView &row) {
            const auto id=row.integer_at(0);
            const auto value=row.integer_at(1);
            require(id>=0 && static_cast<std::uint64_t>(id)<count &&
                    value>=INT32_MIN && value<=INT32_MAX,
                    "BNW route/build technology relation out of source range");
            // PopulateArrayByValue starts with zero and overwrites on each
            // joined row. Its statement has no ORDER BY, as does this query.
            result[id]=value;
        });
    require(report.all_succeeded(),"BNW route/build technology relation query failed");
    return result;
}
void append(std::vector<BnwTechRouteBuildChange> &out,
    BnwTechRouteBuildKind kind,const std::vector<ActiveInfo> &active,
    const saves::TeamHashedArray &saved,
    const std::map<std::int64_t,std::int64_t> &deltas) {
    for(const auto &info:active) {
        const auto before=saved.rows[info.saved_row].values[0];
        require(before>=INT32_MIN && before<=INT32_MAX,
                "BNW route/build saved counter out of source range");
        const auto it=deltas.find(info.id);
        const auto delta=it==deltas.end() ? 0 : it->second;
        const auto after=before+delta;
        require(after>=INT32_MIN && after<=INT32_MAX,
                "BNW route/build counter would overflow source int");
        out.push_back({kind,info.id,before,delta,after,info.type});
    }
}
}

std::vector<BnwTechRouteBuildChange> prepare_bnw_route_build_calls(
 const saves::TeamObservation& team,const content::RulesDatabase& database,
 const content::InfoTypeCatalog& types,std::int64_t technology,const std::string& technology_type) {
    require(technology>=0&&technology<=INT32_MAX&&!technology_type.empty()&&
        types.report().all_succeeded()&&!types.report().hash_collisions&&!types.report().name_overwrites,
        "BNW route/build quotation actual rule/type domain invalid");
    const auto* mapped=types.find(technology_type);
    require(mapped&&mapped->table=="Technologies"&&mapped->id==technology&&
        mapped->hash==content::saved_info_type_hash(technology_type),
        "BNW route/build quotation selected technology identity differs");
    const auto& saved_routes=hashed_read(team,"route_change");
    const auto& saved_builds=hashed_read(team,"build_time_change");
    std::vector<BnwTechRouteBuildChange> result;
        // CvDatabaseUtility::PopulateArrayByValue always allocates an array
        // of MaxRows(Technologies), initialized to zero, before it looks up
        // joined rows. The getter's -1 is only the null-pointer fallback.
        std::int64_t max_technology_rowid=-1;
        const auto techs=database.query_rows({"SELECT max(rowid) FROM Technologies",{}},
            [&](const content::RulesRowView &row) {
                max_technology_rowid=row.integer_at(0);
            },1);
        require(techs.all_succeeded() && techs.rows==1 &&
                max_technology_rowid>=0 && max_technology_rowid<INT32_MAX &&
                technology<max_technology_rowid+1,
                "BNW technology array allocation or selected slot unavailable");

        const auto routes=infos(database,types,"Routes",saved_routes);
        const auto builds=infos(database,types,"Builds",saved_builds);
        const auto route_deltas=changes(database,"Routes",
            "Route_TechMovementChanges","RouteType","MovementChange",
            technology_type,routes.size());
        const auto build_deltas=changes(database,"Builds",
            "Build_TechTimeChanges","BuildType","TimeChange",
            technology_type,builds.size());
        append(result,BnwTechRouteBuildKind::route_movement,
               routes,saved_routes,route_deltas);
        append(result,BnwTechRouteBuildKind::build_time,
               builds,saved_builds,build_deltas);

    return result;
}

BnwTechRouteBuildPrefix run_bnw_barbarian_route_build_prefix(
    LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    std::int64_t technology) {
    BnwTechRouteBuildPrefix result;
    const auto earlier=run_bnw_barbarian_tech_post_prefix(
        state,technologies,database,types,technology);
    if(earlier.first_pending!=BnwTeamTechPostStep::process_route_and_build_changes ||
       earlier.proven_no_effect.size()!=5) {
        result.blocker=earlier.blocker;
        return result;
    }
    try {
        require(types.report().all_succeeded() &&
                !types.report().hash_collisions &&
                !types.report().name_overwrites,
                "BNW route/build active type catalog is ambiguous");
        require(state.world.teams && state.world.teams->teams.size()==64,
                "BNW team state unavailable");
        auto &team=state.world.teams->teams[63];
        auto &saved_routes=hashed(team,"route_change");
        auto &saved_builds=hashed(team,"build_time_change");

        result.calls=prepare_bnw_route_build_calls(team,database,types,technology,earlier.technology_type);
        const auto routes=infos(database,types,"Routes",saved_routes);
        const auto builds=infos(database,types,"Builds",saved_builds);

        // The calls above are a complete preflight. These two CvTeam setters
        // only change their saved int arrays; apply after every row validates.
        result.blocker="Domain, building, improvement, player, map, Lua, era, tech-share and engine effects remain";
        for(const auto &call:result.calls) {
            const auto &info=call.kind==BnwTechRouteBuildKind::route_movement ?
                routes[static_cast<std::size_t>(call.id)] :
                builds[static_cast<std::size_t>(call.id)];
            auto &array=call.kind==BnwTechRouteBuildKind::route_movement ?
                saved_routes : saved_builds;
            array.rows[info.saved_row].values[0]=call.after;
        }
        result.applied=true;
    } catch(const std::exception &error) {
        result.calls.clear();
        result.blocker=error.what();
    }
    return result;
}

} // namespace civ5::runtime
