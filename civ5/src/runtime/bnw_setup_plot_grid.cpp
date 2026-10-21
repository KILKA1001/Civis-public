#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/runtime/bnw_fresh_yield.hpp"
#include "civ5/runtime/bnw_plot_type_transition.hpp"
#include "civ5/content/rules_database.hpp"
#include <algorithm>
#include <bit>
#include <deque>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
std::optional<BnwSelectedFeatureEligibility> resolve_bnw_selected_feature_eligibility(
    const content::RulesDatabase &rules, std::int32_t feature_id) {
    if (feature_id < 0)
        throw std::invalid_argument("Fresh BNW feature ID is negative");
    std::optional<BnwSelectedFeatureEligibility> selected;
    const auto feature_rows = rules.query_rows(
        {"SELECT ROWID AS CivisRowId, ID, Type, NoCoast, NoRiver, NoAdjacent, "
         "RequiresFlatlands, RequiresRiver FROM Features WHERE ID = ? ORDER BY ROWID",
         {static_cast<std::int64_t>(feature_id)}},
        [&](const content::RulesRowView &row) {
            if (selected || !row.has_column("CivisRowId") ||
                !row.has_column("ID") || !row.has_column("Type") ||
                !row.has_column("NoCoast") || !row.has_column("NoRiver") ||
                !row.has_column("NoAdjacent") ||
                !row.has_column("RequiresFlatlands") ||
                !row.has_column("RequiresRiver") ||
                row.integer("CivisRowId") != feature_id ||
                row.integer("ID") != feature_id || !row.text("Type"))
                throw std::invalid_argument("Selected BNW feature Info slot is ambiguous or incomplete");
            selected = BnwSelectedFeatureEligibility{
                &rules, feature_id, row.boolean("NoCoast"),
                row.boolean("NoRiver"), row.boolean("NoAdjacent"),
                row.boolean("RequiresFlatlands"),
                row.boolean("RequiresRiver"), {}};
        }, 2);
    if (!feature_rows.all_succeeded())
        throw std::invalid_argument("Selected BNW feature lookup failed: " +
                                    feature_rows.error);
    if (!selected) return {};
    const auto terrain_rows = rules.query_rows(
        {"SELECT t.ID AS TerrainId, t.ROWID AS TerrainRowId FROM "
         "Feature_TerrainBooleans AS ft JOIN Terrains AS t "
         "ON t.Type = ft.TerrainType WHERE ft.FeatureType = "
         "(SELECT Type FROM Features WHERE ID = ?) ORDER BY ft.ROWID",
         {static_cast<std::int64_t>(feature_id)}},
        [&](const content::RulesRowView &row) {
            if (!row.has_column("TerrainId") ||
                !row.has_column("TerrainRowId") ||
                row.integer("TerrainId") != row.integer("TerrainRowId") ||
                row.integer("TerrainId") < 0 ||
                row.integer("TerrainId") > std::numeric_limits<std::int8_t>::max())
                throw std::invalid_argument("Selected BNW feature terrain Info slot is invalid");
            const auto id = static_cast<std::int32_t>(row.integer("TerrainId"));
            if (std::find(selected->terrain_ids.begin(), selected->terrain_ids.end(), id) !=
                selected->terrain_ids.end())
                throw std::invalid_argument("Selected BNW feature terrain is duplicated");
            selected->terrain_ids.push_back(id);
        }, 256);
    if (!terrain_rows.all_succeeded())
        throw std::invalid_argument("Selected BNW feature terrain lookup failed: " +
                                    terrain_rows.error);
    return selected;
}

bool can_have_bnw_fresh_feature(
    const BnwSetupPlotGrid &grid, std::size_t plot_index,
    std::int32_t feature_id,
    const std::optional<BnwSelectedFeatureEligibility> &feature,
    const BnwFreshAreaStructure &areas, std::int32_t minimum_water_size,
    std::span<const std::int32_t> feature_types) {
    const auto &plot = grid.at(plot_index);
    if (!plot.terrain_type_assigned_by_script || plot.terrain_type < 0 ||
        plot.area_id < 0 || areas.area_ids.size() != grid.size() ||
        areas.area_ids[plot_index] != plot.area_id ||
        feature_types.size() != grid.size())
        throw std::invalid_argument("Fresh BNW feature query requires assigned terrain and area");
    // CvPlot::canHaveFeature checks these branches before reading FeatureInfo.
    if (feature_id == -1) return true;
    if (feature_types[plot_index] != -1 || plot.plot_type == 0) return false;
    // No city objects exist in the detached CvMap::InitPlots stage.
    if (!feature) return false;
    if (feature->id != feature_id)
        throw std::invalid_argument("Selected BNW feature rule identity differs");
    if (std::find(feature->terrain_ids.begin(), feature->terrain_ids.end(),
                  plot.terrain_type) == feature->terrain_ids.end())
        return false;
    if (feature->no_coast &&
        grid.is_coastal_land(plot_index, minimum_water_size, areas))
        return false;
    const bool river = plot.river_crossing_count > 0;
    if (feature->no_river && river) return false;
    if (feature->requires_flatlands && plot.plot_type == 1) return false;
    if (feature->no_adjacent) {
        for (const auto adjacent : grid.adjacent(plot_index)) {
            if (adjacent && feature_types[*adjacent] == feature_id)
                return false;
        }
    }
    if (!river && feature->requires_river) return false;
    return true;
}

namespace {
std::int64_t floor_half(std::int64_t y) {
    return y >= 0 ? y / 2 : (y - 1) / 2;
}

std::optional<std::int32_t> range(std::int64_t value, std::int32_t size, bool wrap) {
    if (wrap) {
        value %= size;
        if (value < 0) value += size;
    }
    if (value < 0 || value >= size) return std::nullopt;
    return static_cast<std::int32_t>(value);
}
} // namespace

const BnwFreshArea &find_bnw_fresh_area(
    const BnwFreshAreaStructure &areas, std::int32_t area_id) {
    const BnwFreshArea *found = nullptr;
    for (const auto &area : areas.areas) {
        if (area.id != area_id) continue;
        if (found || area.tiles <= 0 || area.river_edges < 0)
            throw std::invalid_argument("Fresh BNW area handle is ambiguous or invalid");
        found = &area;
    }
    if (!found)
        throw std::invalid_argument("Fresh BNW area handle is missing");
    return *found;
}

std::optional<std::int32_t> find_bnw_fresh_biggest_area(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    bool water) {
    constexpr std::int64_t bucket = 1 << 13;
    if (grid.size() == 0 || areas.area_ids.size() != grid.size() ||
        areas.landmass_ids.size() != grid.size() ||
        areas.areas.empty() || areas.areas.size() >= bucket)
        throw std::invalid_argument("Fresh BNW biggest-area query has incomplete area state");
    std::vector<std::int32_t> counts(areas.areas.size());
    for (std::size_t ordinal = 0; ordinal < areas.areas.size(); ++ordinal) {
        const auto &area = areas.areas[ordinal];
        const auto expected_id =
            (static_cast<std::int64_t>(ordinal) + 1) * bucket +
            static_cast<std::int64_t>(ordinal);
        if (area.id != expected_id || area.tiles < 0 || area.river_edges < 0)
            throw std::invalid_argument("Fresh BNW area free-list identity differs");
    }
    for (std::size_t index = 0; index < grid.size(); ++index) {
        const auto &plot = grid.at(index);
        const auto id = areas.area_ids[index];
        if (id < 0 || plot.area_id != id)
            throw std::invalid_argument("Fresh BNW biggest-area plot handle is unassigned");
        const auto ordinal = static_cast<std::size_t>(id & (bucket - 1));
        if (ordinal >= areas.areas.size() || areas.areas[ordinal].id != id ||
            areas.areas[ordinal].water != (plot.plot_type == 3) ||
            counts[ordinal] == std::numeric_limits<std::int32_t>::max())
            throw std::invalid_argument("Fresh BNW biggest-area plot/area identity differs");
        ++counts[ordinal];
    }
    std::optional<std::int32_t> best;
    std::int32_t best_tiles = 0;
    // CvMap::firstArea/nextArea iterate the FFreeList in slot order; equal
    // sizes preserve the first slot because the source comparison is `>`.
    for (std::size_t ordinal = 0; ordinal < areas.areas.size(); ++ordinal) {
        const auto &area = areas.areas[ordinal];
        if (counts[ordinal] != area.tiles)
            throw std::invalid_argument("Fresh BNW biggest-area tile count differs");
        if (area.water == water && area.tiles > best_tiles) {
            best_tiles = area.tiles;
            best = area.id;
        }
    }
    return best;
}

