#include "civ5/runtime/bnw_team_tech_barbarian_traits.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
saves::PlayerCoreField &core(LiveSessionState &state,std::string_view name) {
    for(auto &field:state.world.all_players->players[63].core.fields)
        if(field.name && name==field.name) return field;
    throw std::runtime_error("Missing player core field");
}
const saves::PlayerCoreField &core(const LiveSessionState &state,std::string_view name) {
    for(const auto &field:state.world.all_players->players[63].core.fields)
        if(field.name && name==field.name) return field;
    throw std::runtime_error("Missing player core field");
}
saves::PlayerTraitsField &trait(LiveSessionState &state,std::string_view name) {
    for(auto &field:state.world.all_players->players[63].traits.fields)
        if(field.name && name==field.name) return field;
    throw std::runtime_error("Missing player trait field");
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    auto &host=*bootstrap.preview.host;
    auto types=host.database().cache_info_types();
    auto base=host.session().state();
    std::int64_t agriculture=-1;
    for(const auto &[slot,info]:host.technologies().entries)
        if(info.base.type=="TECH_AGRICULTURE") agriculture=slot;
    check(agriculture==0,"Selected Agriculture slot changed");
    base.technologies[63].set_has_tech(agriculture,true);
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:base.world.teams->teams[63].arrays) {
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

    auto draft=base;
    const auto &no_train=trait(draft,"m_abNoTrain");
    const auto unit_report=host.database().prefetch_info("UnitClasses");
    check(unit_report.all_succeeded(),"UnitClasses query failed");
    check(no_train.wire_count && *no_train.wire_count==97 &&
          no_train.values.size()==97 && unit_report.rows==83 &&
          unit_report.holes==14 && unit_report.slots==97,
          "Selected Gandhi trait/rule dimension evidence changed");
    const auto rng=draft.random.gameplay.state();
    const auto before_policy=core(draft,"m_iPolicyCostModifier").values[0];
    const auto result=run_bnw_barbarian_trait_reset(draft,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(result.applied && !result.grant_complete &&
          result.checked_trait_fields>=90 &&
          draft.setup.extensions.find<BnwBarbarianTraitResetTransient>() &&
          draft.setup.extensions.find<BnwBarbarianTraitResetTransient>()
              ->free_resource_city_entries==
              host.database().prefetch_info("Resources").slots &&
          result.policy_cost_before==before_policy &&
          core(draft,"m_iPolicyCostModifier").values[0]==result.policy_cost_after &&
          draft.random.gameplay.state()==rng &&
          !host.session().state().technologies[63].has_tech(agriculture),
          "Selected Gandhi trait reset did not apply after matching sparse slots");
    auto altered_cost=base;
    core(altered_cost,"m_iPolicyCostBuildingModifier").values[0]=5;
    const auto changed=run_bnw_barbarian_trait_reset(altered_cost,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(changed.applied && changed.policy_cost_after==5 &&
          core(altered_cost,"m_iPolicyCostModifier").values[0]==5 &&
          core(base,"m_iPolicyCostModifier").values[0]==before_policy &&
          core(host.session().state(),"m_iPolicyCostModifier").values[0]==before_policy,
          "Policy cost recomputation leaked or omitted the live write");
    auto changed_dimensions=base;
    *trait(changed_dimensions,"m_abNoTrain").wire_count=96;
    const auto rejected_dimensions=run_bnw_barbarian_trait_reset(
        changed_dimensions,host.technologies(),host.database(),types,
        host.defines(),agriculture);
    check(!rejected_dimensions.applied &&
          rejected_dimensions.blocker==
              "BNW barbarian no-train dimensions differ from Reset" &&
          !changed_dimensions.setup.extensions.find<BnwBarbarianTraitResetTransient>(),
          "Mismatched indexed trait dimension crossed the preflight");
    auto changed_trait=base;
    trait(changed_trait,"m_iGreatPeopleRateModifier").values[0]=1;
    const auto rejected_trait=run_bnw_barbarian_trait_reset(changed_trait,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(!rejected_trait.applied &&
          rejected_trait.blocker=="BNW barbarian trait scalar differs from Reset/Init" &&
          trait(changed_trait,"m_iGreatPeopleRateModifier").values[0]==1,
          "Nondefault trait cache crossed no-trait Reset/Init");
    auto changed_policy=base;
    bool marked=false;
    for(auto &block:changed_policy.world.all_players->players[63].strategy.blocks)
        if(block.name && std::string_view(block.name)=="policies")
            for(auto &array:block.hashes)
                if(array.name && std::string_view(array.name)=="has_policy")
                    for(auto &row:array.rows)
                        if(!marked && row.type_hash && row.values.size()==1) {
                            row.values[0]=1;
                            marked=true;
                        }
    check(marked,"Selected barbarian policy hash rows unavailable");
    const auto rejected_policy=run_bnw_barbarian_trait_reset(changed_policy,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(!rejected_policy.applied &&
          rejected_policy.blocker==
              "Barbarian owns a policy requiring numeric modifier" &&
          !changed_policy.setup.extensions.find<BnwBarbarianTraitResetTransient>(),
          "Adopted policy crossed the zero numeric modifier proof");
}
