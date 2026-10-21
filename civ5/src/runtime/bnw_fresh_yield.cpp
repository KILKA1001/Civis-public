#include "civ5/runtime/bnw_fresh_yield.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
std::int32_t source_int(std::int64_t value) {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("Selected BNW yield value exceeds GameCore int32 range");
    return static_cast<std::int32_t>(value);
}
} // namespace

BnwFreshYieldRules load_bnw_fresh_yield_rules(
    const content::RulesDatabase &selected_rules, const BnwSetupPlotGrid &grid,
    std::span<const std::int32_t> additional_terrain_ids) {
    BnwFreshYieldRules rules;
    rules.source_rules = &selected_rules;
    constexpr std::array<std::string_view, 6> yield_names{
        "YIELD_FOOD", "YIELD_PRODUCTION", "YIELD_GOLD", "YIELD_SCIENCE",
        "YIELD_CULTURE", "YIELD_FAITH"};
    const auto yields = selected_rules.prefetch_info("Yields",
        [&](std::uint64_t slot, std::int64_t authored_id,
            const content::RulesRowView &row) {
            if (slot >= rules.yields.size() || authored_id != static_cast<std::int64_t>(slot) ||
                row.text("Type") != std::optional<std::string>(
                    std::string(yield_names[slot])))
                throw std::runtime_error("Selected BNW yields differ from Expansion2 YieldTypes");
            rules.yields[slot] = {source_int(row.integer("HillsChange")),
                                  source_int(row.integer("LakeChange"))};
            return true;
        });
    if (!yields.all_succeeded() || yields.slots != rules.yields.size())
        throw std::runtime_error("Selected BNW six Yield Info rows are incomplete: " +
                                 yields.error);

    std::set<std::int32_t> used;
    for (std::size_t i = 0; i < grid.size(); ++i) {
        const auto &plot = grid.at(i);
        if (!plot.plot_type_assigned_by_script ||
            !plot.terrain_type_assigned_by_script || plot.terrain_type < 0)
            throw std::runtime_error("Fresh BNW yield rules require complete plot and terrain state");
        used.insert(plot.terrain_type);
    }
    for (const auto id : additional_terrain_ids) {
        if (id < 0) throw std::invalid_argument("Fresh BNW additional terrain ID is invalid");
        used.insert(id);
    }
    for (const auto terrain_id : used) {
        std::optional<std::string> type;
        const auto terrain = selected_rules.query_rows(
            {"SELECT ROWID AS SourceRow, ID, Type FROM Terrains WHERE ID = ? ORDER BY ROWID",
             {static_cast<std::int64_t>(terrain_id)}},
            [&](const content::RulesRowView &row) {
                if (type || row.integer("SourceRow") != terrain_id ||
                    row.integer("ID") != terrain_id)
                    throw std::runtime_error("Selected BNW terrain Info identity is ambiguous");
                type = row.text("Type");
            }, 2);
        if (!terrain.all_succeeded() || !type || type->empty())
            throw std::runtime_error("Selected BNW terrain yield Info is missing: " +
                                     terrain.error);
        auto &target = rules.terrains[terrain_id];
        const auto relation = [&](std::string_view table,
                                  std::array<std::int32_t, 6> &values) {
            std::array<bool, 6> seen{};
            const auto sql = "SELECT Yields.ID AS YieldID, Relation.Yield AS Amount "
                "FROM " + std::string(table) + " AS Relation JOIN Yields "
                "ON Relation.YieldType = Yields.Type "
                "WHERE Relation.TerrainType = ? ORDER BY Relation.ROWID";
            const auto rows = selected_rules.query_rows({sql, {*type}},
                [&](const content::RulesRowView &row) {
                    const auto id = row.integer("YieldID");
                    if (id < 0 || id >= static_cast<std::int64_t>(values.size()) ||
                        seen[static_cast<std::size_t>(id)])
                        throw std::runtime_error("Selected BNW terrain yield relation is ambiguous");
                    seen[static_cast<std::size_t>(id)] = true;
                    values[static_cast<std::size_t>(id)] = source_int(row.integer("Amount"));
                });
            if (!rows.all_succeeded())
                throw std::runtime_error("Selected BNW terrain yield relation failed: " +
                                         rows.error);
        };
        relation("Terrain_Yields", target.base);
        relation("Terrain_HillsYieldChanges", target.hills);
        relation("Terrain_RiverYieldChanges", target.river);
    }
    return rules;
}

