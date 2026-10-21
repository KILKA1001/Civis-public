#include "civ5/runtime/bnw_team_tech_domain_yields.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
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
const std::vector<std::int64_t> &extra_moves(const LiveSessionState &state) {
    for(const auto &array:state.world.teams->teams[63].arrays)
        if(array.name && std::string_view(array.name)=="extra_moves")
            return array.values;
    throw std::runtime_error("Selected extra-moves array unavailable");
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    auto base=host.session().state();
    std::int64_t agriculture=-1;
    for(const auto &[slot,info]:host.technologies().entries)
        if(info.base.type=="TECH_AGRICULTURE")
            agriculture=static_cast<std::int64_t>(slot);
    check(agriculture==0,"Selected Agriculture slot changed");
    grant_low_level_flag(base,agriculture,"TECH_AGRICULTURE");
    auto draft=base;
    const auto rng=draft.random.gameplay.state();
    const auto result=run_bnw_barbarian_domain_yield_prefix(
        draft,host.technologies(),host.database(),types,agriculture);
    const auto nonzero=std::count_if(result.domain_calls.begin(),
        result.domain_calls.end(),[](const auto &call){return call.delta!=0;});
    std::cout << "selected Agriculture domains=" << result.domain_calls.size()
              << " nonzero=" << nonzero
              << " building_effects=" << result.building_effects
              << " improvement_calls=" << result.improvement_calls
              << " improvement_nonzero=" << result.improvement_nonzero
              << " applied=" << result.applied
              << " blocker=" << result.blocker << '\n';
    check(result.applied && !result.grant_complete &&
          result.domain_calls.size()==5 && nonzero==0 &&
          result.building_effects==0 && result.improvement_nonzero==0 &&
          result.improvement_calls>0 &&
          draft.random.gameplay.state()==rng &&
          !host.session().state().technologies[63].has_tech(agriculture),
          "Selected Gandhi domain/building/improvement prefix changed");

    auto domain_techs=host.technologies();
    auto &domain_rule=domain_techs.entries[static_cast<std::uint64_t>(agriculture)]
        .domain_extra_moves;
    check(domain_rule.extent>=5,"Selected domain rule extent unavailable");
    domain_rule.values[0]=2;
    auto changed=base;
    const auto before=extra_moves(changed)[0];
    const auto domain=run_bnw_barbarian_domain_yield_prefix(
        changed,domain_techs,host.database(),types,agriculture);
    check(domain.applied && domain.domain_calls[0].delta==2 &&
          extra_moves(changed)[0]==before+2 &&
          extra_moves(base)[0]==before,
          "Detached domain movement delta was not applied in source order");

    auto building_db=host.database().clone_gameplay();
    building_db->apply_sql(
        "UPDATE Buildings SET ObsoleteTech='TECH_AGRICULTURE' WHERE ID=0");
    auto blocked=base;
    const auto building=run_bnw_barbarian_domain_yield_prefix(
        blocked,domain_techs,*building_db,types,agriculture);
    check(!building.applied && building.building_effects>0 &&
          extra_moves(blocked)[0]==before,
          "Building obsolescence was skipped or partly applied");

    auto improvement_db=host.database().clone_gameplay();
    improvement_db->apply_sql(
        "INSERT INTO Improvement_TechYieldChanges"
        "(ImprovementType,TechType,YieldType,Yield) "
        "SELECT i.Type,'TECH_AGRICULTURE',y.Type,1 FROM Improvements i,Yields y "
        "WHERE i.ID=0 AND y.ID=0");
    blocked=base;
    const auto improvement=run_bnw_barbarian_domain_yield_prefix(
        blocked,domain_techs,*improvement_db,types,agriculture);
    check(!improvement.applied && improvement.improvement_nonzero>0 &&
          extra_moves(blocked)[0]==before,
          "Improvement updateYield was skipped or partly applied");
}
}

int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and installed game root");
    selected(argv[1],argv[2]);
}
