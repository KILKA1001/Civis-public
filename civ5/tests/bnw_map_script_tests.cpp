#include "civ5/runtime/bnw_map_script.hpp"
#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include "civ5/runtime/bnw_fractal.hpp"
#include "civ5/runtime/bnw_fresh_terrain.hpp"
#include "civ5/runtime/bnw_fresh_yield.hpp"
#include "civ5/runtime/bnw_fresh_feature_setter.hpp"
#include "civ5/runtime/bnw_plot_type_transition.hpp"
#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::runtime;

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_self_contained_script() {
    constexpr auto source = R"lua(
        function GetMapInitData() return { Width = 3, Height = 2 } end
        function GeneratePlotTypes()
            assert(Map.GetNumPlots() == 6 and Map.GetNumPlots('ignored') == 6)
            assert(select('#', Map.GetNumPlots()) == 1)
            return { 2, 2, 3, 0, 1, 2 }
        end
    )lua";
    const auto first = generate_bnw_plot_map(source, "=small-bnw-map");
    const auto second = generate_bnw_plot_map(source, "=small-bnw-map");
    check(first.map && second.map, "Self-contained BNW map script must execute");
    check(first.map->width == 3 && first.map->height == 2 &&
              first.map->wrap_x && !first.map->wrap_y,
          "Map script dimensions changed");
    check(first.map->plot_types == std::vector<std::uint8_t>({2, 2, 3, 0, 1, 2}) &&
              first.map->plot_types == second.map->plot_types,
          "Plot array must preserve source order and reproduce exactly");
    check(first.has_blocker(BnwMapScriptBlockerCode::terrain_pending) &&
              first.has_blocker(BnwMapScriptBlockerCode::features_rivers_pending) &&
              first.has_blocker(BnwMapScriptBlockerCode::starting_positions_pending) &&
              first.has_blocker(BnwMapScriptBlockerCode::world_setup_pending),
          "A plot-category grid must not claim a playable world");
}

void test_invalid_outputs() {
    const auto oversized = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 4, Height = 4 } end
        function GeneratePlotTypes() error('must not run before dimension validation') end
    )lua", "=oversized", {.maximum_plots = 15});
    check(!oversized.map && oversized.has_blocker(BnwMapScriptBlockerCode::invalid_dimensions) &&
              !oversized.has_blocker(BnwMapScriptBlockerCode::script_failed),
          "Dimensions must be bounded before generating plots");

    const auto source_int_overflow = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 50000, Height = 50000 } end
        function GeneratePlotTypes() error('must not run with CvMap grid overflow') end
    )lua", "=source-grid-overflow", {.maximum_plots = 3'000'000'000ULL});
    check(!source_int_overflow.map &&
              source_int_overflow.has_blocker(BnwMapScriptBlockerCode::invalid_dimensions),
          "CvMap m_iGridSize must stay within signed 32-bit range");

    const auto early_count = generate_bnw_plot_map(R"lua(
        local n = Map.GetNumPlots()
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { PlotTypes.PLOT_LAND } end
    )lua", "=early-plot-count");
    check(!early_count.map &&
              early_count.has_blocker(BnwMapScriptBlockerCode::unresolved_map_dimensions),
          "GetNumPlots before GetMapInitData must report unresolved grid state");

    const auto bad_type = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes() return { 2, 4 } end
    )lua", "=bad-type");
    check(!bad_type.map && bad_type.has_blocker(BnwMapScriptBlockerCode::invalid_plot_types),
          "Unknown plot categories must be rejected");

    const auto sparse = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes() return { [1] = 2, [3] = 3 } end
    )lua", "=sparse");
    check(!sparse.map && sparse.has_blocker(BnwMapScriptBlockerCode::invalid_plot_types),
          "Sparse or extra plot keys must be rejected");
}

void test_unsupported_stock_api() {
    const auto result = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes() return Fractal.Create(2, 1) end
    )lua", "=needs-fractal");
    check(!result.map && result.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Unavailable stock map-generation APIs must stop execution explicitly");
    const auto gameinfo = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = GameInfo.Worlds.WORLDSIZE_STANDARD.ID, Height = 1 } end
        function GeneratePlotTypes() return { 2 } end
    )lua", "=needs-gameinfo");
    check(!gameinfo.map && gameinfo.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Missing selected GameInfo.Worlds must be a typed stock-helper blocker");
    const auto defines = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = GameDefines.LAKE_PLOT_RAND, Height = 1 } end
        function GeneratePlotTypes() return { 2 } end
    )lua", "=needs-game-defines");
    check(!defines.map && defines.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "GameDefines without selected active BNW rules must fail explicitly");
}

void test_bnw_table_fill() {
    const auto map = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 3, Height = 2 } end
        function GeneratePlotTypes()
            local plots = table.fill(PlotTypes.PLOT_OCEAN, 6)
            assert(#plots == 6 and plots[1] == PlotTypes.PLOT_OCEAN and
                   plots[6] == PlotTypes.PLOT_OCEAN)
            local same = table.fill(plots, PlotTypes.PLOT_LAND, 2)
            assert(same == plots and plots[1] == PlotTypes.PLOT_LAND and
                   plots[2] == PlotTypes.PLOT_LAND and
                   plots[3] == PlotTypes.PLOT_OCEAN)
            local false_values = table.fill(false, 2)
            assert(#false_values == 2 and false_values[1] == false and
                   false_values[2] == false)
            assert(#table.fill(0, 0) == 0 and table.concat({ 'a', 'b' }) == 'ab')
            return plots
        end
    )lua", "=stock-table-fill");
    check(map.map && map.map->plot_types == std::vector<std::uint8_t>({2, 2, 3, 3, 3, 3}),
          "Stock table.fill must create dense arrays and mutate an existing destination");

    const auto over_budget = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return table.fill(PlotTypes.PLOT_LAND, 3) end
    )lua", "=table-fill-budget", {.maximum_plots = 2});
    check(!over_budget.map && over_budget.has_blocker(BnwMapScriptBlockerCode::fill_budget_exceeded),
          "table.fill must reject allocations beyond the selected plot budget");

    const auto bad_count = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return table.fill(PlotTypes.PLOT_LAND, 1.5) end
    )lua", "=table-fill-count");
    check(!bad_count.map && bad_count.has_blocker(BnwMapScriptBlockerCode::script_failed),
          "table.fill must reject nonintegral lengths before creating a table");
}

void test_bnw_fractal_source_path() {
    SimulationRandom seed(RandomAlgorithm::bnw, {123, 0, 0});
    auto oracle = seed;
    const int top_left = static_cast<int>(oracle.draw(256));
    const int bottom_left = static_cast<int>(oracle.draw(256));
    const int top_right = static_cast<int>(oracle.draw(256));
    const int bottom_right = static_cast<int>(oracle.draw(256));
    (void)bottom_left;
    (void)bottom_right;
    BnwFractal fractal(2, 2, 0, 0, 0, 0, seed);
    check(fractal.height_at(0, 0) == top_left &&
              fractal.height_at(1, 0) == (top_left + top_right) / 2 &&
              seed.state() == oracle.state(),
          "SDK grain-zero fractal must consume four x-major initial random draws");
    check(fractal.height_at(-1, 0) == 0 && fractal.height_at(2, 0) == 0 &&
              fractal.height_from_percent(0) == 0 &&
              fractal.height_from_percent(100) == 255,
          "Fractal edge and percentile endpoints changed");

    SimulationRandom refined_seed(RandomAlgorithm::bnw, {777, 0, 0});
    auto refined_oracle = refined_seed;
    const int corner00 = static_cast<int>(refined_oracle.draw(256));
    const int corner02 = static_cast<int>(refined_oracle.draw(256));
    const int corner20 = static_cast<int>(refined_oracle.draw(256));
    const int corner22 = static_cast<int>(refined_oracle.draw(256));
    refined_oracle.draw(128); // x=0, y=1: vertical
    refined_oracle.draw(128); // x=1, y=0: horizontal
    const int center_noise = static_cast<int>(refined_oracle.draw(128));
    refined_oracle.draw(128); // x=1, y=2: horizontal
    refined_oracle.draw(128); // x=2, y=1: vertical
    const int center = std::clamp(((corner00 + corner02 + corner20 + corner22) >> 2)
                                  + center_noise - 64, 0, 255);
    BnwFractal refined(2, 2, 0, 0, 1, 1, refined_seed);
    check(refined.height_at(1, 1) == center &&
              refined_seed.state() == refined_oracle.state(),
          "SDK first interpolation pass must consume five x-major map draws");

    const auto generated = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 2 } end
        function GeneratePlotTypes()
            local f = Fractal.Create(2, 2, 0, {}, 0, 0)
            assert(f:GetHeight(0, 0) >= 0 and f:GetHeight(0, 0) <= 255)
            assert(f:GetHeight(100) == 255 and f:GetHeight(-1, 0) == 0)
            return { PlotTypes.PLOT_LAND, PlotTypes.PLOT_OCEAN,
                     PlotTypes.PLOT_LAND, PlotTypes.PLOT_OCEAN }
        end
    )lua", "=fractal-source-path", oracle);
    check(generated.map && generated.map_random_after &&
              generated.map_random_after->calls == oracle.state().calls + 4,
          "Fractal.Create must use only the imported map stream and expose its consumed state");

    const auto false_flag = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            local flags = setmetatable({}, {__index = function(_, key)
                if key == 'FRAC_PERCENT' then return false end
            end})
            local f = Fractal.Create(1, 1, 0, flags, 0, 0)
            assert(f:GetHeight(0, 0) == 60)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=fractal-false-flag", SimulationRandom(RandomAlgorithm::bnw, {123, 0, 0}));
    check(static_cast<bool>(false_flag.map),
          "SDK lua_getfield must treat a present false fractal flag as enabled");

    const auto rifts = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return Fractal.CreateRifts(1, 1, 1, {}) end
    )lua", "=fractal-rifts", oracle);
    check(!rifts.map && rifts.has_blocker(BnwMapScriptBlockerCode::script_failed),
          "Fractal.CreateRifts must refuse an absent genuine rift handle");

    const auto budget = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return Fractal.Create(1, 1, 0, {}, 0, 0) end
    )lua", "=fractal-budget", oracle,
        {.maximum_plots = 1, .maximum_fractals = 0});
    check(!budget.map && budget.has_blocker(BnwMapScriptBlockerCode::fractal_budget_exceeded),
          "Native fractal allocation must obey its separate map setup budget");
}

void test_bnw_fractal_ridges() {
    SimulationRandom input(RandomAlgorithm::bnw, {123, 0, 0});
    BnwFractal fractal(40, 20, 3, 0, -1, -1, input);
    auto oracle = input;
    struct Point { int x, y; };
    std::array<Point, 3> points{};
    const auto distance = [](Point a, Point b) {
        const int dx = a.x - b.x, dy = a.y - b.y;
        const int ax = std::abs(dx), ay = std::abs(dy);
        return (dx >= 0) == (dy >= 0) ? ax + ay : std::max(ax, ay);
    };
    for (int i = 0; i < 3; ++i) {
        const int y = static_cast<int>(oracle.draw(64));
        const int x = static_cast<int>(oracle.draw(128)) - y / 2;
        points[i] = {x,y};
        oracle.draw(7); oracle.draw(6); oracle.draw(8);
        for (int j = 0; j < i; ++j)
            check(distance(points[i], points[j]) >= 7,
                  "Ridge oracle fixture unexpectedly needs a seed reroll");
    }
    std::array<int, 3> distances{};
    for (int i = 0; i < 3; ++i)
        distances[i] = distance({0,0}, points[i]);
    std::sort(distances.begin(), distances.end());
    fractal.build_ridges(3, 0, 1, 0, input, 64, 4096);
    check(fractal.height_at(0,0) == 255 * distances[0] / distances[1] &&
              input.state() == oracle.state(),
          "SDK ridge geometry must use three x-major seed records and no cell draws when flags are empty");

    SimulationRandom noisy_input(RandomAlgorithm::bnw, {123, 0, 0});
    BnwFractal noisy(40, 20, 3, 0, -1, -1, noisy_input);
    noisy.build_ridges(3, BnwFractal::polar, 1, 2, noisy_input, 64, 4096);
    check(noisy_input.state().calls == oracle.state().calls + 3ULL * 128 * 64,
          "Nonempty ridge flags must consume one draw per seed for every fractal cell");

    const auto lua = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            local f = Fractal.Create(40, 20, 3, {}, -1, -1)
            assert(f:BuildRidges(3, {}, 1, 0) == nil)
            assert(f:GetHeight(0, 0) >= 0)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=ridge-lua", SimulationRandom(RandomAlgorithm::bnw, {123, 0, 0}));
    check(lua.map && lua.map_random_after && lua.map_random_after->calls == oracle.state().calls,
          "Lua BuildRidges must mutate the fractal with the same map RNG draw sequence");

    const auto impossible = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            local f = Fractal.Create(2, 2, 0, {}, 1, 1)
            f:BuildRidges(3, {}, 1, 0)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=ridge-retry-budget", SimulationRandom(RandomAlgorithm::bnw, {123, 0, 0}),
        {.maximum_ridge_retries = 0});
    check(!impossible.map && impossible.has_blocker(BnwMapScriptBlockerCode::fractal_budget_exceeded),
          "Impossible SDK ridge placement must stop at the selected retry budget");
}

void test_bnw_setup_plot_handles() {
    const auto plots = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 3, Height = 2 } end
        function GeneratePlotTypes()
            local first = Map.GetPlotByIndex(0)
            local fourth = Map.GetPlotByIndex(3)
            assert(first == Map.GetPlotByIndex(0))
            assert(first == Map.GetPlotByIndex(4294967296))
            assert(first:GetX() == 0 and first:GetY() == 0 and
                   first:GetPlotIndex() == 0)
            assert(fourth:GetX() == 0 and fourth:GetY() == 1 and
                   fourth:GetPlotIndex() == 3)
            assert(first:GetPlotType() == PlotTypes.PLOT_OCEAN and
                   first:GetTerrainType() == -1 and first:IsWater() and
                   not first:IsMountain())
            assert(select('#', Map.GetPlotByIndex(-1)) == 0)
            assert(select('#', Map.GetPlotByIndex(6)) == 0)
            return { 2, 2, 3, 0, 1, 2 }
        end
    )lua", "=setup-plot-handles");
    check(plots.map && plots.map->plot_types == std::vector<std::uint8_t>({2,2,3,0,1,2}),
          "BNW setup plots must be backed by stable indexed CvPlot reset state");

    const auto early = generate_bnw_plot_map(R"lua(
        local plot = Map.GetPlotByIndex(0)
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { PlotTypes.PLOT_LAND } end
    )lua", "=plot-before-init");
    check(!early.map && early.has_blocker(BnwMapScriptBlockerCode::unresolved_map_plots),
          "Plot lookup before CvMap::InitPlots must report unresolved plot state");

    const auto setter = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetPlotType(PlotTypes.PLOT_LAND, false, false)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=plot-setter-boundary");
    check(!setter.map && setter.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Unported CvPlot::setPlotType side effects must stop plot generation");

    const auto state_budget = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() error('must not allocate setup plot') end
    )lua", "=setup-plot-budget", {.maximum_plot_state_bytes = 15});
    check(!state_budget.map && state_budget.has_blocker(BnwMapScriptBlockerCode::invalid_dimensions),
          "Native setup plot storage must obey its explicit byte budget");
}

void test_bnw_fresh_plot_transition_preflight() {
    using Effect = BnwFreshPlotEffect;
    using Status = BnwFreshPlotTransitionStatus;
    const std::vector<Effect> expected = {
        Effect::erase, Effect::remove_sight, Effect::assign_plot_type,
        Effect::update_plot_yield, Effect::restore_sight,
        Effect::assign_land_terrain, Effect::update_terrain_yield,
        Effect::update_impassability, Effect::reset_step_finder,
        Effect::update_neighbor_yields, Effect::update_potential_city_work,
        Effect::increment_land_plot_count};
    for (int new_type = 0; new_type <= 2; ++new_type) {
        const auto plan = inspect_bnw_fresh_plot_transition(3, -1, new_type, false, false);
        check(plan.status == Status::requires_world_effects &&
                  plan.effects == expected && !plan.blocker.empty(),
              "Every fresh ocean-to-land category transition needs SDK-ordered world effects");
    }
    const auto unchanged = inspect_bnw_fresh_plot_transition(3, -1, 3, false, false);
    check(unchanged.status == Status::no_change && unchanged.effects.empty(),
          "SDK setPlotType performs no effects for an unchanged category");
    for (const auto &invalid : std::array{
             inspect_bnw_fresh_plot_transition(2, -1, 3, false, false),
             inspect_bnw_fresh_plot_transition(3, 1, 2, false, false),
             inspect_bnw_fresh_plot_transition(3, -1, 4, false, false),
             inspect_bnw_fresh_plot_transition(3, -1, 2, true, false),
             inspect_bnw_fresh_plot_transition(3, -1, 2, false, true)})
        check(invalid.status == Status::outside_fresh_setup_contract &&
                  invalid.effects.empty() && !invalid.blocker.empty(),
              "Preflight must reject mutations outside the proven fresh-cell SDK path");
}

void test_bnw_owned_setup_plot_draft() {
    BnwSetupPlotGrid original;
    original.reset(3, 3, false, false);
    check(original.size() == 9 && original.land_plot_count() == 0 &&
              original.at(4).x == 1 && original.at(4).y == 1 &&
              original.at(4).plot_type == 3 && original.at(4).terrain_type == -1 &&
              !original.at(4).ne_of_river && !original.at(4).w_of_river &&
              !original.at(4).nw_of_river,
          "CvMap::InitPlots must own ocean/NO_TERRAIN cells in storage order");
    check(inspect_bnw_fresh_river_write(original, 4, BnwFreshRiverEdge::west,
                                        true, 0).blocker ==
              BnwFreshRiverWriteBlockerCode::unassigned_area,
          "River setter preflight must reject an unassigned fresh area");
    const std::array<std::optional<std::size_t>, 6> middle{8, 5, 2, 1, 3, 7};
    check(original.adjacent(4) == middle,
          "SDK plotDirection must preserve six hex neighbors in direction order");
    const auto first = original.draft_fresh_category_change(4, 2);
    check(first.at(4).plot_type == 2 && first.at(4).terrain_type == -1 &&
              first.land_plot_count() == 1 && original.at(4).plot_type == 3 &&
              original.land_plot_count() == 0,
          "Partial category and land-count writes must remain in an isolated owned draft");
    const auto second = first.draft_fresh_category_change(0, 1);
    check(second.at(0).plot_type == 1 && second.land_plot_count() == 2 &&
              first.land_plot_count() == 1,
          "Each fresh ocean-to-land draft must add exactly one land plot");
    const auto same = first.draft_fresh_category_change(0, 3);
    check(same.land_plot_count() == 1 && same.at(0).plot_type == 3,
          "Unchanged plot category must preserve owned state");
    bool refused = false;
    try { (void)first.draft_fresh_category_change(4, 3); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused, "A draft with unfinished terrain must reject a second transition");

    BnwSetupPlotGrid wrapped;
    wrapped.reset(3, 3, true, false);
    check(wrapped.adjacent(3)[4] == std::optional<std::size_t>(5),
          "CvMap::plot must wrap the west neighbor by map width");
    BnwSetupPlotGrid tiny;
    tiny.reset(1, 1, true, true);
    for (const auto neighbor : tiny.adjacent(0))
        check(neighbor == std::optional<std::size_t>(0),
              "Six SDK direction calls may all resolve to the same wrapped plot");
}