namespace {
std::map<std::int32_t, bool> validate_yield_grid(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas) {
    if (grid.size() == 0 || areas.area_ids.size() != grid.size() ||
        areas.landmass_ids.size() != grid.size() ||
        areas.area_assignment_order.size() != grid.size() ||
        areas.landmass_assignment_order.size() != grid.size() ||
        areas.area_assignment_yield_calls != grid.size() ||
        !areas.final_landmass_full_yield_pass_pending)
        throw std::invalid_argument("Fresh BNW yield pass requires complete areas and landmasses");
    std::map<std::int32_t, bool> lakes;
    for (const auto &landmass : areas.landmasses)
        if (!lakes.emplace(landmass.id, landmass.lake).second ||
            landmass.tiles <= 0 || (landmass.lake && !landmass.water))
            throw std::invalid_argument("Fresh BNW landmass identity is invalid");
    for (std::size_t i = 0; i < grid.size(); ++i) {
        const auto &plot = grid.at(i);
        if (areas.area_ids[i] < 0 || !lakes.contains(areas.landmass_ids[i]) ||
            !plot.plot_type_assigned_by_script ||
            !plot.terrain_type_assigned_by_script || plot.terrain_type < 0)
            throw std::invalid_argument("Fresh BNW plot is incomplete for final yield pass");
    }
    return lakes;
}

std::array<std::int16_t, 6> calculate_cell(
    const BnwSetupPlot &plot, bool lake, const BnwFreshYieldRules &rules) {
    std::array<std::int16_t, 6> cache{};
    if (!plot.potential_city_work || plot.impassable || plot.plot_type == 0)
        return cache; // CvPlot::calculateYield / calculateNatureYield guards.
    const auto &terrain = rules.terrains.at(plot.terrain_type);
    const bool hills = plot.plot_type == 1;
    for (std::size_t y = 0; y < cache.size(); ++y) {
        // Fresh CvPlot::reset has no owner, city, route, improvement,
        // feature, resource or plot-extra-yield entries. The river branch
        // becomes active only in a detached AddRivers draft.
        const std::int64_t raw = static_cast<std::int64_t>(terrain.base[y]) +
            (hills ? static_cast<std::int64_t>(rules.yields[y].hills) +
                     terrain.hills[y] : 0) +
            (lake ? rules.yields[y].lake : 0) +
            (plot.river_crossing_count > 0 ? terrain.river[y] : 0);
        const auto clamped = std::max<std::int64_t>(0, raw);
        if (clamped > std::numeric_limits<std::int16_t>::max())
            throw std::runtime_error("Fresh BNW yield exceeds CvPlot short cache range");
        cache[y] = static_cast<std::int16_t>(clamped);
    }
    return cache;
}
} // namespace

std::array<std::int16_t, 6> calculate_bnw_fresh_plot_yields(
    const BnwSetupPlot &plot, bool lake, const BnwFreshYieldRules &rules) {
    if (!rules.source_rules || plot.area_id < 0 || plot.terrain_type < 0 ||
        !plot.plot_type_assigned_by_script ||
        !plot.terrain_type_assigned_by_script ||
        !rules.terrains.contains(plot.terrain_type))
        throw std::invalid_argument("Fresh BNW plot yield has no selected area or terrain rules");
    return calculate_cell(plot, lake, rules);
}

std::vector<std::array<std::int16_t, 6>> calculate_bnw_fresh_final_yields(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    const BnwFreshYieldRules &rules) {
    const auto lakes = validate_yield_grid(grid, areas);
    std::vector<std::array<std::int16_t, 6>> cache(grid.size());
    for (std::size_t i = 0; i < grid.size(); ++i)
        cache[i] = calculate_cell(grid.at(i), lakes.at(areas.landmass_ids[i]), rules);
    return cache;
}

