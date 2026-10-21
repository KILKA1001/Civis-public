// SOURCE DRAFT ONLY: compile/link requires a coherent NEW context ABI window.
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_nature_yield.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

using namespace civ5;
namespace {
void need(bool condition, const char* why) {
    if (!condition) throw std::runtime_error(why);
}
auto& named(auto& rows, std::string_view name) {
    for (auto& row : rows) if (row.name && name == row.name) return row;
    throw std::runtime_error("Actual context fixture raw field absent");
}
template<class F> void refuses_width(F&& operation) {
    bool caught = false;
    try { operation(); }
    catch (const std::exception& error) {
        caught = std::string_view(error.what()).find("city handle exceeds native width") != std::string_view::npos;
    }
    need(caught, "Current-registry branch did not explicitly refuse native-wide handle");
}
struct Answer {
    std::array<std::int64_t,6> nature{}, improvement{}, religion{};
    bool operator==(const Answer&) const = default;
};
void exercise(const ui::SelectedBnwPreviewHost& host) {
    auto current = host.session().state();
    need(current.world.map_projection && current.world.map_plots,
         "Actual selected map unavailable");
    const auto& database = host.database();
    const auto types = database.cache_info_types();
    need(types.report().all_succeeded(), "Actual selected Info catalog unavailable");
    const auto nature_rules = runtime::load_bnw_nature_yield_rules(database);
    const auto improvement_rules = runtime::load_improvement_yield_rules(database, content::TechnologyProfile::bnw);
    const auto religion_rules = runtime::load_improvement_religion_rules(database);
    const auto water_rules = runtime::load_plot_water_rules(database, content::TechnologyProfile::bnw);
    const auto resource_rules = runtime::load_resource_visibility_rules(database, types, content::TechnologyProfile::bnw);
    std::optional<std::size_t> selected;
    const auto* farm_type = types.find("IMPROVEMENT_FARM");
    need(farm_type && farm_type->table == "Improvements" && improvement_rules.improvements.contains(farm_type->id),
         "Actual activated farm rule unavailable");
    // This getter's improvement parameter is a proposed improvement, as in
    // CvPlot::calculateImprovementYieldChange. It does not construct it.
    const auto improvement = farm_type->id;
    for (std::size_t index = 0; index < current.world.map_projection->cells.size(); ++index) {
        const auto& cell = current.world.map_projection->cells[index];
        if (!cell.owner || *cell.owner < 0 || !cell.yield_city || cell.yield_city->owner < 0 ||
            !cell.plot_type || *cell.plot_type == 0 ||
            runtime::bnw_live_plot_impassability(current).plots.at(index).impassable) continue;
        need(cell.yield_city->owner <= INT32_MAX && cell.yield_city->id >= 0 && cell.yield_city->id <= INT32_MAX,
             "Actual city handle outside source width");
        const auto* raw = runtime::resolve_bnw_city_registry(current.world, current.cities,
            static_cast<std::int32_t>(cell.yield_city->owner), static_cast<std::int32_t>(cell.yield_city->id));
        if (!raw) continue;
        selected = index;
        break;
    }
    need(selected.has_value(), "Actual selected fixture lacks a passable working-city plot");
    need(*selected <= static_cast<std::size_t>(INT64_MAX), "Actual plot index outside source query width");
    const auto plot = static_cast<std::int64_t>(*selected);
    auto query = [&](runtime::LiveSessionState state, bool native_registry, bool query_nature = true) {
        // The copied current world retains genuine decoded raw roster and owned
        // source_core identities. Only local derived read models are restored.
        const auto nature_state = runtime::restore_bnw_nature_yields(state.world, nature_rules, state.cities);
        const auto improvement_state = runtime::restore_improvement_yield_state(state.world, improvement_rules, types, state.cities);
        auto resource_state = runtime::restore_resource_visibility(state.world, state.setup, resource_rules, types);
        const auto water_state = runtime::restore_plot_water(state.world);
        const auto passability = runtime::bnw_live_plot_impassability(state);
        runtime::PlotWaterContext water{*state.world.map_projection, water_state, water_rules,
            types, passability, state.setup, state.cities, host.defines()};
        runtime::ResourceVisibilityContext resources{*state.world.map_projection, resource_state,
            resource_rules, state.setup, state.player_research, state.technologies, types, host.defines(), false};
        runtime::BnwNatureYieldContext nature{nature_rules, nature_state, water, resources};
        runtime::ImprovementYieldContext improvements{improvement_rules, improvement_state, water,
            resources, state.maintenance.team_eras, &religion_rules};
        nature.current_city_registry = native_registry ? &state.world : nullptr;
        improvements.current_city_registry = native_registry ? &state.world : nullptr;
        Answer result;
        for (std::int64_t yield = 0; yield < 6; ++yield) {
            const auto at = static_cast<std::size_t>(yield);
            if (query_nature)
                result.nature[at] = runtime::bnw_nature_yield(nature, plot, yield, -1, false).total;
            const auto value = runtime::bnw_improvement_yield(improvements, plot, improvement, yield,
                *state.world.map_projection->cells.at(*selected).owner, false);
            result.improvement[at] = value.total();
            result.religion[at] = value.religion;
        }
        return result;
    };
    auto set_working = [&](auto& state, std::int64_t owner, std::int64_t id) {
        state.world.map_projection->cells.at(*selected).yield_city = saves::SavedPlotCityReference{owner,id};
        auto& fields = state.world.map_plots->plots.at(*selected).fields;
        named(fields,"working_city_owner").value = owner;
        named(fields,"working_city_id").value = id;
    };
    const auto reference = *current.world.map_projection->cells.at(*selected).yield_city;
    const auto* resolved = runtime::resolve_bnw_city_registry(current.world, current.cities,
        static_cast<std::int32_t>(reference.owner), static_cast<std::int32_t>(reference.id));
    need(resolved, "Actual selected current city disappeared");
    const auto full_id = named(resolved->core.fields,"m_iID").values.at(0);
    std::cout << "actual_plot=" << *selected << " full_city_id=" << full_id << " index_only=" << (full_id & 8191) << " proposed_farm=" << improvement << " (getter parameter; no construction)\n";
    need(full_id >= 0 && full_id <= INT32_MAX, "Actual canonical city ID outside source width");
    set_working(current,reference.owner,full_id);
    const auto canonical = query(current,true);
    need(canonical == query(current,false), "Canonical registry changed existing current formula");
    auto index_only = current; set_working(index_only,reference.owner,full_id & 8191);
    need(runtime::resolve_bnw_city_registry(index_only.world,index_only.cities,
        static_cast<std::int32_t>(reference.owner),static_cast<std::int32_t>(full_id & 8191)) ==
        runtime::resolve_bnw_city_registry(index_only.world,index_only.cities,
        static_cast<std::int32_t>(reference.owner),static_cast<std::int32_t>(full_id)),
        "Actual source index-only handle did not resolve identical raw city");
    need(query(index_only,true) == canonical, "Native index-only lookup lost actual city contributions");
    auto absent = current; set_working(absent,-1,-1);
    auto stale = current;
    const auto other_generation = (full_id & ~std::int64_t{8191}) == 8192 ? 16384 : 8192;
    set_working(stale,reference.owner,(full_id & 8191) | other_generation);
    need(query(stale,true) == query(absent,true), "Stale full generation resolved a live city");
    for (const auto wide : {std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::max()}) {
        auto malformed = current; set_working(malformed,wide,full_id);
        refuses_width([&]{(void)query(malformed,true);});
        // Exercise improvement independently: nature's refusal must not hide
        // the negative-owner shortcut regression in the other component.
        refuses_width([&]{(void)query(malformed,true,false);});
        auto wide_id = current; set_working(wide_id,reference.owner,wide);
        refuses_width([&]{(void)query(wide_id,true);});
        refuses_width([&]{(void)query(wide_id,true,false);});
    }
    need(query(current,true) == canonical, "Refused local malformed fixture poisoned actual current input");
    need(!host.session().readiness().can_continue(), "Context getter minted continuation readiness");
    std::cout << "PASS actual selected nature and proposed-farm getter registry canonical/index-only/stale/native-wide handles; pure borrowed getters only\n";
}
}
int main(int argc, char** argv) {
    try {
        need(argc == 3, "Requires actual selected BNW save and shared installation");
        const auto save_before = runtime::identify_file(argv[1]).sha256;
        auto bootstrap = ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
        need(static_cast<bool>(bootstrap.preview), "Actual immutable BNW fixture unavailable");
        exercise(*bootstrap.preview.host);
        need(runtime::identify_file(argv[1]).sha256 == save_before, "Original save changed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