void test_bnw_inland_corner() {
    BnwSetupPlotGrid land;
    land.reset(3, 3, false, false);
    for (std::size_t index = 0; index < land.size(); ++index)
        land.apply_stock_fresh_plot_type(index, 2, false, false, true,
                                         {0, false, false});
    const std::array<std::optional<std::size_t>, 4> expected_candidates{4, 8, 7, 3};
    check(land.inland_corner_candidates(4) == expected_candidates,
          "GetInlandCorner must retain SDK self/north/northwest/west candidate order");
    SimulationRandom actual(RandomAlgorithm::bnw, {123, 0, 0});
    SimulationRandom oracle = actual;
    std::array<int, 4> order{0, 1, 2, 3};
    for (std::size_t i = 0; i < order.size(); ++i) {
        const auto j = i + oracle.draw(order.size() - i);
        std::swap(order[i], order[static_cast<std::size_t>(j)]);
    }
    check(land.get_inland_corner(4, actual) ==
              expected_candidates[static_cast<std::size_t>(order[0])] &&
              actual.state() == oracle.state() && actual.state().calls == 4,
          "GetInlandCorner must use four source-order map RNG draws, including range one");

    BnwSetupPlotGrid ocean;
    ocean.reset(3, 3, false, false);
    SimulationRandom no_corner(RandomAlgorithm::bnw, {123, 0, 0});
    check(ocean.get_inland_corner(4, no_corner) == std::nullopt &&
              no_corner.state() == oracle.state(),
          "Coastal corners must be filtered after consuming the complete shuffle");

    BnwSetupPlotGrid wrapped;
    wrapped.reset(1, 1, true, true);
    wrapped.apply_stock_fresh_plot_type(0, 2, false, false, true,
                                        {0, false, false});
    check(wrapped.inland_corner_candidates(0) ==
              std::array<std::optional<std::size_t>, 4>{0, 0, 0, 0},
          "Wrapped duplicate corner candidates must retain native direction calls");
}

void test_bnw_potential_city_work_draft() {
    BnwSetupPlotGrid ocean;
    ocean.reset(9, 9, false, false);
    const std::size_t center = 4 * 9 + 4;
    const auto ring = ocean.city_plots(center);
    check(ring.size() == 37 && ring[0] == center &&
              ring[1] == std::optional<std::size_t>(5 * 9 + 4) &&
              ring[2] == std::optional<std::size_t>(4 * 9 + 5),
          "SDK city ring must start with center, northeast, east in hex order");
    const auto first = ocean.draft_fresh_category_change(center, 2);
    check(first.land_plot_count() == 1 && first.city_work_yield_calls().size() == 37 &&
              !ocean.at(center).potential_city_work &&
              ocean.city_work_yield_calls().empty(),
          "First land plot must invoke city-work yield updates in its SDK ring");
    std::vector<std::size_t> sdk_order;
    for (const auto member : ring) sdk_order.push_back(*member);
    check(first.city_work_yield_calls() == sdk_order,
          "City-work yield calls must follow SDK plotCity order");
    for (const auto member : ring)
        check(member && first.at(*member).potential_city_work,
              "Every plot whose city ring contains the new land must become potential city work");
    check(!first.at(0).potential_city_work,
          "Plots outside the SDK city radius must remain unchanged");
    const auto second = first.draft_fresh_category_change(center + 1, 2);
    check(second.land_plot_count() == 2 &&
              second.city_work_yield_calls().size() > first.city_work_yield_calls().size(),
          "Another land plot must update previously false city-work bits only");
    auto unique = second.city_work_yield_calls();
    std::sort(unique.begin(), unique.end());
    check(std::adjacent_find(unique.begin(), unique.end()) == unique.end(),
          "Repeated city-ring visits must call yield only when the bit changes");

    BnwSetupPlotGrid wrapped;
    wrapped.reset(1, 1, true, true);
    const auto tiny = wrapped.draft_fresh_category_change(0, 2);
    check(tiny.at(0).potential_city_work &&
              tiny.city_work_yield_calls() == std::vector<std::size_t>{0},
          "Duplicate wrapped city-ring positions must change the bit and call yield once");
}
void test_bnw_stock_fresh_plot_setter() {
    BnwSetupPlotGrid grid;
    grid.reset(9, 9, false, false);
    check(!grid.step_finder_force_reset() && grid.land_plot_count() == 0,
          "Fresh map must start without a forced stepfinder reset or land plots");
    const std::size_t center = 4 * 9 + 4;
    grid.apply_stock_fresh_plot_type(center, 2, false, false, true, {0, false, false});
    check(grid.at(center).plot_type == 2 && grid.at(center).terrain_type == 0 &&
              !grid.at(center).impassable && grid.at(center).area_id == -1 &&
              grid.land_plot_count() == 1 && grid.step_finder_force_reset(),
          "Stock ocean-to-land setter must assign terrain, reset stepfinder and count land");
    check(grid.at(center).potential_city_work &&
              grid.city_work_yield_calls().size() == 37 &&
              grid.at(0).plot_type == 3 && grid.at(0).terrain_type == -1,
          "Setter must update SDK city radius without inventing adjacent terrain");
    const auto before = grid.land_plot_count();
    grid.apply_stock_fresh_plot_type(0, 3, false, false, true, {0, false, false});
    check(grid.land_plot_count() == before && grid.at(0).terrain_type == -1,
          "CvPlot::setPlotType does nothing when requested category is unchanged");
    bool refused = false;
    try { grid.apply_stock_fresh_plot_type(1, 2, true, false, true, {0, false, false}); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused && grid.at(1).plot_type == 3 && grid.land_plot_count() == before,
          "Unported area recalculation must fail before mutating setup state");
    refused = false;
    try { grid.apply_stock_fresh_plot_type(center, 1, false, false, true, {0, false, false}); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused && grid.at(center).plot_type == 2,
          "Setter must reject subsequent changes outside the fresh plot contract");
}

void test_bnw_fresh_adjacent_land() {
    BnwSetupPlotGrid plain;
    plain.reset(3, 3, false, false);
    check(!plain.is_adjacent_to_land(0) && !plain.is_adjacent_to_land(4),
          "Fresh ocean grid must contain no adjacent land");
    plain.apply_stock_fresh_plot_type(4, 2, false, false, true, {0, false, false});
    check(plain.is_adjacent_to_land(1) && plain.is_adjacent_to_land(2) &&
              !plain.is_adjacent_to_land(4),
          "Adjacency must observe current neighboring category and not count self");
    BnwSetupPlotGrid wrapped;
    wrapped.reset(4, 3, true, false);
    wrapped.apply_stock_fresh_plot_type(4, 1, false, false, true, {0, false, false});
    check(wrapped.is_adjacent_to_land(7),
          "CvMap horizontal wrap must expose land across the opposite edge");
    BnwSetupPlotGrid tiny;
    tiny.reset(1, 1, true, true);
    check(!tiny.is_adjacent_to_land(0), "Wrapped self starts as ocean");
    tiny.apply_stock_fresh_plot_type(0, 0, false, false, true, {0, false, false});
    check(tiny.is_adjacent_to_land(0),
          "SDK plotDirection may return self on a tiny wrapped map");
    bool invalid = false;
    try { (void)plain.is_adjacent_to_land(plain.size()); }
    catch (const std::out_of_range &) { invalid = true; }
    check(invalid, "Invalid setup plot identity must fail before reading neighbor state");
}

void test_bnw_fresh_shallow_water_adjacency() {
    BnwSetupPlotGrid plain;
    plain.reset(3, 3, false, false);
    check(!plain.is_adjacent_to_shallow_water(1, 1),
          "Fresh ocean without terrain must not count as shallow water");
    plain.apply_stock_fresh_terrain_type(4, {1, true, false}, false, false);
    check(plain.is_adjacent_to_shallow_water(1, 1) &&
              !plain.is_adjacent_to_shallow_water(4, 1) &&
              !plain.is_adjacent_to_shallow_water(0, 1),
          "SDK shallow-water adjacency must inspect neighbor terrain, not self");
    plain.apply_stock_fresh_terrain_type(4, {2, true, false}, false, false);
    check(!plain.is_adjacent_to_shallow_water(1, 1),
          "Adjacency must read current mutable terrain after a coast change");
    BnwSetupPlotGrid wrapped;
    wrapped.reset(4, 3, true, false);
    wrapped.apply_stock_fresh_terrain_type(4, {1, true, false}, false, false);
    check(wrapped.is_adjacent_to_shallow_water(7, 1),
          "Shallow-water query must use SDK horizontal wrap");
    BnwSetupPlotGrid tiny;
    tiny.reset(1, 1, true, true);
    tiny.apply_stock_fresh_terrain_type(0, {1, true, false}, false, false);
    check(tiny.is_adjacent_to_shallow_water(0, 1),
          "Tiny wrapped maps may expose the current plot through plotDirection");
    bool refused = false;
    try { (void)tiny.is_adjacent_to_shallow_water(0, -1); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused, "Unresolved selected shallow terrain must be rejected");
}

void test_bnw_fresh_area_structure() {
    const auto make_grid = [](bool wrap_x) {
        BnwSetupPlotGrid grid;
        grid.reset(3, 1, wrap_x, false);
        for (std::size_t i = 0; i < 3; ++i) {
            grid.apply_stock_fresh_plot_type(i, i == 1 ? 2 : 3,
                                             false, false, true, {0, false, false});
            grid.apply_stock_fresh_terrain_type(i,
                i == 1 ? BnwSelectedTerrain{0, false, false}
                       : BnwSelectedTerrain{1, true, false}, false, false);
        }
        return grid;
    };
    const auto separate = make_grid(false).draft_fresh_area_structure(1);
    BnwFreshYieldRules yield_rules;
    yield_rules.yields[0].lake = 1;
    yield_rules.terrains[0].base[0] = 2;
    yield_rules.terrains[1].base[0] = 1;
    const auto separate_yields = calculate_bnw_fresh_final_yields(
        make_grid(false), separate, yield_rules);
    check(separate_yields[0][0] == 2 && separate_yields[1][0] == 2 &&
              separate_yields[2][0] == 2,
          "Fresh final yield pass must apply selected lake bonus to separate waters");
    const auto separate_replay = replay_bnw_fresh_yield_passes(
        make_grid(false), separate, yield_rules);
    check(separate_replay.final_cache == separate_yields &&
              separate_replay.progress.set_area_changed_plots == 3 &&
              separate_replay.progress.lake_threshold_passes.empty() &&
              separate_replay.progress.final_pass_changed_plots == 2 &&
              separate_replay.progress.plot_visits == 6,
          "setArea must write base caches before the final lake-yield pass");
    auto applied = make_grid(false);
    applied.apply_fresh_area_handles(separate, separate_replay.final_cache,
                                     separate_replay.symbol_events);
    BnwSelectedFeatureEligibility feature_rule;
    feature_rule.id = 7;
    feature_rule.terrain_ids = {0};
    std::vector<std::int32_t> feature_types(applied.size(), -1);
    feature_rule.requires_river = true;
    check(!can_have_bnw_fresh_feature(applied, 1, 7, feature_rule,
                                      separate, 1, feature_types),
          "Fresh feature eligibility must require a stored river crossing");
    feature_rule.requires_river = false;
    check(can_have_bnw_fresh_feature(applied, 1, 7, feature_rule,
                                     separate, 1, feature_types),
          "Fresh eligible land terrain must accept the selected feature");
    feature_rule.no_coast = true;
    check(!can_have_bnw_fresh_feature(applied, 1, 7, feature_rule,
                                      separate, 1, feature_types),
          "NoCoast must use assigned landmass lake state");
    feature_rule.no_coast = false;
    feature_rule.requires_flatlands = true;
    check(can_have_bnw_fresh_feature(applied, 1, 7, feature_rule,
                                     separate, 1, feature_types),
          "RequiresFlatlands rejects hills but accepts ordinary land");
    feature_rule.no_adjacent = true;
    feature_types[0] = 7;
    check(!can_have_bnw_fresh_feature(applied, 1, 7, feature_rule,
                                      separate, 1, feature_types),
          "NoAdjacent must inspect the six current hex neighbors");
    feature_types[0] = -1;
    feature_types[1] = 7;
    check(!can_have_bnw_fresh_feature(applied, 1, 7, feature_rule,
                                      separate, 1, feature_types),
          "An occupied fresh plot cannot receive a second feature");
    feature_types[1] = -1;
    civ5::content::RulesDatabase selected_features;
    selected_features.apply_sql(R"sql(
        CREATE TABLE Features (ID INTEGER PRIMARY KEY, Impassable INTEGER,
                               Rough INTEGER, NaturalWonder INTEGER);
        INSERT INTO Features VALUES (7, 1, 1, 0);
    )sql");
    const auto selected_feature = resolve_bnw_fresh_feature_set_info(
        selected_features, 7);
    feature_rule.source_rules = &selected_features;
    const auto feature_plan = inspect_bnw_fresh_feature_set(
        applied, separate, feature_types, 1, 7, -1, feature_rule,
        selected_feature, 1);
    check(feature_plan.blocker ==
              BnwFreshFeatureSetBlocker::required_services_pending &&
              feature_plan.effective_variety == 0 &&
              feature_plan.projected_impassable && feature_plan.projected_rough &&
              feature_plan.source_order.front() ==
                  BnwFreshFeatureSetStep::remove_empty_sight &&
              feature_plan.source_order[1] ==
                  BnwFreshFeatureSetStep::gameplay_feature_changed_pending &&
              feature_plan.source_order[2] ==
                  BnwFreshFeatureSetStep::feature_write_pending &&
              feature_plan.source_order[3] ==
                  BnwFreshFeatureSetStep::yield_update_pending &&
              feature_plan.missing_services.size() == 3 &&
              feature_types[1] == -1 && applied.at(1).impassable == false,
          "Selected first feature write must stage SDK effects without mutating owned state");
    const auto wrong_feature = inspect_bnw_fresh_feature_set(
        applied, separate, feature_types, 1, 8, -1, feature_rule,
        selected_feature, 1);
    check(wrong_feature.blocker ==
              BnwFreshFeatureSetBlocker::invalid_fresh_contract &&
              feature_types[1] == -1,
          "Feature Info identity mismatch must reject the draft before mutation");
    check(can_have_bnw_fresh_feature(applied, 1, -1, {}, separate, 1,
                                     feature_types) &&
              !can_have_bnw_fresh_feature(applied, 1, 99, {}, separate, 1,
                                          feature_types),
          "NO_FEATURE and missing FeatureInfo follow the SDK branch order");
    check(find_bnw_fresh_biggest_area(applied, separate, true) == 8192 &&
              find_bnw_fresh_biggest_area(applied, separate, false) == 16385,
          "Biggest-area ties must retain the first fresh free-list slot");
    std::uint64_t water_visits = 100;
    check(applied.is_coastal_land(1, 1, separate) &&
              !applied.is_coastal_land(1, 2, separate) &&
              !applied.is_coastal_land(0, 1, separate) &&
              applied.find_water_during_rivers(1, 1, false, separate, water_visits) &&
              !applied.find_water_during_rivers(1, 0, false, separate, water_visits) &&
              applied.find_water_during_rivers(1, 0, true, separate, water_visits) &&
              !applied.find_water_during_rivers(1, -1, true, separate, water_visits),
          "SDK coast and pre-river water search must use selected landmass lake state");
    check(water_visits < 100, "Map.FindWater calls must share one visit budget");
    bool water_budget_refused = false;
    std::uint64_t no_visits = 0;
    try { (void)applied.find_water_during_rivers(1, 1, true, separate, no_visits); }
    catch (const std::length_error &) { water_budget_refused = true; }
    check(water_budget_refused, "Map.FindWater fresh query must respect its work budget");
    check(applied.symbol_events().size() == 5 &&
              applied.symbol_events()[0].phase ==
                  BnwFreshYieldSymbolPhase::set_area &&
              applied.symbol_events()[0].plot_index == 0 &&
              applied.symbol_events()[3].phase ==
                  BnwFreshYieldSymbolPhase::final_map &&
              applied.symbol_events()[3].plot_index == 0 &&
              applied.symbol_events()[3].yields_after[0] == 2,
          "Owned grid must stage updateSymbols events in source phase order");
    for (std::size_t i = 0; i < applied.size(); ++i)
        check(applied.at(i).area_id == separate.area_ids[i] &&
                  applied.at(i).landmass_id == separate.landmass_ids[i] &&
                  applied.at(i).yields == separate_replay.final_cache[i],
              "Owned fresh plot must retain area, landmass and final yield cache");
    bool duplicate_refused = false;
    try { applied.apply_fresh_area_handles(separate, separate_replay.final_cache,
                                           separate_replay.symbol_events); }
    catch (const std::invalid_argument &) { duplicate_refused = true; }
    check(duplicate_refused, "Fresh area handles must not be applied twice");
    auto corrupt_order = separate;
    std::swap(corrupt_order.area_assignment_order[0],
              corrupt_order.area_assignment_order[1]);
    auto unapplied = make_grid(false);
    bool order_refused = false;
    try { unapplied.apply_fresh_area_handles(corrupt_order,
                                            separate_replay.final_cache,
                                            separate_replay.symbol_events); }
    catch (const std::invalid_argument &) { order_refused = true; }
    check(order_refused && unapplied.at(0).area_id == -1,
          "Invalid source callback order must fail before mutating owned plots");
    check(separate.area_ids == std::vector<std::int32_t>({8192, 16385, 24578}) &&
              separate.landmass_ids == separate.area_ids &&
              separate.areas.size() == 3 && separate.landmasses.size() == 3 &&
              separate.landmasses[0].lake && separate.landmasses[2].lake &&
              !separate.landmasses[1].lake &&
              separate.area_assignment_yield_calls == 3 &&
              separate.lake_threshold_full_yield_passes == 0 &&
              separate.final_landmass_full_yield_pass_pending,
          "SDK area and landmass IDs must follow fresh free-list row-major seed order");
    const auto wrapped = make_grid(true).draft_fresh_area_structure(1);
    const auto wrapped_yields = calculate_bnw_fresh_final_yields(
        make_grid(true), wrapped, yield_rules);
    check(wrapped_yields[0][0] == 1 && wrapped_yields[2][0] == 1,
          "Fresh final yield pass must remove lake bonus after wrapped merger");
    const auto wrapped_replay = replay_bnw_fresh_yield_passes(
        make_grid(true), wrapped, yield_rules);
    check(wrapped_replay.final_cache == wrapped_yields &&
              wrapped_replay.progress.lake_threshold_passes.size() == 1 &&
              wrapped_replay.progress.lake_threshold_passes[0].trigger_landmass_id == 8192 &&
              wrapped_replay.progress.lake_threshold_passes[0].changed_plots == 0 &&
              wrapped_replay.progress.final_pass_changed_plots == 0 &&
              wrapped_replay.progress.plot_visits == 9,
          "Wrapped large lake must schedule its full-map pass without inventing yield changes");
    bool budget_refused = false;
    try { (void)replay_bnw_fresh_yield_passes(make_grid(true), wrapped,
                                               yield_rules, 8); }
    catch (const std::length_error &) { budget_refused = true; }
    check(budget_refused, "Fresh repeated yield passes must obey a plot-visit budget");
    check(wrapped.area_ids == std::vector<std::int32_t>({8192, 16385, 8192}) &&
              wrapped.landmass_ids == wrapped.area_ids &&
              wrapped.area_assignment_order ==
                  std::vector<std::size_t>({0, 2, 1}) &&
              wrapped.landmass_assignment_order ==
                  std::vector<std::size_t>({0, 2, 1}) &&
              wrapped.areas[0].tiles == 2 && wrapped.areas[0].water &&
              wrapped.areas[0].west == 0 && wrapped.areas[0].east == 2 &&
              !wrapped.landmasses[0].lake &&
              wrapped.landmasses[0].lake_threshold_yield_passes == 1 &&
              wrapped.lake_threshold_full_yield_passes == 1,
          "Horizontal wrap must unite water endpoints with source boundary updates");
    BnwSetupPlotGrid astar_order;
    astar_order.reset(3, 2, false, false);
    for (std::size_t i = 0; i < astar_order.size(); ++i) {
        astar_order.apply_stock_fresh_plot_type(i, 2, false, false, true,
                                                {0, false, false});
        astar_order.apply_stock_fresh_terrain_type(i,
            {0, false, false}, false, false);
    }
    const auto astar_areas = astar_order.draft_fresh_area_structure(1);
    check(astar_areas.area_assignment_order.size() == 6 &&
              astar_areas.area_assignment_order[0] == 0 &&
              astar_areas.area_assignment_order[1] == 1 &&
              astar_areas.area_assignment_order[2] == 3,
          "SDK equal-cost A* head insertion must reverse first-neighbor callback order");
    const auto astar_replay = replay_bnw_fresh_yield_passes(
        astar_order, astar_areas, yield_rules);
    check(astar_replay.symbol_events.size() == 6 &&
              astar_replay.symbol_events[0].plot_index == 0 &&
              astar_replay.symbol_events[1].plot_index == 1 &&
              astar_replay.symbol_events[2].plot_index == 3,
          "setArea updateSymbols events must follow A* closure, not plot index");
    bool symbols_refused = false;
    try { (void)replay_bnw_fresh_yield_passes(astar_order, astar_areas,
                                               yield_rules, 20'000'000, 5); }
    catch (const std::length_error &) { symbols_refused = true; }
    check(symbols_refused, "Staged symbol events must obey an explicit budget");
    const auto zero_threshold = make_grid(false).draft_fresh_area_structure(0);
    check(!zero_threshold.landmasses[0].lake &&
              zero_threshold.lake_threshold_full_yield_passes == 2,
          "A zero lake threshold changes each fresh water landmass on its first tile");
    const auto negative_threshold = make_grid(false).draft_fresh_area_structure(-1);
    check(!negative_threshold.landmasses[0].lake &&
              negative_threshold.lake_threshold_full_yield_passes == 0,
          "A negative selected threshold has no lake transition");

    BnwSetupPlotGrid ordered_lakes;
    ordered_lakes.reset(5, 1, false, false);
    for (std::size_t i = 0; i < 5; ++i) {
        const bool water = i == 0 || i == 2 || i == 3;
        ordered_lakes.apply_stock_fresh_plot_type(i, water ? 3 : 2,
            false, false, true, {0, false, false});
        ordered_lakes.apply_stock_fresh_terrain_type(i,
            water ? BnwSelectedTerrain{1, true, false}
                  : BnwSelectedTerrain{0, false, false}, false, false);
    }
    const auto ordered_areas = ordered_lakes.draft_fresh_area_structure(1);
    const auto ordered_replay = replay_bnw_fresh_yield_passes(
        ordered_lakes, ordered_areas, yield_rules);
    check(ordered_replay.progress.lake_threshold_passes.size() == 1 &&
              ordered_replay.progress.lake_threshold_passes[0].changed_plots == 1 &&
              ordered_replay.final_cache[0][0] == 2 &&
              ordered_replay.final_cache[2][0] == 1 &&
              ordered_replay.progress.final_pass_changed_plots == 0,
          "Later lake-threshold map pass must update an earlier completed small lake");
    check(ordered_replay.symbol_events.back().phase ==
              BnwFreshYieldSymbolPhase::lake_threshold &&
              ordered_replay.symbol_events.back().plot_index == 0 &&
              ordered_replay.symbol_events.back().yields_after[0] == 2,
          "Lake-threshold updateSymbols must retain the changed plot snapshot");

    BnwSetupPlotGrid impassable;
    impassable.reset(3, 1, false, false);
    for (std::size_t i = 0; i < 3; ++i) {
        impassable.apply_stock_fresh_plot_type(i, 2, false, false, true,
                                               {0, false, false});
        impassable.apply_stock_fresh_terrain_type(i,
            {i == 1 ? 2 : 0, false, i == 1}, false, false);
    }
    const auto split = impassable.draft_fresh_area_structure(1);
    yield_rules.terrains[2].base[0] = 8;
    const auto split_yields = calculate_bnw_fresh_final_yields(
        impassable, split, yield_rules);
    check(split_yields[1][0] == 0,
          "Fresh impassable plot must produce no nature yield even with terrain base");
    BnwSetupPlotGrid hills;
    hills.reset(1, 1, false, false);
    hills.apply_stock_fresh_plot_type(0, 1, false, false, true,
                                      {0, false, false});
    hills.apply_stock_fresh_terrain_type(0, {0, false, false}, false, false);
    yield_rules.yields[1].hills = 1;
    yield_rules.terrains[0].hills[1] = 2;
    check(calculate_bnw_fresh_final_yields(
              hills, hills.draft_fresh_area_structure(1), yield_rules)[0][1] == 3,
          "Fresh hills must combine selected Yield and Terrain hill modifiers");
    BnwSetupPlotGrid remote_water;
    remote_water.reset(1, 1, false, false);
    remote_water.apply_stock_fresh_plot_type(0, 3, false, false, true,
                                             {0, false, false});
    remote_water.apply_stock_fresh_terrain_type(0, {1, true, false}, false, false);
    check(calculate_bnw_fresh_final_yields(
              remote_water, remote_water.draft_fresh_area_structure(1),
              yield_rules)[0][0] == 0,
          "Fresh isolated water outside city-work range must retain zero cache");
    check(split.area_ids == std::vector<std::int32_t>({8192, 16385, 24578}) &&
              split.landmass_ids == std::vector<std::int32_t>({8192, 8192, 8192}) &&
              split.landmasses[0].tiles == 3 &&
              split.landmasses[0].centroid_x_sum == 3,
          "AreaValid splits impassable land while LandmassValid unites all land");

    BnwSetupPlotGrid incomplete;
    incomplete.reset(1, 1, false, false);
    incomplete.apply_stock_fresh_plot_type(0, 3, false, false, true,
                                           {0, false, false});
    bool refused = false;
    try { (void)incomplete.draft_fresh_area_structure(1); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused, "Area structure must reject a plot without applied terrain");
}

