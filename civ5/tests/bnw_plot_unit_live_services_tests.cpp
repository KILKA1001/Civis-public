#include "civ5/runtime/bnw_plot_unit_live_services.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::int64_t &plot_field(civ5::saves::PlotObservation &plot,
                         std::string_view wanted) {
    for(auto &field:plot.fields)
        if(field.name && std::string_view(field.name)==wanted)
            return field.value;
    throw std::runtime_error("Missing selected plot field");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    auto boot=civ5::ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(!boot.preview.blocker && boot.preview.host,
          "Selected Gandhi host is unavailable");
    const auto &database=boot.preview.host->database();
    const auto rules=civ5::runtime::load_bnw_plot_domain_rules(
        database,database.cache_info_types());
    const auto services=civ5::runtime::make_bnw_plot_unit_no_effect_services(
        rules,&database);
    check(static_cast<bool>(services.visible_enemy) &&
          static_cast<bool>(services.invisible),
          "BNW visibility callbacks were not installed");
    auto &state=boot.preview.host->session().mutable_state();
    const auto roster=civ5::runtime::preflight_bnw_map_occupied(state.world);
    check(roster.shape_valid && roster.units.size()==7,
          "Selected Gandhi unit roster changed");
    for(const auto &unit:roster.units) {
        const auto plan=civ5::runtime::preflight_bnw_verify_unit_valid_plot(
            state,unit.plot,1,services);
        if(!plan.complete_no_effect)
            std::cerr<<"plot="<<unit.plot<<" reason="<<plan.reason<<'\n';
        check(plan.source_valid && plan.complete_no_effect &&
              plan.effects.empty(),
              "Selected Gandhi occupied plot lacks a complete no-effect proof");
        const auto outcome=civ5::runtime::run_bnw_verified_no_effect_plot_units(
            state,unit.plot,1,services);
        check(outcome.outcome==civ5::runtime::LiveActionOutcome::committed,
              "Selected Gandhi no-effect plot unit stage rejected");
    }
    auto incomplete=services;
    incomplete.can_enter_territory={};
    const auto rejected=civ5::runtime::run_bnw_verified_no_effect_plot_units(
        state,roster.units.front().plot,1,incomplete);
    check(rejected.outcome==civ5::runtime::LiveActionOutcome::rejected,
          "Absent territory service was silently accepted");

    // Make the occupied Gandhi plot foreign in a detached world. The source
    // second pass must now request both visibility predicates. Override the
    // earlier branch inputs so the test reaches each callback in SDK order.
    auto foreign=state;
    plot_field(foreign.world.map_plots->plots[592],"owner")=1;
    foreign.world.map_projection->cells[592].owner=1;
    const auto unit_team=foreign.setup.players.at(0).team;
    const auto plot_team=foreign.setup.players.at(1).team;
    check(unit_team!=plot_team,"Selected Gandhi test teams coincide");
    auto required=services;
    required.valid_domain=[](const auto &,auto,auto) {return true;};
    required.can_enter_territory=[](const auto &,auto,auto) {return true;};
    required.unit_team=[unit_team](const auto &,auto,auto) {
        return std::optional<std::int64_t>(unit_team);
    };
    required.plot_team=[plot_team](const auto &,auto,auto) {
        return std::optional<std::int64_t>(plot_team);
    };
    required.visible_enemy={};
    auto no_enemy_answer=civ5::runtime::preflight_bnw_verify_unit_valid_plot(
        foreign,592,1,required);
    check(no_enemy_answer.source_valid && !no_enemy_answer.complete_no_effect &&
          no_enemy_answer.reason.find("visible_enemy")!=std::string::npos,
          "Missing visible-enemy predicate was silently accepted");
    required.visible_enemy=[](const auto &,auto,auto) {return true;};
    required.invisible={};
    auto no_invisibility_answer=civ5::runtime::preflight_bnw_verify_unit_valid_plot(
        foreign,592,1,required);
    check(no_invisibility_answer.source_valid &&
          !no_invisibility_answer.complete_no_effect &&
          no_invisibility_answer.reason.find("invisible")!=std::string::npos,
          "Missing invisibility predicate was silently accepted");
    required.invisible=[](const auto &,auto,auto) {return false;};
    const auto effect=civ5::runtime::preflight_bnw_verify_unit_valid_plot(
        foreign,592,1,required);
    check(effect.source_valid && !effect.effects.empty() &&
          !effect.complete_no_effect,
          "Required visible-enemy jump/kill was silently skipped");
    const auto refused=civ5::runtime::run_bnw_verified_no_effect_plot_units(
        foreign,592,1,required);
    check(refused.outcome==civ5::runtime::LiveActionOutcome::rejected,
          "No-effect service accepted required jump/kill effects");
    std::cout<<"PASS selected Gandhi no-effect unit validity: "
             <<roster.units.size()<<" occupied plots; unknown/effect rejected\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected BNW save and game root");
        selected(argv[1],argv[2]);
        return 0;
    } catch(const std::exception &error) {
        std::cerr<<"FAIL BNW live plot unit services: "<<error.what()<<'\n';
        return 1;
    }
}