BnwFreshYieldReplay replay_bnw_fresh_yield_passes(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    const BnwFreshYieldRules &rules, std::uint64_t maximum_plot_visits,
    std::uint64_t maximum_symbol_events) {
    const auto final_lakes = validate_yield_grid(grid, areas);
    BnwFreshYieldReplay replay;
    replay.final_cache.resize(grid.size()); // CvPlot::reset initializes all yields to zero.
    if (maximum_plot_visits < grid.size())
        throw std::length_error("Fresh BNW yield replay exceeds plot-visit budget");
    const auto stage_symbol = [&](BnwFreshYieldSymbolPhase phase,
                                  std::size_t index, std::int32_t landmass_id) {
        if (replay.symbol_events.size() >= maximum_symbol_events)
            throw std::length_error("Fresh BNW updateSymbols event budget exceeded");
        replay.symbol_events.push_back({phase, static_cast<std::uint32_t>(index),
                                        landmass_id, replay.final_cache[index]});
    };
    // CvMap::calculateAreas invokes CvPlot::setArea exactly once per plot.
    // Landmass IDs are all invalid here, so the six yield values are
    // independent of the A* notification order even though the callbacks
    // emitted by updateSymbols are not.
    for (const auto index : areas.area_assignment_order) {
        const auto next = calculate_cell(grid.at(index), false, rules);
        if (next != replay.final_cache.at(index)) {
            replay.final_cache[index] = next;
            ++replay.progress.set_area_changed_plots;
            stage_symbol(BnwFreshYieldSymbolPhase::set_area, index, -1);
        }
    }
    replay.progress.plot_visits = grid.size();
    const auto full_pass = [&](const std::set<std::int32_t> &lake_ids,
                               BnwFreshYieldSymbolPhase phase,
                               std::int32_t trigger_landmass_id) {
        if (grid.size() > maximum_plot_visits - replay.progress.plot_visits)
            throw std::length_error("Fresh BNW yield replay exceeds plot-visit budget");
        std::uint64_t changed = 0;
        for (std::size_t i = 0; i < grid.size(); ++i) {
            const auto next = calculate_cell(grid.at(i),
                lake_ids.contains(areas.landmass_ids[i]), rules);
            if (next != replay.final_cache[i]) {
                replay.final_cache[i] = next;
                ++changed;
                stage_symbol(phase, i, trigger_landmass_id);
            }
        }
        replay.progress.plot_visits += grid.size();
        return changed;
    };
    std::set<std::int32_t> completed_lakes;
    std::uint64_t threshold_pass_count = 0;
    // CvMap::calculateLandmasses scans component seeds in plot-index order.
    // Each component is completed before the next seed. On a large water
    // component, its only lake transition is to nonlake, so the full-map
    // pass can change yields only on completed earlier small lakes.
    for (const auto &landmass : areas.landmasses) {
        if (landmass.lake_threshold_yield_passes > 1 ||
            (landmass.lake_threshold_yield_passes &&
             (!landmass.water || landmass.lake)))
            throw std::invalid_argument("Fresh BNW lake transition metadata is invalid");
        if (landmass.lake_threshold_yield_passes) {
            replay.progress.lake_threshold_passes.push_back(
                {landmass.id, full_pass(completed_lakes,
                    BnwFreshYieldSymbolPhase::lake_threshold, landmass.id)});
            ++threshold_pass_count;
        }
        if (landmass.lake) completed_lakes.insert(landmass.id);
    }
    if (threshold_pass_count != areas.lake_threshold_full_yield_passes)
        throw std::invalid_argument("Fresh BNW lake transition count differs from area structure");
    for (const auto &[id, lake] : final_lakes)
        if (completed_lakes.contains(id) != lake)
            throw std::invalid_argument("Fresh BNW final lake flags differ from area structure");
    replay.progress.final_pass_changed_plots = full_pass(
        completed_lakes, BnwFreshYieldSymbolPhase::final_map, -1);
    return replay;
}

