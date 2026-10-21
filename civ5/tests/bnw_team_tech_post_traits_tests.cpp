#include "civ5/runtime/bnw_team_tech_post_traits.hpp"
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
saves::PlayerTraitsField &free_units(LiveSessionState &state) {
    for(auto &field:state.world.all_players->players[63].traits.fields)
        if(field.name && std::string_view(field.name)=="m_aFreeTraitUnits")
            return field;
    throw std::runtime_error("Missing saved free-trait-unit field");
}
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

    auto before_traits=base;
    const auto missing_prior=run_bnw_barbarian_post_traits(before_traits,
        host.technologies(),host.database(),types,agriculture);
    check(!missing_prior.applied &&
          missing_prior.blocker==
              "BNW barbarian trait reset must precede free-unit query exactly once" &&
          !before_traits.setup.extensions.find<BnwBarbarianPostTraitsTransient>(),
          "Post-trait stage ran without the prior live effect");

    const auto traits=run_bnw_barbarian_trait_reset(base,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(traits.applied && !traits.grant_complete,
          "Selected barbarian trait reset did not apply");
    auto stage=base;
    const auto rng=stage.random.gameplay.state();
    const auto result=run_bnw_barbarian_post_traits(stage,
        host.technologies(),host.database(),types,agriculture);
    const auto *transient=stage.setup.extensions.find<BnwBarbarianPostTraitsTransient>();
    check(result.applied && !result.grant_complete &&
          result.first_free_unit_class==-1 &&
          !result.resource_reveal_rules && !result.route_build_rules &&
          transient && transient->team==63 && transient->player==63 &&
          transient->free_unit_prereq_tech==agriculture &&
          transient->free_unit_index==0 && transient->free_unit_query_done &&
          transient->map_loops_done &&
          stage.random.gameplay.state()==rng &&
          !base.setup.extensions.find<BnwBarbarianPostTraitsTransient>(),
          "Selected barbarian free-unit/map prefix did not apply in source order");

    auto nonempty=base;
    auto &units=free_units(nonempty);
    *units.wire_count=1;
    units.values={0,0};
    const auto rejected=run_bnw_barbarian_post_traits(nonempty,
        host.technologies(),host.database(),types,agriculture);
    check(!rejected.applied &&
          rejected.blocker=="BNW barbarian trait free-unit query has an effect" &&
          !nonempty.setup.extensions.find<BnwBarbarianPostTraitsTransient>(),
          "Nonempty trait free-unit list skipped unit-grant effects");
    const auto duplicate=run_bnw_barbarian_post_traits(stage,
        host.technologies(),host.database(),types,agriculture);
    check(!duplicate.applied && !duplicate.grant_complete,
          "Post-trait source effects ran twice");
}