void test_bnw_fresh_terrain_setter() {
    BnwSetupPlotGrid grid;
    grid.reset(3, 1, false, false);
    grid.apply_stock_fresh_terrain_type(0, {1, true, false}, false, false);
    check(grid.at(0).plot_type == 3 && grid.at(0).terrain_type == 1 &&
              !grid.at(0).impassable && !grid.step_finder_force_reset() &&
              grid.last_terrain_steps() ==
                  std::vector<BnwFreshTerrainStep>{
                      BnwFreshTerrainStep::assign_terrain,
                      BnwFreshTerrainStep::yield_skipped_without_area,
                      BnwFreshTerrainStep::update_impassability},
          "Fresh ocean coast assignment must update terrain and impassability in SDK order");
    grid.apply_stock_fresh_terrain_type(0, {2, true, true}, false, false);
    check(grid.at(0).terrain_type == 2 && grid.at(0).impassable &&
              grid.at(0).plot_type == 3 && grid.land_plot_count() == 0,
          "Same-water fresh terrain replacement must preserve plot category and count");
    grid.apply_stock_fresh_terrain_type(0, {2, true, true}, false, false);
    check(grid.at(0).terrain_type == 2 && grid.at(0).impassable,
          "Unchanged terrain type must be a no-op");
    bool refused = false;
    try { grid.apply_stock_fresh_terrain_type(0, {3, false, false}, false, false); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused && grid.at(0).terrain_type == 2 && grid.at(0).plot_type == 3,
          "Fresh terrain setter must reject recursive plot-type transition before mutation");
    refused = false;
    try { grid.apply_stock_fresh_terrain_type(1, {1, true, false}, true, false); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused && grid.at(1).terrain_type == -1,
          "Unported terrain recalculation flags must fail before mutation");
}

void test_bnw_fresh_land_terrain_draft() {
    civ5::content::RulesDatabase rules;
    rules.apply_sql(R"sql(
        CREATE TABLE PostDefines (Name TEXT, Key TEXT, [Table] TEXT);
        INSERT INTO PostDefines VALUES ('LAND_TERRAIN', 'TERRAIN_GRASS', 'Terrains');
        CREATE TABLE Terrains (ID INTEGER PRIMARY KEY, Type TEXT, Water INTEGER, Impassable INTEGER);
        INSERT INTO Terrains VALUES (0, 'TERRAIN_COAST', 1, 0);
        INSERT INTO Terrains VALUES (1, 'TERRAIN_GRASS', 0, 1);
    )sql");
    const auto land = resolve_bnw_selected_land_terrain(rules);
    check(land.id == 1 && !land.water && land.impassable,
          "Fresh land terrain must come from selected PostDefine and terrain Info slot");
    BnwSetupPlotGrid fresh;
    fresh.reset(2, 2, false, false);
    const auto category = fresh.draft_fresh_category_change(0, 2);
    const auto terrain = category.draft_assign_selected_land_terrain(0, land);
    check(terrain.last_terrain_steps() ==
              std::vector<BnwFreshTerrainStep>({
                  BnwFreshTerrainStep::assign_terrain,
                  BnwFreshTerrainStep::yield_skipped_without_area,
                  BnwFreshTerrainStep::update_impassability}),
          "Fresh terrain setter must preserve SDK assignment/yield/impassability order");
    check(category.at(0).terrain_type == -1 && !category.at(0).impassable &&
              terrain.at(0).terrain_type == 1 && terrain.at(0).impassable &&
              terrain.at(0).area_id == -1 && terrain.land_plot_count() == 1,
          "Fresh setTerrainType must assign terrain and terrain impassability on the draft only");
    bool refused = false;
    try { (void)fresh.draft_assign_selected_land_terrain(0, land); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused, "Land terrain must not be assigned before plot category changes");
    refused = false;
    try { (void)terrain.draft_assign_selected_land_terrain(0, land); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused, "Fresh terrain assignment must not run twice");
    refused = false;
    try { (void)category.draft_assign_selected_land_terrain(0, {0, true, false}); }
    catch (const std::invalid_argument &) { refused = true; }
    check(refused, "Water terrain cannot satisfy SDK LAND_TERRAIN branch");
    rules.apply_sql("UPDATE Terrains SET Water=1 WHERE Type='TERRAIN_GRASS'");
    refused = false;
    try { (void)resolve_bnw_selected_land_terrain(rules); }
    catch (const std::runtime_error &) { refused = true; }
    check(refused, "Selected LAND_TERRAIN must fail if its terrain row is water");
    rules.apply_sql("UPDATE PostDefines SET Key='TERRAIN_MISSING' WHERE Name='LAND_TERRAIN'");
    refused = false;
    try { (void)resolve_bnw_selected_land_terrain(rules); }
    catch (const std::runtime_error &) { refused = true; }
    check(refused, "Selected LAND_TERRAIN must fail if its Info slot is absent");
}

void test_bnw_map_rng_and_grid() {
    constexpr auto source = R"lua(
        assert(FlowDirectionTypes.NO_FLOWDIRECTION == -1)
        assert(FlowDirectionTypes.FLOWDIRECTION_NORTH == 0 and
               FlowDirectionTypes.FLOWDIRECTION_NORTHEAST == 1 and
               FlowDirectionTypes.FLOWDIRECTION_SOUTHEAST == 2 and
               FlowDirectionTypes.FLOWDIRECTION_SOUTH == 3 and
               FlowDirectionTypes.FLOWDIRECTION_SOUTHWEST == 4 and
               FlowDirectionTypes.FLOWDIRECTION_NORTHWEST == 5 and
               FlowDirectionTypes.NUM_FLOWDIRECTION_TYPES == 6)
        assert(FlowDirectionTypes.FLOWDIRECTION_NORTH_MASK == 1 and
               FlowDirectionTypes.FLOWDIRECTION_SOUTH_MASK == 2 and
               FlowDirectionTypes.FLOWDIRECTION_SOUTHEAST_MASK == 4 and
               FlowDirectionTypes.FLOWDIRECTION_NORTHWEST_MASK == 8 and
               FlowDirectionTypes.FLOWDIRECTION_SOUTHWEST_MASK == 16 and
               FlowDirectionTypes.FLOWDIRECTION_NORTHEAST_MASK == 32)
        assert(DirectionTypes.NO_DIRECTION == -1 and
               DirectionTypes.DIRECTION_NORTHEAST == 0 and
               DirectionTypes.DIRECTION_EAST == 1 and
               DirectionTypes.DIRECTION_SOUTHEAST == 2 and
               DirectionTypes.DIRECTION_SOUTHWEST == 3 and
               DirectionTypes.DIRECTION_WEST == 4 and
               DirectionTypes.DIRECTION_NORTHWEST == 5 and
               DirectionTypes.NUM_DIRECTION_TYPES == 6)
        function GetMapInitData()
            assert(Map.Rand(0, 'zero') == 0)
            assert(Map.Rand(1, 'one') == 0)
            return { Width = 4, Height = 1 }
        end
        function GeneratePlotTypes()
            local w, h = Map.GetGridSize()
            assert(w == 4 and h == 1 and select('#', Map.GetGridSize()) == 2)
            assert(Map.Rand(65537, 'narrow to one') == 0)
            local a = Map.Rand(4, 'terrain category')
            local b = Map.Rand(-1, 'u16 wrap')
            return { PlotTypes.PLOT_LAND, a, b % 4, PlotTypes.PLOT_OCEAN }
        end
    )lua";
    SimulationRandom input(RandomAlgorithm::bnw, {123, 7, 2});
    auto expected = input;
    check(expected.draw(0) == 0 && expected.draw(1) == 0 && expected.draw(1) == 0,
          "Small BNW ranges must return zero");
    const auto a = expected.draw(4), b = expected.draw(65535);
    const auto first = generate_bnw_plot_map(source, "=rng-grid", input);
    const auto second = generate_bnw_plot_map(source, "=rng-grid", input);
    check(first.map && second.map && first.map->plot_types == second.map->plot_types,
          "Same imported map RNG must reproduce identical plot categories");
    check(first.map->plot_types == std::vector<std::uint8_t>(
              {2, static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b % 4), 3}),
          "Map.Rand did not follow source order or u16 narrowing");
    check(first.map_random_after == expected.state() && second.map_random_after == expected.state() &&
              first.map_random_after->calls == 12 && input.state() == SimulationRandomState{123, 7, 2},
          "Map.Rand must advance only the copied map stream, including ranges 0 and 1");
    check(first.has_blocker(BnwMapScriptBlockerCode::terrain_pending) &&
              first.has_blocker(BnwMapScriptBlockerCode::world_setup_pending),
          "RNG and grid support do not complete world creation");
}

void test_bnw_map_rng_arguments() {
    const auto invalid = [](const char *call) {
        const std::string source = std::string(R"lua(
            function GetMapInitData() return { Width = 1, Height = 1 } end
            function GeneratePlotTypes()
                )lua") + call + R"lua(
                return { PlotTypes.PLOT_LAND }
            end
        )lua";
        SimulationRandom random(RandomAlgorithm::bnw, {77, 4, 1});
        const auto result = generate_bnw_plot_map(source, "=invalid-map-rand", random);
        check(!result.map && result.has_blocker(BnwMapScriptBlockerCode::script_failed) &&
                  result.map_random_after == random.state(),
              "Invalid Map.Rand arguments must fail without consuming RNG");
    };
    invalid("Map.Rand(4)");
    invalid("Map.Rand('bad', 'label')");
    invalid("Map.Rand(2147483648, 'out of source int32 range')");
    const auto early_grid = generate_bnw_plot_map(R"lua(
        local w, h = Map.GetGridSize()
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { 2 } end
    )lua", "=early-grid", SimulationRandom(RandomAlgorithm::bnw, {1, 0, 0}));
    check(!early_grid.map && early_grid.has_blocker(BnwMapScriptBlockerCode::script_failed),
          "GetGridSize before map setup must not invent dimensions");
    const auto wrong_profile = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { 2 } end
    )lua", "=vp-rng", SimulationRandom(RandomAlgorithm::vp_reference, {1, 0, 0}));
    check(!wrong_profile.map && wrong_profile.has_blocker(BnwMapScriptBlockerCode::invalid_map_random),
          "VP RNG must not be accepted by the BNW map-script boundary");
    const auto helper = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { Map.GetWorldSize() } end
    )lua", "=other-map-helper", SimulationRandom(RandomAlgorithm::bnw, {1, 0, 0}));
    check(!helper.map && helper.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Other stock Map helpers must retain an explicit blocker");
    const auto missing_stream = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { Map.Rand(4, 'needs stream') } end
    )lua", "=missing-map-stream");
    check(!missing_stream.map &&
              missing_stream.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Map.Rand without an imported map stream must remain an explicit blocker");
}

