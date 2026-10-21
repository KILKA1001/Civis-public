#include "civ5/runtime/bnw_team_tech_after_lua_plan.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/info_types.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
bool proven(const BnwTeamTechAfterLuaPlan &plan,BnwTeamTechAfterLuaStep step) {
    return std::find(plan.proven_no_effect.begin(),plan.proven_no_effect.end(),step)!=
           plan.proven_no_effect.end();
}
void acquire_fixture(LiveSessionState &state) {
    // Fixture for the *post* Lua read-only plan. This does not dispatch the
    // stock Lua callback and must never be used to publish a tech grant.
    state.technologies[63].set_has_tech(0,true);
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:state.world.teams->teams[63].arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags && hashes->size()==flags->size(),
          "Selected raw team technology arrays unavailable");
    std::size_t matches=0;
    for(std::size_t i=0;i<hashes->size();++i)
        if((*hashes)[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            (*flags)[i]=1;
            ++matches;
        }
    check(matches==1,"Selected Agriculture hash is not unique");
    state.setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(
        BnwTeamTechLuaAttemptMarker{63,0,true,false,true});
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.profile.can_activate &&
          bootstrap.profile.enabled_dlc.size()==14 &&
          bootstrap.profile.enabled_mods.empty(),
          "Selected Gandhi active profile changed");
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    auto detached=host.session().state();
    BnwTeamTechLuaAttempt quote;
    quote.status=BnwTeamTechLuaStatus::quoted;
    quote.hook_value=true;
    const auto blocked=plan_bnw_team_tech_after_lua(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(blocked.first_pending==BnwTeamTechAfterLuaStep::lua_quote &&
          blocked.proven_no_effect.empty() && !blocked.grant_complete,
          "Absent real Lua quote passed post-hook plan");
    acquire_fixture(detached);
    const auto before=detached.world.teams->teams[63].fields;
    const auto plan=plan_bnw_team_tech_after_lua(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(proven(plan,BnwTeamTechAfterLuaStep::ends_game) &&
          proven(plan,BnwTeamTechAfterLuaStep::artifact_notifications) &&
          proven(plan,BnwTeamTechAfterLuaStep::era_evaluation) &&
          proven(plan,BnwTeamTechAfterLuaStep::map_reveal) &&
          plan.first_pending==BnwTeamTechAfterLuaStep::best_route &&
          !plan.blocker.empty() && !plan.grant_complete,
          "Selected Gandhi post-Lua no-effect prefix changed");
    check(detached.world.teams->teams[63].fields.size()==before.size() &&
          !host.session().state().technologies[63].has_tech(0),
          "Read-only plan changed live selected session");

    auto altered=host.technologies();
    altered.entries[0].booleans["EndsGame"]=true;
    const auto ends=plan_bnw_team_tech_after_lua(
        detached,quote,altered,host.database(),types,63,0);
    check(ends.first_pending==BnwTeamTechAfterLuaStep::ends_game &&
          ends.proven_no_effect.empty(),
          "EndsGame world mutation was silently skipped");
    altered.entries[0].booleans.erase("EndsGame");
    const auto unknown_ends=plan_bnw_team_tech_after_lua(
        detached,quote,altered,host.database(),types,63,0);
    check(unknown_ends.first_pending==BnwTeamTechAfterLuaStep::ends_game &&
          unknown_ends.proven_no_effect.empty(),
          "Unknown EndsGame rule was treated as false");
    altered=host.technologies();
    altered.entries[0].booleans["MapVisible"]=true;
    const auto map=plan_bnw_team_tech_after_lua(
        detached,quote,altered,host.database(),types,63,0);
    check(map.first_pending==BnwTeamTechAfterLuaStep::map_reveal &&
          !proven(map,BnwTeamTechAfterLuaStep::map_reveal),
          "MapVisible world mutation was silently skipped");

    content::RulesDatabase artifact_rules;
    artifact_rules.apply_sql(
        "CREATE TABLE Resources(Type TEXT,TechReveal TEXT);"
        "INSERT INTO Resources VALUES('RESOURCE_ARTIFACTS','TECH_AGRICULTURE');"
        "INSERT INTO Resources VALUES('RESOURCE_HIDDEN_ARTIFACTS',NULL);");
    const auto artifact=plan_bnw_team_tech_after_lua(
        detached,quote,host.technologies(),artifact_rules,types,63,0);
    check(artifact.first_pending==BnwTeamTechAfterLuaStep::artifact_notifications &&
          proven(artifact,BnwTeamTechAfterLuaStep::ends_game) &&
          !proven(artifact,BnwTeamTechAfterLuaStep::artifact_notifications),
          "Active artifact notification branch was silently skipped");

    auto uncertain=detached;
    uncertain.setup.extensions.find<BnwTeamTechLuaAttemptMarker>()->uncertain=true;
    const auto retry=plan_bnw_team_tech_after_lua(
        uncertain,quote,host.technologies(),host.database(),types,63,0);
    check(retry.first_pending==BnwTeamTechAfterLuaStep::lua_quote &&
          retry.proven_no_effect.empty(),
          "Uncertain Lua dispatch passed the gate");
}
