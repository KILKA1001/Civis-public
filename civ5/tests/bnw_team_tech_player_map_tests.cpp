#include "civ5/runtime/bnw_team_tech_player_map.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
void grant_low_level_flag(LiveSessionState &state,std::int64_t technology,
                          std::string_view type) {
    state.technologies[63].set_has_tech(technology,true);
    auto &team=state.world.teams->teams[63];
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:team.arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags && hashes->size()==flags->size(),
          "Selected raw technology arrays unavailable");
    std::size_t found=0;
    const auto hash=content::saved_info_type_hash(type);
    for(std::size_t row=0;row<hashes->size();++row)
        if((*hashes)[row]==hash) {(*flags)[row]=1;++found;}
    check(found==1,"Selected technology hash ambiguous");
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
    check(agriculture==0,"Selected Agriculture slot changed");
    grant_low_level_flag(state,agriculture,"TECH_AGRICULTURE");
    const auto rng=state.random.gameplay.state();
    const auto plan=preflight_bnw_barbarian_player_map_tech(
        state,host.technologies(),host.database(),types,agriculture);
    std::cout << "selected Agriculture ever-alive members=";
    for(const auto player:plan.ever_alive_members) std::cout << player << ',';
    std::cout << " modifier_effects=" << plan.nonzero_player_modifiers
              << " promotions=" << plan.free_promotions
              << " reveal_rules=" << plan.resource_reveal_rules
              << " route_build_rules=" << plan.route_build_rules
              << " pending=" << static_cast<int>(plan.first_pending)
              << " blocker=" << plan.blocker << '\n';
    check(!plan.grant_complete && plan.nonzero_player_modifiers==0 &&
          plan.free_promotions==0 && plan.resource_reveal_rules==0 &&
          plan.route_build_rules==0 &&
          state.random.gameplay.state()==rng &&
          !host.session().state().technologies[63].has_tech(agriculture),
          "Selected Gandhi player/map preflight changed");

    auto rules=host.technologies();
    rules.entries[static_cast<std::uint64_t>(agriculture)]
        .integers["WorkerSpeedModifier"]=1;
    const auto modifier=preflight_bnw_barbarian_player_map_tech(
        state,rules,host.database(),types,agriculture);
    if(!plan.ever_alive_members.empty())
        check(modifier.first_pending==BnwTechPlayerMapGate::player_modifiers &&
              modifier.nonzero_player_modifiers==plan.ever_alive_members.size(),
              "Nonzero player modifier was skipped");

    rules=host.technologies();
    auto &promotions=rules.entries[static_cast<std::uint64_t>(agriculture)]
        .free_promotions;
    check(promotions.extent>0,"Selected promotion rule extent unavailable");
    promotions.values[0]=1;
    const auto free_promotion=preflight_bnw_barbarian_player_map_tech(
        state,rules,host.database(),types,agriculture);
    if(!plan.ever_alive_members.empty())
        check(free_promotion.first_pending==BnwTechPlayerMapGate::free_promotions &&
              free_promotion.free_promotions==plan.ever_alive_members.size(),
              "Technology free promotion was skipped");

    auto route_db=host.database().clone_gameplay();
    route_db->apply_sql(
        "UPDATE Builds SET PrereqTech='TECH_AGRICULTURE',"
        "RouteType=(SELECT Type FROM Routes ORDER BY ID LIMIT 1) WHERE ID=0");
    const auto route=preflight_bnw_barbarian_player_map_tech(
        state,host.technologies(),*route_db,types,agriculture);
    check(route.route_build_rules>0 && !route.grant_complete,
          "Future city-route effect was not discovered");
}
}

int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and installed game root");
    selected(argv[1],argv[2]);
}