BnwFreshRiverCrossingDraft replay_bnw_fresh_river_yields(
    const BnwFreshRiverCrossingDraft &crossings,
    const BnwFreshAreaStructure &areas, const BnwFreshYieldRules &rules,
    std::uint64_t maximum_symbol_events) {
    if (!rules.source_rules)
        throw std::invalid_argument("Fresh BNW river yields require activated selected RulesDatabase identity");
    const auto lakes = validate_yield_grid(crossings.grid, areas);
    if (crossings.grid.symbol_events_.size() > maximum_symbol_events)
        throw std::length_error("Fresh BNW river symbols exceed selected event budget");
    BnwFreshRiverCrossingDraft draft = crossings;
    std::vector<BnwFreshRiverDraftEvent> ordered;
    ordered.reserve(crossings.source_order.size() + 7);
    for (const auto &event : crossings.source_order) {
        if (event.kind != BnwFreshRiverDraftEventKind::yield_pending) {
            ordered.push_back(event);
            continue;
        }
        if (event.plot_index >= draft.grid.size() ||
            draft.grid.plots_[event.plot_index].area_id != areas.area_ids[event.plot_index] ||
            draft.grid.plots_[event.plot_index].landmass_id !=
                areas.landmass_ids[event.plot_index])
            throw std::invalid_argument("Fresh BNW river yield area identity changed");
        auto &plot = draft.grid.plots_[event.plot_index];
        const auto next = calculate_cell(plot, lakes.at(plot.landmass_id), rules);
        ordered.push_back({BnwFreshRiverDraftEventKind::yield_recalculated,
                           event.plot_index});
        if (next == plot.yields) continue;
        if (draft.grid.symbol_events_.size() >= maximum_symbol_events)
            throw std::length_error("Fresh BNW river updateSymbols event budget exceeded");
        plot.yields = next;
        draft.grid.symbol_events_.push_back({BnwFreshYieldSymbolPhase::river_write,
            static_cast<std::uint32_t>(event.plot_index), -1, next});
        ordered.push_back({BnwFreshRiverDraftEventKind::symbols_staged,
                           event.plot_index});
    }
    draft.source_order = std::move(ordered);
    draft.remaining_services = {"area_river_edge_count", "river_flow_direction_state",
                                "fresh_river_state_persistence"};
    return draft;
}

BnwFreshRiverAreaDraft replay_bnw_fresh_river_area_count(
    const BnwFreshRiverCrossingDraft &yielded,
    const BnwFreshAreaStructure &selected_areas, std::size_t source_plot_index) {
    using Kind = BnwFreshRiverDraftEventKind;
    if (source_plot_index >= yielded.grid.size() ||
        selected_areas.area_ids.size() != yielded.grid.size() ||
        selected_areas.landmass_ids.size() != yielded.grid.size() ||
        yielded.source_order.size() < 3 ||
        yielded.source_order.front().kind != Kind::edge_bit_written ||
        yielded.source_order.front().plot_index != source_plot_index ||
        yielded.source_order[yielded.source_order.size() - 2] !=
            BnwFreshRiverDraftEvent{Kind::area_river_edge_count_pending,
                                    source_plot_index} ||
        yielded.source_order.back() !=
            BnwFreshRiverDraftEvent{Kind::flow_direction_pending,
                                    source_plot_index} ||
        yielded.remaining_services != std::vector<std::string_view>{
            "area_river_edge_count", "river_flow_direction_state",
            "fresh_river_state_persistence"})
        throw std::invalid_argument("Fresh BNW river area draft has incomplete source order");
    std::size_t area_markers = 0;
    for (const auto &event : yielded.source_order) {
        if (event.kind == Kind::yield_pending ||
            event.kind == Kind::area_river_edge_count_applied)
            throw std::invalid_argument("Fresh BNW river area draft has an unfinished phase");
        if (event.kind == Kind::area_river_edge_count_pending)
            ++area_markers;
    }
    if (area_markers != 1)
        throw std::invalid_argument("Fresh BNW river area draft has ambiguous area phase");

    const auto &source = yielded.grid.at(source_plot_index);
    if (source.plot_type == 3 || source.area_id < 0 ||
        !(source.ne_of_river || source.w_of_river || source.nw_of_river))
        throw std::invalid_argument("Fresh BNW river source plot has no selected edge or area");
    for (std::size_t i = 0; i < yielded.grid.size(); ++i) {
        const auto &plot = yielded.grid.at(i);
        if (plot.area_id != selected_areas.area_ids[i] ||
            plot.landmass_id != selected_areas.landmass_ids[i])
            throw std::invalid_argument("Fresh BNW river area identity changed");
    }
    BnwFreshRiverAreaDraft draft{yielded, selected_areas};
    BnwFreshArea *source_area = nullptr;
    for (auto &area : draft.areas.areas) {
        if (area.id != source.area_id) continue;
        if (source_area || area.water || area.tiles <= 0 ||
            area.river_edges < 0 ||
            area.river_edges == std::numeric_limits<std::int32_t>::max())
            throw std::invalid_argument("Fresh BNW river source area count is invalid");
        source_area = &area;
    }
    if (!source_area)
        throw std::invalid_argument("Fresh BNW river source area is missing");
    // CvArea::changeNumRiverEdges(+1) occurs after self and all present
    // neighbor yield callbacks, before CvPlot writes its flow direction.
    ++source_area->river_edges;
    draft.river.source_order[draft.river.source_order.size() - 2].kind =
        Kind::area_river_edge_count_applied;
    draft.river.remaining_services = {"river_flow_direction_state",
                                      "fresh_river_state_persistence"};
    return draft;
}