void test_bnw_fractal_wrap_flags() {
    const auto regional = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1, WrapX = false } end
        function GeneratePlotTypes()
            local flags = Map.GetFractalFlags()
            assert(flags.FRAC_WRAP_X == nil and flags.FRAC_WRAP_Y == nil)
            flags.FRAC_WRAP_X = true
            assert(Map.GetFractalFlags().FRAC_WRAP_X == nil)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=regional-wrap");
    check(regional.map && !regional.map->wrap_x && !regional.map->wrap_y,
          "False WrapX and absent WrapY must produce an empty fresh fractal flag table");

    const auto both = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1, WrapY = true } end
        function GeneratePlotTypes()
            local flags = Map.GetFractalFlags()
            assert(flags.FRAC_WRAP_X == true and flags.FRAC_WRAP_Y == true)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=both-wraps");
    check(both.map && both.map->wrap_x && both.map->wrap_y,
          "Absent WrapX must keep CvMapInitData true; explicit WrapY must add its flag");

    const auto truthiness = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1, WrapX = 0, WrapY = '' } end
        function GeneratePlotTypes()
            local flags = Map.GetFractalFlags()
            assert(flags.FRAC_WRAP_X == true and flags.FRAC_WRAP_Y == true)
            return { PlotTypes.PLOT_LAND }
        end
    )lua", "=source-lua-boolean");
    check(truthiness.map && truthiness.map->wrap_x && truthiness.map->wrap_y,
          "Source lua_toboolean treats non-nil, non-false values as true");

    const auto early = generate_bnw_plot_map(R"lua(
        local flags = Map.GetFractalFlags()
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { PlotTypes.PLOT_LAND } end
    )lua", "=early-fractal-wrap");
    check(!early.map && early.has_blocker(BnwMapScriptBlockerCode::unresolved_map_wrap),
          "GetFractalFlags before GetMapInitData must report unresolved wrap state");
}

void test_selected_bnw_map_setup() {
    BnwMapSetupContext setup;
    setup.world_size = 3;
    setup.custom_options = std::map<std::string, std::int64_t>{{"1", 2}, {"SEA_LEVEL", 3}};
    SimulationRandom random(RandomAlgorithm::bnw, {19, 0, 0});
    const auto result = generate_bnw_plot_map(R"lua(
        function GetMapInitData(worldSize)
            assert(worldSize == 3)
            return { Width = worldSize, Height = Map.GetCustomOption(1) }
        end
        function GeneratePlotTypes()
            local w, h = Map.GetGridSize()
            assert(w == 3 and h == 2)
            assert(Map.GetWorldSize() == 3)
            assert(Map.GetCustomOption('SEA_LEVEL') == 3)
            assert(Map.Rand(1, 'consume map seed') == 0)
            return { 2, 2, 2, 3, 3, 3 }
        end
    )lua", "=selected-setup", setup, random);
    check(result.map && result.map->width == 3 && result.map->height == 2 &&
              result.map_random_after && result.map_random_after->calls == 1 &&
              random.state().calls == 0,
          "Selected world size and pregame map options must reach both stock setup phases");

    const auto missing = generate_bnw_plot_map(R"lua(
        function GetMapInitData(worldSize)
            return { Width = worldSize, Height = Map.GetCustomOption(2) }
        end
        function GeneratePlotTypes() return { 2 } end
    )lua", "=missing-option", setup);
    check(!missing.map && missing.has_blocker(BnwMapScriptBlockerCode::missing_map_option),
          "An unresolved selected map option must stop the setup explicitly");

    BnwMapSetupContext invalid = setup;
    invalid.world_size = -1;
    check(generate_bnw_plot_map("return true", "=invalid-setup", invalid).has_blocker(
              BnwMapScriptBlockerCode::invalid_setup_context),
          "Invalid WorldSizes ID must not be exposed to a stock script");

    const auto absent = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { Map.GetCustomOption(1) } end
    )lua", "=no-option-context");
    check(!absent.map && absent.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Map.GetCustomOption needs explicit selected pregame choices");
}

