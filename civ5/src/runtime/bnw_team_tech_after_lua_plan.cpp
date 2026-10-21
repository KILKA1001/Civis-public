#include "civ5/runtime/bnw_team_tech_after_lua_plan.hpp"
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;

std::optional<std::int64_t> team_field(const saves::TeamObservation &team,
                                       std::string_view name) {
    std::optional<std::int64_t> result;
    for(const auto &field:team.fields) {
        if(field.name && name==field.name) {
            if(result) return std::nullopt;
            result=field.value;
        }
    }
    return result;
}

bool matching_owned_bit(const LiveSessionState &state,
                        const content::InfoTypeCatalog &types,
                        std::int64_t team,std::int64_t technology,
                        const std::string &technology_type) {
    if(!types.report().all_succeeded() || types.report().hash_collisions ||
       types.report().name_overwrites || team<0 || technology<0 ||
       static_cast<std::size_t>(team)>=state.technologies.size()) return false;
    const auto hash=content::saved_info_type_hash(technology_type);
    const auto *mapped=types.find_hash(hash);
    if(!mapped || mapped->table!="Technologies" ||
       mapped->type!=technology_type || mapped->id!=technology ||
       !state.technologies[static_cast<std::size_t>(team)].has_tech(technology))
        return false;
    const auto &saved=state.world.teams->teams[static_cast<std::size_t>(team)];
    const saves::PlotArrayObservation *hashes=nullptr,*flags=nullptr;
    for(const auto &array:saved.arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array;
    }
    if(!hashes || !flags || hashes->values.size()!=flags->values.size()) return false;
    std::size_t matches=0;
    for(std::size_t i=0;i<hashes->values.size();++i)
        if(hashes->values[i]==static_cast<std::int64_t>(hash)) {
            if(flags->values[i]!=1) return false;
            ++matches;
        }
    return matches==1;
}
} // namespace

BnwTeamTechAfterLuaPlan plan_bnw_team_tech_after_lua(
    const LiveSessionState &state,const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,std::int64_t technology) {
    BnwTeamTechAfterLuaPlan result;
    const auto stop=[&](BnwTeamTechAfterLuaStep step,std::string reason) {
        result.first_pending=step;
        result.blocker=std::move(reason);
        return result;
    };
    const auto *marker=state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>();
    if(lua.status!=BnwTeamTechLuaStatus::quoted || lua.grant_complete ||
       !marker || !marker->attempted || !marker->quoted || marker->uncertain ||
       marker->team!=team || marker->technology!=technology)
        return stop(BnwTeamTechAfterLuaStep::lua_quote,
                    "BNW TeamTechResearched selected Lua quote is absent or uncertain");
    if(state.world.game_state.layout!=bnw || !state.world.teams ||
       state.world.teams->layout!=bnw || team!=63 || technology<0 ||
       state.world.teams->teams.size()!=64 || state.technologies.size()!=64 ||
       technologies.profile!=content::TechnologyProfile::bnw ||
       !technologies.all_succeeded() ||
       static_cast<std::uint64_t>(technology)>=technologies.collection.slots)
        return stop(BnwTeamTechAfterLuaStep::ends_game,
                    "BNW detached world or technology rules are incomplete");
    const auto *tech=technologies.find(static_cast<std::uint64_t>(technology));
    if(!tech || tech->base.type!="TECH_AGRICULTURE" ||
       !matching_owned_bit(state,types,team,technology,tech->base.type))
        return stop(BnwTeamTechAfterLuaStep::ends_game,
                    "BNW Agriculture owned/raw technology identity is unverified");
    const auto ends=tech->booleans.find("EndsGame");
    if(ends==tech->booleans.end())
        return stop(BnwTeamTechAfterLuaStep::ends_game,
                    "BNW Agriculture EndsGame rule is unavailable");
    if(ends->second)
        return stop(BnwTeamTechAfterLuaStep::ends_game,
                    "CvGame::SetEndGameTechResearched is required");
    result.proven_no_effect.push_back(BnwTeamTechAfterLuaStep::ends_game);

    // The artifact loop is inert only if neither named resource reveals this
    // technology. This proof is independent of WB scenario and game turn.
    for(const char *type:{"RESOURCE_ARTIFACTS","RESOURCE_HIDDEN_ARTIFACTS"}) {
        std::size_t rows=0;
        bool reveals=false;
        const auto report=database.query_rows(
            {"SELECT TechReveal FROM Resources WHERE Type=?",{std::string(type)}},
            [&](const content::RulesRowView &row) {
                ++rows;
                if(row.text("TechReveal")==tech->base.type) reveals=true;
            },2);
        if(!report.all_succeeded() || rows>1)
            return stop(BnwTeamTechAfterLuaStep::artifact_notifications,
                        "BNW artifact resource reveal rules are incomplete");
        if(reveals)
            return stop(BnwTeamTechAfterLuaStep::artifact_notifications,
                        "BNW artifact notification map loop requires live UI/world effects");
    }
    result.proven_no_effect.push_back(BnwTeamTechAfterLuaStep::artifact_notifications);

    const auto &saved=state.world.teams->teams[static_cast<std::size_t>(team)];
    const auto current=team_field(saved,"era");
    if(!current || *current<0 || tech->era<0)
        return stop(BnwTeamTechAfterLuaStep::era_evaluation,
                    "BNW team or technology era is unavailable");
    if(*current<tech->era)
        return stop(BnwTeamTechAfterLuaStep::era_evaluation,
                    "CvTeam::SetCurrentEra is required for this technology");
    bool all_earlier=true;
    for(std::uint64_t slot=0;slot<technologies.collection.slots;++slot) {
        const auto *other=technologies.find(slot);
        if(!other || other->era<0)
            return stop(BnwTeamTechAfterLuaStep::era_evaluation,
                        "BNW earlier-era technology table is incomplete");
        if(other->era<=*current &&
           !state.technologies[static_cast<std::size_t>(team)].has_tech(
               static_cast<std::int64_t>(slot))) {
            all_earlier=false;
            break;
        }
    }
    if(all_earlier)
        return stop(BnwTeamTechAfterLuaStep::era_evaluation,
                    "CvTeam::SetCurrentEra next-era branch requires evaluation");
    result.proven_no_effect.push_back(BnwTeamTechAfterLuaStep::era_evaluation);

    const auto visible=tech->booleans.find("MapVisible");
    if(visible==tech->booleans.end())
        return stop(BnwTeamTechAfterLuaStep::map_reveal,
                    "BNW Agriculture MapVisible rule is unavailable");
    if(visible->second)
        return stop(BnwTeamTechAfterLuaStep::map_reveal,
                    "BNW map reveal and deferred fog update are required");
    result.proven_no_effect.push_back(BnwTeamTechAfterLuaStep::map_reveal);
    return stop(BnwTeamTechAfterLuaStep::best_route,
                "CvTeam::DoUpdateBestRoute requires ordered Builds and route-value evaluation");
}
} // namespace civ5::runtime