BnwFreshRiverAreaDraft replay_bnw_fresh_river_flow_direction(
    const BnwFreshRiverAreaDraft &counted) {
    using Kind = BnwFreshRiverDraftEventKind;
    const auto &source_order = counted.river.source_order;
    if (source_order.size() < 3 ||
        source_order.front().kind != Kind::edge_bit_written ||
        source_order[source_order.size() - 2].kind !=
            Kind::area_river_edge_count_applied ||
        source_order.back().kind != Kind::flow_direction_pending ||
        source_order.front().plot_index != source_order.back().plot_index ||
        source_order[source_order.size() - 2].plot_index !=
            source_order.back().plot_index ||
        counted.river.remaining_services != std::vector<std::string_view>{
            "river_flow_direction_state", "fresh_river_state_persistence"})
        throw std::invalid_argument("Fresh BNW river flow draft has incomplete source order");
    const auto source_index = source_order.back().plot_index;
    if (source_index >= counted.river.grid.size() ||
        counted.areas.area_ids.size() != counted.river.grid.size())
        throw std::invalid_argument("Fresh BNW river flow draft has invalid plot handles");
    const auto &source = counted.river.grid.at(source_index);
    if (source.area_id < 0 ||
        source.area_id != counted.areas.area_ids[source_index])
        throw std::invalid_argument("Fresh BNW river flow source area changed");
    const auto area = std::find_if(counted.areas.areas.begin(),
        counted.areas.areas.end(), [&](const auto &item) {
            return item.id == source.area_id;
        });
    if (area == counted.areas.areas.end() || area->river_edges <= 0)
        throw std::invalid_argument("Fresh BNW river flow requires counted source area");

    bool edge_written = false;
    std::int8_t old_flow = -1;
    bool valid_flow = false;
    const auto flow = counted.river.requested_flow_direction;
    switch (counted.river.edge) {
    case BnwFreshRiverEdge::north_east:
        edge_written = source.ne_of_river;
        old_flow = source.river_sw_flow_direction;
        valid_flow = flow == 5 || flow == 2;
        break;
    case BnwFreshRiverEdge::west:
        edge_written = source.w_of_river;
        old_flow = source.river_e_flow_direction;
        valid_flow = flow == 0 || flow == 3;
        break;
    case BnwFreshRiverEdge::north_west:
        edge_written = source.nw_of_river;
        old_flow = source.river_se_flow_direction;
        valid_flow = flow == 1 || flow == 4;
        break;
    }
    if (!edge_written || old_flow != -1 || !valid_flow)
        throw std::invalid_argument("Fresh BNW river flow is outside dry-edge SDK contract");
    BnwFreshRiverAreaDraft draft = counted;
    auto &target = draft.river.grid.plots_[source_index];
    const auto stored = static_cast<std::int8_t>(flow);
    switch (draft.river.edge) {
    case BnwFreshRiverEdge::north_east:
        target.river_sw_flow_direction = stored; break;
    case BnwFreshRiverEdge::west:
        target.river_e_flow_direction = stored; break;
    case BnwFreshRiverEdge::north_west:
        target.river_se_flow_direction = stored; break;
    }
    draft.river.source_order.back().kind = Kind::flow_direction_applied;
    draft.river.remaining_services = {"fresh_river_state_atomic_handoff"};
    return draft;
}

} // namespace civ5::runtime