void test_selected_world_rules() {
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.profile.can_activate = true;
    selected.profile.enabled_dlc = {0};
    ContentRequirement expansion2;
    expansion2.kind = civ5::content::PackageKind::dlc;
    expansion2.id = "6da07636-4123-4018-b643-6575b4ec336b";
    expansion2.status = RequirementStatus::located;
    expansion2.package = 0;
    expansion2.wire_value = 1;
    selected.profile.requirements.push_back(expansion2);
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE Worlds (ID INTEGER PRIMARY KEY, Type TEXT NOT NULL UNIQUE,
                             FeatureGrainChange INTEGER NOT NULL);
        INSERT INTO Worlds (ID, Type, FeatureGrainChange)
            VALUES (0, 'WORLDSIZE_DUEL', -1);
        INSERT INTO Worlds (ID, Type, FeatureGrainChange)
            VALUES (3, 'WORLDSIZE_STANDARD', 0);
        CREATE TABLE Features (ID INTEGER PRIMARY KEY, Type TEXT NOT NULL UNIQUE);
        INSERT INTO Features (ID, Type) VALUES (4, 'FEATURE_FOREST');
        CREATE TABLE Climates (ID INTEGER PRIMARY KEY, Type TEXT NOT NULL UNIQUE);
        INSERT INTO Climates (ID, Type) VALUES (0, 'CLIMATE_TEMPERATE');
        INSERT INTO Climates (ID, Type) VALUES (3, 'CLIMATE_ROCKY');
    )sql");
    BnwMapSetupContext setup;
    setup.world_size = 3;
    setup.climate = 3;
    setup.selected_rules = &selected;
    setup.graphics_initialized = false;
    const auto result = generate_bnw_plot_map(R"lua(
        function GetMapInitData(worldSize)
            assert(worldSize == 3)
            assert(GameInfo.Worlds.WORLDSIZE_DUEL.ID == 0)
            assert(GameInfo.Worlds.WORLDSIZE_STANDARD.ID == 3)
            assert(GameInfo.Worlds[3].Type == 'WORLDSIZE_STANDARD')
            assert(GameInfo.Worlds[3].FeatureGrainChange == 0)
            assert(Map.GetClimate() == 3)
            assert(GameInfo.Climates[3].Type == 'CLIMATE_ROCKY')
            assert(GameInfo.Climates.CLIMATE_TEMPERATE.ID == 0)
            local climates, climate_count = '', 0
            for row in GameInfo.Climates() do
                climate_count = climate_count + 1
                climates = climates .. row.Type .. ':' .. row.ID .. ';'
            end
            assert(climate_count == 2 and climates ==
                   'CLIMATE_TEMPERATE:0;CLIMATE_ROCKY:3;')
            assert(not pcall(function()
                GameInfo.Climates.CLIMATE_ROCKY.ID = 0
            end))
            assert(not pcall(function()
                return GameInfo.Climates[3].JungleLatitude
            end))
            assert(FeatureTypes.NO_FEATURE == -1)
            assert(FeatureTypes.FEATURE_FOREST == 4)
            assert(not pcall(function() return FeatureTypes.FEATURE_MISSING end))
            assert(not pcall(function() FeatureTypes.FEATURE_FOREST = 9 end))
            local types, count = '', 0
            for row in GameInfo.Worlds() do
                count = count + 1
                types = types .. row.Type .. ':' .. row.ID .. ';'
            end
            assert(count == 2 and types == 'WORLDSIZE_DUEL:0;WORLDSIZE_STANDARD:3;')
            local writable = pcall(function() GameInfo.Worlds.WORLDSIZE_DUEL.ID = 9 end)
            assert(not writable)
            return { Width = 2, Height = 1 }
        end
        function GeneratePlotTypes() return { 2, 3 } end
    )lua", "=selected-worlds", setup);
    check(result.map && result.world_rules_source &&
              result.world_rules_source->worlds_sha256.size() == 64 &&
              result.world_rules_source->selected_requirements ==
                  std::vector<std::string>{"dlc:6da07636-4123-4018-b643-6575b4ec336b@1"},
          "Selected Worlds rows must support SDK type/ROWID lookup and iteration with provenance");
    auto invalid_climate = setup;
    invalid_climate.climate = 99;
    check(generate_bnw_plot_map("return true", "=unselected-climate",
                                invalid_climate).has_blocker(
              BnwMapScriptBlockerCode::invalid_world_rules),
          "Climate choice absent from selected Info rows must fail closed");

    selected.rules->apply_sql(R"sql(
        CREATE TABLE PostDefines (Name TEXT, Key TEXT, [Table] TEXT);
        INSERT INTO PostDefines VALUES ('LAND_TERRAIN', 'TERRAIN_GRASS', 'Terrains');
        CREATE TABLE Terrains (ID INTEGER PRIMARY KEY, Type TEXT, Water INTEGER, Impassable INTEGER);
        INSERT INTO Terrains VALUES (0, 'TERRAIN_GRASS', 0, 0);
    )sql");
    const auto fresh_setter = generate_bnw_plot_map(R"lua(
        function GetMapInitData(worldSize) return { Width = 3, Height = 1 } end
        function GeneratePlotTypes()
            assert(TerrainTypes.TERRAIN_GRASS == 0)
            local land = Map.GetPlotByIndex(0)
            local adjacent_ocean = Map.GetPlotByIndex(1)
            assert(not adjacent_ocean:IsAdjacentToLand())
            land:SetPlotType(PlotTypes.PLOT_LAND, false, false)
            assert(adjacent_ocean:IsAdjacentToLand())
            assert(not land:IsAdjacentToLand())
            assert(land:GetPlotType() == PlotTypes.PLOT_LAND and
                   land:GetTerrainType() == 0 and not land:IsWater())
            local ocean = Map.GetPlotByIndex(1)
            ocean:SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            assert(ocean:GetTerrainType() == -1 and ocean:IsWater())
            return { PlotTypes.PLOT_LAND, PlotTypes.PLOT_OCEAN, PlotTypes.PLOT_OCEAN }
        end
    )lua", "=selected-fresh-plot-setter", setup);
    check(fresh_setter.map && fresh_setter.map->plot_types ==
              std::vector<std::uint8_t>({2, 3, 3}) &&
              fresh_setter.has_blocker(BnwMapScriptBlockerCode::terrain_pending),
          "Selected stock plot setter must update real setup handles without claiming a world");

    const auto nil_after_full_grid = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetPlotType(PlotTypes.PLOT_LAND, false, false)
            Map.GetPlotByIndex(1):SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            -- Stock MapGenerator ignores the return value of this function.
        end
    )lua", "=stock-nil-full-grid", setup);
    check(nil_after_full_grid.map && nil_after_full_grid.map->plot_types ==
              std::vector<std::uint8_t>({2, 3}) &&
              nil_after_full_grid.has_blocker(BnwMapScriptBlockerCode::terrain_pending),
          "Nil-returning stock script must use every owned plot assignment, including unchanged ocean");
    const auto nil_after_partial_grid = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetPlotType(PlotTypes.PLOT_LAND, false, false)
        end
    )lua", "=stock-nil-partial-grid", setup);
    check(!nil_after_partial_grid.map && nil_after_partial_grid.has_blocker(
              BnwMapScriptBlockerCode::invalid_plot_types),
          "Nil return must fail closed when an initial ocean plot was never assigned");
    const auto nil_without_writes = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() end
    )lua", "=stock-nil-no-writes", setup);
    check(!nil_without_writes.map && nil_without_writes.has_blocker(
              BnwMapScriptBlockerCode::invalid_plot_types),
          "Nil return without any plot mutation must not promote default ocean to generated map");

    selected.rules->apply_sql(R"sql(
        CREATE TABLE Defines (Name TEXT PRIMARY KEY, Value INTEGER);
        INSERT INTO Defines VALUES ('LAKE_PLOT_RAND', 7);
        INSERT INTO Defines VALUES ('LAKE_MAX_AREA_SIZE', 1);
        INSERT INTO Defines VALUES ('MIN_WATER_SIZE_FOR_OCEAN', 1);
        INSERT INTO Defines VALUES ('SHALLOW_WATER_TERRAIN', 99);
        INSERT INTO PostDefines VALUES ('SHALLOW_WATER_TERRAIN', 'TERRAIN_COAST', 'Terrains');
        INSERT INTO PostDefines VALUES ('DEEP_WATER_TERRAIN', 'TERRAIN_OCEAN', 'Terrains');
        INSERT INTO Terrains VALUES (1, 'TERRAIN_COAST', 1, 0);
        INSERT INTO Terrains VALUES (2, 'TERRAIN_OCEAN', 1, 0);
        CREATE TABLE Resources (Type TEXT, ResourceUsage INTEGER, Happiness INTEGER);
        CREATE TABLE Unit_ResourceQuantityRequirements (ResourceType TEXT);
        CREATE TABLE Yields (ID INTEGER PRIMARY KEY, Type TEXT,
            HillsChange INTEGER, LakeChange INTEGER);
        INSERT INTO Yields VALUES (0,'YIELD_FOOD',0,1);
        INSERT INTO Yields VALUES (1,'YIELD_PRODUCTION',1,0);
        INSERT INTO Yields VALUES (2,'YIELD_GOLD',0,0);
        INSERT INTO Yields VALUES (3,'YIELD_SCIENCE',0,0);
        INSERT INTO Yields VALUES (4,'YIELD_CULTURE',0,0);
        INSERT INTO Yields VALUES (5,'YIELD_FAITH',0,0);
        CREATE TABLE Terrain_Yields (TerrainType TEXT, YieldType TEXT, Yield INTEGER);
        CREATE TABLE Terrain_HillsYieldChanges (TerrainType TEXT, YieldType TEXT, Yield INTEGER);
        CREATE TABLE Terrain_RiverYieldChanges (TerrainType TEXT, YieldType TEXT, Yield INTEGER);
        INSERT INTO Terrain_Yields VALUES ('TERRAIN_GRASS','YIELD_FOOD',2);
        INSERT INTO Terrain_RiverYieldChanges VALUES ('TERRAIN_GRASS','YIELD_FOOD',1);
        INSERT INTO Terrain_Yields VALUES ('TERRAIN_COAST','YIELD_FOOD',1);
        INSERT INTO Terrain_Yields VALUES ('TERRAIN_COAST','YIELD_GOLD',1);
    )sql");
    const auto selected_terrain = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes()
            local coast = Map.GetPlotByIndex(0)
            local adjacent = Map.GetPlotByIndex(1)
            assert(not adjacent:IsAdjacentToShallowWater())
            coast:SetTerrainType(GameDefines.SHALLOW_WATER_TERRAIN, false, false)
            assert(coast:GetTerrainType() == 1 and coast:IsWater())
            assert(adjacent:IsAdjacentToShallowWater())
            coast:SetTerrainType(GameDefines.DEEP_WATER_TERRAIN, false, false)
            assert(coast:GetTerrainType() == 2 and coast:IsWater())
            assert(not adjacent:IsAdjacentToShallowWater())
            return { PlotTypes.PLOT_OCEAN, PlotTypes.PLOT_OCEAN }
        end
    )lua", "=selected-fresh-terrain", setup);
    check(selected_terrain.map && selected_terrain.map->plot_types ==
              std::vector<std::uint8_t>({3, 3}),
          "Selected terrain Info must drive fresh ocean terrain assignments");
    constexpr auto full_terrain_source = R"lua(
        function GetMapInitData() return { Width = 2, Height = 1, WrapX = false } end
        function GeneratePlotTypes()
            local water, land = Map.GetPlotByIndex(0), Map.GetPlotByIndex(1)
            water:SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            water:SetTerrainType(GameDefines.SHALLOW_WATER_TERRAIN, false, false)
            land:SetPlotType(PlotTypes.PLOT_LAND, false, false)
        end
        function GenerateTerrain()
            assert(TerrainTypes.NO_TERRAIN == -1)
            assert(GameInfoTypes.TERRAIN_GRASS == 0)
            assert(not pcall(function() GameInfoTypes.TERRAIN_GRASS = 9 end))
            assert(not pcall(function() TerrainTypes.NO_TERRAIN = 0 end))
            assert(math.clamp(2, 0, 1) == 1)
            local land = Map.GetPlot(1, 0)
            assert(land:GetPlotIndex() == 1)
            land:SetTerrainType(GameInfoTypes.TERRAIN_GRASS, false, false)
        end
        function AddRivers()
            local land = Map.GetPlotByIndex(1)
            assert(not land:IsHills() and land:IsCoastalLand())
            assert(Map.FindWater(land, 1, false))
            assert(Map.FindWater(land, 0, true))
            assert(not Map.FindWater(land, -1, true))
            Map.Rand(3, 'rolled-back river probe')
            local corner = land:GetInlandCorner()
            assert(corner and corner == Map.GetPlotByIndex(corner:GetPlotIndex()))
            assert(not corner:IsNEOfRiver() and not corner:IsWOfRiver() and
                   not corner:IsNWOfRiver())
            assert(Map.PlotDirection(land:GetX(), land:GetY(),
                   DirectionTypes.DIRECTION_WEST) == Map.GetPlotByIndex(0))
            assert(Map.PlotDirection(land:GetX(), land:GetY(),
                   DirectionTypes.DIRECTION_EAST) == nil)
            corner:SetWOfRiver(true, 0)
            error('forced river rollback after selected write')
        end
    )lua";
    auto full_terrain = generate_bnw_plot_map(
        full_terrain_source, "=selected-full-terrain", setup,
        SimulationRandom(RandomAlgorithm::bnw, {123, 0, 0}));
    check(full_terrain.map && full_terrain.map->terrain_types ==
              std::optional<std::vector<std::int32_t>>({1, 0}) &&
              full_terrain.map->area_structure &&
              full_terrain.map->area_structure->area_ids ==
                  std::vector<std::int32_t>({8192, 16385}) &&
              full_terrain.map->area_structure->lake_max_area_size == 1 &&
              full_terrain.map->area_structure->landmasses[0].lake &&
              full_terrain.map->area_structure->area_assignment_yield_calls == 2 &&
              full_terrain.map->final_yields &&
              full_terrain.map->owned_setup_grid &&
              full_terrain.map->owned_setup_grid->at(0).area_id == 8192 &&
              full_terrain.map->owned_setup_grid->at(0).landmass_id == 8192 &&
              full_terrain.map->owned_setup_grid->at(0).yields[0] == 2 &&
              !full_terrain.map->owned_setup_grid->at(1).ne_of_river &&
              !full_terrain.map->owned_setup_grid->at(1).w_of_river &&
              !full_terrain.map->owned_setup_grid->at(1).nw_of_river &&
              full_terrain.map->owned_setup_grid->symbol_events().size() == 3 &&
              full_terrain.map->river_symbol_events == 0 &&
              full_terrain.map->river_edge_writes == 0 &&
              full_terrain.map->yield_progress &&
              full_terrain.map->yield_progress->set_area_changed_plots == 2 &&
              full_terrain.map->yield_progress->final_pass_changed_plots == 1 &&
              (*full_terrain.map->final_yields)[0][0] == 2 &&
              (*full_terrain.map->final_yields)[0][2] == 1 &&
              (*full_terrain.map->final_yields)[1][0] == 2 &&
              !full_terrain.has_blocker(BnwMapScriptBlockerCode::terrain_pending) &&
              full_terrain.has_blocker(BnwMapScriptBlockerCode::areas_pending) &&
              full_terrain.has_blocker(BnwMapScriptBlockerCode::river_generation_pending) &&
              full_terrain.map_random_before == full_terrain.map_random_after,
          "Complete selected terrain generation must retain owned terrain and clear only its blocker");
    const auto river_write = inspect_bnw_fresh_river_write(
        *full_terrain.map->owned_setup_grid, 1, BnwFreshRiverEdge::west, true, 0);
    check(river_write.blocker == BnwFreshRiverWriteBlockerCode::required_services_pending &&
              river_write.effects == std::array<BnwFreshRiverWriteEffect, 6>{
                  BnwFreshRiverWriteEffect::edge_bit,
                  BnwFreshRiverWriteEffect::self_crossings,
                  BnwFreshRiverWriteEffect::self_yield,
                  BnwFreshRiverWriteEffect::neighbor_crossing_then_yield_each_direction,
                  BnwFreshRiverWriteEffect::area_river_edge_count,
                  BnwFreshRiverWriteEffect::flow_direction} &&
              river_write.missing_services == std::vector<std::string_view>{
                  "river_crossing_mask_and_count",
                  "selected_terrain_river_yield_and_symbols",
                  "area_river_edge_count", "river_flow_direction_state"} &&
              inspect_bnw_fresh_river_write(*full_terrain.map->owned_setup_grid,
                  1, BnwFreshRiverEdge::west, true, 1).blocker ==
                  BnwFreshRiverWriteBlockerCode::outside_fresh_stock_contract,
          "River preflight must preserve setter source order and list missing atomic services");
    check(std::any_of(full_terrain.blockers.begin(), full_terrain.blockers.end(),
              [](const auto &blocker) {
                  return blocker.code == BnwMapScriptBlockerCode::river_generation_pending &&
                         blocker.message.find("forced river rollback after selected write") !=
                             std::string::npos;
              }),
          "Selected AddRivers must roll back a committed river after a later Lua error");
    const auto all_land_river = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 3, Height = 3, WrapX = false } end
        function GeneratePlotTypes()
            for i = 0, Map.GetNumPlots() - 1 do
                Map.GetPlotByIndex(i):SetPlotType(PlotTypes.PLOT_LAND, false, false)
            end
        end
        function GenerateTerrain()
            for i = 0, Map.GetNumPlots() - 1 do
                Map.GetPlotByIndex(i):SetTerrainType(GameInfoTypes.TERRAIN_GRASS,
                                                     false, false)
            end
        end
        function AddRivers()
            local center = Map.GetPlotByIndex(4)
            local corner = Map.GetPlotByIndex(0)
            assert(center:Area() == corner:Area())
            assert(center:Area():GetNumTiles() == 9)
            assert(center:Area():GetNumRiverEdges() == 0)
            Map.GetPlotByIndex(4):SetNWOfRiver(true,
                FlowDirectionTypes.FLOWDIRECTION_NORTHEAST)
            assert(center:IsNWOfRiver() and center:Area():GetNumRiverEdges() == 1)
            assert(Map.FindWater(center, 0, true))
            center:SetWOfRiver(true, FlowDirectionTypes.FLOWDIRECTION_NORTH)
            assert(center:Area():GetNumRiverEdges() == 2)
            error('forced rollback after two river writes')
        end
    )lua", "=all-land-river-crossing-draft", setup,
        SimulationRandom(RandomAlgorithm::bnw, {17, 0, 0}));
    check(all_land_river.map && all_land_river.map->owned_setup_grid &&
              all_land_river.map->owned_setup_grid->size() == 9 &&
              all_land_river.has_blocker(BnwMapScriptBlockerCode::river_generation_pending) &&
              all_land_river.map->river_symbol_events == 0 &&
              all_land_river.map->river_edge_writes == 0 &&
              all_land_river.map->owned_setup_grid->at(4).river_crossing_mask == 0,
          "Later Lua error must roll back two published river writes and all selected state");
    constexpr auto committed_river_source = R"lua(
        function GetMapInitData() return { Width = 3, Height = 3, WrapX = false } end
        function GeneratePlotTypes()
            for i = 0, Map.GetNumPlots() - 1 do
                Map.GetPlotByIndex(i):SetPlotType(PlotTypes.PLOT_LAND, false, false)
            end
        end
        function GenerateTerrain()
            for i = 0, Map.GetNumPlots() - 1 do
                Map.GetPlotByIndex(i):SetTerrainType(GameInfoTypes.TERRAIN_GRASS,
                                                     false, false)
            end
        end
        function AddRivers()
            local center = Map.GetPlotByIndex(4)
            center:SetNWOfRiver(true, FlowDirectionTypes.FLOWDIRECTION_NORTHEAST)
            assert(center:IsNWOfRiver())
            assert(center:Area():GetNumRiverEdges() == 1)
            assert(Map.FindWater(center, 0, true))
            assert(Map.FindWater(Map.GetPlotByIndex(2), 0, true))
        end
        function AddLakes()
            assert(Map.GetPlotByIndex(4):IsRiver())
            assert(not Map.GetPlotByIndex(0):IsRiver())
            Map.GetPlotByIndex(4):SetWOfRiver(true,
                FlowDirectionTypes.FLOWDIRECTION_NORTH)
            Map.Rand(160, "Lake probe rollback")
            local plot = Map.GetPlotByIndex(0)
            local area = plot:Area()
            local old_tiles = area:GetNumTiles()
            plot:SetArea(-1)
            assert(area:GetNumTiles() == old_tiles - 1)
            plot:SetPlotType(PlotTypes.PLOT_OCEAN)
        end
    )lua";
    const auto committed_river = generate_bnw_plot_map(
        committed_river_source, "=committed-fresh-river", setup,
        SimulationRandom(RandomAlgorithm::bnw, {17, 0, 0}));
    check(committed_river.map && committed_river.map->owned_setup_grid &&
              !committed_river.has_blocker(BnwMapScriptBlockerCode::river_generation_pending) &&
              committed_river.map->river_edge_writes == 1 &&
              committed_river.map->river_symbol_events == 2 &&
              committed_river.map->area_structure &&
              committed_river.map->area_structure->areas.size() == 1 &&
              committed_river.map->area_structure->areas[0].river_edges == 1 &&
              committed_river.map->final_yields &&
              (*committed_river.map->final_yields)[4][0] == 3 &&
              (*committed_river.map->final_yields)[2][0] == 3 &&
              committed_river.map->owned_setup_grid->at(4).nw_of_river &&
              committed_river.map->owned_setup_grid->at(4).river_se_flow_direction == 1 &&
              committed_river.map->owned_setup_grid->at(4).yields ==
                  (*committed_river.map->final_yields)[4] &&
              committed_river.map_random_before == committed_river.map_random_after &&
              committed_river.has_blocker(BnwMapScriptBlockerCode::lake_generation_pending) &&
              std::any_of(committed_river.blockers.begin(), committed_river.blockers.end(),
                          [](const auto &blocker) {
                              return blocker.code == BnwMapScriptBlockerCode::lake_generation_pending &&
                                     blocker.message.find("selected AddLakes call order differs") != std::string::npos;
                          }),
          "River publication must survive a failed lake probe while grid and RNG roll back");
    std::string unassigned_area_source(committed_river_source);
    const auto setter_at = unassigned_area_source.find(
        "plot:SetPlotType(PlotTypes.PLOT_OCEAN)");
    check(setter_at != std::string::npos, "Area-only lake regression fixture is intact");
    unassigned_area_source.replace(setter_at,
        std::string("plot:SetPlotType(PlotTypes.PLOT_OCEAN)").size(),
        "Map.CalculateAreas()");
    const auto rejected_calculate = generate_bnw_plot_map(
        unassigned_area_source, "=unassigned-lake-area", setup,
        SimulationRandom(RandomAlgorithm::bnw, {17, 0, 0}));
    check(rejected_calculate.map && rejected_calculate.map->owned_setup_grid &&
              rejected_calculate.map->area_structure &&
              committed_river.map->area_structure &&
              rejected_calculate.map->area_structure->area_ids ==
                  committed_river.map->area_structure->area_ids &&
              rejected_calculate.map->owned_setup_grid->at(0).plot_type ==
                  committed_river.map->owned_setup_grid->at(0).plot_type &&
              rejected_calculate.map->owned_setup_grid->at(0).terrain_type ==
                  committed_river.map->owned_setup_grid->at(0).terrain_type &&
              rejected_calculate.map->owned_setup_grid->symbol_events().size() ==
                  committed_river.map->owned_setup_grid->symbol_events().size() &&
              rejected_calculate.map_random_after == rejected_calculate.map_random_before &&
              rejected_calculate.has_blocker(BnwMapScriptBlockerCode::lake_generation_pending) &&
              std::any_of(rejected_calculate.blockers.begin(),
                          rejected_calculate.blockers.end(), [](const auto &blocker) {
                              return blocker.code == BnwMapScriptBlockerCode::lake_generation_pending &&
                                  blocker.message.find("unassigned plot needs area A-star") !=
                                      std::string::npos;
                          }),
          "CalculateAreas must reject an unassigned lake and restore grid and map RNG");
    const auto &committed_grid = *committed_river.map->owned_setup_grid;
    const auto &committed_areas = *committed_river.map->area_structure;
    const auto unassigned_area_draft = committed_grid.draft_fresh_area_unassignment(
        4, committed_areas);
    using AreaStep = BnwFreshAreaUnassignmentEventKind;
    check(unassigned_area_draft.grid.at(4).area_id == -1 &&
              unassigned_area_draft.areas.area_ids[4] == -1 &&
              unassigned_area_draft.grid.at(4).landmass_id == committed_grid.at(4).landmass_id &&
              unassigned_area_draft.grid.at(4).yields == committed_grid.at(4).yields &&
              find_bnw_fresh_area(unassigned_area_draft.areas, committed_grid.at(0).area_id).tiles == 8 &&
              find_bnw_fresh_area(unassigned_area_draft.areas, committed_grid.at(0).area_id).river_edges == 0 &&
              committed_grid.at(4).area_id == committed_areas.area_ids[4] &&
              committed_areas.areas[0].tiles == 9 &&
              committed_areas.areas[0].river_edges == 1 &&
              unassigned_area_draft.source_order == std::vector<BnwFreshAreaUnassignmentEvent>{
                  {AreaStep::old_lake_read, std::nullopt, false},
                  {AreaStep::old_area_tiles_decremented, std::nullopt, false},
                  {AreaStep::old_area_river_edge_decremented,
                   BnwFreshRiverEdge::north_west},
                  {AreaStep::area_handle_cleared, std::nullopt, false}},
          "Fresh SetArea(-1) draft must decrement old area before clearing the handle");
    const auto no_river_area_draft = committed_grid.draft_fresh_area_unassignment(
        0, committed_areas);
    const auto selected_grass = resolve_bnw_selected_terrain_by_id(
        *selected.rules, committed_grid.at(0).terrain_type);
    const auto lake_prefix = no_river_area_draft.grid.draft_fresh_lake_plot_type_prefix(
        0, no_river_area_draft.areas, selected_grass);
    using LakeStep = BnwFreshLakePlotTypePrefixEventKind;
    check(lake_prefix.grid.at(0).plot_type == 3 &&
              lake_prefix.grid.at(0).area_id == -1 &&
              lake_prefix.grid.at(0).terrain_type == committed_grid.at(0).terrain_type &&
              lake_prefix.grid.at(0).yields == committed_grid.at(0).yields &&
              lake_prefix.grid.land_plot_count() == committed_grid.land_plot_count() &&
              committed_grid.at(0).plot_type != 3 &&
              lake_prefix.source_order == std::vector<LakeStep>{
                  LakeStep::erase_empty_fresh_plot, LakeStep::old_water_read,
                  LakeStep::sight_removed_empty, LakeStep::plot_type_written,
                  LakeStep::yield_skipped_without_area, LakeStep::sight_added_empty,
                  LakeStep::terrain_conversion_required} &&
              !lake_prefix.remaining_services.empty() &&
              lake_prefix.remaining_services.front() ==
                  "selected_shallow_water_terrain_and_coast",
          "Fresh lake prefix must stop at selected coast terrain before later effects");
    bool rejected_river_lake = false;
    try {
        (void)unassigned_area_draft.grid.draft_fresh_lake_plot_type_prefix(
            4, unassigned_area_draft.areas, selected_grass);
    } catch (const std::invalid_argument &) {
        rejected_river_lake = true;
    }
    check(rejected_river_lake,
          "Fresh lake prefix must not erase a river plot outside stock AddLakes");
    const auto shallow_terrain = resolve_bnw_selected_terrain_by_id(*selected.rules, 1);
    const auto shallow_lake = BnwSetupPlotGrid::advance_fresh_lake_terrain(
        lake_prefix, 0, shallow_terrain);
    auto expected_lake_order = lake_prefix.source_order;
    expected_lake_order.insert(expected_lake_order.end(), {
        LakeStep::adjacent_land_read, LakeStep::selected_water_terrain_written,
        LakeStep::terrain_yield_skipped_without_area,
        LakeStep::terrain_impassability_updated,
        LakeStep::terrain_recursion_skipped_matching_water,
        LakeStep::coast_cache_written});
    check(shallow_lake.shallow_water && shallow_lake.grid.at(0).plot_type == 3 &&
              shallow_lake.grid.at(0).terrain_type == shallow_terrain.id &&
              shallow_lake.grid.at(0).impassable == shallow_terrain.impassable &&
              shallow_lake.grid.at(0).area_id == -1 &&
              shallow_lake.grid.at(0).yields == committed_grid.at(0).yields &&
              shallow_lake.grid.land_plot_count() == committed_grid.land_plot_count() &&
              shallow_lake.source_order == expected_lake_order &&
              shallow_lake.remaining_services.front() == "step_finder_reset",
          "Fresh lake conversion must select coast terrain and stop before step finder");
    const std::array lake_water_terrains{shallow_terrain.id};
    const auto lake_yield_rules = load_bnw_fresh_yield_rules(
        *selected.rules, committed_grid, lake_water_terrains);
    const auto lake_neighbors = BnwSetupPlotGrid::advance_fresh_lake_neighbors(
        shallow_lake, 0, no_river_area_draft.areas, lake_yield_rules,
        shallow_terrain, resolve_bnw_selected_terrain_by_id(*selected.rules, 2),
        2'000'000);
    auto expected_neighbor_steps = expected_lake_order;
    expected_neighbor_steps.insert(expected_neighbor_steps.end(), {
        LakeStep::step_finder_reset, LakeStep::neighbor_terrain_pass,
        LakeStep::neighbor_yield_pass});
    std::vector<BnwFreshLakeNeighborEvent> expected_neighbor_order;
    const auto sdk_neighbors = lake_neighbors.grid.adjacent(0);
    for (std::size_t direction = 0; direction < sdk_neighbors.size(); ++direction)
        if (sdk_neighbors[direction])
            expected_neighbor_order.push_back({
                BnwFreshLakeNeighborEventKind::yield_checked,
                *sdk_neighbors[direction], static_cast<std::uint8_t>(direction)});
    check(lake_neighbors.grid.step_finder_force_reset() &&
              lake_neighbors.grid.at(0).area_id == -1 &&
              lake_neighbors.grid.land_plot_count() == committed_grid.land_plot_count() &&
              lake_neighbors.source_order == expected_neighbor_steps &&
              lake_neighbors.neighbor_order == expected_neighbor_order &&
              lake_neighbors.remaining_services.front() == "city_work_yields" &&
              committed_grid.at(0).plot_type != 3,
          "Fresh lake neighbor draft must reset step finder then replay six yield calls without publication");
    const auto lake_city_area =
        BnwSetupPlotGrid::advance_fresh_lake_city_area_preflight(
            lake_neighbors, 0, no_river_area_draft.areas, lake_yield_rules,
            2'000'000);
    check(lake_city_area.grid.land_plot_count() ==
              committed_grid.land_plot_count() - 1 &&
              lake_city_area.city_order.size() == 37 &&
              !lake_city_area.adjacent_water_area_id &&
              !lake_city_area.recalculate_areas &&
              lake_city_area.remaining_services.front() ==
                  "incremental_area_attach_or_create" &&
              lake_city_area.source_order.back() ==
                  LakeStep::area_preflight_complete &&
              committed_grid.land_plot_count() == 9,
          "Fresh lake city pass must visit 37 slots then decrement detached land count");
    const auto lake_area = BnwSetupPlotGrid::advance_fresh_lake_incremental_area(
        lake_city_area, 0, no_river_area_draft.areas, lake_yield_rules,
        2'000'000);
    using LakeAreaStep = BnwFreshLakeAreaEventKind;
    constexpr std::int32_t new_lake_area_id = 16385;
    check(lake_area.grid.at(0).area_id == new_lake_area_id &&
              lake_area.areas.area_ids[0] == new_lake_area_id &&
              lake_area.areas.areas.size() == 2 &&
              lake_area.areas.areas.back().id == new_lake_area_id &&
              lake_area.areas.areas.back().water &&
              lake_area.areas.areas.back().tiles == 1 &&
              lake_area.areas.areas.back().west == 0 &&
              lake_area.areas.areas.back().east == 0 &&
              lake_area.grid.at(0).landmass_id == committed_grid.at(0).landmass_id &&
              lake_area.grid.at(0).yields[0] == 1 &&
              lake_area.area_order == std::vector<BnwFreshLakeAreaEvent>{
                  {LakeAreaStep::invalid_set_area_noop},
                  {LakeAreaStep::old_area_delete_skipped},
                  {LakeAreaStep::new_area_allocated, new_lake_area_id},
                  {LakeAreaStep::new_area_initialized, new_lake_area_id},
                  {LakeAreaStep::old_lake_read},
                  {LakeAreaStep::old_process_skipped},
                  {LakeAreaStep::area_handle_written, new_lake_area_id},
                  {LakeAreaStep::area_tiles_incremented, new_lake_area_id},
                  {LakeAreaStep::yield_checked, new_lake_area_id},
                  {LakeAreaStep::symbols_staged, new_lake_area_id}} &&
              lake_area.remaining_services.front() == "landmass_consistency" &&
              no_river_area_draft.grid.at(0).area_id == -1,
          "Fresh inland lake must allocate SDK free-list area and apply setArea/yield only in detached copy");
    const auto headless_lake = BnwSetupPlotGrid::inspect_fresh_lake_post_setter(
        lake_area, no_river_area_draft.grid, no_river_area_draft.areas,
        0, false);
    const auto unresolved_graphics = BnwSetupPlotGrid::inspect_fresh_lake_post_setter(
        lake_area, no_river_area_draft.grid, no_river_area_draft.areas,
        0, std::nullopt);
    const auto graphical_lake = BnwSetupPlotGrid::inspect_fresh_lake_post_setter(
        lake_area, no_river_area_draft.grid, no_river_area_draft.areas,
        0, true);
    using Post = BnwFreshLakePostSetterEventKind;
    check(headless_lake.grid.at(0).landmass_id ==
              no_river_area_draft.grid.at(0).landmass_id &&
              headless_lake.areas.landmass_ids ==
                  no_river_area_draft.areas.landmass_ids &&
              headless_lake.post_order ==
                  std::vector<Post>{Post::landmass_handle_preserved,
                                    Post::landmass_pool_preserved,
                                    Post::graphics_guard_false} &&
              headless_lake.remaining_services.front() ==
                  "map_calculate_areas_handoff" &&
              unresolved_graphics.remaining_services.front() ==
                  "graphics_state_unresolved" &&
              unresolved_graphics.post_order.back() ==
                  Post::graphics_state_unresolved &&
              graphical_lake.remaining_services.front() ==
                  "graphics_layout_dirty_callback" &&
              graphical_lake.post_order.back() ==
                  Post::graphics_layout_dirty_pending &&
              lake_area.source_order.back() == LakeStep::incremental_area_applied,
          "SDK lake setter must preserve old landmass and gate graphics callback explicitly");
    const auto deep_terrain = resolve_bnw_selected_terrain_by_id(*selected.rules, 2);
    const auto &mixed_grid = *full_terrain.map->owned_setup_grid;
    const auto mixed_area = mixed_grid.draft_fresh_area_unassignment(
        1, *full_terrain.map->area_structure);
    const auto mixed_prefix = mixed_area.grid.draft_fresh_lake_plot_type_prefix(
        1, mixed_area.areas, selected_grass);
    auto mixed_terrain = BnwSetupPlotGrid::advance_fresh_lake_terrain(
        mixed_prefix, 1, deep_terrain);
    const std::array additional_terrains{shallow_terrain.id, deep_terrain.id};
    const auto mixed_rules = load_bnw_fresh_yield_rules(
        *selected.rules, mixed_grid, additional_terrains);
    const auto mixed_neighbors = BnwSetupPlotGrid::advance_fresh_lake_neighbors(
        std::move(mixed_terrain), 1, mixed_area.areas, mixed_rules,
        shallow_terrain, deep_terrain, 2'000'000);
    const auto mixed_adjacent = mixed_neighbors.grid.adjacent(1);
    const auto water_direction = static_cast<std::uint8_t>(
        std::distance(mixed_adjacent.begin(),
            std::find(mixed_adjacent.begin(), mixed_adjacent.end(),
                      std::optional<std::size_t>{0})));
    using NeighborStep = BnwFreshLakeNeighborEventKind;
    check(mixed_neighbors.grid.at(0).terrain_type == deep_terrain.id &&
              mixed_neighbors.grid.at(0).yields[0] == 1 &&
              mixed_neighbors.grid.at(0).yields[2] == 0 &&
              mixed_neighbors.grid.symbol_events().size() ==
                  mixed_grid.symbol_events().size() + 1 &&
              mixed_neighbors.neighbor_order == std::vector<BnwFreshLakeNeighborEvent>{
                  {NeighborStep::terrain_written, 0, water_direction},
                  {NeighborStep::terrain_yield_checked, 0, water_direction},
                  {NeighborStep::terrain_symbols_staged, 0, water_direction},
                  {NeighborStep::terrain_impassability_updated, 0, water_direction},
                  {NeighborStep::coast_cache_written, 1, water_direction},
                  {NeighborStep::yield_checked, 0, water_direction}} &&
              mixed_grid.at(0).terrain_type == shallow_terrain.id &&
              mixed_grid.at(0).yields[0] == 2,
          "Fresh lake neighbor coast conversion must update selected yields and stage symbols in SDK order");
    const auto mixed_city_area =
        BnwSetupPlotGrid::advance_fresh_lake_city_area_preflight(
            mixed_neighbors, 1, mixed_area.areas, mixed_rules, 2'000'000);
    check(mixed_city_area.grid.land_plot_count() == 0 &&
              !mixed_city_area.grid.at(0).potential_city_work &&
              mixed_city_area.grid.at(0).yields == std::array<std::int16_t, 6>{} &&
              mixed_city_area.grid.symbol_events().size() ==
                  mixed_neighbors.grid.symbol_events().size() + 1 &&
              mixed_city_area.adjacent_water_area_id ==
                  mixed_grid.at(0).area_id &&
              !mixed_city_area.recalculate_areas &&
              mixed_city_area.remaining_services.front() ==
                  "incremental_area_attach_or_create" &&
              mixed_grid.land_plot_count() == 1,
          "Last-land lake conversion must clear city-work yields before area preflight");
    const auto mixed_area_applied =
        BnwSetupPlotGrid::advance_fresh_lake_incremental_area(
            mixed_city_area, 1, mixed_area.areas, mixed_rules, 2'000'000);
    check(mixed_area_applied.areas.areas.size() == mixed_area.areas.areas.size() &&
              mixed_area_applied.grid.at(1).area_id == mixed_grid.at(0).area_id &&
              mixed_area_applied.areas.area_ids[1] == mixed_grid.at(0).area_id &&
              find_bnw_fresh_area(mixed_area_applied.areas,
                  mixed_grid.at(0).area_id).tiles == 2 &&
              mixed_area_applied.grid.at(1).yields ==
                  std::array<std::int16_t, 6>{} &&
              mixed_area_applied.area_order[2] == BnwFreshLakeAreaEvent{
                  LakeAreaStep::neighboring_area_selected,
                  mixed_grid.at(0).area_id} &&
              mixed_city_area.grid.at(1).area_id == -1,
          "Adjacent fresh lake must attach selected water area without allocating another ID");
    const auto bridging_water = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 3, Height = 1, WrapX = false } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            Map.GetPlotByIndex(1):SetPlotType(PlotTypes.PLOT_LAND, false, false)
            Map.GetPlotByIndex(2):SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
        end
        function GenerateTerrain()
            Map.GetPlotByIndex(0):SetTerrainType(GameDefines.SHALLOW_WATER_TERRAIN,
                                                 false, false)
            Map.GetPlotByIndex(1):SetTerrainType(GameInfoTypes.TERRAIN_GRASS,
                                                 false, false)
            Map.GetPlotByIndex(2):SetTerrainType(GameDefines.DEEP_WATER_TERRAIN,
                                                 false, false)
        end
    )lua", "=fresh-lake-two-water-areas", setup);
    check(bridging_water.map && bridging_water.map->owned_setup_grid &&
              bridging_water.map->area_structure &&
              bridging_water.map->final_yields,
          "Two selected water areas must have a complete detached setup grid");
    const auto &bridge_grid = *bridging_water.map->owned_setup_grid;
    const auto bridge_area = bridge_grid.draft_fresh_area_unassignment(
        1, *bridging_water.map->area_structure);
    const auto bridge_prefix = bridge_area.grid.draft_fresh_lake_plot_type_prefix(
        1, bridge_area.areas, selected_grass);
    const auto bridge_terrain = BnwSetupPlotGrid::advance_fresh_lake_terrain(
        bridge_prefix, 1, deep_terrain);
    const auto bridge_rules = load_bnw_fresh_yield_rules(
        *selected.rules, bridge_grid);
    const auto bridge_neighbors = BnwSetupPlotGrid::advance_fresh_lake_neighbors(
        bridge_terrain, 1, bridge_area.areas, bridge_rules,
        shallow_terrain, deep_terrain, 2'000'000);
    const auto bridge_preflight =
        BnwSetupPlotGrid::advance_fresh_lake_city_area_preflight(
            bridge_neighbors, 1, bridge_area.areas, bridge_rules, 2'000'000);
    std::optional<std::int32_t> first_water_area;
    for (const auto neighbor : bridge_grid.adjacent(1)) {
        if (neighbor && bridge_grid.at(*neighbor).plot_type == 3) {
            first_water_area = bridge_grid.at(*neighbor).area_id;
            break;
        }
    }
    check(bridge_grid.at(0).area_id != bridge_grid.at(2).area_id &&
              bridge_preflight.recalculate_areas &&
              bridge_preflight.adjacent_water_area_id == first_water_area &&
              bridge_preflight.adjacent_area_transitions == 0 &&
              bridge_preflight.remaining_services.front() ==
                  "map_recalculate_areas" &&
              bridge_preflight.grid.land_plot_count() == 0 &&
              bridge_grid.land_plot_count() == 1,
          "Joining two distinct water areas must choose full SDK Map.RecalculateAreas");
    bool rejected_full_area_recalc = false;
    try {
        (void)BnwSetupPlotGrid::advance_fresh_lake_incremental_area(
            bridge_preflight, 1, bridge_area.areas, bridge_rules, 2'000'000);
    } catch (const std::invalid_argument &) {
        rejected_full_area_recalc = true;
    }
    check(rejected_full_area_recalc,
          "Full Map.RecalculateAreas must remain typed blocked by incremental area draft");
    const auto &isolated_grid = *full_terrain.map->owned_setup_grid;
    const auto isolated_area = isolated_grid.draft_fresh_area_unassignment(
        1, *full_terrain.map->area_structure);
    const auto isolated_prefix = isolated_area.grid.draft_fresh_lake_plot_type_prefix(
        1, isolated_area.areas, selected_grass);
    const auto deep_lake = BnwSetupPlotGrid::advance_fresh_lake_terrain(
        isolated_prefix, 1, deep_terrain);
    check(!deep_lake.shallow_water && deep_lake.grid.at(1).terrain_type == deep_terrain.id &&
              deep_lake.grid.at(1).area_id == -1 &&
              deep_lake.grid.land_plot_count() == isolated_grid.land_plot_count(),
          "Isolated fresh lake plot must select deep water from selected rules");
    auto committed_handoff_input = committed_river;
    const std::string committed_source_bytes(committed_river_source);
    committed_handoff_input.source = BnwMapScriptSource{
        "Assets/Maps/CommittedRiver.lua",
        identify_bytes(std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t *>(committed_source_bytes.data()),
            committed_source_bytes.size())).sha256,
        {civ5::assets::SourceKind::loose, "selected-test-mount",
         "Assets/Maps/CommittedRiver.lua", {}}};
    auto corrupted_river_events = committed_handoff_input;
    ++corrupted_river_events.map->river_symbol_events;
    const auto rejected_river_handoff = prepare_bnw_fresh_map_handoff(
        std::move(corrupted_river_events), selected);
    check(!rejected_river_handoff.map &&
              rejected_river_handoff.has_blocker(
                  BnwFreshMapHandoffBlockerCode::map_unverified),
          "Handoff must reject a river symbol count that differs from staged events");
    auto committed_handoff = prepare_bnw_fresh_map_handoff(
        std::move(committed_handoff_input), selected);
    check(committed_handoff.map &&
              committed_handoff.map->river_symbol_events == 2 &&
              committed_handoff.map->river_edge_writes == 1 &&
              committed_handoff.map->owned_setup_grid &&
              committed_handoff.map->owned_setup_grid->symbol_events().size() >= 2 &&
              committed_handoff.map->owned_setup_grid->symbol_events()[
                  committed_handoff.map->owned_setup_grid->symbol_events().size() - 2]
                  .plot_index == 4 &&
              committed_handoff.map->owned_setup_grid->symbol_events().back()
                  .plot_index == 2 &&
              !committed_handoff.has_blocker(
                  BnwFreshMapHandoffBlockerCode::map_unverified) &&
              !committed_handoff.has_blocker(
                  BnwFreshMapHandoffBlockerCode::river_generation_pending) &&
              committed_handoff.has_blocker(
                  BnwFreshMapHandoffBlockerCode::lake_generation_pending) &&
              committed_handoff.has_blocker(
                  BnwFreshMapHandoffBlockerCode::update_symbols_delivery_pending) &&
              !committed_handoff.ready_for_live_session(),
          "River symbols must survive handoff in order without claiming a live game");
    const auto &before_river = *all_land_river.map->owned_setup_grid;
    const auto crossing_draft = before_river.draft_fresh_river_crossings(
        4, BnwFreshRiverEdge::north_west, 1);
    using RiverEvent = BnwFreshRiverDraftEventKind;
    check(crossing_draft.grid.at(4).nw_of_river &&
              crossing_draft.grid.at(4).river_crossing_mask == 4 &&
              crossing_draft.grid.at(4).river_crossing_count == 1 &&
              crossing_draft.grid.at(2).river_crossing_mask == 32 &&
              crossing_draft.grid.at(2).river_crossing_count == 1 &&
              !before_river.at(4).nw_of_river &&
              before_river.at(4).river_crossing_mask == 0 &&
              before_river.at(2).river_crossing_mask == 0 &&
              crossing_draft.remaining_services == std::vector<std::string_view>{
                  "selected_terrain_river_yield_and_symbols",
                  "area_river_edge_count", "river_flow_direction_state",
                  "fresh_river_state_persistence"},
          "Typed river draft must update reciprocal crossings without publishing a partial write");
    const auto &events = crossing_draft.source_order;
    check(events.size() == 52 &&
              events[0] == BnwFreshRiverDraftEvent{RiverEvent::edge_bit_written, 4} &&
              events[3] == BnwFreshRiverDraftEvent{RiverEvent::crossing_checked, 4, 2, true} &&
              events[7] == BnwFreshRiverDraftEvent{RiverEvent::yield_pending, 4} &&
              events[8] == BnwFreshRiverDraftEvent{RiverEvent::crossing_checked, 8, 0, false} &&
              events[27] == BnwFreshRiverDraftEvent{RiverEvent::crossing_checked, 2, 5, true} &&
              events[50] == BnwFreshRiverDraftEvent{
                  RiverEvent::area_river_edge_count_pending, 4} &&
              events[51] == BnwFreshRiverDraftEvent{RiverEvent::flow_direction_pending, 4},
          "River crossing draft must retain SDK self/yield/neighbor/area/flow source order");
    const auto river_rules = load_bnw_fresh_yield_rules(*selected.rules, before_river);
    check(river_rules.source_rules == selected.rules.get(),
          "River yield rules must retain the activated selected database identity");
    const auto yielded_draft = replay_bnw_fresh_river_yields(
        crossing_draft, *all_land_river.map->area_structure, river_rules);
    const auto &river_symbols = yielded_draft.grid.symbol_events();
    check(yielded_draft.grid.at(4).yields[0] == 3 &&
              yielded_draft.grid.at(2).yields[0] == 3 &&
              yielded_draft.grid.at(8).yields[0] == 2 &&
              before_river.at(4).yields[0] == 2 &&
              crossing_draft.grid.at(4).yields[0] == 2 &&
              river_symbols.size() == before_river.symbol_events().size() + 2 &&
              river_symbols[river_symbols.size() - 2].phase ==
                  BnwFreshYieldSymbolPhase::river_write &&
              river_symbols[river_symbols.size() - 2].plot_index == 4 &&
              river_symbols.back().plot_index == 2 &&
              yielded_draft.remaining_services == std::vector<std::string_view>{
                  "area_river_edge_count", "river_flow_direction_state",
                  "fresh_river_state_persistence"},
          "Detached river yield replay must apply selected river Info and stage symbols in order");
    const auto &yield_events = yielded_draft.source_order;
    check(yield_events.size() == 54 &&
              yield_events[7].kind == RiverEvent::yield_recalculated &&
              yield_events[8].kind == RiverEvent::symbols_staged &&
              yield_events[29].kind == RiverEvent::yield_recalculated &&
              yield_events[29].plot_index == 2 &&
              yield_events[30].kind == RiverEvent::symbols_staged &&
              yield_events[30].plot_index == 2 &&
              yield_events[52].kind == RiverEvent::area_river_edge_count_pending &&
              yield_events[53].kind == RiverEvent::flow_direction_pending,
          "River updateYield and updateSymbols must keep SDK source order gaps");
    const auto area_draft = replay_bnw_fresh_river_area_count(
        yielded_draft, *all_land_river.map->area_structure, 4);
    const auto source_area_id = before_river.at(4).area_id;
    const auto count_for_source = [source_area_id](const BnwFreshAreaStructure &areas) {
        for (const auto &area : areas.areas)
            if (area.id == source_area_id) return area.river_edges;
        return -1;
    };
    check(count_for_source(area_draft.areas) == 1 &&
              count_for_source(*all_land_river.map->area_structure) == 0 &&
              area_draft.river.grid.at(4).nw_of_river &&
              !before_river.at(4).nw_of_river &&
              area_draft.river.source_order.size() == yield_events.size() &&
              area_draft.river.source_order[52] == BnwFreshRiverDraftEvent{
                  RiverEvent::area_river_edge_count_applied, 4} &&
              area_draft.river.source_order[53] == BnwFreshRiverDraftEvent{
                  RiverEvent::flow_direction_pending, 4} &&
              area_draft.river.remaining_services == std::vector<std::string_view>{
                  "river_flow_direction_state", "fresh_river_state_persistence"},
          "Area river-edge increment must be detached and follow all selected yield callbacks");
    const auto flow_draft = replay_bnw_fresh_river_flow_direction(area_draft);
    check(flow_draft.river.grid.at(4).river_se_flow_direction == 1 &&
              flow_draft.river.grid.at(4).river_e_flow_direction == -1 &&
              flow_draft.river.grid.at(4).river_sw_flow_direction == -1 &&
              area_draft.river.grid.at(4).river_se_flow_direction == -1 &&
              before_river.at(4).river_se_flow_direction == -1 &&
              count_for_source(flow_draft.areas) == 1 &&
              flow_draft.river.source_order.back() == BnwFreshRiverDraftEvent{
                  RiverEvent::flow_direction_applied, 4} &&
              flow_draft.river.remaining_services == std::vector<std::string_view>{
                  "fresh_river_state_atomic_handoff"},
          "SDK SE flow char must be written last without publishing the river draft");
    std::uint64_t river_water_visits = 100;
    check(flow_draft.river.grid.find_water_during_rivers(
              4, 0, true, flow_draft.areas, river_water_visits) &&
              flow_draft.river.grid.find_water_during_rivers(
                  2, 0, true, flow_draft.areas, river_water_visits) &&
              !flow_draft.river.grid.find_water_during_rivers(
                  0, 0, true, flow_draft.areas, river_water_visits) &&
              !flow_draft.river.grid.find_water_during_rivers(
                  4, 0, false, flow_draft.areas, river_water_visits) &&
              !before_river.find_water_during_rivers(
                  4, 0, true, *all_land_river.map->area_structure,
                  river_water_visits) &&
              find_bnw_fresh_area(flow_draft.areas, source_area_id).tiles == 9 &&
              find_bnw_fresh_area(flow_draft.areas, source_area_id).river_edges == 1 &&
              find_bnw_fresh_area(*all_land_river.map->area_structure,
                                  source_area_id).river_edges == 0,
          "Post-river fresh-water and area queries must read only detached river state");
    ScriptMapProvider detached_map;
    detached_map.grid_size = [] {
        return std::pair<std::int64_t, std::int64_t>{3, 3};
    };
    detached_map.setup_plot_by_index = [](std::int64_t index)
        -> std::optional<std::int64_t> {
        if (index < 0 || index >= 9) return std::nullopt;
        return index;
    };
    detached_map.setup_plot_value = [&](std::int64_t index,
                                        ScriptMapSetupPlotQuery query) {
        const auto &plot = flow_draft.river.grid.at(
            static_cast<std::size_t>(index));
        switch (query) {
        case ScriptMapSetupPlotQuery::x: return static_cast<std::int64_t>(plot.x);
        case ScriptMapSetupPlotQuery::y: return static_cast<std::int64_t>(plot.y);
        case ScriptMapSetupPlotQuery::index: return index;
        case ScriptMapSetupPlotQuery::plot_type:
            return static_cast<std::int64_t>(plot.plot_type);
        case ScriptMapSetupPlotQuery::terrain_type:
            return static_cast<std::int64_t>(plot.terrain_type);
        case ScriptMapSetupPlotQuery::is_river:
            return static_cast<std::int64_t>(plot.river_crossing_count > 0);
        case ScriptMapSetupPlotQuery::feature_type:
            return static_cast<std::int64_t>(-1);
        case ScriptMapSetupPlotQuery::is_lake: {
            const auto &pool=flow_draft.areas.landmasses;
            const auto landmass=std::find_if(pool.begin(),pool.end(),[&](const auto &entry) {
                return entry.id==plot.landmass_id;
            });
            check(landmass!=pool.end() &&
                  flow_draft.areas.landmass_ids.at(static_cast<std::size_t>(index))==plot.landmass_id &&
                  std::count_if(pool.begin(),pool.end(),[&](const auto &entry) {
                      return entry.id==plot.landmass_id;
                  })==1,"Detached IsLake requires its retained unique native landmass");
            return static_cast<std::int64_t>(landmass->lake);
        }
        }
        throw std::invalid_argument("Unknown detached BNW plot query");
    };
    detached_map.setup_plot_area_id = [&](std::int64_t index) {
        return static_cast<std::int64_t>(flow_draft.river.grid.at(
            static_cast<std::size_t>(index)).area_id);
    };
    detached_map.setup_area_value = [&](std::int64_t id,
                                        ScriptMapSetupAreaQuery query) {
        const auto &area = find_bnw_fresh_area(flow_draft.areas,
                                                static_cast<std::int32_t>(id));
        switch (query) {
        case ScriptMapSetupAreaQuery::num_tiles:
            return static_cast<std::int64_t>(area.tiles);
        case ScriptMapSetupAreaQuery::num_river_edges:
            return static_cast<std::int64_t>(area.river_edges);
        }
        throw std::invalid_argument("Unknown detached BNW area query");
    };
    detached_map.setup_find_water = [&](std::int64_t index,
                                        std::int64_t radius, bool fresh) {
        return flow_draft.river.grid.find_water_during_rivers(
            static_cast<std::size_t>(index), static_cast<std::int32_t>(radius),
            fresh, flow_draft.areas, river_water_visits);
    };
    LuaScriptHost detached_host({}, {}, {}, {}, {}, {}, std::move(detached_map));
    detached_host.execute("map", R"lua(
        local center=Map.GetPlotByIndex(4)
        local near=Map.GetPlotByIndex(2)
        assert(center:Area()==near:Area())
        assert(center:Area():GetNumTiles()==9)
        assert(center:Area():GetNumRiverEdges()==1)
        assert(center:GetFeatureType()==FeatureTypes.NO_FEATURE)
        assert(not center:IsLake())
        assert(Map.FindWater(center,0,true))
        assert(Map.FindWater(near,0,true))
        assert(not Map.FindWater(Map.GetPlotByIndex(0),0,true))
    )lua", "=detached-post-river-readers");
    const auto check_other_flow = [&](BnwFreshRiverEdge edge,
                                      std::int32_t direction) {
        const auto crossings = before_river.draft_fresh_river_crossings(
            4, edge, direction);
        const auto yields = replay_bnw_fresh_river_yields(
            crossings, *all_land_river.map->area_structure, river_rules);
        const auto area = replay_bnw_fresh_river_area_count(
            yields, *all_land_river.map->area_structure, 4);
        return replay_bnw_fresh_river_flow_direction(area);
    };
    const auto east_flow = check_other_flow(BnwFreshRiverEdge::west, 0);
    const auto southwest_flow = check_other_flow(BnwFreshRiverEdge::north_east, 5);
    check(east_flow.river.grid.at(4).river_e_flow_direction == 0 &&
              east_flow.river.grid.at(4).river_se_flow_direction == -1 &&
              southwest_flow.river.grid.at(4).river_sw_flow_direction == 5 &&
              southwest_flow.river.grid.at(4).river_e_flow_direction == -1,
          "Each BNW edge must persist its matching E, SE or SW flow char");
    auto invalid_flow_draft = area_draft;
    invalid_flow_draft.river.requested_flow_direction = 0;
    bool invalid_flow_rejected = false;
    try {
        (void)replay_bnw_fresh_river_flow_direction(invalid_flow_draft);
    } catch (const std::invalid_argument &) { invalid_flow_rejected = true; }
    check(invalid_flow_rejected && area_draft.river.grid.at(4).river_se_flow_direction == -1,
          "A mismatched river flow must reject the detached draft atomically");
    auto wrong_area_identity = *all_land_river.map->area_structure;
    wrong_area_identity.area_ids[4] = -1;
    bool wrong_area_rejected = false;
    try {
        (void)replay_bnw_fresh_river_area_count(yielded_draft,
                                                wrong_area_identity, 4);
    } catch (const std::invalid_argument &) { wrong_area_rejected = true; }
    check(wrong_area_rejected && count_for_source(*all_land_river.map->area_structure) == 0,
          "River area replay must reject mismatched selected area handles atomically");
    auto overflowing_areas = *all_land_river.map->area_structure;
    for (auto &area : overflowing_areas.areas)
        if (area.id == source_area_id)
            area.river_edges = std::numeric_limits<std::int32_t>::max();
    bool overflow_rejected = false;
    try {
        (void)replay_bnw_fresh_river_area_count(yielded_draft,
                                                overflowing_areas, 4);
    } catch (const std::invalid_argument &) { overflow_rejected = true; }
    check(overflow_rejected && count_for_source(*all_land_river.map->area_structure) == 0,
          "River area counter overflow must not publish a partial write");
    bool river_budget_rejected = false;
    try {
        (void)replay_bnw_fresh_river_yields(crossing_draft,
            *all_land_river.map->area_structure, river_rules,
            before_river.symbol_events().size() + 1);
    } catch (const std::length_error &) { river_budget_rejected = true; }
    check(river_budget_rejected &&
              crossing_draft.grid.symbol_events().size() ==
                  before_river.symbol_events().size() &&
              crossing_draft.grid.at(4).yields[0] == 2,
          "River yield budget failure must leave the incoming draft untouched");
    const auto rejected_river_setter = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes()
            local water = Map.GetPlotByIndex(0)
            water:SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            water:SetTerrainType(GameDefines.SHALLOW_WATER_TERRAIN, false, false)
            Map.GetPlotByIndex(1):SetPlotType(PlotTypes.PLOT_LAND, false, false)
        end
        function GenerateTerrain()
            Map.GetPlotByIndex(1):SetTerrainType(GameInfoTypes.TERRAIN_GRASS, false, false)
        end
        function AddRivers()
            Map.Rand(5, 'speculative river source')
            local plot = Map.GetPlotByIndex(1)
            plot:SetPlotType(PlotTypes.PLOT_HILLS, false, false)
            plot:GetInlandCorner()
        end
    )lua", "=rejected-post-area-river-setter", setup,
        SimulationRandom(RandomAlgorithm::bnw, {345, 0, 0}));
    check(rejected_river_setter.map && rejected_river_setter.map->owned_setup_grid &&
              rejected_river_setter.map->owned_setup_grid->at(1).plot_type == 2 &&
              rejected_river_setter.map->owned_setup_grid->at(1).terrain_type == 0 &&
              rejected_river_setter.map->owned_setup_grid->at(1).area_id == 16385 &&
              rejected_river_setter.map->owned_setup_grid->symbol_events().size() == 3 &&
              rejected_river_setter.map_random_before == rejected_river_setter.map_random_after &&
              std::any_of(rejected_river_setter.blockers.begin(),
                          rejected_river_setter.blockers.end(), [](const auto &blocker) {
                  return blocker.code == BnwMapScriptBlockerCode::river_generation_pending &&
                         blocker.message.find("Plot.SetPlotType") != std::string::npos;
              }),
          "A speculative AddRivers setter must fail closed and restore pre-river grid and RNG");
    const auto original_events = full_terrain.map->owned_setup_grid->symbol_events();
    const auto original_plot_address = &full_terrain.map->owned_setup_grid->at(0);
    const std::string source_bytes(full_terrain_source);
    full_terrain.source = BnwMapScriptSource{
        "Assets/Maps/Test.lua",
        identify_bytes(std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t *>(source_bytes.data()), source_bytes.size())).sha256,
        {civ5::assets::SourceKind::loose, "selected-test-mount", "Assets/Maps/Test.lua", {}}};
    auto inconsistent = full_terrain;
    ++inconsistent.map->yield_progress->final_pass_changed_plots;
    const auto rejected = prepare_bnw_fresh_map_handoff(std::move(inconsistent), selected);
    check(!rejected.map && inconsistent.map &&
              rejected.has_blocker(BnwFreshMapHandoffBlockerCode::map_unverified),
          "Inconsistent updateSymbols replay must not transfer the owned grid");
    auto unproven = full_terrain;
    unproven.source.reset();
    const auto no_source = prepare_bnw_fresh_map_handoff(std::move(unproven), selected);
    check(!no_source.map && unproven.map &&
              no_source.has_blocker(BnwFreshMapHandoffBlockerCode::provenance_unverified),
          "Fresh handoff must reject missing selected script provenance");
    auto handoff = prepare_bnw_fresh_map_handoff(std::move(full_terrain), selected);
    check(handoff.map && !full_terrain.map && handoff.map_random_before &&
              handoff.map_random_before->seed == 123 && handoff.map_random_after &&
              handoff.world_rules_source && handoff.selected_rules_sha256.size() == 64 &&
              &handoff.map->owned_setup_grid->at(0) == original_plot_address &&
              handoff.map->owned_setup_grid->symbol_events().size() == original_events.size() &&
              handoff.has_blocker(BnwFreshMapHandoffBlockerCode::update_symbols_delivery_pending) &&
              handoff.has_blocker(BnwFreshMapHandoffBlockerCode::river_generation_pending) &&
              handoff.has_blocker(BnwFreshMapHandoffBlockerCode::game_state_players_teams_pending) &&
              !handoff.ready_for_live_session(),
          "Fresh map handoff must move owned grid and preserve selected provenance without readiness");
    for (std::size_t i = 0; i < original_events.size(); ++i) {
        const auto &actual = handoff.map->owned_setup_grid->symbol_events()[i];
        const auto &expected = original_events[i];
        check(actual.phase == expected.phase && actual.plot_index == expected.plot_index &&
                  actual.trigger_landmass_id == expected.trigger_landmass_id &&
                  actual.yields_after == expected.yields_after,
              "Handoff must neither reorder nor duplicate staged updateSymbols events");
    }
    BnwMapScriptBudget replay_budget;
    replay_budget.maximum_yield_replay_plot_visits = 1;
    const auto limited_replay = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetPlotType(PlotTypes.PLOT_LAND, false, false)
        end
        function GenerateTerrain()
            Map.GetPlotByIndex(0):SetTerrainType(0, false, false)
        end
    )lua", "=limited-yield-replay", setup, std::nullopt, replay_budget);
    check(limited_replay.map && limited_replay.map->area_structure &&
              !limited_replay.map->final_yields &&
              !limited_replay.map->owned_setup_grid &&
              limited_replay.has_blocker(
                  BnwMapScriptBlockerCode::yield_replay_budget_exceeded) &&
              limited_replay.has_blocker(BnwMapScriptBlockerCode::areas_pending),
          "Yield replay budget must fail closed without promoting area completion");
    const auto partial_terrain = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes()
            local water, land = Map.GetPlotByIndex(0), Map.GetPlotByIndex(1)
            water:SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            water:SetTerrainType(GameDefines.SHALLOW_WATER_TERRAIN, false, false)
            land:SetPlotType(PlotTypes.PLOT_LAND, false, false)
        end
        function GenerateTerrain() end
    )lua", "=selected-partial-terrain", setup);
    check(partial_terrain.map && !partial_terrain.map->terrain_types &&
              partial_terrain.has_blocker(BnwMapScriptBlockerCode::terrain_pending) &&
              partial_terrain.has_blocker(BnwMapScriptBlockerCode::invalid_terrain_types),
          "A land plot's initial terrain must not pass for GenerateTerrain application");
    const auto unapplied_categories = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes() return { PlotTypes.PLOT_LAND } end
        function GenerateTerrain() error('unapplied categories were used') end
    )lua", "=unapplied-plot-categories", setup);
    check(unapplied_categories.map && !unapplied_categories.map->terrain_types &&
              unapplied_categories.has_blocker(BnwMapScriptBlockerCode::invalid_terrain_types),
          "Returned categories alone must not create owned plot state for terrain generation");
    const auto missing_info_type = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            local plot = Map.GetPlotByIndex(0)
            plot:SetPlotType(PlotTypes.PLOT_OCEAN, false, false)
            plot:SetTerrainType(GameDefines.SHALLOW_WATER_TERRAIN, false, false)
        end
        function GenerateTerrain()
            return GameInfoTypes.TERRAIN_NOT_SELECTED
        end
    )lua", "=missing-selected-info-type", setup);
    check(missing_info_type.map && !missing_info_type.map->terrain_types &&
              missing_info_type.has_blocker(BnwMapScriptBlockerCode::invalid_world_rules),
          "Missing selected Terrain Info type must stop terrain completion");
    const auto missing_terrain = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetTerrainType(99, false, false)
            return { PlotTypes.PLOT_OCEAN }
        end
    )lua", "=missing-selected-terrain", setup);
    check(!missing_terrain.map && missing_terrain.has_blocker(
              BnwMapScriptBlockerCode::invalid_world_rules),
          "Missing selected terrain Info must stop map setup with a rules blocker");
    const auto water_mismatch = generate_bnw_plot_map(R"lua(
        function GetMapInitData() return { Width = 1, Height = 1 } end
        function GeneratePlotTypes()
            Map.GetPlotByIndex(0):SetTerrainType(0, false, false)
            return { PlotTypes.PLOT_OCEAN }
        end
    )lua", "=recursive-terrain-transition", setup);
    check(!water_mismatch.map && water_mismatch.has_blocker(
              BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Terrain waterness mismatch must reject unported recursive setPlotType");
    setup.runtime_defines.push_back({"CIVIS_TEST_RUNTIME_DEFINE", 23});
    const auto selected_defines = generate_bnw_plot_map(R"lua(
        function GetMapInitData(worldSize)
            assert(GameDefines.LAKE_PLOT_RAND == 7)
            assert(GameDefines.SHALLOW_WATER_TERRAIN == 1)
            assert(GameDefines.DEEP_WATER_TERRAIN == 2)
            assert(GameDefines.MISSING_CIVIS_TEST == nil)
            assert(GameDefines.CIVIS_TEST_RUNTIME_DEFINE == 23)
            return { Width = 1, Height = 1 }
        end
        function GeneratePlotTypes() return { PlotTypes.PLOT_OCEAN } end
    )lua", "=selected-defines", setup);
    check(selected_defines.map && selected_defines.world_rules_source &&
              selected_defines.world_rules_source->defines_sha256.size() == 64,
          "Map GameDefines must read selected Defines after BNW PostDefines with provenance");
    std::uint64_t source_shallow_rows = 0;
    selected.rules->query(
        {"SELECT Value FROM Defines WHERE Name = 'SHALLOW_WATER_TERRAIN'", {}},
        [&](std::span<const civ5::content::DatabaseValue> row) {
            ++source_shallow_rows;
            check(row.size() == 1 && std::get<std::int64_t>(row[0]) == 99,
                  "Map PostDefines must leave authoritative selected rules unchanged");
        });
    check(source_shallow_rows == 1,
          "Selected rules must retain their original shallow-water define");
    selected.rules->apply_sql(
        "UPDATE Defines SET Value = 8 WHERE Name = 'LAKE_PLOT_RAND'");
    const auto changed_defines = generate_bnw_plot_map(R"lua(
        function GetMapInitData()
            assert(GameDefines.LAKE_PLOT_RAND == 8)
            return { Width = 1, Height = 1 }
        end
        function GeneratePlotTypes() return { PlotTypes.PLOT_OCEAN } end
    )lua", "=changed-selected-defines", setup);
    check(changed_defines.map && changed_defines.world_rules_source &&
              changed_defines.world_rules_source->defines_sha256 !=
                  selected_defines.world_rules_source->defines_sha256,
          "Map define identity must change when the selected rules change");

    const auto unsupported_field = generate_bnw_plot_map(R"lua(
        function GetMapInitData(worldSize)
            return { Width = GameInfo.Worlds.WORLDSIZE_DUEL.GridWidth, Height = 1 }
        end
        function GeneratePlotTypes() return { 2 } end
    )lua", "=world-field-not-exposed", setup);
    check(!unsupported_field.map && unsupported_field.has_blocker(
              BnwMapScriptBlockerCode::unsupported_stock_helper),
          "Fields outside the bounded Worlds ID/Type slice must remain explicit blockers");

    setup.world_size = 5;
    check(generate_bnw_plot_map("return true", "=absent-world", setup).has_blocker(
              BnwMapScriptBlockerCode::invalid_world_rules),
          "Chosen world size absent from selected Worlds must stop setup");
    setup.world_size = 3;
    selected.profile.requirements.clear();
    check(generate_bnw_plot_map("return true", "=unselected-bnw", setup).has_blocker(
              BnwMapScriptBlockerCode::invalid_world_rules),
          "Worlds cannot be exposed without selected Expansion2 provenance");
    selected.profile.requirements.push_back(expansion2);
    selected.profile.requirements.front().id = "invalid-guid";
    check(generate_bnw_plot_map("return true", "=invalid-package-id", setup).has_blocker(
              BnwMapScriptBlockerCode::invalid_world_rules),
          "Malformed selected package identity must report a blocker instead of throwing");
}

void write(const std::filesystem::path &path, const char *source) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << source;
    if (!output) throw std::runtime_error("Cannot write map-script fixture");
}

