#include "civ5/runtime/bnw_team_tech_post.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *message) {
    if(!value) throw std::runtime_error(message);
}
bool contains(const BnwTeamTechPostPrefix &prefix,BnwTeamTechPostStep step) {
    return std::find(prefix.proven_no_effect.begin(),
                     prefix.proven_no_effect.end(),step)!=prefix.proven_no_effect.end();
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    auto state=host.session().state();
    std::int64_t agriculture=-1;
    for(const auto &[slot,info]:host.technologies().entries)
        if(info.base.type=="TECH_AGRICULTURE")
            agriculture=static_cast<std::int64_t>(slot);
    check(agriculture>=0 && !state.technologies[63].has_tech(agriculture),
          "Selected Gandhi barbarian grant changed");
    const auto before=run_bnw_barbarian_tech_post_prefix(
        state,host.technologies(),host.database(),types,agriculture);
    check(!before.grant_complete && before.proven_no_effect.empty() &&
          before.first_pending==BnwTeamTechPostStep::city_strength,
          "Post-low-level continuation accepted missing technology flag");

    // Simulate only the owned/raw flags supplied by the separate low-level
    // acquisition service. No caller-visible session or phase is committed.
    state.technologies[63].set_has_tech(agriculture,true);
    auto &team=state.world.teams->teams[63];
    const auto hash=content::saved_info_type_hash("TECH_AGRICULTURE");
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:team.arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags && hashes->size()==flags->size(),
          "Selected Gandhi raw technology arrays unavailable");
    std::size_t found=0;
    for(std::size_t row=0;row<hashes->size();++row)
        if((*hashes)[row]==static_cast<std::int64_t>(hash)) {
            check((*flags)[row]==0,"Selected Gandhi Agriculture already raw-owned");
            (*flags)[row]=1;
            ++found;
        }
    check(found==1,"Agriculture hash did not map to one saved row");

    const auto result=run_bnw_barbarian_tech_post_prefix(
        state,host.technologies(),host.database(),types,agriculture);
    check(result.technology_type=="TECH_AGRICULTURE" &&
          !result.grant_complete &&
          contains(result,BnwTeamTechPostStep::city_strength) &&
          contains(result,BnwTeamTechPostStep::resource_reveal_and_trade) &&
          contains(result,BnwTeamTechPostStep::process_team_scalar_prefix) &&
          result.first_pending==BnwTeamTechPostStep::process_route_and_build_changes &&
          result.pending_effects.size()==6 &&
          result.pending_effects[3]==BnwTeamTechPostStep::process_lua_team_tech_researched &&
          result.pending_effects.back()==BnwTeamTechPostStep::gameplay_tech_acquired,
          "Selected Gandhi Agriculture no-effect prefix changed");
    check(!host.session().state().technologies[63].has_tech(agriculture),
          "Post-acquisition prefix mutated the live Gandhi session");

    auto nonzero=host.technologies();
    nonzero.entries[static_cast<std::uint64_t>(agriculture)].booleans["TechTrading"]=true;
    const auto scalar=run_bnw_barbarian_tech_post_prefix(
        state,nonzero,host.database(),types,agriculture);
    check(!scalar.grant_complete &&
          scalar.first_pending==BnwTeamTechPostStep::process_team_scalar_prefix &&
          contains(scalar,BnwTeamTechPostStep::resource_reveal_and_trade),
          "Nonzero processTech scalar was silently skipped");
    content::RulesDatabase resource_database;
    resource_database.apply_sql(
        "CREATE TABLE Resources(TechReveal TEXT,TechCityTrade TEXT);"
        "INSERT INTO Resources VALUES('TECH_AGRICULTURE',NULL);");
    const auto reveal=run_bnw_barbarian_tech_post_prefix(
        state,host.technologies(),resource_database,types,agriculture);
    check(!reveal.grant_complete &&
          reveal.first_pending==BnwTeamTechPostStep::resource_reveal_and_trade &&
          contains(reveal,BnwTeamTechPostStep::city_strength),
          "Active resource reveal was silently skipped");

    (*flags)[std::distance(hashes->begin(),std::find(hashes->begin(),hashes->end(),
                              static_cast<std::int64_t>(hash)))]=0;
    const auto divergent=run_bnw_barbarian_tech_post_prefix(
        state,host.technologies(),host.database(),types,agriculture);
    check(divergent.proven_no_effect.empty() && !divergent.grant_complete,
          "Divergent raw/owned technology flags passed post-acquisition preflight");
}
}

int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and installed game root");
    selected(argv[1],argv[2]);
}