void BnwSetupPlotGrid::reset(std::int32_t width, std::int32_t height,
                             bool wrap_x, bool wrap_y) {
    if (width <= 0 || height <= 0 ||
        static_cast<std::int64_t>(width) * height > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("Invalid BNW setup plot dimensions");
    std::vector<BnwSetupPlot> fresh(static_cast<std::size_t>(width) * height);
    for (std::size_t i = 0; i < fresh.size(); ++i) {
        fresh[i].x = static_cast<std::int32_t>(i % static_cast<std::size_t>(width));
        fresh[i].y = static_cast<std::int32_t>(i / static_cast<std::size_t>(width));
    }
    plots_ = std::move(fresh);
    width_ = width;
    height_ = height;
    wrap_x_ = wrap_x;
    wrap_y_ = wrap_y;
    land_plot_count_ = 0;
    step_finder_force_reset_ = false;
    last_terrain_steps_.clear();
    city_work_yield_calls_.clear();
    symbol_events_.clear();
}

std::optional<std::size_t> BnwSetupPlotGrid::hex_offset(
    std::size_t index, std::int32_t dx, std::int32_t dy) const {
    const auto &plot = plots_.at(index);
    const auto hex_x = static_cast<std::int64_t>(plot.x) - floor_half(plot.y);
    const auto y = static_cast<std::int64_t>(plot.y) + dy;
    const auto x = hex_x + dx + floor_half(y);
    const auto mapped_x = range(x, width_, wrap_x_);
    const auto mapped_y = range(y, height_, wrap_y_);
    if (mapped_x && mapped_y)
        return static_cast<std::size_t>(*mapped_y) * width_ + *mapped_x;
    return std::nullopt;
}

std::array<std::optional<std::size_t>, 6>
BnwSetupPlotGrid::adjacent(std::size_t index) const {
    // CvGameCoreUtils::plotDirection uses these CvGlobals direction deltas.
    static constexpr std::array<std::int32_t, 6> dx{0, 1, 1, 0, -1, -1};
    static constexpr std::array<std::int32_t, 6> dy{1, 0, -1, -1, 0, 1};
    std::array<std::optional<std::size_t>, 6> result{};
    for (std::size_t direction = 0; direction < result.size(); ++direction)
        result[direction] = hex_offset(index, dx[direction], dy[direction]);
    return result;
}

bool BnwSetupPlotGrid::is_adjacent_to_land(std::size_t index) const {
    // Expansion2 CvPlot::isAdjacentToLand: plotDirection in DirectionTypes
    // order, then !neighbor->isWater(). isWater is plot_type == PLOT_OCEAN.
    for (const auto neighbor : adjacent(index))
        if (neighbor && plots_[*neighbor].plot_type != 3) return true;
    return false;
}

bool BnwSetupPlotGrid::is_adjacent_to_shallow_water(
    std::size_t index, std::int32_t shallow_terrain_id) const {
    if (shallow_terrain_id < 0)
        throw std::invalid_argument("Selected BNW shallow-water terrain is unresolved");
    // Expansion2 CvPlot::isAdjacentToShallowWater compares only terrain type,
    // without checking the neighbor's plot category or area.
    for (const auto neighbor : adjacent(index))
        if (neighbor && plots_[*neighbor].terrain_type == shallow_terrain_id)
            return true;
    return false;
}

bool BnwSetupPlotGrid::is_coastal_land(
    std::size_t index, std::int32_t minimum_water_size,
    const BnwFreshAreaStructure &areas) const {
    if (areas.landmass_ids.size() != plots_.size())
        throw std::invalid_argument("Fresh BNW coastal query requires assigned landmasses");
    if (plots_.at(index).plot_type == 3) return false;
    for (const auto neighbor : adjacent(index)) {
        if (!neighbor || plots_[*neighbor].plot_type != 3) continue;
        if (minimum_water_size <= 0) return true;
        const auto id = areas.landmass_ids[*neighbor];
        const auto landmass = std::find_if(areas.landmasses.begin(), areas.landmasses.end(),
            [id](const auto &item) { return item.id == id; });
        if (landmass == areas.landmasses.end())
            throw std::invalid_argument("Fresh BNW coastal query has an invalid landmass");
        if (landmass->tiles >= minimum_water_size) return true;
    }
    return false;
}

bool BnwSetupPlotGrid::find_water_during_rivers(
    std::size_t index, std::int32_t radius, bool fresh_water,
    const BnwFreshAreaStructure &areas, std::uint64_t &remaining_visits) const {
    if (areas.landmass_ids.size() != plots_.size())
        throw std::invalid_argument("Fresh BNW water query requires assigned landmasses");
    if (index >= plots_.size())
        throw std::out_of_range("Fresh BNW water query plot index is invalid");
    if (radius < 0) return false; // SDK loop has no iterations for a negative range.
    const auto is_lake = [&](std::size_t plot_index) {
        const auto id = areas.landmass_ids[plot_index];
        const auto landmass = std::find_if(areas.landmasses.begin(), areas.landmasses.end(),
            [id](const auto &item) { return item.id == id; });
        if (landmass == areas.landmasses.end())
            throw std::invalid_argument("Fresh BNW water query has an invalid landmass");
        return landmass->lake;
    };
    for (std::int64_t dx = -static_cast<std::int64_t>(radius); dx <= radius; ++dx) {
        for (std::int64_t dy = -static_cast<std::int64_t>(radius); dy <= radius; ++dy) {
            if (remaining_visits == 0)
                throw std::length_error("Fresh BNW Map.FindWater exceeds its query budget");
            --remaining_visits;
            const auto abs_x = dx < 0 ? -dx : dx;
            const auto abs_y = dy < 0 ? -dy : dy;
            const auto hex_range = (dx >= 0) == (dy >= 0)
                ? abs_x + abs_y : std::max(abs_x, abs_y);
            if (hex_range > radius) continue;
            const auto plot_index = hex_offset(index, static_cast<std::int32_t>(dx),
                                               static_cast<std::int32_t>(dy));
            if (!plot_index) continue;
            const auto &plot = plots_[*plot_index];
            if (!fresh_water) {
                if (plot.plot_type == 3) return true;
                continue;
            }
            if (plot.plot_type == 3 || plot.plot_type == 0 || plot.impassable)
                continue;
            // CvPlot::isFreshWater tests isRiver before nearby lakes. Fresh
            // AddRivers has no features, owner, or improvements yet.
            if (plot.river_crossing_count > 0) return true;
            for (const auto neighbor : adjacent(*plot_index))
                if (neighbor && is_lake(*neighbor)) return true;
        }
    }
    return false;
}

std::array<std::optional<std::size_t>, 4>
BnwSetupPlotGrid::inland_corner_candidates(std::size_t index) const {
    if (index >= plots_.size())
        throw std::out_of_range("Fresh BNW inland-corner plot index is invalid");
    const auto source_neighbors = adjacent(index);
    // Expansion2 DirectionTypes: NORTH is a deprecated alias for NORTHEAST.
    std::array<std::optional<std::size_t>, 4> candidates{
        index, source_neighbors[0], source_neighbors[5], source_neighbors[4]};
    for (auto &candidate : candidates) {
        if (!candidate) continue;
        const auto at_corner = adjacent(*candidate);
        // CvPlot::hasCoastAtSECorner checks the candidate, EAST, SOUTHEAST.
        if (plots_[*candidate].plot_type == 3 ||
            (at_corner[1] && plots_[*at_corner[1]].plot_type == 3) ||
            (at_corner[2] && plots_[*at_corner[2]].plot_type == 3))
            candidate.reset();
    }
    return candidates;
}

std::optional<std::size_t> BnwSetupPlotGrid::get_inland_corner(
    std::size_t index, SimulationRandom &map_random) const {
    if (map_random.algorithm() != RandomAlgorithm::bnw)
        throw std::invalid_argument("Fresh inland corner requires the BNW map RNG");
    if (index >= plots_.size())
        throw std::out_of_range("Fresh BNW inland-corner plot index is invalid");
    // Expansion2 shuffleArray initializes [0,1,2,3] and draws 4,3,2,1.
    std::array<int, 4> shuffled{0, 1, 2, 3};
    for (std::size_t i = 0; i < shuffled.size(); ++i) {
        const auto j = i + map_random.draw(shuffled.size() - i);
        std::swap(shuffled[i], shuffled[static_cast<std::size_t>(j)]);
    }
    const auto candidates = inland_corner_candidates(index);
    for (const auto ordinal : shuffled)
        if (candidates[static_cast<std::size_t>(ordinal)])
            return candidates[static_cast<std::size_t>(ordinal)];
    return std::nullopt;
}

BnwFreshAreaStructure BnwSetupPlotGrid::draft_fresh_area_structure(
    std::int32_t lake_max_area_size) const {
    if (plots_.empty()) throw std::invalid_argument("Fresh BNW area grid is empty");
    for (const auto &plot : plots_)
        if (!plot.plot_type_assigned_by_script ||
            !plot.terrain_type_assigned_by_script || plot.terrain_type < 0 ||
            plot.area_id != -1)
            throw std::invalid_argument("Fresh BNW areas require complete unapplied plot and terrain state");

    // CvMap::init initializes empty FFreeListTrashArray containers. The first
    // Add uses currentID=1<<13; each next Add advances it by the same value
    // and uses the row-major component ordinal as the low index.
    constexpr std::int32_t free_list_bucket = 1 << 13;
    const auto source_id = [](std::size_t ordinal) {
        return static_cast<std::int32_t>(
            (ordinal + 1) * static_cast<std::size_t>(free_list_bucket) + ordinal);
    };
    BnwFreshAreaStructure result;
    result.lake_max_area_size = lake_max_area_size;
    result.area_ids.assign(plots_.size(), -1);
    result.landmass_ids.assign(plots_.size(), -1);
    std::vector<std::size_t> area_seeds;
    std::vector<std::int32_t> node_cost(plots_.size(), -1);
    const auto visit_component = [&](std::size_t seed,
                                     std::vector<std::int32_t> &ids,
                                     std::int32_t id, bool check_impassable,
                                     const auto &on_closed) {
        std::deque<std::size_t> open;
        open.push_back(seed);
        ids[seed] = id;
        node_cost[seed] = 0;
        while (!open.empty()) {
            const auto index = open.front();
            open.pop_front();
            on_closed(index);
            for (const auto neighbor : adjacent(index)) {
                if (!neighbor || ids[*neighbor] != -1) continue;
                const auto &parent = plots_[index];
                const auto &candidate = plots_[*neighbor];
                if ((candidate.plot_type == 3) != (parent.plot_type == 3) ||
                    (check_impassable && candidate.impassable != parent.impassable))
                    continue;
                ids[*neighbor] = id;
                const auto cost = node_cost[index] + 1;
                node_cost[*neighbor] = cost;
                // CvMap::setup passes a null cost and heuristic to CvAStar:
                // udFunc(null) returns 1. AddToOpen inserts equal-cost new
                // nodes at the head if they tie the current best, otherwise
                // at the tail if they tie the current worst.
                if (open.empty())
                    open.push_back(*neighbor);
                else if (cost <= node_cost[open.front()])
                    open.push_front(*neighbor);
                else if (cost >= node_cost[open.back()])
                    open.push_back(*neighbor);
                else
                    throw std::invalid_argument(
                        "Fresh BNW area A* reached an unverified open-list cost order");
            }
        }
    };

    // CvMap::calculateAreas scans plotByIndexUnchecked in index order.
    // AreaValid requires both isWater and isImpassable to match the parent.
    for (std::size_t seed = 0; seed < plots_.size(); ++seed) {
        if (result.area_ids[seed] != -1) continue;
        if (result.areas.size() >= free_list_bucket)
            throw std::invalid_argument("Fresh BNW area count exceeds SDK free-list capacity");
        const auto &first = plots_[seed];
        const auto id = source_id(result.areas.size());
        result.areas.push_back({id, first.plot_type == 3, first.plot_type == 0,
                                0, first.x, first.x, first.y, first.y, 0});
        area_seeds.push_back(seed);
        auto &area = result.areas.back();
        visit_component(seed, result.area_ids, id, true,
                        [&](std::size_t index) {
            result.area_assignment_order.push_back(index);
            ++area.tiles; // CvPlot::setArea -> processArea(+1).
            ++result.area_assignment_yield_calls; // CvPlot::setArea -> updateYield.
        });
    }
    // Preserve calculateAreas' source boundary update order and its else-if
    // chain. It updates at most one edge for each later plot in index order.
    for (std::size_t index = 0; index < plots_.size(); ++index) {
        const auto ordinal = static_cast<std::size_t>(
            result.area_ids[index] & (free_list_bucket - 1));
        if (index == area_seeds[ordinal]) continue;
        auto &area = result.areas[ordinal];
        const auto &plot = plots_[index];
        if (plot.x < area.west) area.west = plot.x;
        else if (plot.x > area.east) area.east = plot.x;
        else if (plot.y < area.south) area.south = plot.y;
        else if (plot.y > area.north) area.north = plot.y;
    }

    // recalculateLandmasses runs next. LandmassValid compares only isWater;
    // it can unite separate impassable and passable CvArea components.
    std::fill(node_cost.begin(), node_cost.end(), -1);
    for (std::size_t seed = 0; seed < plots_.size(); ++seed) {
        if (result.landmass_ids[seed] != -1) continue;
        if (result.landmasses.size() >= free_list_bucket)
            throw std::invalid_argument("Fresh BNW landmass count exceeds SDK free-list capacity");
        const auto id = source_id(result.landmasses.size());
        result.landmasses.push_back({id, plots_[seed].plot_type == 3,
                                     false, 0, 0, 0, 0});
        auto &landmass = result.landmasses.back();
        visit_component(seed, result.landmass_ids, id, false,
                        [&](std::size_t index) {
            result.landmass_assignment_order.push_back(index);
            const auto &plot = plots_[index];
            if (static_cast<std::int64_t>(landmass.centroid_x_sum) + plot.x >
                    std::numeric_limits<std::int32_t>::max() ||
                static_cast<std::int64_t>(landmass.centroid_y_sum) + plot.y >
                    std::numeric_limits<std::int32_t>::max())
                throw std::invalid_argument("Fresh BNW landmass centroid exceeds SDK int32 range");
            // CvLandmass::changeNumTiles compares isLake before and after
            // changing tile count. An initially empty water landmass counts
            // as a lake when the selected threshold is nonnegative.
            const bool old_lake = landmass.water &&
                                  landmass.tiles <= lake_max_area_size;
            ++landmass.tiles;
            const bool new_lake = landmass.water &&
                                  landmass.tiles <= lake_max_area_size;
            if (old_lake != new_lake) {
                ++landmass.lake_threshold_yield_passes;
                ++result.lake_threshold_full_yield_passes;
            }
            landmass.lake = new_lake;
            landmass.centroid_x_sum += plot.x;
            landmass.centroid_y_sum += plot.y;
        });
    }
    return result;
}

void BnwSetupPlotGrid::apply_fresh_area_handles(
    const BnwFreshAreaStructure &areas,
    const std::vector<std::array<std::int16_t, 6>> &final_yields,
    std::vector<BnwFreshYieldSymbolEvent> symbol_events) {
    const auto count = plots_.size();
    if (!count || areas.area_ids.size() != count ||
        areas.landmass_ids.size() != count || final_yields.size() != count ||
        areas.area_assignment_yield_calls != count)
        throw std::invalid_argument("Fresh BNW area handles require complete plots and yields");
    const auto source = draft_fresh_area_structure(areas.lake_max_area_size);
    if (areas.area_ids != source.area_ids ||
        areas.landmass_ids != source.landmass_ids ||
        areas.area_assignment_order != source.area_assignment_order ||
        areas.landmass_assignment_order != source.landmass_assignment_order ||
        areas.lake_threshold_full_yield_passes !=
            source.lake_threshold_full_yield_passes ||
        areas.landmasses.size() != source.landmasses.size())
        throw std::invalid_argument("Fresh BNW area handles differ from SDK callback trace");
    for (std::size_t i = 0; i < areas.landmasses.size(); ++i)
        if (areas.landmasses[i].id != source.landmasses[i].id ||
            areas.landmasses[i].tiles != source.landmasses[i].tiles ||
            areas.landmasses[i].lake != source.landmasses[i].lake ||
            areas.landmasses[i].lake_threshold_yield_passes !=
                source.landmasses[i].lake_threshold_yield_passes)
            throw std::invalid_argument("Fresh BNW lake handles differ from selected rules");
    const auto validate_order = [&](const std::vector<std::size_t> &order,
                                    const std::vector<std::int32_t> &ids,
                                    const auto &components) {
        if (order.size() != count)
            throw std::invalid_argument("Fresh BNW area callback trace is incomplete");
        std::vector<bool> seen(count);
        std::size_t component = 0;
        std::int32_t current_id = -1;
        for (const auto index : order) {
            if (index >= count || seen[index])
                throw std::invalid_argument("Fresh BNW area callback trace repeats a plot");
            seen[index] = true;
            if (ids[index] != current_id) {
                if (component >= components.size() ||
                    ids[index] != components[component].id)
                    throw std::invalid_argument("Fresh BNW component callback order differs from SDK");
                current_id = components[component++].id;
            }
        }
        if (component != components.size())
            throw std::invalid_argument("Fresh BNW component callback trace omits an area");
    };
    validate_order(areas.area_assignment_order, areas.area_ids, areas.areas);
    validate_order(areas.landmass_assignment_order, areas.landmass_ids,
                   areas.landmasses);
    for (std::size_t i = 0; i < count; ++i) {
        const auto &plot = plots_[i];
        if (!plot.plot_type_assigned_by_script ||
            !plot.terrain_type_assigned_by_script || plot.terrain_type < 0 ||
            plot.area_id != -1 || plot.landmass_id != -1 ||
            std::any_of(plot.yields.begin(), plot.yields.end(),
                        [](std::int16_t yield) { return yield != 0; }) ||
            std::any_of(final_yields[i].begin(), final_yields[i].end(),
                        [](std::int16_t yield) { return yield < 0; }))
            throw std::invalid_argument("Fresh BNW owned plot is not ready for area handles");
    }
    std::vector<std::size_t> area_rank(count);
    for (std::size_t i = 0; i < count; ++i)
        area_rank[areas.area_assignment_order[i]] = i;
    std::map<std::int32_t, std::size_t> threshold_rank;
    for (const auto &landmass : areas.landmasses)
        if (landmass.lake_threshold_yield_passes)
            threshold_rank.emplace(landmass.id, threshold_rank.size());
    int last_phase = -1;
    std::size_t last_area_rank = 0, last_threshold_rank = 0;
    std::uint32_t last_plot_index = 0;
    for (const auto &event : symbol_events) {
        const auto phase = static_cast<int>(event.phase);
        if (event.plot_index >= count || phase < last_phase || phase > 2 ||
            std::any_of(event.yields_after.begin(), event.yields_after.end(),
                        [](std::int16_t yield) { return yield < 0; }))
            throw std::invalid_argument("Fresh BNW updateSymbols event is invalid");
        if (phase == 0) {
            const auto rank = area_rank[event.plot_index];
            if (event.trigger_landmass_id != -1 ||
                (last_phase == 0 && rank <= last_area_rank))
                throw std::invalid_argument("Fresh BNW setArea symbol order differs from A*");
            last_area_rank = rank;
        } else if (phase == 1) {
            const auto trigger = threshold_rank.find(event.trigger_landmass_id);
            if (trigger == threshold_rank.end() ||
                (last_phase == 1 &&
                 (trigger->second < last_threshold_rank ||
                  (trigger->second == last_threshold_rank &&
                   event.plot_index <= last_plot_index))))
                throw std::invalid_argument("Fresh BNW lake symbol order differs from map pass");
            last_threshold_rank = trigger->second;
        } else if (event.trigger_landmass_id != -1 ||
                   (last_phase == 2 && event.plot_index <= last_plot_index)) {
            throw std::invalid_argument("Fresh BNW final symbol order differs from map pass");
        }
        last_phase = phase;
        last_plot_index = event.plot_index;
    }
    // recalculateAreas clears the initially invalid area handles, then
    // calculateAreas assigns them at ASNL_ADDCLOSED. The seed's callback is
    // a no-op because CvMap::calculateAreas set it before GeneratePath.
    for (const auto index : areas.area_assignment_order)
        plots_[index].area_id = areas.area_ids[index];
    // recalculateLandmasses clears initially invalid handles before the
    // second CvAStar scan and assigns each on its ASNL_ADDCLOSED callback.
    for (const auto index : areas.landmass_assignment_order)
        plots_[index].landmass_id = areas.landmass_ids[index];
    // The verified replay has executed the cache calculations for setArea,
    // lake-threshold passes and the final map pass. External updateSymbols
    // callbacks are still unavailable, so this is only an owned setup grid.
    for (std::size_t i = 0; i < count; ++i)
        plots_[i].yields = final_yields[i];
    symbol_events_ = std::move(symbol_events);
}

BnwSetupPlotGrid BnwSetupPlotGrid::draft_fresh_category_change(
    std::size_t index, std::int32_t new_plot_type) const {
    const auto &old = plots_.at(index);
    const auto plan = inspect_bnw_fresh_plot_transition(
        old.plot_type, old.terrain_type, new_plot_type, false, false);
    if (plan.status == BnwFreshPlotTransitionStatus::outside_fresh_setup_contract)
        throw std::invalid_argument("BNW plot transition is outside fresh setup contract");
    auto draft = *this;
    if (plan.status == BnwFreshPlotTransitionStatus::requires_world_effects) {
        // CvPlot::setPlotType assigns category before calling
        // updatePotentialCityWork on the city ring and changeLandPlots(+1).
        // Yield calls generated by that city-work update return without an area.
        draft.plots_[index].plot_type = new_plot_type;
        draft.update_potential_city_work_after_land_added(index);
        ++draft.land_plot_count_;
    }
    return draft;
}

BnwFreshAreaUnassignmentDraft BnwSetupPlotGrid::draft_fresh_area_unassignment(
    std::size_t index, const BnwFreshAreaStructure &areas) const {
    if (index >= plots_.size() || areas.area_ids.size() != plots_.size() ||
        areas.landmass_ids.size() != plots_.size())
        throw std::invalid_argument("Fresh BNW SetArea has no complete selected grid");
    const auto &plot = plots_[index];
    if (!plot.plot_type_assigned_by_script || !plot.terrain_type_assigned_by_script ||
        plot.area_id != areas.area_ids[index] ||
        plot.landmass_id != areas.landmass_ids[index])
        throw std::invalid_argument("Fresh BNW SetArea plot and area identity differ");
    BnwFreshAreaUnassignmentDraft draft{*this, areas, {}};
    if (plot.area_id == -1) return draft; // CvPlot::setArea no-op.

    const auto &old_area = find_bnw_fresh_area(areas, plot.area_id);
    if (old_area.water != (plot.plot_type == 3))
        throw std::invalid_argument("Fresh BNW SetArea water identity differs");
    const auto river_edges = static_cast<std::int32_t>(plot.ne_of_river) +
                             static_cast<std::int32_t>(plot.nw_of_river) +
                             static_cast<std::int32_t>(plot.w_of_river);
    if (old_area.river_edges < river_edges)
        throw std::invalid_argument("Fresh BNW SetArea river edge count underflows");
    const BnwFreshLandmass *old_landmass = nullptr;
    for (const auto &landmass : areas.landmasses) {
        if (landmass.id != plot.landmass_id) continue;
        if (old_landmass || landmass.tiles <= 0 ||
            landmass.water != (plot.plot_type == 3))
            throw std::invalid_argument("Fresh BNW SetArea landmass is invalid");
        old_landmass = &landmass;
    }
    if (!old_landmass)
        throw std::invalid_argument("Fresh BNW SetArea landmass is missing");

    // CvPlot::setArea reads isLake before processArea(-1). The selected
    // fresh world has no owners, resources, improvements, players, revealed
    // teams, units or cities, so processArea changes tiles and the present
    // NE/NW/W river edges only. INVALID_INDEX has no destination updateYield.
    draft.source_order.push_back({BnwFreshAreaUnassignmentEventKind::old_lake_read,
                                  std::nullopt, old_landmass->lake});
    auto &draft_area = *std::find_if(draft.areas.areas.begin(), draft.areas.areas.end(),
        [&](const auto &area) { return area.id == plot.area_id; });
    --draft_area.tiles;
    draft.source_order.push_back({
        BnwFreshAreaUnassignmentEventKind::old_area_tiles_decremented,
        std::nullopt, false});
    const auto decrement_edge = [&](bool present, BnwFreshRiverEdge edge) {
        if (!present) return;
        --draft_area.river_edges;
        draft.source_order.push_back({
            BnwFreshAreaUnassignmentEventKind::old_area_river_edge_decremented,
            edge});
    };
    decrement_edge(plot.ne_of_river, BnwFreshRiverEdge::north_east);
    decrement_edge(plot.nw_of_river, BnwFreshRiverEdge::north_west);
    decrement_edge(plot.w_of_river, BnwFreshRiverEdge::west);
    draft.grid.plots_[index].area_id = -1;
    draft.areas.area_ids[index] = -1;
    draft.source_order.push_back({
        BnwFreshAreaUnassignmentEventKind::area_handle_cleared,
        std::nullopt, false});
    return draft;
}

BnwFreshLakePlotTypePrefixDraft BnwSetupPlotGrid::draft_fresh_lake_plot_type_prefix(
    std::size_t index, const BnwFreshAreaStructure &areas,
    const BnwSelectedTerrain &current_terrain) const {
    if (index >= plots_.size() || areas.area_ids.size() != plots_.size() ||
        areas.landmass_ids.size() != plots_.size())
        throw std::invalid_argument("Fresh BNW lake plot has no complete selected grid");
    const auto &plot = plots_[index];
    if (!plot.plot_type_assigned_by_script || !plot.terrain_type_assigned_by_script ||
        plot.plot_type == 3 || plot.area_id != -1 || areas.area_ids[index] != -1 ||
        plot.landmass_id < 0 || plot.landmass_id != areas.landmass_ids[index] ||
        current_terrain.id != plot.terrain_type || current_terrain.water ||
        land_plot_count_ <= 0)
        throw std::invalid_argument("Fresh BNW lake plot is outside post-SetArea land contract");
    if (plot.ne_of_river || plot.nw_of_river || plot.w_of_river ||
        plot.river_crossing_mask || plot.river_crossing_count ||
        plot.river_e_flow_direction != -1 ||
        plot.river_se_flow_direction != -1 ||
        plot.river_sw_flow_direction != -1)
        throw std::invalid_argument("Fresh BNW lake plot still has river state to erase");
    const auto landmass = std::find_if(areas.landmasses.begin(), areas.landmasses.end(),
        [&](const auto &body) { return body.id == plot.landmass_id; });
    if (landmass == areas.landmasses.end() || landmass->water ||
        landmass->tiles <= 0 ||
        std::count_if(areas.landmasses.begin(), areas.landmasses.end(),
            [&](const auto &body) { return body.id == plot.landmass_id; }) != 1)
        throw std::invalid_argument("Fresh BNW lake plot landmass is invalid");

    using Step = BnwFreshLakePlotTypePrefixEventKind;
    BnwFreshLakePlotTypePrefixDraft draft{
        *this,
        {Step::erase_empty_fresh_plot, Step::old_water_read,
         Step::sight_removed_empty, Step::plot_type_written,
         Step::yield_skipped_without_area, Step::sight_added_empty,
         Step::terrain_conversion_required},
        {"selected_shallow_water_terrain_and_coast",
         "terrain_update_yield_impassable_sight",
         "step_finder_reset", "neighbor_terrain_and_yields",
         "city_work_yields", "land_plot_count",
         "area_recalculate_or_new_area", "landmass_consistency",
         "graphics_layout"}};
    // Fresh setup has no units, cities, resources, improvements, routes,
    // features, owners or visibility sources. AddLakes excludes rivers, so
    // erase(true) and both sight passes have no live effects. The earlier
    // SetArea(-1) makes updateYield return immediately. The native setter
    // writes m_ePlotType before reaching the selected water terrain branch.
    draft.grid.plots_[index].plot_type = 3;
    return draft;
}

BnwFreshLakeTerrainDraft BnwSetupPlotGrid::advance_fresh_lake_terrain(
    BnwFreshLakePlotTypePrefixDraft prefix, std::size_t index,
    const BnwSelectedTerrain &selected_water_terrain) {
    if (index >= prefix.grid.size() || prefix.source_order.empty() ||
        prefix.source_order.back() !=
            BnwFreshLakePlotTypePrefixEventKind::terrain_conversion_required ||
        selected_water_terrain.id < 0 ||
        selected_water_terrain.id > std::numeric_limits<std::int8_t>::max() ||
        !selected_water_terrain.water)
        throw std::invalid_argument("Fresh BNW lake water terrain is not selected");
    const auto &old = prefix.grid.at(index);
    if (old.plot_type != 3 || old.area_id != -1 || old.terrain_type < 0 ||
        old.terrain_type == selected_water_terrain.id)
        throw std::invalid_argument("Fresh BNW lake terrain conversion is out of order");

    // CvPlot::setPlotType tests adjacency after writing PLOT_OCEAN. The
    // selected SHALLOW/DEEP_WATER_TERRAIN define was resolved by the caller
    // for this branch, with its active Terrains Info row.
    const bool shallow = prefix.grid.is_adjacent_to_land(index);
    using Step = BnwFreshLakePlotTypePrefixEventKind;
    BnwFreshLakeTerrainDraft draft{
        std::move(prefix.grid), shallow, std::move(prefix.source_order),
        {"step_finder_reset", "neighbor_terrain_and_yields",
         "city_work_yields", "land_plot_count",
         "area_recalculate_or_new_area", "landmass_consistency",
         "graphics_layout"}};
    draft.source_order.push_back(Step::adjacent_land_read);
    auto &plot = draft.grid.plots_[index];
    // setTerrainType(bRecalculate=true,bRebuildGraphics=true): both sight
    // callbacks are inert in the empty fresh world even if selected sight
    // levels differ. updateYield returns at area()==NULL. NO_FEATURE makes
    // updateImpassable read the selected terrain; matching waterness avoids
    // recursive setPlotType. The flags are unused on this nested path.
    plot.terrain_type = selected_water_terrain.id;
    draft.source_order.push_back(Step::selected_water_terrain_written);
    draft.grid.last_terrain_steps_ = {
        BnwFreshTerrainStep::assign_terrain,
        BnwFreshTerrainStep::yield_skipped_without_area,
        BnwFreshTerrainStep::update_impassability};
    draft.source_order.push_back(Step::terrain_yield_skipped_without_area);
    plot.impassable = selected_water_terrain.impassable;
    draft.source_order.push_back(Step::terrain_impassability_updated);
    draft.source_order.push_back(Step::terrain_recursion_skipped_matching_water);
    draft.source_order.push_back(Step::coast_cache_written);
    return draft;
}

BnwFreshLakeNeighborDraft BnwSetupPlotGrid::advance_fresh_lake_neighbors(
    BnwFreshLakeTerrainDraft terrain, std::size_t index,
    const BnwFreshAreaStructure &areas, const BnwFreshYieldRules &rules,
    const BnwSelectedTerrain &shallow_terrain,
    const BnwSelectedTerrain &deep_terrain,
    std::uint64_t maximum_symbol_events) {
    using Step = BnwFreshLakePlotTypePrefixEventKind;
    using Neighbor = BnwFreshLakeNeighborEventKind;
    if (index >= terrain.grid.size() || terrain.source_order.empty() ||
        terrain.source_order.back() != Step::coast_cache_written ||
        terrain.remaining_services.empty() ||
        terrain.remaining_services.front() != "step_finder_reset" ||
        terrain.grid.at(index).plot_type != 3 ||
        terrain.grid.at(index).area_id != -1 || !rules.source_rules ||
        areas.area_ids.size() != terrain.grid.size() ||
        areas.landmass_ids.size() != terrain.grid.size() ||
        shallow_terrain.id < 0 || deep_terrain.id < 0 ||
        !shallow_terrain.water || !deep_terrain.water)
        throw std::invalid_argument("Fresh BNW lake neighbor rules or area state are unresolved");

    const auto adjacent = terrain.grid.adjacent(index);
    for (const auto neighbor : adjacent) {
        if (!neighbor) continue;
        if (*neighbor == index)
            throw std::invalid_argument("Fresh BNW lake wraps onto itself");
        const auto &plot = terrain.grid.at(*neighbor);
        if (plot.area_id < 0 || plot.area_id != areas.area_ids[*neighbor] ||
            plot.landmass_id < 0 ||
            plot.landmass_id != areas.landmass_ids[*neighbor] ||
            !plot.plot_type_assigned_by_script ||
            !plot.terrain_type_assigned_by_script ||
            !rules.terrains.contains(plot.terrain_type))
            throw std::invalid_argument("Fresh BNW lake neighbor is outside selected area/yield state");
        const auto &area = find_bnw_fresh_area(areas, plot.area_id);
        if (area.water != (plot.plot_type == 3))
            throw std::invalid_argument("Fresh BNW lake neighbor water area differs");
        const auto selected = resolve_bnw_selected_terrain_by_id(
            *rules.source_rules, plot.terrain_type);
        if (selected.water != (plot.plot_type == 3))
            throw std::invalid_argument("Fresh BNW lake neighbor terrain category differs");
    }

    BnwFreshLakeNeighborDraft draft{
        std::move(terrain.grid), terrain.shallow_water,
        std::move(terrain.source_order), {},
        {"city_work_yields", "land_plot_count",
         "area_recalculate_or_new_area", "landmass_consistency",
         "graphics_layout"}};
    // CvAStar::ForceReset only sets its flag. This fresh grid has no path
    // queries between this call and the still-blocked city/area work.
    draft.grid.step_finder_force_reset_ = true;
    draft.source_order.push_back(Step::step_finder_reset);

    const auto update_neighbor_yield = [&](std::size_t plot_index,
                                           std::uint8_t direction,
                                           Neighbor checked,
                                           Neighbor symbols) {
        auto &plot = draft.grid.plots_[plot_index];
        const auto landmass = std::find_if(areas.landmasses.begin(), areas.landmasses.end(),
            [&](const auto &body) { return body.id == plot.landmass_id; });
        if (landmass == areas.landmasses.end() || landmass->tiles <= 0 ||
            landmass->water != (plot.plot_type == 3))
            throw std::invalid_argument("Fresh BNW lake neighbor landmass is unresolved");
        const auto next = calculate_bnw_fresh_plot_yields(plot, landmass->lake, rules);
        draft.neighbor_order.push_back({checked, plot_index, direction});
        if (next == plot.yields) return;
        if (draft.grid.symbol_events_.size() >= maximum_symbol_events)
            throw std::length_error("Fresh BNW lake symbols exceed selected event budget");
        plot.yields = next;
        draft.grid.symbol_events_.push_back({BnwFreshYieldSymbolPhase::lake_conversion,
            static_cast<std::uint32_t>(plot_index), -1, next});
        draft.neighbor_order.push_back({symbols, plot_index, direction});
    };

    // The first SDK loop is gated by bRecalculate and reads each water
    // neighbor's adjacency after the center was changed to PLOT_OCEAN.
    // setTerrainType performs its own updateYield before updateImpassable.
    for (std::size_t direction = 0; direction < adjacent.size(); ++direction) {
        const auto neighbor = adjacent[direction];
        if (!neighbor || draft.grid.plots_[*neighbor].plot_type != 3) continue;
        const bool shallow = draft.grid.is_adjacent_to_land(*neighbor);
        const auto &selected = shallow ? shallow_terrain : deep_terrain;
        auto &plot = draft.grid.plots_[*neighbor];
        if (plot.terrain_type != selected.id) {
            if (!rules.terrains.contains(selected.id))
                throw std::invalid_argument("Fresh BNW lake selected neighbor terrain yields are unresolved");
            plot.terrain_type = selected.id;
            draft.neighbor_order.push_back({Neighbor::terrain_written,
                                            *neighbor, static_cast<std::uint8_t>(direction)});
            update_neighbor_yield(*neighbor, static_cast<std::uint8_t>(direction),
                                  Neighbor::terrain_yield_checked,
                                  Neighbor::terrain_symbols_staged);
            plot.impassable = selected.impassable;
            draft.neighbor_order.push_back({Neighbor::terrain_impassability_updated,
                                            *neighbor, static_cast<std::uint8_t>(direction)});
        }
        // CvPlot.cpp writes the source plot's m_bIsAdjacentToLand here,
        // even though pLoopPlot is the terrain recipient.
        draft.shallow_water = shallow;
        draft.neighbor_order.push_back({Neighbor::coast_cache_written,
                                        index, static_cast<std::uint8_t>(direction)});
    }
    draft.source_order.push_back(Step::neighbor_terrain_pass);
    for (std::size_t direction = 0; direction < adjacent.size(); ++direction) {
        const auto neighbor = adjacent[direction];
        if (!neighbor) continue;
        update_neighbor_yield(*neighbor, static_cast<std::uint8_t>(direction),
                              Neighbor::yield_checked, Neighbor::symbols_staged);
    }
    draft.source_order.push_back(Step::neighbor_yield_pass);
    return draft;
}

BnwFreshLakeCityAreaDraft BnwSetupPlotGrid::advance_fresh_lake_city_area_preflight(
    BnwFreshLakeNeighborDraft neighbors, std::size_t index,
    const BnwFreshAreaStructure &areas, const BnwFreshYieldRules &rules,
    std::uint64_t maximum_symbol_events) {
    using Step = BnwFreshLakePlotTypePrefixEventKind;
    using City = BnwFreshLakeCityEventKind;
    if (index >= neighbors.grid.size() || neighbors.source_order.empty() ||
        neighbors.source_order.back() != Step::neighbor_yield_pass ||
        neighbors.remaining_services.empty() ||
        neighbors.remaining_services.front() != "city_work_yields" ||
        neighbors.grid.at(index).plot_type != 3 ||
        neighbors.grid.at(index).area_id != -1 ||
        areas.area_ids.size() != neighbors.grid.size() ||
        areas.landmass_ids.size() != neighbors.grid.size() ||
        areas.area_ids[index] != -1 || !rules.source_rules ||
        neighbors.grid.land_plot_count_ <= 0 ||
        neighbors.grid.land_plot_count_ > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("Fresh BNW lake city/area preflight is out of order");
    const auto actual_land = std::count_if(neighbors.grid.plots_.begin(),
        neighbors.grid.plots_.end(), [](const auto &plot) {
            return plot.plot_type != 3;
        });
    if (neighbors.grid.land_plot_count_ !=
        static_cast<std::int64_t>(actual_land) + 1)
        throw std::invalid_argument("Fresh BNW lake land count differs from owned grid");

    BnwFreshLakeCityAreaDraft draft{
        std::move(neighbors.grid), neighbors.shallow_water,
        std::move(neighbors.source_order), std::move(neighbors.neighbor_order),
        {}, std::nullopt, 0, false,
        {"area_transition_pending", "landmass_consistency", "graphics_layout"}};
    // CvPlot::setPlotType visits NUM_CITY_PLOTS in slot order. Each non-null
    // plot recomputes m_bPotentialCityWork from its own 37-slot ring. The
    // selected fresh world has no working city, owner or improvement; a
    // changed bit invokes only the proven fresh updateYield and symbol path.
    const auto city_ring = draft.grid.city_plots(index);
    for (std::size_t slot = 0; slot < city_ring.size(); ++slot) {
        const auto candidate = city_ring[slot];
        draft.city_order.push_back({City::city_plot_checked,
            static_cast<std::uint8_t>(slot), candidate});
        if (!candidate) continue;
        auto &plot = draft.grid.plots_[*candidate];
        bool valid = false;
        for (const auto member : draft.grid.city_plots(*candidate)) {
            if (member && draft.grid.plots_[*member].plot_type != 3) {
                valid = true;
                break;
            }
        }
        if (plot.potential_city_work == valid) continue;
        plot.potential_city_work = valid;
        draft.city_order.push_back({City::potential_city_work_written,
            static_cast<std::uint8_t>(slot), candidate});
        if (plot.area_id < 0) {
            if (*candidate != index || areas.area_ids[*candidate] != -1)
                throw std::invalid_argument("Fresh BNW city-work yield lost its area");
            draft.city_order.push_back({City::yield_skipped_without_area,
                static_cast<std::uint8_t>(slot), candidate});
            continue;
        }
        if (plot.area_id != areas.area_ids[*candidate] ||
            plot.landmass_id != areas.landmass_ids[*candidate] ||
            find_bnw_fresh_area(areas, plot.area_id).water !=
                (plot.plot_type == 3))
            throw std::invalid_argument("Fresh BNW city-work area identity differs");
        const auto landmass = std::find_if(areas.landmasses.begin(),
            areas.landmasses.end(), [&](const auto &body) {
                return body.id == plot.landmass_id;
            });
        if (landmass == areas.landmasses.end() || landmass->tiles <= 0 ||
            landmass->water != (plot.plot_type == 3))
            throw std::invalid_argument("Fresh BNW city-work landmass is unresolved");
        const auto next = calculate_bnw_fresh_plot_yields(
            plot, landmass->lake, rules);
        draft.city_order.push_back({City::yield_checked,
            static_cast<std::uint8_t>(slot), candidate});
        if (next == plot.yields) continue;
        if (draft.grid.symbol_events_.size() >= maximum_symbol_events)
            throw std::length_error("Fresh BNW lake city symbols exceed selected event budget");
        plot.yields = next;
        draft.grid.symbol_events_.push_back({
            BnwFreshYieldSymbolPhase::lake_conversion,
            static_cast<std::uint32_t>(*candidate), -1, next});
        draft.city_order.push_back({City::symbols_staged,
            static_cast<std::uint8_t>(slot), candidate});
    }
    draft.source_order.push_back(Step::city_work_pass);
    --draft.grid.land_plot_count_; // CvMap::changeLandPlots(-1).
    draft.source_order.push_back(Step::land_plot_count_decremented);

    // NO_RESOURCE and NO_PLAYER make both intervening SDK count branches
    // inert. The remaining code first searches distinct adjacent water area
    // pointers in DirectionTypes order, then counts cyclic area transitions.
    const auto adjacent = draft.grid.adjacent(index);
    const auto area_id = [&](std::optional<std::size_t> neighbor) {
        if (!neighbor) return std::int32_t{-1};
        const auto id = draft.grid.plots_[*neighbor].area_id;
        if (id < 0 || id != areas.area_ids[*neighbor])
            throw std::invalid_argument("Fresh BNW lake adjacent area is unresolved");
        return id;
    };
    for (const auto neighbor : adjacent) {
        if (!neighbor) continue;
        const auto id = area_id(neighbor);
        const auto &area = find_bnw_fresh_area(areas, id);
        if (!area.water) continue;
        if (!draft.adjacent_water_area_id)
            draft.adjacent_water_area_id = id;
        else if (*draft.adjacent_water_area_id != id) {
            draft.recalculate_areas = true;
            break;
        }
    }
    if (!draft.recalculate_areas) {
        auto last = area_id(adjacent.back());
        for (const auto neighbor : adjacent) {
            const auto current = area_id(neighbor);
            if (current != last) ++draft.adjacent_area_transitions;
            last = current;
        }
        draft.recalculate_areas = draft.adjacent_area_transitions > 2;
    }
    draft.remaining_services.front() = draft.recalculate_areas
        ? "map_recalculate_areas" : "incremental_area_attach_or_create";
    draft.source_order.push_back(Step::area_preflight_complete);
    return draft;
}

BnwFreshLakeAreaDraft BnwSetupPlotGrid::advance_fresh_lake_incremental_area(
    BnwFreshLakeCityAreaDraft preflight, std::size_t index,
    BnwFreshAreaStructure areas, const BnwFreshYieldRules &rules,
    std::uint64_t maximum_symbol_events) {
    using Step = BnwFreshLakePlotTypePrefixEventKind;
    using Event = BnwFreshLakeAreaEventKind;
    if (index >= preflight.grid.size() || preflight.source_order.empty() ||
        preflight.source_order.back() != Step::area_preflight_complete ||
        preflight.recalculate_areas || preflight.remaining_services.empty() ||
        preflight.remaining_services.front() !=
            "incremental_area_attach_or_create" ||
        !rules.source_rules || areas.area_ids.size() != preflight.grid.size() ||
        areas.landmass_ids.size() != preflight.grid.size() ||
        areas.area_ids[index] != -1 ||
        preflight.grid.at(index).area_id != -1 ||
        preflight.grid.at(index).plot_type != 3)
        throw std::invalid_argument("Fresh BNW incremental lake area is out of order");
    const auto &source = preflight.grid.at(index);
    if (!source.plot_type_assigned_by_script ||
        !source.terrain_type_assigned_by_script || source.terrain_type < 0 ||
        source.ne_of_river || source.nw_of_river || source.w_of_river ||
        source.river_crossing_count || source.river_crossing_mask ||
        source.landmass_id < 0 ||
        source.landmass_id != areas.landmass_ids[index])
        throw std::invalid_argument("Fresh BNW incremental lake plot has unresolved effects");
    const auto source_landmass = std::find_if(areas.landmasses.begin(),
        areas.landmasses.end(), [&](const auto &body) {
            return body.id == source.landmass_id;
        });
    if (source_landmass == areas.landmasses.end() ||
        source_landmass->water || source_landmass->tiles <= 0)
        throw std::invalid_argument("Fresh BNW lake source landmass identity differs");
    const bool old_lake = source_landmass->lake;

    // The selected fresh map has only the original CvMap::calculateAreas
    // allocations. Validate that FFreeList's currentID and next slot can be
    // reconstructed; no deleted/reused area slot is guessed here.
    constexpr std::int64_t bucket = 1 << 13;
    if (areas.areas.empty() || areas.areas.size() >= bucket)
        throw std::invalid_argument("Fresh BNW area free-list allocation is unavailable");
    for (std::size_t ordinal = 0; ordinal < areas.areas.size(); ++ordinal) {
        const auto expected = (static_cast<std::int64_t>(ordinal) + 1) * bucket +
                              static_cast<std::int64_t>(ordinal);
        if (areas.areas[ordinal].id != expected || areas.areas[ordinal].tiles < 0)
            throw std::invalid_argument("Fresh BNW area free-list identity is not sequential");
    }
    BnwFreshLakeAreaDraft draft{
        std::move(preflight.grid), std::move(areas),
        std::move(preflight.source_order), {},
        {"landmass_consistency", "graphics_layout"}};
    // Lua AddLakes already called SetArea(-1); the native setter repeats it
    // after the preflight. m_iArea is invalid, so the old-area deletion test
    // is false even if the old area's tile count became zero.
    draft.area_order.push_back({Event::invalid_set_area_noop});
    draft.area_order.push_back({Event::old_area_delete_skipped});
    std::int32_t destination = -1;
    if (preflight.adjacent_water_area_id) {
        destination = *preflight.adjacent_water_area_id;
        const auto &area = find_bnw_fresh_area(draft.areas, destination);
        if (!area.water)
            throw std::invalid_argument("Fresh BNW neighboring area is not water");
        draft.area_order.push_back({Event::neighboring_area_selected, destination});
    } else {
        const auto ordinal = draft.areas.areas.size();
        const auto id = (static_cast<std::int64_t>(ordinal) + 1) * bucket +
                        static_cast<std::int64_t>(ordinal);
        if (id > std::numeric_limits<std::int32_t>::max())
            throw std::invalid_argument("Fresh BNW area free-list ID overflows");
        destination = static_cast<std::int32_t>(id);
        draft.area_order.push_back({Event::new_area_allocated, destination});
        // CvArea::init(reset) initializes zero tiles, river edges and all
        // four boundaries to zero. setPlotType does not set boundaries here.
        draft.areas.areas.push_back({destination, true, false,
                                     0, 0, 0, 0, 0, 0});
        draft.area_order.push_back({Event::new_area_initialized, destination});
    }
    auto &plot = draft.grid.plots_[index];
    draft.area_order.push_back({Event::old_lake_read});
    draft.area_order.push_back({Event::old_process_skipped});
    plot.area_id = destination;
    draft.areas.area_ids[index] = destination;
    draft.area_order.push_back({Event::area_handle_written, destination});
    auto &area = *std::find_if(draft.areas.areas.begin(),
        draft.areas.areas.end(), [&](const auto &body) {
            return body.id == destination;
        });
    if (area.tiles == std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("Fresh BNW water area tile count overflows");
    ++area.tiles; // CvPlot::processArea(+1); no other owned fresh counters.
    draft.area_order.push_back({Event::area_tiles_incremented, destination});
    // CvPlot::isLake still reads the old landmass handle at this point.
    const auto next = calculate_bnw_fresh_plot_yields(
        plot, old_lake, rules);
    draft.area_order.push_back({Event::yield_checked, destination});
    if (next != plot.yields) {
        if (draft.grid.symbol_events_.size() >= maximum_symbol_events)
            throw std::length_error("Fresh BNW lake area symbols exceed selected event budget");
        plot.yields = next;
        draft.grid.symbol_events_.push_back({
            BnwFreshYieldSymbolPhase::lake_conversion,
            static_cast<std::uint32_t>(index), -1, next});
        draft.area_order.push_back({Event::symbols_staged, destination});
    }
    draft.source_order.push_back(Step::incremental_area_applied);
    return draft;
}

BnwFreshLakePostSetterDraft BnwSetupPlotGrid::inspect_fresh_lake_post_setter(
    BnwFreshLakeAreaDraft area, const BnwSetupPlotGrid &before,
    const BnwFreshAreaStructure &before_areas, std::size_t index,
    std::optional<bool> graphics_initialized) {
    using Step = BnwFreshLakePlotTypePrefixEventKind;
    using Event = BnwFreshLakePostSetterEventKind;
    if (index >= before.size() || area.grid.size() != before.size() ||
        area.source_order.empty() ||
        area.source_order.back() != Step::incremental_area_applied ||
        area.remaining_services.empty() ||
        area.remaining_services.front() != "landmass_consistency" ||
        before_areas.landmass_ids.size() != before.size() ||
        area.areas.landmass_ids.size() != before.size() ||
        area.areas.area_ids.size() != before.size() ||
        before.at(index).plot_type == 3 ||
        area.grid.at(index).plot_type != 3 ||
        area.grid.at(index).area_id < 0)
        throw std::invalid_argument("Fresh BNW lake post-setter state is out of order");
    const auto compare_landmass = [](const BnwFreshLandmass &left,
                                     const BnwFreshLandmass &right) {
        return left.id == right.id && left.water == right.water &&
            left.lake == right.lake && left.tiles == right.tiles &&
            left.centroid_x_sum == right.centroid_x_sum &&
            left.centroid_y_sum == right.centroid_y_sum &&
            left.lake_threshold_yield_passes ==
                right.lake_threshold_yield_passes;
    };
    if (area.areas.landmass_ids != before_areas.landmass_ids ||
        area.areas.landmasses.size() != before_areas.landmasses.size() ||
        !std::equal(area.areas.landmasses.begin(), area.areas.landmasses.end(),
                    before_areas.landmasses.begin(), compare_landmass))
        throw std::invalid_argument("Fresh BNW lake mutated SDK landmass pool before recalculateLandmasses");
    for (std::size_t plot_index = 0; plot_index < before.size(); ++plot_index)
        if (area.grid.at(plot_index).landmass_id !=
            before.at(plot_index).landmass_id)
            throw std::invalid_argument("Fresh BNW lake changed landmass handle inside setPlotType");
    const auto old_landmass = std::find_if(before_areas.landmasses.begin(),
        before_areas.landmasses.end(), [&](const auto &body) {
            return body.id == before.at(index).landmass_id;
        });
    const auto new_area = std::find_if(area.areas.areas.begin(),
        area.areas.areas.end(), [&](const auto &body) {
            return body.id == area.grid.at(index).area_id;
        });
    if (old_landmass == before_areas.landmasses.end() || old_landmass->water ||
        new_area == area.areas.areas.end() || !new_area->water ||
        new_area->tiles <= 0 ||
        area.areas.area_ids[index] != area.grid.at(index).area_id)
        throw std::invalid_argument("Fresh BNW lake old landmass/new water area identity differs");

    BnwFreshLakePostSetterDraft draft{
        std::move(area.grid), std::move(area.areas),
        std::move(area.source_order),
        {Event::landmass_handle_preserved, Event::landmass_pool_preserved},
        {"graphics_state_unresolved", "map_calculate_areas_handoff",
         "later_feature_recalculate_landmasses"}};
    // CvPlot::setPlotType has no setLandmass call. Lua Map.CalculateAreas
    // invokes calculateAreas only; the later Map.RecalculateAreas after
    // AddFeatures finally calls recalculateLandmasses. Stale landmass IDs at
    // this exact boundary are the native source state, not a repair target.
    if (!graphics_initialized) {
        draft.post_order.push_back(Event::graphics_state_unresolved);
        return draft;
    }
    draft.source_order.push_back(Step::graphics_guard_checked);
    if (!*graphics_initialized) {
        draft.post_order.push_back(Event::graphics_guard_false);
        draft.remaining_services.erase(draft.remaining_services.begin());
    } else {
        draft.post_order.push_back(Event::graphics_layout_dirty_pending);
        draft.remaining_services.front() = "graphics_layout_dirty_callback";
    }
    return draft;
}

BnwFreshFeatureHillDraft BnwSetupPlotGrid::draft_fresh_feature_mountain_to_hill(
    std::size_t index, const BnwFreshAreaStructure &areas,
    const BnwFreshYieldRules &rules, std::optional<bool> graphics_initialized,
    std::uint64_t maximum_symbol_events) const {
    using Event = BnwFreshFeatureHillEventKind;
    if (index >= plots_.size() || !graphics_initialized || *graphics_initialized ||
        !rules.source_rules || areas.area_ids.size() != plots_.size() ||
        areas.landmass_ids.size() != plots_.size() ||
        index > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Fresh BNW feature hill transition has unresolved context");
    const auto &old = plots_[index];
    if (old.plot_type != 0 || old.terrain_type < 0 || old.area_id < 0 ||
        old.landmass_id < 0 || !old.plot_type_assigned_by_script ||
        !old.terrain_type_assigned_by_script ||
        old.area_id != areas.area_ids[index] ||
        old.landmass_id != areas.landmass_ids[index] ||
        !rules.terrains.contains(old.terrain_type))
        throw std::invalid_argument("Fresh BNW coastal mountain state is incomplete");
    const auto area = std::find_if(areas.areas.begin(), areas.areas.end(),
        [&](const auto &body) { return body.id == old.area_id; });
    const auto landmass = std::find_if(areas.landmasses.begin(), areas.landmasses.end(),
        [&](const auto &body) { return body.id == old.landmass_id; });
    if (area == areas.areas.end() || area->water || area->tiles <= 0 ||
        landmass == areas.landmasses.end() || landmass->water ||
        landmass->tiles <= 0)
        throw std::invalid_argument("Fresh BNW coastal mountain area/landmass differs");

    BnwFreshFeatureHillDraft draft{*this, {
        Event::empty_erase_skipped, Event::sight_removed_empty,
        Event::plot_type_written}};
    auto &plot = draft.grid.plots_[index];
    // CvPlot::setPlotType does not call erase for land to land. The fresh
    // setup world has no visibility sources, so both sight passes are inert.
    plot.plot_type = 1;
    const auto next = calculate_bnw_fresh_plot_yields(plot, false, rules);
    draft.source_order.push_back(Event::yield_checked);
    if (next != plot.yields) {
        if (draft.grid.symbol_events_.size() >= maximum_symbol_events)
            throw std::length_error("Fresh BNW feature hill symbols exceed selected budget");
        plot.yields = next;
        draft.grid.symbol_events_.push_back({
            BnwFreshYieldSymbolPhase::feature_mountain_conversion,
            static_cast<std::uint32_t>(index), -1, next});
        draft.source_order.push_back(Event::symbols_staged);
    }
    draft.source_order.push_back(Event::sight_restored_empty);
    // Mountain and hill are both land. Terrain waterness already matches,
    // so setTerrainType, coastal neighbors, city work, land count and area
    // reassignment are skipped by the SDK branches.
    draft.source_order.push_back(Event::terrain_preserved);
    draft.grid.step_finder_force_reset_ = true;
    draft.source_order.push_back(Event::step_finder_reset);
    draft.source_order.push_back(Event::water_branch_skipped);
    // The selected fresh host has GC.IsGraphicsInitialized == false; the
    // requested bRebuildGraphics=true therefore skips setLayoutDirty.
    draft.source_order.push_back(Event::graphics_guard_false);
    return draft;
}

BnwFreshRiverWritePreflight inspect_bnw_fresh_river_write(
    const BnwSetupPlotGrid &grid, std::size_t index, BnwFreshRiverEdge edge,
    bool enabled, std::int32_t flow_direction) {
    using Effect = BnwFreshRiverWriteEffect;
    BnwFreshRiverWritePreflight result{
        BnwFreshRiverWriteBlockerCode::required_services_pending,
        {Effect::edge_bit, Effect::self_crossings, Effect::self_yield,
         Effect::neighbor_crossing_then_yield_each_direction,
         Effect::area_river_edge_count, Effect::flow_direction},
        {"river_crossing_mask_and_count", "selected_terrain_river_yield_and_symbols",
         "area_river_edge_count", "river_flow_direction_state"}};
    if (index >= grid.size()) {
        result.blocker = BnwFreshRiverWriteBlockerCode::invalid_plot;
        return result;
    }
    const auto &plot = grid.at(index);
    if (plot.area_id < 0 || plot.landmass_id < 0) {
        result.blocker = BnwFreshRiverWriteBlockerCode::unassigned_area;
        return result;
    }
    // AddRivers calls this only on a fresh, dry edge. Other transitions need
    // the full live CvPlot setter and are outside this selected setup path.
    const bool valid_flow =
        (edge == BnwFreshRiverEdge::north_east &&
         (flow_direction == 5 || flow_direction == 2) && !plot.ne_of_river) ||
        (edge == BnwFreshRiverEdge::west &&
         (flow_direction == 0 || flow_direction == 3) && !plot.w_of_river) ||
        (edge == BnwFreshRiverEdge::north_west &&
         (flow_direction == 1 || flow_direction == 4) && !plot.nw_of_river);
    if (!enabled || plot.plot_type == 3 || !valid_flow)
        result.blocker = BnwFreshRiverWriteBlockerCode::outside_fresh_stock_contract;
    return result;
}

BnwFreshRiverCrossingDraft BnwSetupPlotGrid::draft_fresh_river_crossings(
    std::size_t index, BnwFreshRiverEdge edge,
    std::int32_t flow_direction) const {
    if (inspect_bnw_fresh_river_write(*this, index, edge, true, flow_direction)
            .blocker != BnwFreshRiverWriteBlockerCode::required_services_pending)
        throw std::invalid_argument("Fresh BNW river crossing draft is outside selected AddRivers contract");
    BnwFreshRiverCrossingDraft draft{*this, {},
        {"selected_terrain_river_yield_and_symbols", "area_river_edge_count",
         "river_flow_direction_state", "fresh_river_state_persistence"},
        edge, flow_direction};
    auto &target = draft.grid.plots_[index];
    switch (edge) {
    case BnwFreshRiverEdge::north_east: target.ne_of_river = true; break;
    case BnwFreshRiverEdge::west: target.w_of_river = true; break;
    case BnwFreshRiverEdge::north_west: target.nw_of_river = true; break;
    }
    using Kind = BnwFreshRiverDraftEventKind;
    draft.source_order.push_back({Kind::edge_bit_written, index});
    const auto update_crossings = [&](std::size_t plot_index) {
        auto &plot = draft.grid.plots_[plot_index];
        const auto neighbors = draft.grid.adjacent(plot_index);
        for (std::size_t direction = 0; direction < neighbors.size(); ++direction) {
            const auto neighbor = neighbors[direction];
            bool crossing = false;
            if (!neighbor || (plot.plot_type != 3 &&
                              draft.grid.plots_[*neighbor].plot_type != 3)) {
                switch (direction) {
                case 0: crossing = neighbor && draft.grid.plots_[*neighbor].ne_of_river; break;
                case 1: crossing = plot.w_of_river; break;
                case 2: crossing = plot.nw_of_river; break;
                case 3: crossing = plot.ne_of_river; break;
                case 4: crossing = neighbor && draft.grid.plots_[*neighbor].w_of_river; break;
                case 5: crossing = neighbor && draft.grid.plots_[*neighbor].nw_of_river; break;
                }
            }
            const auto bit = static_cast<std::uint8_t>(1U << direction);
            const bool old = (plot.river_crossing_mask & bit) != 0;
            if (old != crossing) {
                if (crossing) plot.river_crossing_mask |= bit;
                else plot.river_crossing_mask &= static_cast<std::uint8_t>(~bit);
                plot.river_crossing_count += crossing ? 1 : -1;
            }
            if (plot.river_crossing_count !=
                std::popcount(static_cast<unsigned>(plot.river_crossing_mask)))
                throw std::invalid_argument("Fresh BNW river crossing cache is inconsistent");
            draft.source_order.push_back({Kind::crossing_checked, plot_index,
                static_cast<std::int8_t>(direction), crossing});
        }
    };
    update_crossings(index);
    draft.source_order.push_back({Kind::yield_pending, index});
    // CvPlot::set*OfRiver invokes both callbacks on each non-null
    // plotDirection result, including duplicates on a wrapped tiny map.
    for (const auto neighbor : draft.grid.adjacent(index)) {
        if (!neighbor) continue;
        update_crossings(*neighbor);
        draft.source_order.push_back({Kind::yield_pending, *neighbor});
    }
    draft.source_order.push_back({Kind::area_river_edge_count_pending, index});
    draft.source_order.push_back({Kind::flow_direction_pending, index});
    return draft;
}

} // namespace civ5::runtime