void test_selected_resource_source() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() /
        ("civis-bnw-map-source-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto script = R"lua(
        function GetMapInitData() return { Width = 2, Height = 1 } end
        function GeneratePlotTypes() return { 2, 3 } end
    )lua";
    try {
        write(root / "Assets/Maps/Base.lua", script);
        write(root / "Assets/DLC/Expansion2/Maps/Shared.lua", script);
        write(root / "Assets/DLC/Other/Maps/Shared.lua", R"lua(
            function GetMapInitData() return { Width = 1, Height = 1 } end
            function GeneratePlotTypes() return { 0 } end
        )lua");
        write(root / "Assets/DLC/Unselected/Maps/Hidden.lua", script);
        write(root / "Assets/DLC/Expansion2/Maps/NeedsFractal.lua", R"lua(
            function GetMapInitData() return { Width = 1, Height = 1 } end
            function GeneratePlotTypes() return Fractal.Create(1, 1) end
        )lua");

        civ5::content::Catalog catalog;
        for (const auto &name : {"Expansion2", "Other", "Unselected"}) {
            civ5::content::PackageManifest package;
            package.kind = civ5::content::PackageKind::dlc;
            package.path = root / "Assets/DLC" / name / (std::string(name) + ".Civ5Pkg");
            catalog.packages.push_back(std::move(package));
        }
        civ5::runtime::SavedContentProfile selected;
        selected.enabled_dlc = {0};
        auto resources = build_bnw_map_script_resources(root, selected, catalog);
        const auto base = generate_bnw_plot_map(resources, {"Assets/Maps/Base.lua"});
        check(base.map && base.source && base.source->provenance.source_relative_path ==
                  "Assets/Maps/Base.lua", "Base map source provenance changed");
        const auto dlc = generate_bnw_plot_map(resources, {"Maps/Shared.lua"});
        check(dlc.map && dlc.source && dlc.map->plot_types == std::vector<std::uint8_t>({2, 3}),
              "Selected DLC map script did not execute exact source bytes");
        check(generate_bnw_plot_map(resources, {"Maps/Hidden.lua"}).has_blocker(
                  BnwMapScriptBlockerCode::source_missing),
              "Unselected DLC map source must remain unavailable");
        check(generate_bnw_plot_map(resources, {"Other/Shared.lua"}).has_blocker(
                  BnwMapScriptBlockerCode::source_missing),
              "A basename collision must not satisfy another relative path");
        check(generate_bnw_plot_map(resources, {"../Maps/Shared.lua"}).has_blocker(
                  BnwMapScriptBlockerCode::invalid_source_key),
              "Parent traversal must not select a map script");
        const auto wrong_hash = generate_bnw_plot_map(
            resources, {"Maps/Shared.lua", {}, std::string(64, '0')});
        check(!wrong_hash.map && wrong_hash.has_blocker(BnwMapScriptBlockerCode::source_hash_mismatch),
              "A pinned hash mismatch must stop execution");
        const auto unsupported = generate_bnw_plot_map(resources, {"Maps/NeedsFractal.lua"});
        check(!unsupported.map && unsupported.source &&
                  unsupported.has_blocker(BnwMapScriptBlockerCode::unsupported_stock_helper),
              "Selected stock-helper failure must retain provenance and a typed blocker");

        selected.enabled_dlc = {0, 1};
        resources = build_bnw_map_script_resources(root, selected, catalog);
        const auto ambiguous = generate_bnw_plot_map(resources, {"Maps/Shared.lua"});
        check(!ambiguous.map && ambiguous.has_blocker(BnwMapScriptBlockerCode::source_ambiguous),
              "Two selected sources with the same logical path need a pin");
        const auto pinned = generate_bnw_plot_map(
            resources, {"Maps/Shared.lua", {}, dlc.source->sha256});
        check(pinned.map && pinned.source && pinned.source->sha256 == dlc.source->sha256,
              "Pinned source hash must choose the matching selected bytes");
        const auto pinned_root = generate_bnw_plot_map(
            resources, {"Maps/Shared.lua", dlc.source->provenance.source_id, {}});
        check(pinned_root.map && pinned_root.source &&
                  pinned_root.source->provenance.source_id == dlc.source->provenance.source_id,
              "Pinned resource root must disambiguate selected DLC scripts");
    } catch (...) {
        fs::remove_all(root);
        throw;
    }
    fs::remove_all(root);
}

