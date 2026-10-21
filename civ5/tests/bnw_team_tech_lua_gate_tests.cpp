#include "civ5/runtime/bnw_team_tech_lua_gate.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
LiveSessionState after_post_traits(ui::SelectedBnwPreviewHost &host,
                                  const content::InfoTypeCatalog &types) {
    auto state=host.session().state();
    constexpr std::int64_t agriculture=0;
    state.technologies[63].set_has_tech(agriculture,true);
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:state.world.teams->teams[63].arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags,"Missing raw technology arrays");
    std::size_t found=0;
    for(std::size_t i=0;i<hashes->size();++i)
        if((*hashes)[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            (*flags)[i]=1;++found;
        }
    check(found==1,"Ambiguous Agriculture raw technology row");
    const auto traits=run_bnw_barbarian_trait_reset(state,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(traits.applied && !traits.grant_complete,"BNW trait prefix unavailable");
    const auto post=run_bnw_barbarian_post_traits(state,
        host.technologies(),host.database(),types,agriculture);
    check(post.applied && !post.grant_complete,"BNW post-trait prefix unavailable");
    return state;
}
BnwTeamTechLuaEpoch fixture_epoch(
    LiveSessionState &state,const SavedContentProfile &profile,
    const SessionCheckpointIdentity &identity,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,std::string_view source) {
    BnwTeamTechLuaEpoch epoch;
    epoch.world=std::make_unique<LiveSessionState>(state);
    epoch.source_world=&state;
    epoch.getter_world=epoch.world.get();
    epoch.identity=identity;
    epoch.selected_dlc=profile.enabled_dlc;
    epoch.selected_mods=profile.enabled_mods;
    epoch.host=std::make_unique<LuaScriptHost>(ScriptBudget{8ULL*1024*1024,1'000'000},
        ScriptSourceProvider{},ScriptModdingProvider{},
        lua_gameplay_services(database,epoch.world->setup,epoch.world->technologies,types));
    epoch.host->execute("gameplay",source,"=test-only-hook");
    epoch.host->complete_loading();
    return epoch;
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
          "Selected Gandhi is not the pinned stock BNW profile");
    auto &host=*bootstrap.preview.host;
    auto types=host.database().cache_info_types();
    const auto identity=host.identity();
    auto state=after_post_traits(host,types);
    const auto prior=state;

    const auto no_owner=quote_bnw_team_tech_researched(
        state,bootstrap.profile,identity,nullptr,0);
    check(no_owner.status==BnwTeamTechLuaStatus::blocked &&
          no_owner.reason==
              "BNW TeamTechResearched has no selected gameplay Lua context epoch" &&
          !state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>(),
          "Selected Gandhi incorrectly treated an absent Lua owner as no listeners");

    // A loading-complete empty VM still has no evidence of the actual selected
    // stock/DLC context epoch and must not satisfy the retail callback.
    auto empty=fixture_epoch(state,bootstrap.profile,identity,
        host.database(),types,"");
    const auto no_sources=quote_bnw_team_tech_researched(
        state,bootstrap.profile,identity,&empty,0);
    check(no_sources.status==BnwTeamTechLuaStatus::blocked &&
          no_sources.reason==
              "BNW TeamTechResearched detached selected Lua epoch is unverified" &&
          !state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>(),
          "An empty Lua VM was accepted as selected listener proof");

    // Contract fixture only: this tests exact args and getters bound to the
    // copied world. It does not claim stock contexts are currently loaded.
    auto fixture=fixture_epoch(state,bootstrap.profile,identity,
        host.database(),types,
        "GameEvents.TeamTechResearched.Add(function(team,tech,change) "
        "assert(team==63 and tech==0 and change==1); "
        "assert(Teams[63]:IsHasTech(0)); return true end)");
    fixture.all_selected_contexts_loaded=true;
    fixture.all_listeners_registered=true;
    fixture.all_external_writes_escrowed=true;
    const auto quoted=quote_bnw_team_tech_researched(
        state,bootstrap.profile,identity,&fixture,0);
    const auto *marker=state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>();
    const auto *fork_marker=fixture.world->setup.extensions.find<BnwTeamTechLuaAttemptMarker>();
    check(quoted.status==BnwTeamTechLuaStatus::quoted &&
          quoted.hook_value==true && !quoted.grant_complete &&
          marker && marker->attempted && marker->quoted && !marker->uncertain &&
          fork_marker && fork_marker->quoted && !fork_marker->uncertain &&
          !prior.setup.extensions.find<BnwTeamTechLuaAttemptMarker>(),
          "Fixture hook did not read the detached Agriculture world");
    const auto duplicate=quote_bnw_team_tech_researched(
        state,bootstrap.profile,identity,&fixture,0);
    check(duplicate.status==BnwTeamTechLuaStatus::blocked,
          "Quoted Lua hook was retried");

    auto failing=prior;
    auto error_fixture=fixture_epoch(failing,bootstrap.profile,identity,
        host.database(),types,
        "GameEvents.TeamTechResearched.Add(function() "
        "error('test-hook-failure') end)");
    error_fixture.all_selected_contexts_loaded=true;
    error_fixture.all_listeners_registered=true;
    error_fixture.all_external_writes_escrowed=true;
    const auto uncertain=quote_bnw_team_tech_researched(
        failing,bootstrap.profile,identity,&error_fixture,0);
    const auto *failed=failing.setup.extensions.find<BnwTeamTechLuaAttemptMarker>();
    const auto *failed_fork=error_fixture.world->setup.extensions.find<BnwTeamTechLuaAttemptMarker>();
    check(uncertain.status==BnwTeamTechLuaStatus::uncertain &&
          !uncertain.grant_complete && failed && failed->attempted &&
          failed->uncertain && !failed->quoted &&
          failed_fork && failed_fork->uncertain && !failed_fork->quoted &&
          quote_bnw_team_tech_researched(failing,bootstrap.profile,identity,
              &error_fixture,0).status==BnwTeamTechLuaStatus::blocked,
          "Throwing Lua callback did not become uncertain without retry");
}
