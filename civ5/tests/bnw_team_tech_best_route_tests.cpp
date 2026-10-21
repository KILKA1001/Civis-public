#include "civ5/runtime/bnw_team_tech_best_route.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/info_types.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
std::int64_t saved_best_route(const LiveSessionState &state) {
    for(const auto &field:state.world.teams->teams[63].fields)
        if(field.name && std::string_view(field.name)=="best_route")
            return field.value;
    throw std::runtime_error("Missing selected best route");
}
void set_saved_best_route(LiveSessionState &state,std::int64_t route) {
    for(auto &field:state.world.teams->teams[63].fields)
        if(field.name && std::string_view(field.name)=="best_route") {
            field.value=route;
            return;
        }
    throw std::runtime_error("Missing mutable selected best route");
}
void agriculture_quote_fixture(LiveSessionState &state) {
    // A fixture for this effect test, not a selected stock Lua dispatch.
    state.technologies[63].set_has_tech(0,true);
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:state.world.teams->teams[63].arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags && hashes->size()==flags->size(),
          "Selected raw technology arrays unavailable");
    std::size_t found=0;
    for(std::size_t i=0;i<hashes->size();++i)
        if((*hashes)[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            (*flags)[i]=1;
            ++found;
        }
    check(found==1,"Selected Agriculture hash is ambiguous");
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
    const auto live_before=saved_best_route(detached);
    BnwTeamTechLuaAttempt quote;
    quote.status=BnwTeamTechLuaStatus::quoted;
    quote.hook_value=true;
    const auto blocked=run_bnw_team_tech_best_route(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(!blocked.applied && !blocked.grant_complete &&
          saved_best_route(detached)==live_before,
          "Best route executed before Lua quote marker");

    agriculture_quote_fixture(detached);
    const auto applied=run_bnw_team_tech_best_route(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(applied.applied && !applied.grant_complete &&
          applied.inspected_builds>0 &&
          applied.before==live_before &&
          applied.after==saved_best_route(detached) &&
          detached.setup.extensions.find<BnwTeamTechBestRouteMarker>() &&
          saved_best_route(host.session().state())==live_before,
          "Selected BNW best route did not update only the detached team");
    const auto repeated=run_bnw_team_tech_best_route(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(!repeated.applied && !repeated.grant_complete &&
          saved_best_route(detached)==applied.after,
          "Best route effect replayed");
    std::cout << "selected Gandhi best_route " << applied.before << " -> "
              << applied.after << ", winning build " << applied.winning_build
              << ", value " << applied.route_value << ", inspected "
              << applied.inspected_builds << " builds\n";

    auto fake=host.session().state();
    agriculture_quote_fixture(fake);
    const auto agriculture_cost=host.technologies().entries.at(0).integers.at("Cost");
    check(agriculture_cost>=1,"Selected Agriculture raw research cost unavailable");
    const std::array<BnwBuildRouteRule,4> rules{{
        {0,0,-1}, {1,1,0}, {2,0,0}, {3,1,1}}};
    const auto chosen=select_bnw_best_route(rules,fake.technologies[63],
                                            host.technologies());
    check(chosen.route==0 && chosen.value==agriculture_cost &&
          chosen.winning_build==2,
          "Source last-build tie or raw tech cost changed");
    auto missing=fake;
    missing.technologies[63].set_has_tech(0,false);
    const auto no_tech=select_bnw_best_route(rules,missing.technologies[63],
                                             host.technologies());
    check(no_tech.route==0 && no_tech.value==1 && no_tech.winning_build==0,
          "Unowned prerequisite route was considered available");
    auto altered=host.technologies();
    altered.entries[0].integers.erase("Cost");
    bool rejected=false;
    try { (void)select_bnw_best_route(rules,fake.technologies[63],altered); }
    catch(const std::runtime_error &) { rejected=true; }
    check(rejected,"Unknown raw technology cost was accepted");

    auto stale=host.session().state();
    agriculture_quote_fixture(stale);
    const auto *road=types.find("ROUTE_ROAD");
    check(road && road->table=="Routes","Selected road type unavailable");
    set_saved_best_route(stale,road->id);
    const auto corrected=run_bnw_team_tech_best_route(
        stale,quote,host.technologies(),host.database(),types,63,0);
    check(corrected.applied && corrected.before==road->id &&
          corrected.after==applied.after &&
          saved_best_route(stale)==applied.after &&
          saved_best_route(host.session().state())==live_before,
          "Detached team best-route scalar was not corrected");

    auto incomplete=host.session().state();
    agriculture_quote_fixture(incomplete);
    set_saved_best_route(incomplete,road->id);
    content::RulesDatabase no_build_rules;
    no_build_rules.apply_sql(
        "CREATE TABLE Resources(Type TEXT,TechReveal TEXT);");
    const auto missing_builds=run_bnw_team_tech_best_route(
        incomplete,quote,host.technologies(),no_build_rules,types,63,0);
    check(!missing_builds.applied &&
          saved_best_route(incomplete)==road->id &&
          !incomplete.setup.extensions.find<BnwTeamTechBestRouteMarker>(),
          "Missing active Builds rules partially changed detached team");
}