void test_selected_map_includes() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() /
        ("civis-bnw-map-include-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        write(root / "Assets/Maps/Stock.lua", R"lua(
            include('First')
            include('First.lua')
            assert(sequence == 'NFNF' and selected_plot == 3)
            function GetMapInitData() return { Width = 2, Height = 1 } end
            function GeneratePlotTypes() return { selected_plot, 2 } end
        )lua");
        write(root / "Assets/Gameplay/Lua/First.lua", "sequence = 'BASE'; selected_plot = 0");
        write(root / "Assets/Gameplay/Lua/Nested.lua", "sequence = (sequence or '') .. 'N'");
        write(root / "Assets/DLC/Expansion2/Gameplay/Lua/First.lua", R"lua(
            include('Nested')
            sequence = sequence .. 'F'
            selected_plot = 3
        )lua");
        write(root / "Assets/DLC/Unselected/Gameplay/Lua/Hidden.lua", "selected_plot = 1");
        write(root / "Assets/Maps/Missing.lua", R"lua(
            include('Hidden')
            function GetMapInitData() return { Width = 1, Height = 1 } end
            function GeneratePlotTypes() return { 2 } end
        )lua");
        write(root / "Assets/Maps/Traversal.lua", "include('../Hidden')");
        write(root / "Assets/Maps/OversizedHelper.lua", "include('Large')");
        write(root / "Assets/Gameplay/Lua/Large.lua", std::string(1024, ' ').c_str());
        write(root / "Assets/Maps/Cycle.lua", "include('CycleA')");
        write(root / "Assets/Gameplay/Lua/CycleA.lua", "include('CycleB')");
        write(root / "Assets/Gameplay/Lua/CycleB.lua", "include('CycleA')");

        civ5::content::Catalog catalog;
        for (const auto &name : {"Expansion2", "Unselected"}) {
            civ5::content::PackageManifest package;
            package.kind = civ5::content::PackageKind::dlc;
            package.path = root / "Assets/DLC" / name / (std::string(name) + ".Civ5Pkg");
            catalog.packages.push_back(std::move(package));
        }
        civ5::runtime::SavedContentProfile selected;
        selected.enabled_dlc = {0};
        auto resources = build_bnw_map_script_resources(root, selected, catalog);
        const auto stock = generate_bnw_plot_map(resources, {"Assets/Maps/Stock.lua"});
        check(stock.map && stock.map->plot_types == std::vector<std::uint8_t>({3, 2}) &&
                  stock.included_sources.size() == 4 &&
                  stock.included_sources[0].name == "first.lua" &&
                  stock.included_sources[1].name == "nested.lua" &&
                  stock.included_sources[2].name == "first.lua" &&
                  stock.included_sources[3].name == "nested.lua" &&
                  stock.included_sources[0].provenance.source_relative_path ==
                      "Gameplay/Lua/First.lua",
              "Selected DLC include must override base and preserve repeated nested execution order");
        check(generate_bnw_plot_map(resources, {"Assets/Maps/Missing.lua"}).has_blocker(
                  BnwMapScriptBlockerCode::include_missing),
              "Unselected DLC helper must not enter the active include VFS");
        check(generate_bnw_plot_map(resources, {"Assets/Maps/Traversal.lua"}).has_blocker(
                  BnwMapScriptBlockerCode::include_invalid),
              "Map include must reject paths outside selected VFS");
        BnwMapScriptBudget small_source_budget;
        small_source_budget.maximum_source_bytes = 256;
        check(generate_bnw_plot_map(resources, {"Assets/Maps/OversizedHelper.lua"},
                                    small_source_budget).has_blocker(
                  BnwMapScriptBlockerCode::include_io_failed),
              "Oversized selected include must fail at its bounded source read");
        const auto cycle = generate_bnw_plot_map(resources, {"Assets/Maps/Cycle.lua"});
        check(!cycle.map && cycle.has_blocker(BnwMapScriptBlockerCode::include_cycle) &&
                  cycle.included_sources.size() == 2,
              "Nested include cycle must stop after reading each helper once");

        write(root / "Assets/Gameplay/Lua/Sub/Nested.lua", "sequence = 'WRONG'");
        resources = build_bnw_map_script_resources(root, selected, catalog);
        check(generate_bnw_plot_map(resources, {"Assets/Maps/Stock.lua"}).has_blocker(
                  BnwMapScriptBlockerCode::include_ambiguous),
              "Same-mount helper basename collision must not choose a traversal order");
    } catch (...) {
        fs::remove_all(root);
        throw;
    }
    fs::remove_all(root);
}

