#include "civ5/runtime/bnw_fresh_start_coast.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::string_view stock_mapmaker_sha256 =
    "ea463458f9fde6571ce2269c2ebf994f58d8f5b9859201bc506703d176c03f47";
bool selected_stock_mapmaker(
    std::span<const BnwMapIncludedSource> includes) {
    bool found = false;
    for (const auto &source : includes) {
        if (source.name != "MapmakerUtilities" &&
            source.name != "MapmakerUtilities.lua") continue;
        if (source.sha256 != stock_mapmaker_sha256) return false;
        found = true;
    }
    return found;
}
void block(BnwFreshStartCoastTables &result, BnwStartCoastBlockerCode code,
           std::string message) {
    result.blockers.push_back({code, std::move(message)});
}

bool complete_owned_grid(const BnwSetupPlotGrid &grid,
                         const BnwFreshAreaStructure &areas,
                         std::uint64_t maximum_plots,
                         std::int32_t &width, std::int32_t &height,
                         std::unordered_map<std::int32_t,
                                            const BnwFreshLandmass *> &landmasses) {
    const auto count = grid.size();
    if (!count || count > maximum_plots ||
        areas.area_ids.size() != count || areas.landmass_ids.size() != count ||
        areas.areas.empty() || areas.landmasses.empty())
        return false;
    const auto &last = grid.at(count - 1);
    if (last.x < 0 || last.y < 0 ||
        last.x == std::numeric_limits<std::int32_t>::max() ||
        last.y == std::numeric_limits<std::int32_t>::max())
        return false;
    width = last.x + 1;
    height = last.y + 1;
    if (static_cast<std::uint64_t>(width) * height != count) return false;
    std::unordered_map<std::int32_t, const BnwFreshArea *> area_by_id;
    area_by_id.reserve(areas.areas.size());
    landmasses.reserve(areas.landmasses.size());
    for (const auto &area : areas.areas)
        if (area.id < 0 || !area_by_id.emplace(area.id, &area).second)
            return false;
    for (const auto &landmass : areas.landmasses)
        if (landmass.id < 0 || !landmasses.emplace(landmass.id, &landmass).second ||
            (landmass.lake && !landmass.water))
            return false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto &plot = grid.at(i);
        if (plot.x != static_cast<std::int32_t>(i % width) ||
            plot.y != static_cast<std::int32_t>(i / width) ||
            !plot.plot_type_assigned_by_script ||
            !plot.terrain_type_assigned_by_script || plot.terrain_type < 0 ||
            plot.plot_type < 0 || plot.plot_type > 3 ||
            plot.area_id != areas.area_ids[i] ||
            plot.landmass_id != areas.landmass_ids[i] ||
            plot.area_id < 0 || plot.landmass_id < 0)
            return false;
        const auto landmass = landmasses.find(plot.landmass_id);
        if (landmass == landmasses.end() ||
            landmass->second->water != (plot.plot_type == 3))
            return false;
        const auto area = area_by_id.find(plot.area_id);
        if (area == area_by_id.end() ||
            area->second->water != (plot.plot_type == 3))
            return false;
    }
    return true;
}
} // namespace

bool BnwFreshStartCoastTables::has_blocker(BnwStartCoastBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshStartCoastTables::rollback() noexcept {
    coastal.clear();
    next_to_coast.clear();
    queries.clear();
    coastal_tables_complete = false;
}

BnwFreshStartCoastTables draft_bnw_fresh_start_coast_tables(
    const BnwFreshGameContentResult &selected,
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    const SimulationRandom &map_random,
    std::span<const BnwMapIncludedSource> selected_includes,
    std::string_view map_selected_rules_sha256,
    bool post_feature_area_recalculation_complete,
    std::uint64_t maximum_plots) {
    BnwFreshStartCoastTables result;
    result.map_random_before = map_random.state();
    result.map_random_after = map_random.state();
    if (!selected.content_ready() || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        map_random.algorithm() != RandomAlgorithm::bnw ||
        map_selected_rules_sha256.empty()) {
        block(result, BnwStartCoastBlockerCode::selected_rules_unverified,
              "Selected BNW rules, profile, map RNG and source identity are required");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != map_selected_rules_sha256) {
        block(result, BnwStartCoastBlockerCode::selected_rules_unverified,
              "Map and starting plots must use the same activated BNW rules");
        return result;
    }
    if (!selected_stock_mapmaker(selected_includes)) {
        block(result, BnwStartCoastBlockerCode::stock_helper_unverified,
              "Selected MapmakerUtilities helper differs from verified Expansion2 stock bytes");
        return result;
    }
    if (!post_feature_area_recalculation_complete) {
        block(result, BnwStartCoastBlockerCode::post_feature_areas_pending,
              "MapGenerator recalculates areas after AddFeatures before StartPlotSystem");
        return result;
    }
    std::int32_t width = 0, height = 0;
    std::unordered_map<std::int32_t, const BnwFreshLandmass *> landmasses;
    if (!complete_owned_grid(grid, areas, maximum_plots, width, height,
                             landmasses)) {
        block(result, BnwStartCoastBlockerCode::owned_map_incomplete,
              "Coastal tables require fully assigned post-feature plot and landmass handles");
        return result;
    }

    const auto count = grid.size();
    result.coastal.assign(count, false);
    result.next_to_coast.assign(count, false);
    // MapmakerUtilities.lua: x outer, y inner. AdjacentToSaltWater short
    // circuits on the first non-lake ocean neighbor in DirectionTypes order.
    for (std::int32_t x = 0; x < width; ++x) {
        for (std::int32_t y = 0; y < height; ++y) {
            const auto index = static_cast<std::size_t>(y) * width + x;
            if (grid.at(index).plot_type == 3) continue;
            const auto adjacent = grid.adjacent(index);
            for (std::uint8_t direction = 0; direction < 6; ++direction) {
                const auto neighbor = adjacent[direction];
                result.queries.push_back({BnwStartCoastQueryStage::adjacent_to_salt_water,
                                          index, direction, neighbor});
                if (!neighbor || grid.at(*neighbor).plot_type != 3) continue;
                const auto landmass = landmasses.find(
                    grid.at(*neighbor).landmass_id);
                if (landmass != landmasses.end() && !landmass->second->lake) {
                    result.coastal[index] = true;
                    break;
                }
            }
        }
    }
    // Second source loop fetches all six Map.PlotDirection handles first,
    // then checks each against the coastal table, without short circuiting.
    for (std::int32_t x = 0; x < width; ++x) {
        for (std::int32_t y = 0; y < height; ++y) {
            const auto index = static_cast<std::size_t>(y) * width + x;
            if (result.coastal[index] || grid.at(index).plot_type == 3) continue;
            const auto adjacent = grid.adjacent(index);
            for (std::uint8_t direction = 0; direction < 6; ++direction) {
                const auto neighbor = adjacent[direction];
                result.queries.push_back({BnwStartCoastQueryStage::next_to_coastal_land,
                                          index, direction, neighbor});
                if (neighbor && result.coastal[*neighbor])
                    result.next_to_coast[index] = true;
            }
        }
    }
    result.coastal_tables_complete = true;
    block(result, BnwStartCoastBlockerCode::start_placement_pending,
          "AssignStartingPlots resource, region, location and city-state stages remain pending");
    return result;
}
} // namespace civ5::runtime
