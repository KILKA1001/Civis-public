#include "civ5/runtime/bnw_plot_domain_binding.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool valid,const char *message) {
    if(!valid) throw std::runtime_error(message);
}
const char *domain_name(runtime::BnwUnitDomain value) {
    switch(value) {
    case runtime::BnwUnitDomain::land:return "LAND";
    case runtime::BnwUnitDomain::sea:return "SEA";
    case runtime::BnwUnitDomain::air:return "AIR";
    case runtime::BnwUnitDomain::immobile:return "IMMOBILE";
    case runtime::BnwUnitDomain::hover:return "HOVER";
    }
    return "UNKNOWN";
}
saves::NamedSaveInteger &plot_field(saves::PlotObservation &plot,
                                     std::string_view wanted) {
    for(auto &field:plot.fields)
        if(field.name && wanted==field.name) return field;
    throw std::runtime_error("Synthetic BNW plot field unavailable");
}
saves::UnitCoreField &unit_field(saves::SavedUnitObservation &unit,
                                 std::string_view wanted) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name) return field;
    throw std::runtime_error("Synthetic BNW unit field unavailable");
}
std::int64_t unit_type(runtime::LiveSessionState &state,
                       saves::SavedPlotUnitReference handle) {
    for(auto &player:state.world.all_players->players)
        for(auto &unit:player.units.units)
            if(unit_field(unit,"m_eOwner").values.front()==handle.owner &&
               unit_field(unit,"m_iID").values.front()==handle.id)
                return unit_field(unit,"m_eUnitType").values.front();
    throw std::runtime_error("Synthetic BNW unit absent");
}
void synthetic(const runtime::LiveSessionState &baseline,
               const runtime::BnwPlotDomainRules &active,
               const runtime::BnwOccupiedPlotUnit &site) {
    auto state=baseline;
    auto rules=active;
    const auto type=unit_type(state,site.handle);
    auto &kind=rules.units.at(type).domain;
    kind=runtime::BnwUnitDomain::sea;
    auto verdict=runtime::evaluate_bnw_plot_valid_domain(
        state,site.plot,site.handle,rules);
    check(verdict.valid && !*verdict.valid,
          "Sea unit on a land plot passed domain check");
    auto &plot=state.world.map_plots->plots[site.plot];
    auto &cell=state.world.map_projection->cells[site.plot];
    plot_field(plot,"plot_type").value=3;
    cell.plot_type=3;
    verdict=runtime::evaluate_bnw_plot_valid_domain(
        state,site.plot,site.handle,rules);
    check(verdict.valid && *verdict.valid && verdict.water,
          "Sea unit on water failed domain check");
    kind=runtime::BnwUnitDomain::land;
    verdict=runtime::evaluate_bnw_plot_valid_domain(
        state,site.plot,site.handle,rules);
    check(verdict.valid && !*verdict.valid,
          "Unembarked land unit on water passed domain check");
    for(auto &player:state.world.all_players->players)
        for(auto &unit:player.units.units)
            if(unit_field(unit,"m_eOwner").values.front()==site.handle.owner &&
               unit_field(unit,"m_iID").values.front()==site.handle.id) {
                unit_field(unit,"m_bEmbarked").values.front()=1;
            }
    verdict=runtime::evaluate_bnw_plot_valid_domain(
        state,site.plot,site.handle,rules);
    check(verdict.valid && *verdict.valid && verdict.embarked,
          "Embarked land unit on water failed domain check");
    for(auto &player:state.world.all_players->players)
        for(auto &unit:player.units.units)
            if(unit_field(unit,"m_eOwner").values.front()==site.handle.owner &&
               unit_field(unit,"m_iID").values.front()==site.handle.id)
                unit_field(unit,"m_bEmbarked").values.front()=0;
    kind=runtime::BnwUnitDomain::hover;
    verdict=runtime::evaluate_bnw_plot_valid_domain(
        state,site.plot,site.handle,rules);
    check(verdict.valid && *verdict.valid &&
          verdict.effective_domain==runtime::BnwUnitDomain::sea,
          "DOMAIN_HOVER did not become sea over water");
    kind=runtime::BnwUnitDomain::air;
    verdict=runtime::evaluate_bnw_plot_valid_domain(
        state,site.plot,site.handle,rules);
    check(verdict.valid && !*verdict.valid,
          "Air unit outside city passed domain check");
    kind=runtime::BnwUnitDomain::land;
    if(!rules.improvements.empty()) {
        const auto id=rules.improvements.begin()->first;
        rules.improvements.at(id).allows_walk_water=true;
        plot_field(plot,"improvement_type_hash").value=
            rules.improvements.at(id).hash;
        cell.improvement_type=id;
        verdict=runtime::evaluate_bnw_plot_valid_domain(
            state,site.plot,site.handle,rules);
        check(verdict.valid && *verdict.valid && verdict.walk_water,
              "AllowsWalkWater improvement did not permit land unit");
        ++plot_field(plot,"improvement_type_hash").value;
        verdict=runtime::evaluate_bnw_plot_valid_domain(
            state,site.plot,site.handle,rules);
        check(!verdict.valid,
              "Mismatched improvement hash was accepted");
    }
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    auto boot=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!boot.preview.blocker && boot.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto frame=boot.preview.host->preview_input().frame_sources;
    check(frame.database,"Selected BNW rules database unavailable");
    const auto types=frame.database->cache_info_types();
    const auto rules=runtime::load_bnw_plot_domain_rules(*frame.database,types);
    const auto &state=boot.preview.host->session().state();
    const auto occupied=runtime::preflight_bnw_map_occupied(state.world);
    check(occupied.shape_valid && occupied.units.size()==7,
          "Selected Gandhi occupied unit roster changed");
    std::size_t valid=0,invalid=0;
    for(const auto &unit:occupied.units) {
        const auto verdict=runtime::evaluate_bnw_plot_valid_domain(
            state,unit.plot,unit.handle,rules);
        check(verdict.valid.has_value() && verdict.effective_domain,
              "Selected Gandhi domain binding is unresolved");
        valid+=*verdict.valid;
        invalid+=!*verdict.valid;
        std::cout << "plot=" << unit.plot << " owner=" << unit.handle.owner
                  << " id=" << unit.handle.id
                  << " domain=" << domain_name(*verdict.effective_domain)
                  << " water=" << verdict.water << " city=" << verdict.city
                  << " hovering=" << verdict.hovering
                  << " all_terrain=" << verdict.all_terrain
                  << " embarked=" << verdict.embarked
                  << " walk_water=" << verdict.walk_water
                  << " valid=" << *verdict.valid << '\n';
        const auto callback=runtime::bind_bnw_plot_valid_domain(rules);
        check(callback(state,unit.plot,unit.handle)==verdict.valid,
              "BNW domain callback disagrees with direct verdict");
        runtime::BnwPlotUnitReadServices queries;
        queries.valid_domain=callback;
        const auto phase=runtime::preflight_bnw_verify_unit_valid_plot(
            state,unit.plot,1,queries);
        check(phase.source_valid && !phase.complete_no_effect,
              "Domain callback unexpectedly completed entire unit validity");
        if(*verdict.valid)
            check(phase.effects.empty() && !phase.steps.empty() &&
                  phase.steps.back().check==
                      runtime::BnwPlotUnitCheck::can_enter_territory,
                  "Valid domain did not advance to territory predicate");
        else
            check(phase.effects.size()==1 &&
                  phase.effects.front().cause==
                      runtime::BnwPlotUnitEffectCause::invalid_domain_or_territory,
                  "Invalid domain did not require jump/kill");
    }
    check(valid+invalid==7,"Selected Gandhi domain verdict count changed");
    synthetic(state,rules,occupied.units.front());
    std::cout << "PASS selected Gandhi plot domain binding valid=" << valid
              << " invalid=" << invalid << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW plot domain binding: " << error.what()
                  << '\n';return 1;
    }
}
