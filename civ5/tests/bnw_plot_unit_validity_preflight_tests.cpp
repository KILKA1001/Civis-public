#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
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
saves::NamedSaveInteger &field(saves::PlotObservation &plot,
                                std::string_view wanted) {
    for(auto &entry:plot.fields)
        if(entry.name && wanted==entry.name) return entry;
    throw std::runtime_error("Selected BNW plot field unavailable");
}
runtime::BnwPlotUnitReadServices accepting() {
    runtime::BnwPlotUnitReadServices services;
    services.exists=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return true;
    };
    services.delayed_death=services.cargo=services.in_combat=
        [](const runtime::LiveSessionState &,
           std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
            return false;
        };
    services.at_plot=services.exists;
    services.valid_domain=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return true;
    };
    services.can_enter_territory=services.valid_domain;
    return services;
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    auto boot=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!boot.preview.blocker && boot.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto frame=boot.preview.host->preview_input().frame_sources;
    check(frame.database,"Selected BNW rules database unavailable");
    const auto defines=frame.database->cache_global_defines(
        content::GlobalDefineProfile::bnw);
    const auto rules=runtime::resolve_bnw_map_turn_rules(defines);
    check(rules.plot_unit_limit>=1,"Selected BNW stack cap changed");
    const auto &state=boot.preview.host->session().state();
    const auto sites=runtime::preflight_bnw_map_occupied(state.world);
    check(sites.shape_valid && sites.units.size()==7,
          "Selected Gandhi occupied plot roster changed");
    for(const auto &unit:sites.units) {
        const auto plan=runtime::preflight_bnw_verify_unit_valid_plot(
            state,unit.plot,rules.plot_unit_limit,{});
        check(plan.source_valid && !plan.complete_no_effect &&
              plan.effects.empty() && plan.steps.size()==7 &&
              plan.steps.back().check==runtime::BnwPlotUnitCheck::valid_domain &&
              !plan.steps.back().resolved,
              "Selected BNW unit validity stopped outside SDK domain predicate");
        auto draft=state;
        check(runtime::run_bnw_verified_no_effect_plot_units(draft,unit.plot,
                  rules.plot_unit_limit,{}).outcome==
                  runtime::LiveActionOutcome::rejected,
              "Unresolved selected unit validity was executed");
        std::cout << "blocker plot=" << unit.plot << " owner="
                  << unit.handle.owner << " id=" << unit.handle.id
                  << " predicate=valid_domain; territory follows if true\n";
    }
    // Synthetic predicate responses exercise the exact SDK branch order.
    auto queries=accepting();
    for(const auto &unit:sites.units) {
        const auto no_effect=runtime::preflight_bnw_verify_unit_valid_plot(
            state,unit.plot,rules.plot_unit_limit,queries);
        check(no_effect.source_valid && no_effect.complete_no_effect &&
              no_effect.effects.empty(),
              "Synthetic valid-domain/territory path required an effect");
        auto draft=state;
        check(runtime::run_bnw_verified_no_effect_plot_units(draft,unit.plot,
                  rules.plot_unit_limit,queries).outcome==
                  runtime::LiveActionOutcome::committed,
              "Complete no-effect proof did not execute");
    }
    const auto chosen=sites.units.front();
    {
        auto saved_only=queries;
        saved_only.exists={};
        auto draft=state;
        check(runtime::run_bnw_verified_no_effect_plot_units(draft,chosen.plot,
                  rules.plot_unit_limit,saved_only).outcome==
                  runtime::LiveActionOutcome::rejected,
              "Saved unit identity alone authorized live execution");
        saved_only=queries;
        saved_only.exists=[](const runtime::LiveSessionState &,
            std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
            return false;
        };
        const auto conflict=runtime::preflight_bnw_verify_unit_valid_plot(
            state,chosen.plot,rules.plot_unit_limit,saved_only);
        check(!conflict.source_valid && !conflict.complete_no_effect,
              "Divergent live unit registry authorized execution");
    }
    queries.valid_domain=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return false;
    };
    auto plan=runtime::preflight_bnw_verify_unit_valid_plot(
        state,chosen.plot,rules.plot_unit_limit,queries);
    check(!plan.complete_no_effect && plan.effects.size()==1 &&
          plan.effects[0].cause==
              runtime::BnwPlotUnitEffectCause::invalid_domain_or_territory &&
          plan.effects[0].jump_required && plan.effects[0].kill_if_jump_fails,
          "Invalid domain did not stage SDK jump/kill obligation");
    queries=accepting();
    queries.can_enter_territory=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return false;
    };
    plan=runtime::preflight_bnw_verify_unit_valid_plot(
        state,chosen.plot,rules.plot_unit_limit,queries);
    check(plan.effects.size()==1 && plan.effects[0].cause==
          runtime::BnwPlotUnitEffectCause::invalid_domain_or_territory &&
          plan.steps.back().check==runtime::BnwPlotUnitCheck::can_enter_territory,
          "Territory branch did not follow domain branch");
    queries=accepting();
    queries.friendly_units_of_type=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) ->
        std::optional<std::int64_t> {return 1;};
    plan=runtime::preflight_bnw_verify_unit_valid_plot(state,chosen.plot,0,queries);
    check(plan.effects.size()==1 && plan.effects[0].cause==
          runtime::BnwPlotUnitEffectCause::stack_limit &&
          plan.steps.back().check==runtime::BnwPlotUnitCheck::stack_limit,
          "Stack jump did not precede domain/territory checks");
    // Make one saved unowned plot foreign-owned to reach the SDK second pass.
    auto foreign=state;
    field(foreign.world.map_plots->plots[chosen.plot],"owner").value=0;
    foreign.world.map_projection->cells[chosen.plot].owner=0;
    queries=accepting();
    queries.unit_team=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) ->
        std::optional<std::int64_t> {return 24;};
    queries.plot_team=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) ->
        std::optional<std::int64_t> {return 0;};
    queries.visible_enemy=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return true;
    };
    queries.invisible=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return false;
    };
    plan=runtime::preflight_bnw_verify_unit_valid_plot(
        foreign,chosen.plot,rules.plot_unit_limit,queries);
    check(plan.effects.size()==1 && plan.effects[0].pass==2 &&
          plan.effects[0].cause==runtime::BnwPlotUnitEffectCause::visible_enemy &&
          plan.steps.back().check==runtime::BnwPlotUnitCheck::invisible,
          "Visible-enemy second pass did not plan jump/kill");
    queries.invisible=[](const runtime::LiveSessionState &,
        std::uint64_t,saves::SavedPlotUnitReference) -> std::optional<bool> {
        return true;
    };
    plan=runtime::preflight_bnw_verify_unit_valid_plot(
        foreign,chosen.plot,rules.plot_unit_limit,queries);
    check(plan.complete_no_effect && plan.effects.empty(),
          "Invisible foreign unit incorrectly required relocation");
    std::cout << "PASS selected Gandhi SDK-order unit validity preflight\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW unit validity preflight: " << error.what()
                  << '\n';return 1;
    }
}