void test_other_lua_include_contract() {
    ScriptSourceProvider sources;
    sources.files = [](std::string_view) { return std::vector<std::string>{"Counter.lua"}; };
    sources.read = [](std::string_view, std::string_view) {
        return std::string("counter = (counter or 0) + 1");
    };
    LuaScriptHost host({}, std::move(sources));
    host.execute("native", R"lua(
        include('Counter')
        include('Counter')
        GameEvents.CivisNativeInclude.Add(function() return counter end)
    )lua", "=native-include-regression");
    const auto value = host.accumulate_int("CivisNativeInclude");
    check(value.value == 2 && value.errors.empty(),
          "Ordinary Lua hosts must retain native repeated pattern include behavior");
}

void test_stock_inland_sea(const std::filesystem::path &game_root) {
    civ5::content::Catalog catalog;
    catalog.packages.push_back(civ5::content::read_manifest(
        game_root / "Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(civ5::content::read_manifest(
        game_root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t, 2> selected_dlc{0, 1};
    auto selected = bootstrap_bnw_fresh_game_content(game_root, catalog, selected_dlc);
    check(selected.content_ready() && selected.profile.kind == ContentProfileKind::dlc,
          "Stock InlandSea requires verified selected BNW base and Expansion DLC rules");

    auto resources = build_bnw_map_script_resources(game_root, selected.profile, catalog);
    BnwMapSetupContext setup;
    setup.world_size = 3;
    setup.custom_options = std::map<std::string, std::int64_t>{{"1", 2}, {"2", 2}, {"3", 2}};
    setup.climate = 0;
    setup.selected_rules = &selected;
    setup.graphics_initialized = false;
    auto generated = generate_bnw_plot_map(
        resources, {"Assets/Maps/InlandSea.lua"}, setup,
        SimulationRandom(RandomAlgorithm::bnw, {123, 0, 0}));
    check(generated.source &&
              generated.source->relative_path == "Assets/Maps/InlandSea.lua" &&
              generated.world_rules_source &&
              generated.world_rules_source->profile_kind == ContentProfileKind::dlc &&
              generated.included_sources.size() == 9,
          "Stock InlandSea must execute the selected source and active helper chain");
    check(generated.map && generated.map->width == 52 && generated.map->height == 32 &&
              generated.map->owned_setup_grid &&
              generated.map->owned_setup_grid->size() == 1664 &&
              generated.map->owned_setup_grid->symbol_events().size() == 1338 &&
              generated.map->river_edge_writes == 430 &&
              generated.map->river_symbol_events == 0 &&
              generated.map->lake_plot_conversions == 3 &&
              generated.map->lake_symbol_events == 3 &&
              generated.map->terrain_types && generated.map->area_structure &&
              generated.map->area_structure->areas.size() == 7 &&
              generated.map->area_structure->areas[4].id == 40964 &&
              generated.map->area_structure->areas[5].id == 49157 &&
              generated.map->area_structure->areas[6].id == 57350 &&
              generated.map->final_yields &&
              generated.map_random_before == SimulationRandomState{123, 0, 0} &&
              generated.map_random_after && generated.map_random_after->calls == 300131 &&
              !generated.has_blocker(BnwMapScriptBlockerCode::river_generation_pending) &&
              !generated.has_blocker(BnwMapScriptBlockerCode::lake_generation_pending) &&
              generated.has_blocker(BnwMapScriptBlockerCode::features_rivers_pending) &&
              std::any_of(generated.blockers.begin(), generated.blockers.end(),
                          [](const auto &blocker) {
                              return blocker.code == BnwMapScriptBlockerCode::features_rivers_pending &&
                                     blocker.message.find("CIVIS_BNW_FEATURE_WRITE_PENDING:Plot.SetFeatureType:GameplayFeatureChanged") !=
                                         std::string::npos;
                          }),
          "Stock InlandSea must retain the completed lake map at the next feature blocker");
    std::uint64_t stock_plot_edges = 0, stock_area_edges = 0;
    for (std::size_t i = 0; i < generated.map->owned_setup_grid->size(); ++i) {
        const auto &plot = generated.map->owned_setup_grid->at(i);
        stock_plot_edges += static_cast<std::uint64_t>(plot.ne_of_river) +
                            static_cast<std::uint64_t>(plot.w_of_river) +
                            static_cast<std::uint64_t>(plot.nw_of_river);
        check(plot.yields == (*generated.map->final_yields)[i],
              "Stock river yields must be committed with their owned plot caches");
    }
    for (const auto &area : generated.map->area_structure->areas)
        stock_area_edges += static_cast<std::uint64_t>(area.river_edges);
    check(stock_plot_edges == 430 && stock_area_edges == 430,
          "Stock river edges must match the selected plot bits and area counters");

    const auto largest_water = find_bnw_fresh_biggest_area(
        *generated.map->owned_setup_grid, *generated.map->area_structure, true);
    const auto largest_land = find_bnw_fresh_biggest_area(
        *generated.map->owned_setup_grid, *generated.map->area_structure, false);
    check(largest_water && largest_land &&
              find_bnw_fresh_area(*generated.map->area_structure,
                                  *largest_water).water &&
              !find_bnw_fresh_area(*generated.map->area_structure,
                                   *largest_land).water,
          "Stock biggest-area query must select existing water and land FFreeList handles");
    auto invalid_biggest_areas = *generated.map->area_structure;
    ++invalid_biggest_areas.areas.front().tiles;
    bool invalid_biggest_rejected = false;
    try {
        (void)find_bnw_fresh_biggest_area(
            *generated.map->owned_setup_grid, invalid_biggest_areas, true);
    } catch (const std::invalid_argument &) { invalid_biggest_rejected = true; }
    check(invalid_biggest_rejected,
          "Biggest-area query must reject incomplete owned area counts");

    const auto &fresh_grid = *generated.map->owned_setup_grid;
    const auto mountain = std::find_if(generated.map->plot_types.begin(),
        generated.map->plot_types.end(), [](std::uint8_t value) { return value == 0; });
    check(mountain != generated.map->plot_types.end(),
          "Stock InlandSea must provide a fresh mountain transition target");
    const auto mountain_index = static_cast<std::size_t>(
        mountain - generated.map->plot_types.begin());
    const auto yield_rules = load_bnw_fresh_yield_rules(*selected.rules, fresh_grid);
    const auto hill_draft = fresh_grid.draft_fresh_feature_mountain_to_hill(
        mountain_index, *generated.map->area_structure, yield_rules, false, 2'000'000);
    const auto &old_mountain = fresh_grid.at(mountain_index);
    const auto &new_hill = hill_draft.grid.at(mountain_index);
    using HillStep = BnwFreshFeatureHillEventKind;
    std::vector<HillStep> expected_hill_order{
        HillStep::empty_erase_skipped, HillStep::sight_removed_empty,
        HillStep::plot_type_written, HillStep::yield_checked};
    if (new_hill.yields != old_mountain.yields)
        expected_hill_order.push_back(HillStep::symbols_staged);
    expected_hill_order.insert(expected_hill_order.end(), {
        HillStep::sight_restored_empty, HillStep::terrain_preserved,
        HillStep::step_finder_reset, HillStep::water_branch_skipped,
        HillStep::graphics_guard_false});
    check(old_mountain.plot_type == 0 && new_hill.plot_type == 1 &&
              old_mountain.terrain_type == new_hill.terrain_type &&
              old_mountain.area_id == new_hill.area_id &&
              old_mountain.landmass_id == new_hill.landmass_id &&
              hill_draft.grid.land_plot_count() == fresh_grid.land_plot_count() &&
              hill_draft.grid.step_finder_force_reset() &&
              new_hill.yields == calculate_bnw_fresh_plot_yields(
                  new_hill, false, yield_rules) &&
              hill_draft.source_order == expected_hill_order &&
              hill_draft.grid.symbol_events().size() ==
                  fresh_grid.symbol_events().size() +
                      static_cast<std::size_t>(new_hill.yields != old_mountain.yields),
          "Fresh mountain-to-hill draft must preserve land/area/terrain and update selected yields");
    bool graphics_rejected = false;
    try {
        (void)fresh_grid.draft_fresh_feature_mountain_to_hill(
            mountain_index, *generated.map->area_structure,
            yield_rules, true, 2'000'000);
    } catch (const std::invalid_argument &) { graphics_rejected = true; }
    check(graphics_rejected,
          "Feature hill setter must block when live graphics effects would be required");

    auto handoff = prepare_bnw_fresh_map_handoff(std::move(generated), selected);
    check(handoff.map && handoff.map->owned_setup_grid &&
              handoff.map->owned_setup_grid->size() == 1664 &&
              handoff.map_random_after && handoff.map_random_after->calls == 300131 &&
              !handoff.has_blocker(BnwFreshMapHandoffBlockerCode::river_generation_pending) &&
              !handoff.has_blocker(BnwFreshMapHandoffBlockerCode::lake_generation_pending) &&
              !handoff.has_blocker(BnwFreshMapHandoffBlockerCode::map_unverified) &&
              handoff.has_blocker(BnwFreshMapHandoffBlockerCode::game_state_players_teams_pending) &&
              !handoff.ready_for_live_session(),
          "Completed stock rivers must hand off without certifying a playable world");
}
} // namespace

int main(int argc, char **argv) {
    try {
        test_self_contained_script();
        test_invalid_outputs();
        test_unsupported_stock_api();
        test_bnw_table_fill();
        test_bnw_fractal_source_path();
        test_bnw_fractal_ridges();
        test_bnw_setup_plot_handles();
        test_bnw_fresh_plot_transition_preflight();
        test_bnw_owned_setup_plot_draft();
        test_bnw_inland_corner();
        test_bnw_potential_city_work_draft();
        test_bnw_fresh_land_terrain_draft();
        test_bnw_stock_fresh_plot_setter();
        test_bnw_fresh_adjacent_land();
        test_bnw_fresh_terrain_setter();
        test_bnw_fresh_shallow_water_adjacency();
        test_bnw_fresh_area_structure();
        test_bnw_map_rng_and_grid();
        test_bnw_map_rng_arguments();
        test_bnw_fractal_wrap_flags();
        test_selected_bnw_map_setup();
        test_selected_world_rules();
        test_selected_resource_source();
        test_selected_map_includes();
        test_other_lua_include_contract();
        if (argc == 3 && std::strcmp(argv[1], "--stock-game-root") == 0)
            test_stock_inland_sea(argv[2]);
        else if (argc != 1)
            throw std::invalid_argument("Usage: bnw_map_script_tests [--stock-game-root <game-root>]");
        std::cout << "BNW map-script boundary passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
