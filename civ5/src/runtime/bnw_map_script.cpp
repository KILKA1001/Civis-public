#include "civ5/runtime/bnw_map_script.hpp"
#include "civ5/runtime/bnw_selected_map_init.hpp"
#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include "civ5/runtime/bnw_fresh_terrain.hpp"
#include "civ5/runtime/bnw_fresh_yield.hpp"
#include "civ5/runtime/bnw_fresh_feature_setter.hpp"
#include "civ5/runtime/bnw_fractal.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <algorithm>
#include <bit>
#include <memory>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace civ5::runtime {
// Private scoped provider-job access. No source descriptor or VM escapes.
struct BnwMapScriptSelectedAccess {
 static std::shared_ptr<BnwSelectedMapSourceOwner> make(
  std::shared_ptr<const BnwFreshGameContentResult> parent,assets::AssetHandle root,
  std::vector<std::pair<std::string,assets::AssetHandle>> includes,
  const assets::ResourceMountTable& resources,const BnwMapSetupContext& setup,const SimulationRandom& random,std::uint64_t limit){
   const auto epoch=BnwFreshGameContentResult::retain_epoch(parent);
   return std::shared_ptr<BnwSelectedMapSourceOwner>(new BnwSelectedMapSourceOwner(
    std::move(parent),std::move(root),std::move(includes),resources,setup,random,limit,epoch));
 }
 static std::shared_ptr<BnwSelectedMapSourceOwner> make_epoch(
   const std::shared_ptr<BnwFreshGameContentResult::ContentEpoch>& epoch,assets::AssetHandle root,
   std::vector<std::pair<std::string,assets::AssetHandle>> includes,const assets::ResourceMountTable& resources,
   const BnwMapSetupContext& setup,const SimulationRandom& random,std::uint64_t limit){
   epoch->require_current();auto stable=std::shared_ptr<const BnwFreshGameContentResult>(epoch,&epoch->view);
   return std::shared_ptr<BnwSelectedMapSourceOwner>(new BnwSelectedMapSourceOwner(
    std::move(stable),std::move(root),std::move(includes),resources,setup,random,limit,epoch));
 }
 static void begin(BnwSelectedMapSourceOwner& owner,std::string_view bytes){owner.begin_load(bytes);}
 static void retire(BnwSelectedMapSourceOwner& owner)noexcept{owner.retire();}
 static void same_job(BnwSelectedMapSourceOwner& owner,const assets::ResourceMountTable& resources,
  const BnwMapSetupContext& setup,const SimulationRandom& random){owner.require_same_provider_operands(resources,setup,random);}
 static void loaded(BnwSelectedMapSourceOwner& owner,std::string_view bytes){owner.loaded(bytes);}
 static void include(BnwSelectedMapSourceOwner& owner,std::string_view name,
  std::span<const std::uint8_t> bytes){owner.require_include_bytes(name,bytes);}
 static ScriptMapInitQueryResult query(BnwSelectedMapSourceOwner& owner,
  LuaScriptHost& host,std::int32_t world){return owner.query(host,world);}
};
namespace {
struct BnwSelectedMapInitJob {
 std::shared_ptr<const BnwFreshGameContentResult> parent;
 assets::AssetHandle root;
 const assets::ResourceMountTable* resources;
 std::optional<ScriptMapInitQueryResult> init;
};
void block(BnwMapScriptResult &result, BnwMapScriptBlockerCode code,
           std::string message) {
    result.blockers.push_back({code, std::move(message)});
}

std::int64_t read_integer(LuaScriptHost &host, std::string_view event,
                          std::span<const ScriptValue> args = {}) {
    const auto result = host.accumulate_int(event, args);
    if (!result.errors.empty()) throw ScriptError(result.errors.front());
    if (!result.value) throw ScriptError("Map script returned no integer for " + std::string(event));
    return *result.value;
}
bool read_boolean(LuaScriptHost &host, std::string_view event) {
    const auto result = host.test_all(event);
    if (!result.errors.empty()) throw ScriptError(result.errors.front());
    if (!result.value) throw ScriptError("Map script returned no boolean for " + std::string(event));
    return *result.value;
}
struct BnwIncludeManifest {
    std::map<std::string, assets::AssetHandle> handles;
    std::vector<std::string> names;
    std::vector<BnwMapIncludedSource> trace;
};
bool include_path(std::string_view path) {
    const auto normalized = assets::normalize_asset_name(std::string(path));
    return (normalized.starts_with("assets/gameplay/lua/") ||
            normalized.starts_with("gameplay/lua/")) && normalized.ends_with(".lua");
}
bool discover_includes(const assets::ResourceMountTable &resources,
                       BnwIncludeManifest &manifest, BnwMapScriptResult &result) {
    std::set<std::string> names;
    for (const auto &path : resources.keys())
        if (include_path(path)) names.insert(assets::resource_basename(path));
    for (const auto &name : names) {
        const assets::AssetKey request(name);
        auto selected = resources.resolve(request);
        if (std::holds_alternative<assets::AmbiguousAsset>(selected.asset)) {
            block(result, BnwMapScriptBlockerCode::include_ambiguous,
                  "Selected map helper is ambiguous in active VFS: " + name);
            return false;
        }
        auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
        if (!handle || !include_path(handle->candidate().provenance.source_relative_path)) {
            block(result, BnwMapScriptBlockerCode::include_ambiguous,
                  "Active VFS resolves map helper outside Gameplay/Lua: " + name);
            return false;
        }
        std::size_t same_mount = 0;
        for (const auto &candidate : resources.resolve_all(request)) {
            const auto *other = std::get_if<assets::AssetHandle>(&candidate.asset);
            if (candidate.mount_id == selected.mount_id && other &&
                include_path(other->candidate().provenance.source_relative_path)) ++same_mount;
        }
        if (same_mount != 1) {
            block(result, BnwMapScriptBlockerCode::include_ambiguous,
                  "Several map helpers share a name within one selected VFS mount: " + name);
            return false;
        }
        manifest.names.push_back(name);
        manifest.handles.emplace(name, std::move(*handle));
    }
    return true;
}
bool load_selected_worlds(const BnwMapSetupContext &setup,
                          BnwMapScriptResult &result,
                          std::vector<ScriptMapWorldRow> &worlds) {
    const auto &selected = *setup.selected_rules;
    constexpr std::string_view expansion2_id = "6da07636-4123-4018-b643-6575b4ec336b";
    bool expansion2 = false;
    try {
        expansion2 = std::any_of(selected.profile.requirements.begin(),
                                 selected.profile.requirements.end(),
                                 [&](const ContentRequirement &requirement) {
            return requirement.kind == content::PackageKind::dlc &&
                   requirement.status == RequirementStatus::located &&
                   requirement.package &&
                   std::find(selected.profile.enabled_dlc.begin(),
                             selected.profile.enabled_dlc.end(), *requirement.package) !=
                       selected.profile.enabled_dlc.end() &&
                   content::canonical_guid(requirement.id) == expansion2_id;
        });
    } catch (const std::exception &error) {
        block(result, BnwMapScriptBlockerCode::invalid_world_rules,
              std::string("Invalid selected BNW package identity: ") + error.what());
        return false;
    }
    if (!selected.content_ready() || !selected.profile.can_activate || !selected.activation ||
        selected.activation->profile_kind != selected.profile.kind ||
        selected.profile.kind == ContentProfileKind::vanilla || !expansion2 ||
        !setup.world_size) {
        block(result, BnwMapScriptBlockerCode::invalid_world_rules,
              "Map GameInfo.Worlds requires activated rules from the selected BNW profile and world size");
        return false;
    }
    try {
        std::set<std::int64_t> ids, rowids;
        std::set<std::string> types;
        const auto report = selected.rules->query_rows(
            {"SELECT ROWID AS CivisRowId, ID, Type, FeatureGrainChange FROM Worlds ORDER BY ROWID", {}},
            [&](const content::RulesRowView &row) {
                if (!row.has_column("CivisRowId") || !row.has_column("ID") ||
                    !row.has_column("Type") || !row.has_column("FeatureGrainChange"))
                    throw ScriptError("Worlds row lacks required BNW map setup columns");
                auto type = row.text("Type");
                const auto rowid = row.integer("CivisRowId"), id = row.integer("ID");
                const auto feature_grain_change = row.integer("FeatureGrainChange");
                if (!type || type->empty() || type->size() > 4096 || rowid < 0 || id < 0 ||
                    feature_grain_change < std::numeric_limits<std::int32_t>::min() ||
                    feature_grain_change > std::numeric_limits<std::int32_t>::max() ||
                    !rowids.insert(rowid).second || !ids.insert(id).second ||
                    !types.insert(*type).second)
                    throw ScriptError("Activated Worlds has an invalid or duplicate row");
                worlds.push_back({rowid, id, std::move(*type), feature_grain_change});
            }, 1024);
        if (!report.all_succeeded() || worlds.empty() ||
            !ids.contains(*setup.world_size))
            throw ScriptError("Selected Worlds rows are incomplete or omit the chosen world size: " +
                              report.error);
        std::vector<std::uint8_t> identity;
        const auto append_number = [&](std::uint64_t value) {
            for (unsigned shift = 0; shift < 64; shift += 8)
                identity.push_back(static_cast<std::uint8_t>(value >> shift));
        };
        for (const auto &row : worlds) {
            append_number(static_cast<std::uint64_t>(row.rowid));
            append_number(static_cast<std::uint64_t>(row.id));
            append_number(row.type.size());
            identity.insert(identity.end(), row.type.begin(), row.type.end());
            append_number(static_cast<std::uint64_t>(row.feature_grain_change));
        }
        BnwMapWorldRulesSource provenance;
        provenance.worlds_sha256 = identify_bytes(identity).sha256;
        provenance.profile_kind = selected.profile.kind;
        for (const auto &requirement : selected.profile.requirements)
            provenance.selected_requirements.push_back(
                std::string(requirement.kind == content::PackageKind::dlc ? "dlc:" : "mod:") +
                requirement.id + "@" + std::to_string(requirement.wire_value));
        result.world_rules_source = std::move(provenance);
        return true;
    } catch (const std::exception &error) {
        block(result, BnwMapScriptBlockerCode::invalid_world_rules,
              std::string("Cannot read selected BNW Worlds rows: ") + error.what());
        return false;
    }
}

bool load_selected_climates(const BnwMapSetupContext &setup,
                            BnwMapScriptResult &result,
                            std::vector<ScriptMapClimateRow> &climates) {
    if (!setup.selected_rules || !setup.selected_rules->rules ||
        !setup.selected_rules->content_ready() ||
        !result.world_rules_source ||
        result.world_rules_source->profile_kind !=
            setup.selected_rules->profile.kind) {
        block(result, BnwMapScriptBlockerCode::invalid_world_rules,
              "Map GameInfo.Climates requires the selected activated BNW rules");
        return false;
    }
    try {
        std::set<std::int64_t> ids, rowids;
        std::set<std::string> types;
        const auto report = setup.selected_rules->rules->query_rows(
            {"SELECT ROWID AS CivisRowId, ID, Type FROM Climates ORDER BY ROWID", {}},
            [&](const content::RulesRowView &row) {
                if (!row.has_column("CivisRowId") || !row.has_column("ID") ||
                    !row.has_column("Type"))
                    throw ScriptError("Selected Climates row is incomplete");
                auto type = row.text("Type");
                const auto rowid = row.integer("CivisRowId"), id = row.integer("ID");
                if (!type || type->empty() || type->size() > 4096 ||
                    rowid < 0 || id < 0 || rowid != id ||
                    !rowids.insert(rowid).second || !ids.insert(id).second ||
                    !types.insert(*type).second)
                    throw ScriptError("Selected Climates Info slots are invalid or ambiguous");
                climates.push_back({rowid, id, std::move(*type)});
            }, 1024);
        if (!report.all_succeeded() || climates.empty() ||
            (setup.climate && !ids.contains(*setup.climate)))
            throw ScriptError("Selected Climates omit the chosen pregame climate: " +
                              report.error);
        return true;
    } catch (const std::exception &error) {
        block(result, BnwMapScriptBlockerCode::invalid_world_rules,
              std::string("Cannot read selected BNW Climates rows: ") + error.what());
        return false;
    }
}

void publish_fresh_river_draft(BnwSetupPlotGrid &owned,
                               BnwGeneratedPlotMap &map,
                               BnwFreshRiverAreaDraft draft) {
    using Kind = BnwFreshRiverDraftEventKind;
    if (!map.area_structure || !map.final_yields || !map.yield_progress ||
        draft.river.remaining_services != std::vector<std::string_view>{
            "fresh_river_state_atomic_handoff"} ||
        draft.river.source_order.empty() ||
        draft.river.source_order.back().kind != Kind::flow_direction_applied ||
        draft.river.grid.size() != owned.size() ||
        map.final_yields->size() != owned.size() ||
        draft.areas.area_ids != map.area_structure->area_ids ||
        draft.areas.landmass_ids != map.area_structure->landmass_ids ||
        draft.areas.areas.size() != map.area_structure->areas.size())
        throw std::invalid_argument("Fresh BNW river publication has incomplete selected state");
    const auto source_index = draft.river.source_order.front().plot_index;
    if (source_index >= owned.size() ||
        draft.river.source_order.front().kind != Kind::edge_bit_written ||
        draft.river.source_order.back().plot_index != source_index)
        throw std::invalid_argument("Fresh BNW river publication has invalid source order");
    const auto &before_symbols = owned.symbol_events();
    const auto &after_symbols = draft.river.grid.symbol_events();
    if (after_symbols.size() < before_symbols.size())
        throw std::invalid_argument("Fresh BNW river publication lost staged symbols");
    for (std::size_t i = 0; i < before_symbols.size(); ++i) {
        const auto &before = before_symbols[i], &after = after_symbols[i];
        if (before.phase != after.phase || before.plot_index != after.plot_index ||
            before.trigger_landmass_id != after.trigger_landmass_id ||
            before.yields_after != after.yields_after)
            throw std::invalid_argument("Fresh BNW river publication changed older symbols");
    }
    std::size_t symbol_index = before_symbols.size();
    for (const auto &event : draft.river.source_order) {
        if (event.kind != Kind::symbols_staged) continue;
        if (symbol_index >= after_symbols.size() ||
            after_symbols[symbol_index].phase != BnwFreshYieldSymbolPhase::river_write ||
            after_symbols[symbol_index].plot_index != event.plot_index)
            throw std::invalid_argument("Fresh BNW river symbols differ from SDK call order");
        ++symbol_index;
    }
    if (symbol_index != after_symbols.size())
        throw std::invalid_argument("Fresh BNW river publication has untraced symbols");
    const auto appended = after_symbols.size() - before_symbols.size();
    if (map.river_symbol_events > std::numeric_limits<std::uint64_t>::max() - appended ||
        map.river_edge_writes == std::numeric_limits<std::uint64_t>::max())
        throw std::length_error("Fresh BNW river publication counters overflow");
    const auto source_area_id = owned.at(source_index).area_id;
    for (std::size_t i = 0; i < draft.areas.areas.size(); ++i) {
        const auto &old_area = map.area_structure->areas[i];
        const auto &new_area = draft.areas.areas[i];
        const auto delta = old_area.id == source_area_id ? 1 : 0;
        if (new_area.id != old_area.id || new_area.water != old_area.water ||
            new_area.mountains != old_area.mountains ||
            new_area.tiles != old_area.tiles || new_area.west != old_area.west ||
            new_area.east != old_area.east || new_area.south != old_area.south ||
            new_area.north != old_area.north ||
            static_cast<std::int64_t>(new_area.river_edges) !=
                static_cast<std::int64_t>(old_area.river_edges) + delta)
            throw std::invalid_argument("Fresh BNW river publication changed non-source areas");
    }
    std::vector<std::array<std::int16_t, 6>> next_yields;
    next_yields.reserve(owned.size());
    for (std::size_t i = 0; i < owned.size(); ++i) {
        const auto &before = owned.at(i), &after = draft.river.grid.at(i);
        if (before.plot_type != after.plot_type ||
            before.terrain_type != after.terrain_type ||
            before.area_id != after.area_id ||
            before.landmass_id != after.landmass_id ||
            before.yields != (*map.final_yields)[i] ||
            after.area_id != draft.areas.area_ids[i] ||
            after.landmass_id != draft.areas.landmass_ids[i] ||
            after.river_crossing_count !=
                std::popcount(static_cast<unsigned>(after.river_crossing_mask)))
            throw std::invalid_argument("Fresh BNW river publication changed plot identity or caches");
        next_yields.push_back(after.yields);
    }
    static_assert(std::is_nothrow_swappable_v<BnwSetupPlotGrid> &&
                  std::is_nothrow_swappable_v<BnwFreshAreaStructure> &&
                  std::is_nothrow_swappable_v<decltype(next_yields)>);
    using std::swap;
    swap(owned, draft.river.grid);
    swap(*map.area_structure, draft.areas);
    swap(*map.final_yields, next_yields);
    map.river_symbol_events += appended;
    ++map.river_edge_writes;
}
} // namespace

bool BnwMapScriptResult::has_blocker(BnwMapScriptBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

bool BnwFreshMapHandoff::has_blocker(BnwFreshMapHandoffBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

BnwFreshMapHandoff prepare_bnw_fresh_map_handoff(
    BnwMapScriptResult &&generated, const BnwFreshGameContentResult &selected) {
    BnwFreshMapHandoff handoff;
    const auto fail = [&](BnwFreshMapHandoffBlockerCode code, std::string message) {
        handoff.blockers.push_back({code, std::move(message)});
    };
    if (!selected.content_ready() || !selected.rules || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind) {
        fail(BnwFreshMapHandoffBlockerCode::content_unverified,
             "Selected BNW rules and activation are required for a fresh map handoff");
        return handoff;
    }
    const bool only_expected_pending = std::all_of(
        generated.blockers.begin(), generated.blockers.end(), [](const auto &blocker) {
            return blocker.code == BnwMapScriptBlockerCode::areas_pending ||
                   blocker.code == BnwMapScriptBlockerCode::river_generation_pending ||
                   blocker.code == BnwMapScriptBlockerCode::lake_generation_pending ||
                   blocker.code == BnwMapScriptBlockerCode::features_rivers_pending ||
                   blocker.code == BnwMapScriptBlockerCode::starting_positions_pending ||
                   blocker.code == BnwMapScriptBlockerCode::world_setup_pending;
        });
    if (!only_expected_pending || !generated.map || !generated.map->area_structure ||
        !generated.map->final_yields || !generated.map->yield_progress ||
        !generated.map->owned_setup_grid || !generated.map->terrain_types) {
        fail(BnwFreshMapHandoffBlockerCode::map_unverified,
             "Complete owned BNW plot, terrain, area and yield setup is required");
        return handoff;
    }
    const auto &map = *generated.map;
    const auto &grid = *map.owned_setup_grid;
    const auto count = map.plot_types.size();
    std::uint64_t expected_events = map.yield_progress->set_area_changed_plots +
                                    map.yield_progress->final_pass_changed_plots;
    for (const auto &pass : map.yield_progress->lake_threshold_passes)
        expected_events += pass.changed_plots;
    const auto base_events = expected_events;
    if (map.river_symbol_events > std::numeric_limits<std::uint64_t>::max() -
                                  expected_events) {
        fail(BnwFreshMapHandoffBlockerCode::map_unverified,
             "BNW river symbol count overflows the ordered map events");
        return handoff;
    }
    expected_events += map.river_symbol_events;
    const auto lake_events_begin = expected_events;
    if (map.lake_symbol_events > std::numeric_limits<std::uint64_t>::max() -
                                 expected_events) {
        fail(BnwFreshMapHandoffBlockerCode::map_unverified,
             "BNW lake symbol count overflows the ordered map events");
        return handoff;
    }
    expected_events += map.lake_symbol_events;
    if (!count || count != grid.size() || count != map.terrain_types->size() ||
        count != map.final_yields->size() ||
        count != map.area_structure->area_ids.size() ||
        count != map.area_structure->landmass_ids.size() ||
        expected_events != grid.symbol_events().size()) {
        fail(BnwFreshMapHandoffBlockerCode::map_unverified,
             "Owned BNW grid or staged updateSymbols count does not match replay");
        return handoff;
    }
    const auto &symbols = grid.symbol_events();
    for (std::size_t i = static_cast<std::size_t>(base_events);
         i < static_cast<std::size_t>(lake_events_begin); ++i) {
        if (symbols[i].phase != BnwFreshYieldSymbolPhase::river_write ||
            symbols[i].plot_index >= count) {
            fail(BnwFreshMapHandoffBlockerCode::map_unverified,
                 "BNW river updateSymbols suffix differs from selected plot order");
            return handoff;
        }
    }
    for (std::size_t i = static_cast<std::size_t>(lake_events_begin);
         i < symbols.size(); ++i) {
        if (symbols[i].phase != BnwFreshYieldSymbolPhase::lake_conversion ||
            symbols[i].plot_index >= count) {
            fail(BnwFreshMapHandoffBlockerCode::map_unverified,
                 "BNW lake updateSymbols suffix differs from selected plot order");
            return handoff;
        }
    }
    std::uint64_t plot_edges = 0, area_edges = 0;
    std::vector<std::int32_t> area_tile_counts(map.area_structure->areas.size());
    for (const auto &area : map.area_structure->areas) {
        if (area.river_edges < 0) {
            fail(BnwFreshMapHandoffBlockerCode::map_unverified,
                 "BNW area has a negative river edge count");
            return handoff;
        }
        area_edges += static_cast<std::uint64_t>(area.river_edges);
    }
    for (std::size_t i = 0; i < count; ++i) {
        const auto &plot = grid.at(i);
        plot_edges += static_cast<std::uint64_t>(plot.ne_of_river) +
                      static_cast<std::uint64_t>(plot.w_of_river) +
                      static_cast<std::uint64_t>(plot.nw_of_river);
        const auto valid_flow =
            (plot.ne_of_river ? (plot.river_sw_flow_direction == 2 ||
                                 plot.river_sw_flow_direction == 5)
                              : plot.river_sw_flow_direction == -1) &&
            (plot.w_of_river ? (plot.river_e_flow_direction == 0 ||
                                plot.river_e_flow_direction == 3)
                             : plot.river_e_flow_direction == -1) &&
            (plot.nw_of_river ? (plot.river_se_flow_direction == 1 ||
                                 plot.river_se_flow_direction == 4)
                              : plot.river_se_flow_direction == -1);
        if (!plot.plot_type_assigned_by_script || !plot.terrain_type_assigned_by_script ||
            plot.plot_type != map.plot_types[i] ||
            plot.terrain_type != (*map.terrain_types)[i] ||
            plot.area_id != map.area_structure->area_ids[i] ||
            plot.landmass_id != map.area_structure->landmass_ids[i] ||
            plot.yields != (*map.final_yields)[i] || !valid_flow ||
            plot.river_crossing_count !=
                std::popcount(static_cast<unsigned>(plot.river_crossing_mask))) {
            fail(BnwFreshMapHandoffBlockerCode::map_unverified,
                 "Owned BNW plot state differs from generated map evidence");
            return handoff;
        }
        const auto area = std::find_if(map.area_structure->areas.begin(),
            map.area_structure->areas.end(), [&](const auto &candidate) {
                return candidate.id == plot.area_id;
            });
        if (area == map.area_structure->areas.end() ||
            area->water != (plot.plot_type == 3)) {
            fail(BnwFreshMapHandoffBlockerCode::map_unverified,
                 "Owned BNW plot refers to a missing or mismatched area");
            return handoff;
        }
        ++area_tile_counts[static_cast<std::size_t>(area - map.area_structure->areas.begin())];
    }
    for (std::size_t i = 0; i < area_tile_counts.size(); ++i)
        if (area_tile_counts[i] != map.area_structure->areas[i].tiles) {
            fail(BnwFreshMapHandoffBlockerCode::map_unverified,
                 "Owned BNW area tile counts differ from plot handles");
            return handoff;
        }
    if (plot_edges != map.river_edge_writes ||
        area_edges != map.river_edge_writes) {
        fail(BnwFreshMapHandoffBlockerCode::map_unverified,
             "BNW river edge counts differ between owned plots and areas");
        return handoff;
    }
    if (!generated.source || generated.source->relative_path.empty() ||
        generated.source->sha256.size() != 64 ||
        generated.source->provenance.source_id.empty() ||
        generated.source->provenance.source_relative_path.empty() ||
        !generated.world_rules_source ||
        generated.world_rules_source->worlds_sha256.size() != 64 ||
        generated.world_rules_source->defines_sha256.size() != 64 ||
        generated.world_rules_source->profile_kind != selected.profile.kind ||
        !generated.map_random_before || !generated.map_random_after ||
        generated.map_random_after->calls < generated.map_random_before->calls) {
        fail(BnwFreshMapHandoffBlockerCode::provenance_unverified,
             "Selected script, rules and initial BNW map RNG provenance are required");
        return handoff;
    }
    std::vector<std::string> requirements;
    for (const auto &requirement : selected.profile.requirements)
        requirements.push_back(
            std::string(requirement.kind == content::PackageKind::dlc ? "dlc:" : "mod:") +
            requirement.id + "@" + std::to_string(requirement.wire_value));
    if (requirements != generated.world_rules_source->selected_requirements) {
        fail(BnwFreshMapHandoffBlockerCode::provenance_unverified,
             "Map rules provenance differs from the selected content profile");
        return handoff;
    }
    try {
        handoff.selected_rules_sha256 =
            identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    } catch (const std::exception &error) {
        fail(BnwFreshMapHandoffBlockerCode::provenance_unverified,
             std::string("Cannot identify selected BNW rules: ") + error.what());
        return handoff;
    }
    handoff.map.emplace(std::move(*generated.map));
    generated.map.reset();
    handoff.source.emplace(std::move(*generated.source));
    generated.source.reset();
    handoff.included_sources = std::move(generated.included_sources);
    handoff.world_rules_source = std::move(generated.world_rules_source);
    handoff.map_random_before = generated.map_random_before;
    handoff.map_random_after = generated.map_random_after;
    fail(BnwFreshMapHandoffBlockerCode::update_symbols_delivery_pending,
         "Ordered GameplayYieldMightHaveChanged events await the live map host");
    for (const auto &blocker : generated.blockers)
        if (blocker.code == BnwMapScriptBlockerCode::river_generation_pending)
            fail(BnwFreshMapHandoffBlockerCode::river_generation_pending,
                 blocker.message);
        else if (blocker.code == BnwMapScriptBlockerCode::lake_generation_pending)
            fail(BnwFreshMapHandoffBlockerCode::lake_generation_pending,
                 blocker.message);
    fail(BnwFreshMapHandoffBlockerCode::features_rivers_pending,
         "Features, resources and later map generation remain incomplete");
    fail(BnwFreshMapHandoffBlockerCode::starting_positions_pending,
         "Starting positions and natural wonders are not assigned");
    fail(BnwFreshMapHandoffBlockerCode::game_state_players_teams_pending,
         "Fresh BNW game state, players and teams are absent");
    fail(BnwFreshMapHandoffBlockerCode::live_session_factory_pending,
         "LiveSession has no fresh-world constructor");
    return handoff;
}

BnwMapScriptResult generate_bnw_plot_map_impl(std::string_view verified_source,
                                              std::string_view chunk_name,
                                              const BnwMapSetupContext &setup,
                                              std::optional<SimulationRandom> map_random,
                                              BnwMapScriptBudget budget,
                                              std::shared_ptr<BnwIncludeManifest> includes = {},
                                              BnwSelectedMapInitJob* selected_init_job = nullptr) {
    BnwMapScriptResult result;
    block(result, BnwMapScriptBlockerCode::terrain_pending,
          "Terrain types have not been generated for these plot categories");
    block(result, BnwMapScriptBlockerCode::areas_pending,
          "Map.RecalculateAreas updateSymbols events are staged, but live delivery remains unresolved");
    block(result, BnwMapScriptBlockerCode::features_rivers_pending,
          "Features, resources and later map generation remain incomplete");
    block(result, BnwMapScriptBlockerCode::starting_positions_pending,
          "Starting positions and natural wonders have not been assigned");
    block(result, BnwMapScriptBlockerCode::world_setup_pending,
          "No live BNW players, teams, cities, units, or turn state exist");

    if (verified_source.empty()) {
        block(result, BnwMapScriptBlockerCode::script_failed, "No verified map script source supplied");
        return result;
    }
    if (map_random && map_random->algorithm() != RandomAlgorithm::bnw) {
        block(result, BnwMapScriptBlockerCode::invalid_map_random,
              "BNW map script requires the BNW map RNG algorithm");
        return result;
    }
    if (map_random) result.map_random_before = map_random->state();
    if (!setup.selected_rules && !setup.runtime_defines.empty()) {
        block(result, BnwMapScriptBlockerCode::invalid_setup_context,
              "Native BNW runtime defines require a selected active rules profile");
        return result;
    }
    if (setup.world_size && (*setup.world_size < 0 ||
                             *setup.world_size > std::numeric_limits<std::int32_t>::max())) {
        block(result, BnwMapScriptBlockerCode::invalid_setup_context,
              "Selected BNW WorldSizes ID is outside source int32 range");
        return result;
    }
    if (setup.custom_options) {
        for (const auto &[name, value] : *setup.custom_options) {
            if (name.empty() || name.find('\0') != name.npos ||
                value < std::numeric_limits<std::int32_t>::min() ||
                value > std::numeric_limits<std::int32_t>::max()) {
                block(result, BnwMapScriptBlockerCode::invalid_setup_context,
                      "Selected BNW map option name or value is invalid");
                return result;
            }
        }
    }
    if (!includes) includes = std::make_shared<BnwIncludeManifest>();
    std::shared_ptr<BnwSelectedMapSourceOwner> selected_source_owner;
    if(selected_init_job){
        if(!map_random||!setup.world_size||!setup.runtime_defines.empty()||
           setup.selected_rules!=selected_init_job->parent.get())
            throw ScriptError("Selected map init requires actual parent, world, BNW RNG and no supplied runtime defines");
        std::vector<std::pair<std::string,assets::AssetHandle>> handles;
        for(const auto& [name,handle]:includes->handles)handles.emplace_back(name,handle);
        selected_source_owner=BnwMapScriptSelectedAccess::make(selected_init_job->parent,
            selected_init_job->root,std::move(handles),*selected_init_job->resources,setup,*map_random,budget.maximum_source_bytes);
    }
    ScriptSourceProvider source_provider;
    source_provider.strict_map_includes = true;
    source_provider.files = [includes](std::string_view context) {
        if (context != "map") throw ScriptError("BNW map include requested outside map context");
        return includes->names;
    };
    source_provider.read = [includes, selected_source_owner, limit = budget.maximum_source_bytes](
                               std::string_view context, std::string_view name) {
        if (context != "map") throw ScriptError("BNW map include read outside map context");
        const auto found = includes->handles.find(std::string(name));
        if (found == includes->handles.end()) throw ScriptError("Unselected BNW map helper");
        if(selected_source_owner)selected_source_owner->require_current(context,nullptr);
        auto bytes = found->second.read_bytes(limit);
        if(selected_source_owner)BnwMapScriptSelectedAccess::include(*selected_source_owner,name,bytes);
        includes->trace.push_back({std::string(name), identify_bytes(bytes).sha256,
                                   found->second.candidate().provenance});
        return std::string(bytes.begin(), bytes.end());
    };
    std::int64_t width = 0, height = 0;
    std::optional<std::int64_t> resolved_plot_count;
    std::optional<std::pair<bool,bool>> resolved_wraps;
    // CvMap::InitPlots creates indexed CvPlot state after the grid is set;
    // CvPlot::reset starts each cell as PLOT_OCEAN / NO_TERRAIN.
    BnwSetupPlotGrid setup_plots;
    std::uint64_t remaining_river_water_visits =
        budget.maximum_river_water_query_visits;
    std::optional<BnwSelectedLandTerrain> selected_land_terrain;
    std::optional<BnwFreshYieldRules> selected_yield_rules;
    std::map<std::int32_t, BnwSelectedTerrain> selected_terrains;
    std::map<std::int32_t, std::optional<BnwSelectedFeatureEligibility>>
        selected_features;
    // CvPlot::reset has NO_FEATURE on every fresh plot. This detached feature
    // state remains read-only until SetFeatureType is implemented atomically.
    std::vector<std::int32_t> setup_feature_types;
    std::unique_ptr<content::RulesDatabase> selected_define_database;
    std::function<std::optional<ScriptValue>(std::string_view)> selected_define_lookup;
    std::optional<std::int32_t> selected_shallow_terrain_id;
    bool setup_plots_ready = false;
    bool lake_probe_active = false;
    bool feature_probe_active = false;
    std::uint64_t lake_plot_conversions = 0;
    std::uint64_t lake_calculate_areas_calls = 0;
    // Published Fractal IDs are never reused. A cleared slot is a real
    // retired owner, so a later userdata finalizer cannot release a new object.
    std::vector<std::unique_ptr<BnwFractal>> fractals;
    std::size_t live_fractals = 0;
    ScriptMapProvider map_provider;
    map_provider.grid_size = [&] { return std::pair{width, height}; };
    map_provider.setup_plot_count = [&] {
        if (!resolved_plot_count)
            throw ScriptError("CIVIS_BNW_MAP_GRID_UNRESOLVED:GetMapInitData has not resolved plot count");
        return *resolved_plot_count;
    };
    map_provider.setup_plot_by_index = [&](std::int64_t raw) -> std::optional<std::int64_t> {
        if (!setup_plots_ready)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:CvMap::InitPlots has not run");
        // CvLuaMap narrows lua_tointeger to C++ int before plotByIndex.
        const auto low = static_cast<std::uint32_t>(raw);
        const auto index = low <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
            ? static_cast<std::int64_t>(low) : static_cast<std::int64_t>(low) - (1LL << 32);
        if (index < 0 || static_cast<std::uint64_t>(index) >= setup_plots.size())
            return std::nullopt;
        return index;
    };
    map_provider.setup_plot_direction = [&](std::int64_t x, std::int64_t y,
                                            std::int64_t direction)
        -> std::optional<std::int64_t> {
        if (!setup_plots_ready)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Map.PlotDirection requires CvMap::InitPlots");
        if (x < 0 || x >= width || y < 0 || y >= height ||
            direction < 0 || direction >= 6)
            throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.PlotDirection outside verified grid/direction");
        const auto source = static_cast<std::size_t>(y * width + x);
        const auto adjacent = setup_plots.adjacent(source)[static_cast<std::size_t>(direction)];
        if (adjacent) return static_cast<std::int64_t>(*adjacent);
        return {};
    };
    map_provider.setup_plot_river_edge = [&](std::int64_t id,
                                             ScriptMapSetupRiverEdge edge) {
        if (!setup_plots_ready)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:fresh river-edge read requires CvMap::InitPlots");
        if (id < 0 || static_cast<std::uint64_t>(id) >= setup_plots.size())
            throw ScriptError("Invalid BNW setup plot handle");
        const auto &plot = setup_plots.at(static_cast<std::size_t>(id));
        switch (edge) {
        case ScriptMapSetupRiverEdge::north_east: return plot.ne_of_river;
        case ScriptMapSetupRiverEdge::west: return plot.w_of_river;
        case ScriptMapSetupRiverEdge::north_west: return plot.nw_of_river;
        }
        throw ScriptError("Unknown BNW fresh river edge");
    };
    map_provider.setup_plot_set_river_edge = [&]
        (std::int64_t id, ScriptMapSetupRiverEdge query_edge, bool enabled,
         std::int64_t flow_direction) {
        if (!setup_plots_ready || id < 0 ||
            static_cast<std::uint64_t>(id) >= setup_plots.size() ||
            flow_direction < std::numeric_limits<std::int32_t>::min() ||
            flow_direction > std::numeric_limits<std::int32_t>::max())
            throw ScriptError("CIVIS_BNW_RIVER_WRITE_INVALID:plot handle or flow direction");
        BnwFreshRiverEdge edge;
        std::string_view name;
        switch (query_edge) {
        case ScriptMapSetupRiverEdge::north_east:
            edge = BnwFreshRiverEdge::north_east; name = "Plot.SetNEOfRiver"; break;
        case ScriptMapSetupRiverEdge::west:
            edge = BnwFreshRiverEdge::west; name = "Plot.SetWOfRiver"; break;
        case ScriptMapSetupRiverEdge::north_west:
            edge = BnwFreshRiverEdge::north_west; name = "Plot.SetNWOfRiver"; break;
        default:
            throw ScriptError("CIVIS_BNW_RIVER_WRITE_INVALID:unknown river edge");
        }
        const auto preflight = inspect_bnw_fresh_river_write(
            setup_plots, static_cast<std::size_t>(id), edge, enabled,
            static_cast<std::int32_t>(flow_direction));
        if (preflight.blocker != BnwFreshRiverWriteBlockerCode::required_services_pending)
            throw ScriptError("CIVIS_BNW_RIVER_WRITE_INVALID:" + std::string(name) +
                              ":outside fresh AddRivers contract");
        const auto crossing_draft = setup_plots.draft_fresh_river_crossings(
            static_cast<std::size_t>(id), edge,
            static_cast<std::int32_t>(flow_direction));
        if (!selected_yield_rules || !setup.selected_rules ||
            !setup.selected_rules->rules || !result.map ||
            !result.map->area_structure ||
            selected_yield_rules->source_rules != setup.selected_rules->rules.get())
            throw ScriptError("CIVIS_BNW_RIVER_WRITE_INVALID:selected yield rules identity is unresolved");
        const auto yield_draft = replay_bnw_fresh_river_yields(
            crossing_draft, *result.map->area_structure, *selected_yield_rules,
            budget.maximum_symbol_events);
        const auto area_draft = replay_bnw_fresh_river_area_count(
            yield_draft, *result.map->area_structure, static_cast<std::size_t>(id));
        auto flow_draft = replay_bnw_fresh_river_flow_direction(area_draft);
        // Every SDK setter effect is complete in the detached result. The
        // publication helper validates the selected state and swaps all
        // Lua-visible plot, area, cache and symbol data without a callback
        // between swaps. AddRivers restores the entire phase on later error.
        publish_fresh_river_draft(setup_plots, *result.map,
                                  std::move(flow_draft));
    };
    map_provider.setup_plot_value = [&](std::int64_t id, ScriptMapSetupPlotQuery query) {
        if (!setup_plots_ready)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:CvMap::InitPlots has not run");
        if (id < 0 || static_cast<std::uint64_t>(id) >= setup_plots.size())
            throw ScriptError("Invalid BNW setup plot handle");
        const auto &plot = setup_plots.at(static_cast<std::size_t>(id));
        switch (query) {
        case ScriptMapSetupPlotQuery::x: return static_cast<std::int64_t>(plot.x);
        case ScriptMapSetupPlotQuery::y: return static_cast<std::int64_t>(plot.y);
        case ScriptMapSetupPlotQuery::index: return id;
        case ScriptMapSetupPlotQuery::plot_type: return static_cast<std::int64_t>(plot.plot_type);
        case ScriptMapSetupPlotQuery::terrain_type: return static_cast<std::int64_t>(plot.terrain_type);
        case ScriptMapSetupPlotQuery::is_river:
            return static_cast<std::int64_t>(plot.river_crossing_count > 0);
        case ScriptMapSetupPlotQuery::feature_type:
            if (!feature_probe_active || setup_feature_types.size() != setup_plots.size())
                throw ScriptError("CIVIS_BNW_FEATURE_QUERY_UNRESOLVED:Plot.GetFeatureType requires fresh AddFeatures state");
            return static_cast<std::int64_t>(setup_feature_types[static_cast<std::size_t>(id)]);
        }
        throw ScriptError("Unknown BNW setup plot query");
    };
    map_provider.setup_plot_adjacent_land = [&](std::int64_t id) {
        if (!setup_plots_ready)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:CvMap::InitPlots has not run");
        if (id < 0 || static_cast<std::uint64_t>(id) >= setup_plots.size())
            throw ScriptError("Invalid BNW setup plot handle");
        return setup_plots.is_adjacent_to_land(static_cast<std::size_t>(id));
    };
    map_provider.setup_plot_coastal_land = [&](std::int64_t id) {
        if (!result.map || !result.map->area_structure || id < 0 ||
            static_cast<std::uint64_t>(id) >= setup_plots.size())
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:coastal query requires assigned fresh landmasses");
        const auto minimum = selected_define_lookup("MIN_WATER_SIZE_FOR_OCEAN");
        const auto *integer = minimum ? std::get_if<std::int64_t>(&*minimum) : nullptr;
        if (!integer || *integer < std::numeric_limits<std::int32_t>::min() ||
            *integer > std::numeric_limits<std::int32_t>::max())
            throw ScriptError("CIVIS_BNW_MAP_RULES_UNRESOLVED:selected MIN_WATER_SIZE_FOR_OCEAN is invalid");
        return setup_plots.is_coastal_land(static_cast<std::size_t>(id),
            static_cast<std::int32_t>(*integer), *result.map->area_structure);
    };
    map_provider.setup_find_water = [&](std::int64_t id, std::int64_t radius, bool fresh) {
        if (!result.map || !result.map->area_structure || id < 0 ||
            static_cast<std::uint64_t>(id) >= setup_plots.size())
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Map.FindWater requires assigned fresh landmasses");
        if (radius < std::numeric_limits<std::int32_t>::min() ||
            radius > std::numeric_limits<std::int32_t>::max())
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Map.FindWater radius exceeds SDK int32 range");
        return setup_plots.find_water_during_rivers(static_cast<std::size_t>(id),
            static_cast<std::int32_t>(radius), fresh, *result.map->area_structure,
            remaining_river_water_visits);
    };
    map_provider.setup_plot_area_id = [&](std::int64_t id) {
        if (!result.map || !result.map->area_structure || id < 0 ||
            static_cast<std::uint64_t>(id) >= setup_plots.size())
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Plot.Area requires assigned fresh areas");
        const auto index = static_cast<std::size_t>(id);
        const auto area_id = setup_plots.at(index).area_id;
        if (index >= result.map->area_structure->area_ids.size() ||
            area_id < 0 || result.map->area_structure->area_ids[index] != area_id)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Plot.Area identity changed");
        (void)find_bnw_fresh_area(*result.map->area_structure, area_id);
        return static_cast<std::int64_t>(area_id);
    };
    map_provider.setup_area_value = [&](std::int64_t raw_id,
                                        ScriptMapSetupAreaQuery query) {
        if (!result.map || !result.map->area_structure ||
            raw_id < std::numeric_limits<std::int32_t>::min() ||
            raw_id > std::numeric_limits<std::int32_t>::max())
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Area query requires selected fresh areas");
        const auto &area = find_bnw_fresh_area(*result.map->area_structure,
                                                static_cast<std::int32_t>(raw_id));
        switch (query) {
        case ScriptMapSetupAreaQuery::num_tiles: return static_cast<std::int64_t>(area.tiles);
        case ScriptMapSetupAreaQuery::num_river_edges:
            return static_cast<std::int64_t>(area.river_edges);
        }
        throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Area query");
    };
    map_provider.setup_find_biggest_area = [&](bool water)
        -> std::optional<std::int64_t> {
        if (!setup_plots_ready || !result.map || !result.map->area_structure)
            throw ScriptError("CIVIS_BNW_MAP_AREAS_UNRESOLVED:FindBiggestArea requires assigned fresh areas");
        try {
            const auto id = find_bnw_fresh_biggest_area(
                setup_plots, *result.map->area_structure, water);
            if (id) return static_cast<std::int64_t>(*id);
            return {};
        } catch (const std::invalid_argument &error) {
            throw ScriptError(std::string("CIVIS_BNW_MAP_AREAS_UNRESOLVED:FindBiggestArea:") +
                              error.what());
        }
    };
    map_provider.setup_plot_inland_corner = [&](std::int64_t id)
        -> std::optional<std::int64_t> {
        if (!setup_plots_ready || !result.map || !result.map->area_structure ||
            !result.map->final_yields || id < 0 ||
            static_cast<std::uint64_t>(id) >= setup_plots.size() ||
            setup_plots.at(static_cast<std::size_t>(id)).area_id < 0)
            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Plot.GetInlandCorner requires assigned fresh areas");
        if (!map_random)
            throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.GetInlandCorner requires map RNG");
        const auto corner = setup_plots.get_inland_corner(
            static_cast<std::size_t>(id), *map_random);
        result.map_random_after = map_random->state();
        if (corner) return static_cast<std::int64_t>(*corner);
        return {};
    };
    map_provider.setup_wraps = [&] {
        if (!resolved_wraps)
            throw ScriptError("CIVIS_BNW_MAP_WRAP_UNRESOLVED:GetMapInitData has not resolved wrap flags");
        return *resolved_wraps;
    };
    if (setup.selected_rules) {
        std::vector<ScriptMapWorldRow> worlds;
        if (!load_selected_worlds(setup, result, worlds)) return result;
        map_provider.setup_worlds = std::move(worlds);
        std::vector<ScriptMapClimateRow> climates;
        if (!load_selected_climates(setup, result, climates)) return result;
        map_provider.setup_climates = std::move(climates);
        map_provider.setup_climate = setup.climate;
        selected_define_lookup = [&](std::string_view name) -> std::optional<ScriptValue> {
            try {
                if (!selected_define_database) {
                    // The active content DB is not mutated by map scripts.
                    // BNW initializes PostDefines before Lua's GameDefines
                    // SELECT Value FROM Defines WHERE Name = ? lookup.
                    auto database = setup.selected_rules->rules->clone_gameplay();
                    const auto postprocess = database->postprocess_gameplay(
                        content::GameplayPostprocessProfile::bnw, setup.runtime_defines);
                    if (!postprocess.all_succeeded || !postprocess.transaction_released)
                        throw ScriptError("Selected BNW PostDefines did not complete");
                    std::vector<std::uint8_t> identity;
                    const auto append = [&](std::string_view value) {
                        const auto length = static_cast<std::uint64_t>(value.size());
                        for (unsigned shift = 0; shift < 64; shift += 8)
                            identity.push_back(static_cast<std::uint8_t>(length >> shift));
                        identity.insert(identity.end(), value.begin(), value.end());
                    };
                    const auto rows = database->query_rows(
                        {"SELECT quote(ROWID) AS SourceRow, quote(Name) AS SourceName, "
                         "quote(Value) AS SourceValue FROM Defines ORDER BY ROWID", {}},
                        [&](const content::RulesRowView &row) {
                            for (const auto field : {"SourceRow", "SourceName", "SourceValue"}) {
                                const auto value = row.text(field);
                                if (!value) throw ScriptError("Selected BNW Defines row is incomplete");
                                append(*value);
                            }
                        }, 100'000);
                    if (!rows.all_succeeded() || rows.rows == 0)
                        throw ScriptError("Selected BNW Defines are unavailable: " + rows.error);
                    result.world_rules_source->defines_sha256 = identify_bytes(identity).sha256;
                    selected_define_database = std::move(database);
                }
                std::optional<ScriptValue> value;
                const auto rows = selected_define_database->query(
                    {"SELECT Value FROM Defines WHERE Name = ? LIMIT 1", {std::string(name)}},
                    [&](std::span<const content::DatabaseValue> row) {
                        if (row.size() != 1 || std::holds_alternative<std::monostate>(row[0]))
                            return;
                        if (const auto *integer = std::get_if<std::int64_t>(&row[0]))
                            value = *integer;
                        else if (const auto *real = std::get_if<double>(&row[0]))
                            value = *real;
                        else if (const auto *string = std::get_if<std::string>(&row[0]))
                            value = *string;
                    });
                if (rows > 1) throw ScriptError("Selected BNW define lookup returned several rows");
                return value;
            } catch (const std::exception &error) {
                throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") + error.what());
            }
        };
        map_provider.setup_define = selected_define_lookup;
        map_provider.setup_terrain_info_type = [&](std::string_view name) -> std::int64_t {
            if (!name.starts_with("TERRAIN_"))
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:GameInfoTypes." +
                                  std::string(name));
            try {
                std::optional<std::int64_t> id;
                const auto report = setup.selected_rules->rules->query_rows(
                    {"SELECT ID FROM Terrains WHERE Type = ? ORDER BY ROWID", {std::string(name)}},
                    [&](const content::RulesRowView &row) {
                        if (id || !row.has_column("ID"))
                            throw ScriptError("Selected Terrain Info type is ambiguous");
                        id = row.integer("ID");
                    }, 2);
                if (!report.all_succeeded() || !id || *id < 0 ||
                    *id > std::numeric_limits<std::int8_t>::max())
                    throw ScriptError("Selected Terrain Info type is missing or invalid: " +
                                      std::string(name) + ": " + report.error);
                (void)resolve_bnw_selected_terrain_by_id(
                    *setup.selected_rules->rules, static_cast<std::int32_t>(*id));
                return *id;
            } catch (const std::exception &error) {
                throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") + error.what());
            }
        };
        map_provider.setup_feature_info_type = [&](std::string_view name) -> std::int64_t {
            if (!name.starts_with("FEATURE_"))
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:FeatureTypes." +
                                  std::string(name));
            try {
                std::optional<std::int64_t> id;
                const auto report = setup.selected_rules->rules->query_rows(
                    {"SELECT ID FROM Features WHERE Type = ? ORDER BY ROWID", {std::string(name)}},
                    [&](const content::RulesRowView &row) {
                        if (id || !row.has_column("ID"))
                            throw ScriptError("Selected Feature Info type is ambiguous");
                        id = row.integer("ID");
                    }, 2);
                if (!report.all_succeeded() || !id || *id < 0 ||
                    *id > std::numeric_limits<std::int32_t>::max())
                    throw ScriptError("Selected Feature Info type is missing or invalid: " +
                                      std::string(name) + ": " + report.error);
                return *id;
            } catch (const std::exception &error) {
                throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") + error.what());
            }
        };
        map_provider.setup_plot_can_have_feature =
            [&](std::int64_t id, std::int64_t raw_feature_id) {
            if (!feature_probe_active || !setup_plots_ready || !result.map ||
                !result.map->area_structure || !setup.selected_rules ||
                !setup.selected_rules->rules || id < 0 ||
                static_cast<std::uint64_t>(id) >= setup_plots.size() ||
                raw_feature_id < -1 ||
                raw_feature_id > std::numeric_limits<std::int32_t>::max())
                throw ScriptError("CIVIS_BNW_FEATURE_QUERY_UNRESOLVED:outside selected fresh AddFeatures context");
            try {
                const auto feature_id = static_cast<std::int32_t>(raw_feature_id);
                std::optional<BnwSelectedFeatureEligibility> feature;
                if (feature_id >= 0) {
                    auto [found, inserted] = selected_features.try_emplace(feature_id);
                    if (inserted)
                        found->second = resolve_bnw_selected_feature_eligibility(
                            *setup.selected_rules->rules, feature_id);
                    feature = found->second;
                    if (feature && feature->source_rules !=
                                       setup.selected_rules->rules.get())
                        throw std::invalid_argument("Selected BNW feature rules identity differs");
                }
                std::int32_t minimum_water_size = 0;
                if (feature && feature->no_coast) {
                    const auto minimum = selected_define_lookup("MIN_WATER_SIZE_FOR_OCEAN");
                    const auto *integer = minimum ?
                        std::get_if<std::int64_t>(&*minimum) : nullptr;
                    if (!integer ||
                        *integer < std::numeric_limits<std::int32_t>::min() ||
                        *integer > std::numeric_limits<std::int32_t>::max())
                        throw std::invalid_argument("Selected MIN_WATER_SIZE_FOR_OCEAN is invalid");
                    minimum_water_size = static_cast<std::int32_t>(*integer);
                }
                return can_have_bnw_fresh_feature(
                    setup_plots, static_cast<std::size_t>(id), feature_id,
                    feature, *result.map->area_structure, minimum_water_size,
                    setup_feature_types);
            } catch (const std::exception &error) {
                throw ScriptError(std::string("CIVIS_BNW_FEATURE_QUERY_UNRESOLVED:") +
                                  error.what());
            }
        };
        map_provider.setup_plot_set_feature_type =
            [&](std::int64_t id, std::int64_t raw_feature_id,
                std::int64_t raw_variety) {
            if (!feature_probe_active || !setup_plots_ready || !result.map ||
                !result.map->area_structure || !setup.selected_rules ||
                !setup.selected_rules->rules || id < 0 ||
                static_cast<std::uint64_t>(id) >= setup_plots.size() ||
                raw_feature_id < 0 ||
                raw_feature_id > std::numeric_limits<std::int32_t>::max() ||
                raw_variety < std::numeric_limits<std::int32_t>::min() ||
                raw_variety > std::numeric_limits<std::int32_t>::max())
                throw ScriptError("CIVIS_BNW_FEATURE_WRITE_INVALID:Plot.SetFeatureType outside selected AddFeatures state");
            try {
                const auto feature_id = static_cast<std::int32_t>(raw_feature_id);
                auto [found, inserted] = selected_features.try_emplace(feature_id);
                if (inserted)
                    found->second = resolve_bnw_selected_feature_eligibility(
                        *setup.selected_rules->rules, feature_id);
                if (!found->second)
                    throw std::invalid_argument("Selected feature eligibility is unresolved");
                const auto info = resolve_bnw_fresh_feature_set_info(
                    *setup.selected_rules->rules, feature_id);
                std::int32_t minimum_water_size = 0;
                if (found->second->no_coast) {
                    const auto minimum = selected_define_lookup("MIN_WATER_SIZE_FOR_OCEAN");
                    const auto *integer = minimum
                        ? std::get_if<std::int64_t>(&*minimum) : nullptr;
                    if (!integer ||
                        *integer < std::numeric_limits<std::int32_t>::min() ||
                        *integer > std::numeric_limits<std::int32_t>::max())
                        throw std::invalid_argument(
                            "Selected MIN_WATER_SIZE_FOR_OCEAN is invalid");
                    minimum_water_size = static_cast<std::int32_t>(*integer);
                }
                const auto plan = inspect_bnw_fresh_feature_set(
                    setup_plots, *result.map->area_structure, setup_feature_types,
                    static_cast<std::size_t>(id), feature_id,
                    static_cast<std::int32_t>(raw_variety), *found->second,
                    info, minimum_water_size);
                if (plan.blocker ==
                    BnwFreshFeatureSetBlocker::invalid_fresh_contract)
                    throw ScriptError("CIVIS_BNW_FEATURE_WRITE_INVALID:Plot.SetFeatureType fresh state or eligibility differs");
                // CvPlot::setFeatureType calls GameplayFeatureChanged before
                // changing m_eFeatureType. A detached preflight cannot skip
                // that callback or expose any later state as committed.
                throw ScriptError("CIVIS_BNW_FEATURE_WRITE_PENDING:Plot.SetFeatureType:GameplayFeatureChanged before feature write");
            } catch (const ScriptError &) {
                throw;
            } catch (const std::exception &error) {
                throw ScriptError(std::string("CIVIS_BNW_FEATURE_WRITE_INVALID:Plot.SetFeatureType:") +
                                  error.what());
            }
        };
        map_provider.setup_plot_adjacent_shallow_water = [&](std::int64_t id) {
            if (!setup_plots_ready || id < 0 || static_cast<std::uint64_t>(id) >= setup_plots.size())
                throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:invalid shallow-water adjacency target");
            if (!selected_shallow_terrain_id) {
                const auto value = selected_define_lookup("SHALLOW_WATER_TERRAIN");
                const auto *integer = value ? std::get_if<std::int64_t>(&*value) : nullptr;
                if (!integer || *integer < 0 ||
                    *integer > std::numeric_limits<std::int8_t>::max())
                    throw ScriptError("CIVIS_BNW_MAP_RULES_UNRESOLVED:selected SHALLOW_WATER_TERRAIN is invalid");
                try {
                    const auto terrain = resolve_bnw_selected_terrain_by_id(
                        *setup.selected_rules->rules, static_cast<std::int32_t>(*integer));
                    if (!terrain.water)
                        throw ScriptError("Selected SHALLOW_WATER_TERRAIN is not water");
                } catch (const std::exception &error) {
                    throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") + error.what());
                }
                selected_shallow_terrain_id = static_cast<std::int32_t>(*integer);
            }
            return setup_plots.is_adjacent_to_shallow_water(
                static_cast<std::size_t>(id), *selected_shallow_terrain_id);
        };
        map_provider.setup_plot_set_type = [&](std::int64_t id, std::int64_t new_type,
                                               bool recalculate, bool rebuild_graphics,
                                               bool erase_units_if_water) {
            if (!setup_plots_ready || id < 0 || static_cast<std::uint64_t>(id) >= setup_plots.size())
                throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:invalid setup plot setter target");
            if (new_type < 0 || new_type > 3)
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:invalid category");
            if (feature_probe_active) {
                if (new_type != 1 || recalculate || !rebuild_graphics ||
                    !erase_units_if_water || !result.map ||
                    !result.map->area_structure || !result.map->final_yields ||
                    result.map->plot_types.size() != setup_plots.size() ||
                    result.map->final_yields->size() != setup_plots.size())
                    throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:"
                                      "outside fresh feature hill contract");
                const auto index = static_cast<std::size_t>(id);
                if (setup_plots.at(index).yields != (*result.map->final_yields)[index] ||
                    setup_plots.at(index).plot_type != result.map->plot_types[index] ||
                    !selected_yield_rules ||
                    selected_yield_rules->source_rules != setup.selected_rules->rules.get())
                    throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:feature hill cache/rules identity differs");
                try {
                    const auto terrain = resolve_bnw_selected_terrain_by_id(
                        *setup.selected_rules->rules, setup_plots.at(index).terrain_type);
                    if (terrain.water)
                        throw std::invalid_argument("Fresh feature hill terrain is water");
                    auto draft = setup_plots.draft_fresh_feature_mountain_to_hill(
                        index, *result.map->area_structure, *selected_yield_rules,
                        setup.graphics_initialized, budget.maximum_symbol_events);
                    auto next_plot_types = result.map->plot_types;
                    auto next_yields = *result.map->final_yields;
                    next_plot_types[index] = 1;
                    next_yields[index] = draft.grid.at(index).yields;
                    setup_plots = std::move(draft.grid);
                    result.map->plot_types.swap(next_plot_types);
                    result.map->final_yields->swap(next_yields);
                    return;
                } catch (const std::invalid_argument &error) {
                    throw ScriptError(std::string("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:feature hill:") +
                                      error.what());
                }
            }
            if (lake_probe_active) {
                if (lake_calculate_areas_calls != 0)
                    throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:"
                                      "after AddLakes CalculateAreas");
                if (new_type != 3 || !recalculate || !rebuild_graphics ||
                    !erase_units_if_water || !result.map ||
                    !result.map->area_structure || !result.map->final_yields)
                    throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:outside fresh AddLakes contract");
                const auto index = static_cast<std::size_t>(id);
                if (result.map->final_yields->size() != setup_plots.size() ||
                    setup_plots.at(index).yields != (*result.map->final_yields)[index])
                    throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:lake yield identity differs");
                try {
                    const auto old_terrain = resolve_bnw_selected_terrain_by_id(
                        *setup.selected_rules->rules, setup_plots.at(index).terrain_type);
                    auto prefix = setup_plots.draft_fresh_lake_plot_type_prefix(
                        index, *result.map->area_structure, old_terrain);
                    const auto name = prefix.grid.is_adjacent_to_land(index)
                        ? "SHALLOW_WATER_TERRAIN" : "DEEP_WATER_TERRAIN";
                    const auto define = selected_define_lookup(name);
                    const auto *water_id = define
                        ? std::get_if<std::int64_t>(&*define) : nullptr;
                    if (!water_id || *water_id < 0 ||
                        *water_id > std::numeric_limits<std::int8_t>::max())
                        throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") +
                                          name + " is invalid in selected Defines");
                    const auto water_terrain = resolve_bnw_selected_terrain_by_id(
                        *setup.selected_rules->rules,
                        static_cast<std::int32_t>(*water_id));
                    auto draft = BnwSetupPlotGrid::advance_fresh_lake_terrain(
                        std::move(prefix), index, water_terrain);
                    if (!selected_yield_rules ||
                        selected_yield_rules->source_rules != setup.selected_rules->rules.get())
                        throw ScriptError("CIVIS_BNW_MAP_RULES_UNRESOLVED:selected lake yield rules identity differs");
                    const auto selected_water = [&](std::string_view key) {
                        const auto value = selected_define_lookup(key);
                        const auto *id = value ? std::get_if<std::int64_t>(&*value) : nullptr;
                        if (!id || *id < 0 ||
                            *id > std::numeric_limits<std::int8_t>::max())
                            throw ScriptError("CIVIS_BNW_MAP_RULES_UNRESOLVED:selected lake water define is invalid");
                        const auto terrain = resolve_bnw_selected_terrain_by_id(
                            *setup.selected_rules->rules,
                            static_cast<std::int32_t>(*id));
                        if (!terrain.water)
                            throw ScriptError("CIVIS_BNW_MAP_RULES_UNRESOLVED:selected lake terrain is not water");
                        return terrain;
                    };
                    const auto shallow = selected_water("SHALLOW_WATER_TERRAIN");
                    const auto deep = selected_water("DEEP_WATER_TERRAIN");
                    if (!selected_yield_rules->terrains.contains(shallow.id) ||
                        !selected_yield_rules->terrains.contains(deep.id)) {
                        const std::array additional{shallow.id, deep.id};
                        selected_yield_rules = load_bnw_fresh_yield_rules(
                            *setup.selected_rules->rules, setup_plots, additional);
                    }
                    auto neighbors = BnwSetupPlotGrid::advance_fresh_lake_neighbors(
                        std::move(draft), index, *result.map->area_structure,
                        *selected_yield_rules,
                        shallow, deep,
                        budget.maximum_symbol_events);
                    auto city_area =
                        BnwSetupPlotGrid::advance_fresh_lake_city_area_preflight(
                            std::move(neighbors), index, *result.map->area_structure,
                            *selected_yield_rules, budget.maximum_symbol_events);
                    if (!city_area.recalculate_areas) {
                        auto area =
                            BnwSetupPlotGrid::advance_fresh_lake_incremental_area(
                                std::move(city_area), index,
                                *result.map->area_structure,
                                *selected_yield_rules,
                                budget.maximum_symbol_events);
                        auto post =
                            BnwSetupPlotGrid::inspect_fresh_lake_post_setter(
                                std::move(area), setup_plots,
                                *result.map->area_structure, index,
                                setup.graphics_initialized);
                        if (post.remaining_services.empty() ||
                            post.remaining_services.front() != "map_calculate_areas_handoff" ||
                            !result.map->terrain_types ||
                            result.map->terrain_types->size() != setup_plots.size() ||
                            result.map->plot_types.size() != setup_plots.size())
                            throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:"
                                              "fresh lake " + std::string(
                                                  post.remaining_services.empty()
                                                      ? "state_handoff" : post.remaining_services.front()) +
                                              " and downstream effects pending");
                        auto next_plot_types = result.map->plot_types;
                        auto next_terrain_types = *result.map->terrain_types;
                        auto next_yields = *result.map->final_yields;
                        for (std::size_t i = 0; i < post.grid.size(); ++i) {
                            next_plot_types[i] = static_cast<std::uint8_t>(post.grid.at(i).plot_type);
                            next_terrain_types[i] = post.grid.at(i).terrain_type;
                            next_yields[i] = post.grid.at(i).yields;
                        }
                        // No Lua callback runs between these writes. The enclosing
                        // AddLakes snapshot restores all of them with the map RNG
                        // if any later selected helper refuses the transaction.
                        setup_plots = std::move(post.grid);
                        *result.map->area_structure = std::move(post.areas);
                        result.map->plot_types.swap(next_plot_types);
                        result.map->terrain_types->swap(next_terrain_types);
                        result.map->final_yields->swap(next_yields);
                        ++lake_plot_conversions;
                        return;
                    }
                    throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:"
                                      "fresh lake " + std::string(
                                          city_area.remaining_services.front()) +
                                      " and downstream effects pending");
                } catch (const std::invalid_argument &error) {
                    throw ScriptError(std::string("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:lake plot prefix:") +
                                      error.what());
                }
            }
            if (!selected_land_terrain) {
                try { selected_land_terrain = resolve_bnw_selected_land_terrain(*setup.selected_rules->rules); }
                catch (const std::exception &error) {
                    throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") + error.what());
                }
            }
            setup_plots.apply_stock_fresh_plot_type(static_cast<std::size_t>(id),
                static_cast<std::int32_t>(new_type), recalculate, rebuild_graphics,
                erase_units_if_water, *selected_land_terrain);
        };
        map_provider.setup_plot_set_area = [&](std::int64_t id, std::int64_t new_area) {
            if (lake_calculate_areas_calls != 0)
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetArea:"
                                  "after AddLakes CalculateAreas");
            if (!lake_probe_active || !result.map || !result.map->area_structure ||
                !result.map->final_yields || id < 0 ||
                static_cast<std::uint64_t>(id) >= setup_plots.size() || new_area != -1)
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetArea:outside fresh AddLakes contract");
            const auto index = static_cast<std::size_t>(id);
            if (result.map->final_yields->size() != setup_plots.size() ||
                setup_plots.at(index).yields != (*result.map->final_yields)[index])
                throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Plot.SetArea yield identity differs");
            try {
                auto draft = setup_plots.draft_fresh_area_unassignment(
                    index, *result.map->area_structure);
                // The detached draft records CvPlot::setArea source order.
                // No Lua callback can observe between these two publishes;
                // the enclosing AddLakes snapshot restores both on failure.
                setup_plots = std::move(draft.grid);
                *result.map->area_structure = std::move(draft.areas);
            } catch (const std::invalid_argument &error) {
                throw ScriptError(std::string("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Plot.SetArea:") +
                                  error.what());
            }
        };
        map_provider.setup_calculate_areas = [&] {
            if (lake_calculate_areas_calls != 0)
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.CalculateAreas:"
                                  "repeated AddLakes call");
            if (!lake_probe_active || !result.map || !result.map->area_structure ||
                result.map->area_structure->area_ids.size() != setup_plots.size())
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.CalculateAreas:outside fresh AddLakes contract");
            auto next = *result.map->area_structure;
            std::vector<std::int32_t> counts(next.areas.size());
            for (std::size_t i = 0; i < setup_plots.size(); ++i) {
                const auto &plot = setup_plots.at(i);
                if (plot.area_id < 0 || next.area_ids[i] != plot.area_id)
                    throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.CalculateAreas:unassigned plot needs area A-star");
                const auto found = std::find_if(next.areas.begin(), next.areas.end(),
                    [&](const auto &area) { return area.id == plot.area_id; });
                if (found == next.areas.end() || found->water != (plot.plot_type == 3))
                    throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Map.CalculateAreas area identity differs");
                const auto ordinal = static_cast<std::size_t>(found - next.areas.begin());
                if (++counts[ordinal] > found->tiles)
                    throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Map.CalculateAreas tile count differs");
                // CvMap::calculateAreas visits plotByIndexUnchecked in order.
                // Existing area handles take its single-branch bounds update.
                if (plot.x < found->west) found->west = plot.x;
                else if (plot.x > found->east) found->east = plot.x;
                else if (plot.y < found->south) found->south = plot.y;
                else if (plot.y > found->north) found->north = plot.y;
            }
            for (std::size_t i = 0; i < counts.size(); ++i)
                if (counts[i] != next.areas[i].tiles)
                    throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:Map.CalculateAreas area count differs");
            *result.map->area_structure = std::move(next);
            ++lake_calculate_areas_calls;
        };
        map_provider.setup_plot_set_terrain = [&](std::int64_t id, std::int64_t new_type,
                                                  bool recalculate, bool rebuild_graphics) {
            if (!setup_plots_ready || id < 0 || static_cast<std::uint64_t>(id) >= setup_plots.size())
                throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:invalid setup terrain setter target");
            if (new_type < 0 || new_type > std::numeric_limits<std::int8_t>::max())
                throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetTerrainType:invalid ID");
            const auto terrain_id = static_cast<std::int32_t>(new_type);
            auto found = selected_terrains.find(terrain_id);
            if (found == selected_terrains.end()) {
                try {
                    const auto terrain = resolve_bnw_selected_terrain_by_id(
                        *setup.selected_rules->rules, terrain_id);
                    found = selected_terrains.emplace(terrain_id, terrain).first;
                } catch (const std::exception &error) {
                    throw ScriptError(std::string("CIVIS_BNW_MAP_RULES_UNRESOLVED:") + error.what());
                }
            }
            try {
                setup_plots.apply_stock_fresh_terrain_type(static_cast<std::size_t>(id),
                    found->second, recalculate, rebuild_graphics);
            } catch (const std::invalid_argument &error) {
                throw ScriptError(std::string("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetTerrainType:") +
                                  error.what());
            }
        };
    }
    if (setup.world_size)
        map_provider.setup_world_size = [&] { return *setup.world_size; };
    if (setup.custom_options)
        map_provider.setup_custom_option = [&](std::string_view name) -> std::optional<std::int64_t> {
            const auto it = setup.custom_options->find(std::string(name));
            if (it == setup.custom_options->end()) return {};
            return it->second;
        };
    if (map_random) {
        result.map_random_after = map_random->state();
        map_provider.random = [&](std::int64_t range, std::string_view) {
            // CvLuaMap::lRand passes an int through CvGame::getMapRandNum to
            // CvRandom::get(unsigned short). Even 0 and 1 advance the stream.
            const auto source_range = static_cast<std::uint16_t>(static_cast<std::int32_t>(range));
            const auto value = map_random->draw(source_range);
            result.map_random_after = map_random->state();
            return static_cast<std::int64_t>(value);
        };
        map_provider.setup_fractal_create = [&](std::int64_t w, std::int64_t h,
                                                std::int64_t grain, std::int64_t flags,
                                                std::int64_t x_exp, std::int64_t y_exp) {
            if (live_fractals >= budget.maximum_fractals)
                throw ScriptError("CIVIS_BNW_FRACTAL_BUDGET:too many fractal objects");
            const auto fits_int = [](std::int64_t value) {
                return value >= std::numeric_limits<int>::min() &&
                       value <= std::numeric_limits<int>::max();
            };
            if (!fits_int(w) || !fits_int(h) || !fits_int(grain) ||
                !fits_int(x_exp) || !fits_int(y_exp) || flags < 0 || flags > 63)
                throw ScriptError("Fractal.Create arguments are outside the verified SDK range");
            // Reserve the new slot before the constructor/random draws. No
            // ID is published until a real physical Fractal exists.
            fractals.emplace_back();
            try {
                fractals.back() = std::make_unique<BnwFractal>(
                    static_cast<int>(w), static_cast<int>(h),
                    static_cast<int>(grain), static_cast<std::uint32_t>(flags),
                    static_cast<int>(x_exp), static_cast<int>(y_exp), *map_random);
                ++live_fractals;
            } catch (...) {
                fractals.pop_back(); // this unpublished ID never escaped
                result.map_random_after = map_random->state();
                throw;
            }
            result.map_random_after = map_random->state();
            return static_cast<std::int64_t>(fractals.size());
        };
        // Lua userdata GC and failed-feature rollback share this genuine
        // physical resource retirement. The outer session storage outlives
        // LuaScriptHost, including its lua_close finalizers.
        map_provider.setup_fractal_release = [&](std::int64_t id) {
            if (id <= 0 || static_cast<std::uint64_t>(id) > fractals.size())
                throw ScriptError("Fractal release has no published map-session ID");
            auto& slot = fractals[static_cast<std::size_t>(id - 1)];
            if (!slot) return; // already retired by the real rollback/GC path
            if (!live_fractals)
                throw ScriptError("Fractal live ownership counter changed");
            slot.reset(); // actual BnwFractal/vector destructor releases storage
            --live_fractals;
        };
        map_provider.setup_fractal_height = [&](std::int64_t id, std::int64_t x,
                                                std::optional<std::int64_t> y) {
            if (id <= 0 || static_cast<std::uint64_t>(id) > fractals.size() ||
                x < std::numeric_limits<int>::min() ||
                x > std::numeric_limits<int>::max() ||
                (y && (*y < std::numeric_limits<int>::min() ||
                       *y > std::numeric_limits<int>::max())))
                throw ScriptError("Fractal.GetHeight arguments are outside the verified SDK range");
            const auto& slot = fractals[static_cast<std::size_t>(id - 1)];
            if (!slot) throw ScriptError("Fractal.GetHeight receiver was retired");
            const auto& fractal = *slot;
            return static_cast<std::int64_t>(y
                ? fractal.height_at(static_cast<int>(x), static_cast<int>(*y))
                : fractal.height_from_percent(static_cast<int>(x)));
        };
        map_provider.setup_fractal_ridges = [&](std::int64_t id, std::int64_t seeds,
                                                std::int64_t flags, std::int64_t ridge_blend,
                                                std::int64_t fractal_blend) {
            const auto fits_int = [](std::int64_t value) {
                return value >= std::numeric_limits<int>::min() &&
                       value <= std::numeric_limits<int>::max();
            };
            if (id <= 0 || static_cast<std::uint64_t>(id) > fractals.size() ||
                !fits_int(seeds) || !fits_int(ridge_blend) || !fits_int(fractal_blend) ||
                flags < 0 || flags > 63)
                throw ScriptError("Fractal.BuildRidges arguments are outside the verified SDK range");
            auto& slot = fractals[static_cast<std::size_t>(id-1)];
            if (!slot) throw ScriptError("Fractal.BuildRidges receiver was retired");
            try {
                slot->build_ridges(
                    static_cast<int>(seeds), static_cast<std::uint32_t>(flags),
                    static_cast<int>(ridge_blend), static_cast<int>(fractal_blend),
                    *map_random, budget.maximum_ridge_seeds, budget.maximum_ridge_retries);
            } catch (...) {
                result.map_random_after = map_random->state();
                throw;
            }
            result.map_random_after = map_random->state();
        };
    }

    try {
        LuaScriptHost host(budget.lua, std::move(source_provider), {}, {}, {}, {},
                           std::move(map_provider));
        if(selected_source_owner){
            // Binding precedes the very first execute in this SAME VM.
            host.enable_ui_context_sources(selected_source_owner);
            BnwMapScriptSelectedAccess::begin(*selected_source_owner,verified_source);
        }
        // These names exist in stock map scripts, but their native behavior
        // is not yet implemented here. Fail at the first actual use rather
        // than letting a nil-global error look like a broken source file.
        const auto fill_limit = std::min<std::uint64_t>(budget.maximum_plots,
                                                        9'007'199'254'740'992ULL);
        host.execute("map", "local maximum_fill = " + std::to_string(fill_limit) + R"lua(
            local function unavailable(name)
                return setmetatable({}, {
                    __index = function() error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:' .. name, 0) end,
                    __call = function() error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:' .. name, 0) end
                })
            end
            if Fractal == nil then Fractal = unavailable('Fractal') end
            MapGenerator = unavailable('MapGenerator')
            TerrainGenerator = unavailable('TerrainGenerator')
            AssignStartingPlots = unavailable('AssignStartingPlots')
            -- Firaxis' map helpers use both table.fill(value, count) and
            -- table.fill(destination, value, count). Keep the extension in
            -- this map environment; the standard library is shared by the
            -- host's other script contexts.
            local standard_table = table
            table = setmetatable({}, { __index = standard_table })
            table.fill = function(...)
                local argc = select('#', ...)
                local destination, value, count
                if argc == 2 then
                    value, count = ...
                    destination = {}
                elseif argc == 3 then
                    destination, value, count = ...
                    if type(destination) ~= 'table' then
                        error('table.fill destination must be a table', 2)
                    end
                else
                    error('table.fill requires two or three arguments', 2)
                end
                if type(count) ~= 'number' or count ~= count or
                   count < 0 or count % 1 ~= 0 then
                    error('table.fill count must be a nonnegative integer', 2)
                end
                if count > maximum_fill then
                    error('CIVIS_BNW_MAP_FILL_BUDGET:table.fill exceeds selected plot budget', 0)
                end
                for i = 1, count do destination[i] = value end
                return destination
            end
            if GameInfo == nil then GameInfo = unavailable('GameInfo') end
            if GameDefines == nil then GameDefines = unavailable('GameDefines') end
            if Map == nil then Map = unavailable('Map') end
            setmetatable(Map, {
                __index = function(_, key)
                    error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.' .. tostring(key), 0)
                end
            })
        )lua", "=bnw-unported-map-helpers");
        host.execute("map", verified_source,
                     chunk_name.empty() ? "=bnw-map-script" : chunk_name);
        if(selected_source_owner){
            BnwMapScriptSelectedAccess::loaded(*selected_source_owner,verified_source);
            const auto observed=BnwMapScriptSelectedAccess::query(*selected_source_owner,host,
                static_cast<std::int32_t>(*setup.world_size));
            selected_source_owner->require_current("map",nullptr);
            selected_init_job->init=observed;
            if(map_random)result.map_random_after=map_random->state();
            result.included_sources=includes->trace;
            // Constructor/init-data observation only: no InitMap or GenerateMap.
            return result;
        }
        host.execute("map", R"lua(
            if type(GetMapInitData) ~= 'function' then error('GetMapInitData is unavailable') end
            local init
            if rawget(Map, 'GetWorldSize') then init = GetMapInitData(Map.GetWorldSize())
            else init = GetMapInitData() end
            if type(init) ~= 'table' then error('GetMapInitData must return a table') end
            GameEvents.CivisBnwMapWidth.Add(function() return init.Width end)
            GameEvents.CivisBnwMapHeight.Add(function() return init.Height end)
            GameEvents.CivisBnwMapWrapX.Add(function()
                if init.WrapX == nil then return true end
                return not not init.WrapX
            end)
            GameEvents.CivisBnwMapWrapY.Add(function()
                if init.WrapY == nil then return false end
                return not not init.WrapY
            end)
        )lua", "=bnw-map-dimensions");

        try {
            width = read_integer(host, "CivisBnwMapWidth");
            height = read_integer(host, "CivisBnwMapHeight");
            resolved_wraps = std::pair{read_boolean(host, "CivisBnwMapWrapX"),
                                       read_boolean(host, "CivisBnwMapWrapY")};
        } catch (const std::exception &error) {
            block(result, BnwMapScriptBlockerCode::invalid_dimensions, error.what());
            return result;
        }
        if (width <= 0 || height <= 0 ||
            width > std::numeric_limits<std::int32_t>::max() ||
            height > std::numeric_limits<std::int32_t>::max() ||
            budget.maximum_plots == 0 ||
            static_cast<std::uint64_t>(height) > budget.maximum_plots ||
            static_cast<std::uint64_t>(width) >
                budget.maximum_plots / static_cast<std::uint64_t>(height) ||
            static_cast<std::uint64_t>(width) >
                static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) /
                    static_cast<std::uint64_t>(height)) {
            block(result, BnwMapScriptBlockerCode::invalid_dimensions,
                  "Map dimensions are nonpositive or exceed the selected host plot budget");
            return result;
        }
        const auto count = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
        resolved_plot_count = static_cast<std::int64_t>(count);
        if (count > budget.maximum_plot_state_bytes / sizeof(BnwSetupPlot)) {
            block(result, BnwMapScriptBlockerCode::invalid_dimensions,
                  "BNW setup plot state exceeds the selected native memory budget");
            return result;
        }
        setup_plots.reset(static_cast<std::int32_t>(width), static_cast<std::int32_t>(height),
                          resolved_wraps->first, resolved_wraps->second);
        setup_plots_ready = true;
        host.execute("map", R"lua(
            if type(GeneratePlotTypes) ~= 'function' then error('GeneratePlotTypes is unavailable') end
            local plots = GeneratePlotTypes()
            if plots ~= nil and type(plots) ~= 'table' then
                error('GeneratePlotTypes must return a table or mutate the plot grid')
            end
            GameEvents.CivisBnwPlotsMutated.Add(function() return plots == nil end)
            if plots ~= nil then
                GameEvents.CivisBnwPlotType.Add(function(i) return plots[i + 1] end)
                GameEvents.CivisBnwPlotCount.Add(function() return #plots end)
                GameEvents.CivisBnwPlotKeysValid.Add(function(n)
                    for key in pairs(plots) do
                        if type(key) ~= 'number' or key < 1 or key > n or key % 1 ~= 0 then
                            return false
                        end
                    end
                    return true
                end)
            end
        )lua", "=bnw-map-plots");

        try {
            const bool mutated_grid = read_boolean(host, "CivisBnwPlotsMutated");
            if (!mutated_grid) {
                if (read_integer(host, "CivisBnwPlotCount") != static_cast<std::int64_t>(count))
                    throw ScriptError("GeneratePlotTypes returned the wrong plot count");
                const std::array<ScriptValue, 1> count_arg{static_cast<std::int64_t>(count)};
                const auto keys = host.test_all("CivisBnwPlotKeysValid", count_arg);
                if (!keys.errors.empty()) throw ScriptError(keys.errors.front());
                if (keys.value != true) throw ScriptError("GeneratePlotTypes has invalid plot keys");
            }
            BnwGeneratedPlotMap map;
            map.width = static_cast<std::uint64_t>(width);
            map.height = static_cast<std::uint64_t>(height);
            map.wrap_x = resolved_wraps->first;
            map.wrap_y = resolved_wraps->second;
            map.plot_types.reserve(static_cast<std::size_t>(count));
            for (std::uint64_t i = 0; i < count; ++i) {
                std::int64_t value;
                if (mutated_grid) {
                    const auto &plot = setup_plots.at(static_cast<std::size_t>(i));
                    if (!plot.plot_type_assigned_by_script)
                        throw ScriptError("GeneratePlotTypes left an initialized ocean plot without a SetPlotType assignment");
                    value = plot.plot_type;
                } else {
                    const std::array<ScriptValue, 1> index{static_cast<std::int64_t>(i)};
                    value = read_integer(host, "CivisBnwPlotType", index);
                }
                if (value < 0 || value > 3)
                    throw ScriptError("GeneratePlotTypes returned a plot outside BNW PlotTypes 0..3");
                map.plot_types.push_back(static_cast<std::uint8_t>(value));
            }
            result.map = std::move(map);
        } catch (const std::exception &error) {
            block(result, BnwMapScriptBlockerCode::invalid_plot_types, error.what());
        }
        if (result.map) {
            host.execute("map", R"lua(
                GameEvents.CivisBnwHasTerrainGenerator.Add(function()
                    return type(GenerateTerrain)=='function'
                end)
            )lua", "=bnw-terrain-availability");
            if (read_boolean(host, "CivisBnwHasTerrainGenerator")) {
                // Expansion2 MapGenerator.GenerateMap invokes this only after
                // GeneratePlotTypes. Its SetTerrainTypes mutates owned plots.
                for (std::uint64_t i = 0; i < count; ++i) {
                    const auto &plot = setup_plots.at(static_cast<std::size_t>(i));
                    if (!plot.plot_type_assigned_by_script ||
                        plot.plot_type != result.map->plot_types[static_cast<std::size_t>(i)])
                        throw ScriptError("CIVIS_BNW_TERRAIN_INCOMPLETE:GenerateTerrain requires plot categories applied to the owned grid");
                }
                host.execute("map", "GenerateTerrain()", "=bnw-generate-terrain");
                std::vector<std::int32_t> terrain_types;
                terrain_types.reserve(static_cast<std::size_t>(count));
                for (std::uint64_t i = 0; i < count; ++i) {
                    const auto &plot = setup_plots.at(static_cast<std::size_t>(i));
                    if (!plot.terrain_type_assigned_by_script || plot.terrain_type < 0)
                        throw ScriptError("CIVIS_BNW_TERRAIN_INCOMPLETE:GenerateTerrain left a plot without an applied Terrain Info");
                    terrain_types.push_back(plot.terrain_type);
                }
                result.map->terrain_types = std::move(terrain_types);
                std::erase_if(result.blockers, [](const BnwMapScriptBlocker &blocker) {
                    return blocker.code == BnwMapScriptBlockerCode::terrain_pending;
                });
                const auto lake_limit = selected_define_lookup("LAKE_MAX_AREA_SIZE");
                const auto *lake_integer = lake_limit
                    ? std::get_if<std::int64_t>(&*lake_limit) : nullptr;
                if (!lake_integer || *lake_integer < std::numeric_limits<std::int32_t>::min() ||
                    *lake_integer > std::numeric_limits<std::int32_t>::max())
                    throw ScriptError("CIVIS_BNW_MAP_RULES_UNRESOLVED:selected LAKE_MAX_AREA_SIZE is missing or outside SDK int32 range");
                try {
                    // Structural part of CvMap::recalculateAreas. The native
                    // setArea and landmass passes also recalculate plot yields,
                    // so this draft never clears areas_pending.
                    result.map->area_structure =
                        setup_plots.draft_fresh_area_structure(
                            static_cast<std::int32_t>(*lake_integer));
                } catch (const std::exception &error) {
                    block(result, BnwMapScriptBlockerCode::invalid_area_structure,
                          error.what());
                }
                if (result.map->area_structure) {
                    try {
                        selected_yield_rules = load_bnw_fresh_yield_rules(
                            *setup.selected_rules->rules, setup_plots);
                        auto replay = replay_bnw_fresh_yield_passes(
                            setup_plots, *result.map->area_structure, *selected_yield_rules,
                            budget.maximum_yield_replay_plot_visits,
                            budget.maximum_symbol_events);
                        setup_plots.apply_fresh_area_handles(
                            *result.map->area_structure, replay.final_cache,
                            std::move(replay.symbol_events));
                        result.map->final_yields = std::move(replay.final_cache);
                        result.map->yield_progress = std::move(replay.progress);
                        // Expansion2 MapGenerator.GenerateMap calls AddRivers
                        // immediately after this area pass. Run the selected
                        // Lua through verified fresh river setters; later
                        // feature/resource and world stages stay blocked.
                        // The snapshot also guards future setup APIs that may
                        // mutate plots before a later river blocker.
                        auto pre_river_plots = setup_plots;
                        auto pre_river_areas = *result.map->area_structure;
                        auto pre_river_yields = *result.map->final_yields;
                        const auto pre_river_symbols = result.map->river_symbol_events;
                        const auto pre_river_edges = result.map->river_edge_writes;
                        const auto pre_river_random = map_random;
                        const auto pre_river_water_visits = remaining_river_water_visits;
                        const auto rollback_rivers = [&] {
                            setup_plots = std::move(pre_river_plots);
                            *result.map->area_structure = std::move(pre_river_areas);
                            *result.map->final_yields = std::move(pre_river_yields);
                            result.map->river_symbol_events = pre_river_symbols;
                            result.map->river_edge_writes = pre_river_edges;
                            map_random = pre_river_random;
                            remaining_river_water_visits = pre_river_water_visits;
                            result.map_random_after = map_random
                                ? std::optional{map_random->state()} : std::nullopt;
                        };
                        try {
                            host.execute("map", R"lua(
                                GameEvents.CivisBnwHasRiverGenerator.Add(function()
                                    return type(AddRivers)=='function'
                                end)
                            )lua", "=bnw-river-availability");
                            if (read_boolean(host, "CivisBnwHasRiverGenerator")) {
                                host.execute("map", "AddRivers()", "=bnw-add-rivers");
                                result.map_random_after = map_random
                                    ? std::optional{map_random->state()} : std::nullopt;
                            } else {
                                rollback_rivers();
                                block(result, BnwMapScriptBlockerCode::river_generation_pending,
                                      "Selected map script has no AddRivers entrypoint");
                            }
                        } catch (const std::exception &error) {
                            rollback_rivers();
                            block(result, BnwMapScriptBlockerCode::river_generation_pending,
                                  std::string("Selected AddRivers stopped before river edges: ") +
                                      error.what());
                        }
                        if (!result.has_blocker(BnwMapScriptBlockerCode::river_generation_pending)) {
                            // AddLakes runs against the owned setup grid. A completed
                            // selected CalculateAreas commits that stage; any missing
                            // callback restores its plots, areas, yields and map RNG.
                            auto pre_lake_plots = setup_plots;
                            auto pre_lake_map = *result.map;
                            const auto pre_lake_random = map_random;
                            const auto pre_lake_water_visits = remaining_river_water_visits;
                            const auto rollback_lakes = [&] {
                                setup_plots = std::move(pre_lake_plots);
                                *result.map = std::move(pre_lake_map);
                                map_random = pre_lake_random;
                                remaining_river_water_visits = pre_lake_water_visits;
                                result.map_random_after = map_random
                                    ? std::optional{map_random->state()} : std::nullopt;
                            };
                            try {
                                host.execute("map", R"lua(
                                    GameEvents.CivisBnwHasLakeGenerator.Add(function()
                                        return type(AddLakes)=='function'
                                    end)
                                )lua", "=bnw-lake-availability");
                                if (read_boolean(host, "CivisBnwHasLakeGenerator")) {
                                    lake_probe_active = true;
                                    host.execute("map", "AddLakes()", "=bnw-add-lakes");
                                    lake_probe_active = false;
                                    if ((lake_plot_conversions > 0 &&
                                         lake_calculate_areas_calls != 1) ||
                                        (lake_plot_conversions == 0 &&
                                         lake_calculate_areas_calls != 0))
                                        throw ScriptError("CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.CalculateAreas:"
                                                          "selected AddLakes call order differs");
                                    if (!result.map->area_structure ||
                                        result.map->area_structure->area_ids.size() != setup_plots.size())
                                        throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:lake area handoff");
                                    for (std::size_t i = 0; i < setup_plots.size(); ++i)
                                        if (setup_plots.at(i).area_id < 0 ||
                                            setup_plots.at(i).area_id !=
                                                result.map->area_structure->area_ids[i])
                                            throw ScriptError("CIVIS_BNW_MAP_PLOTS_UNRESOLVED:"
                                                              "unassigned lake area at handoff");
                                    result.map->lake_plot_conversions = lake_plot_conversions;
                                    result.map->lake_symbol_events =
                                        setup_plots.symbol_events().size() -
                                        pre_lake_plots.symbol_events().size();
                                    result.map_random_after = map_random
                                        ? std::optional{map_random->state()} : std::nullopt;
                                } else {
                                    rollback_lakes();
                                    block(result, BnwMapScriptBlockerCode::lake_generation_pending,
                                          "Selected map script has no AddLakes entrypoint");
                                }
                            } catch (const std::exception &error) {
                                lake_probe_active = false;
                                rollback_lakes();
                                block(result, BnwMapScriptBlockerCode::lake_generation_pending,
                                      std::string("Selected AddLakes stopped before lake publication: ") +
                                          error.what());
                            }
                        }
                        if (!result.has_blocker(BnwMapScriptBlockerCode::river_generation_pending) &&
                            !result.has_blocker(BnwMapScriptBlockerCode::lake_generation_pending)) {
                            // FeatureGenerator may consume map RNG and allocate
                            // fractals before its first unimplemented plot API.
                            // None of that can escape a failed feature phase.
                            auto pre_feature_plots = setup_plots;
                            auto pre_feature_map = *result.map;
                            const auto pre_feature_random = map_random;
                            const auto pre_feature_fractals = fractals.size();
                            const auto rollback_features = [&] {
                                setup_plots = std::move(pre_feature_plots);
                                *result.map = std::move(pre_feature_map);
                                map_random = pre_feature_random;
                                setup_feature_types.clear();
                                // Keep issued IDs stable while physically
                                // releasing every failed-phase allocation.
                                for (std::size_t id = pre_feature_fractals;
                                     id < fractals.size(); ++id) {
                                    if (fractals[id]) {
                                        fractals[id].reset();
                                        --live_fractals;
                                    }
                                }
                                result.map_random_after = map_random
                                    ? std::optional{map_random->state()} : std::nullopt;
                            };
                            const auto feature_blocker = [&](std::string message) {
                                const auto found = std::find_if(result.blockers.begin(),
                                    result.blockers.end(), [](const auto &entry) {
                                        return entry.code ==
                                            BnwMapScriptBlockerCode::features_rivers_pending;
                                    });
                                if (found != result.blockers.end()) found->message = std::move(message);
                            };
                            try {
                                host.execute("map", R"lua(
                                    GameEvents.CivisBnwHasFeatureGenerator.Add(function()
                                        return type(AddFeatures)=='function'
                                    end)
                                )lua", "=bnw-feature-availability");
                                if (read_boolean(host, "CivisBnwHasFeatureGenerator")) {
                                    setup_feature_types.assign(setup_plots.size(), -1);
                                    feature_probe_active = true;
                                    host.execute("map", "AddFeatures()", "=bnw-add-features");
                                    feature_probe_active = false;
                                    rollback_features();
                                    feature_blocker("Selected AddFeatures returned without verified feature state");
                                } else {
                                    rollback_features();
                                    feature_blocker("Selected map script has no AddFeatures entrypoint");
                                }
                            } catch (const std::exception &error) {
                                feature_probe_active = false;
                                rollback_features();
                                feature_blocker(std::string("Selected AddFeatures stopped before feature publication: ") +
                                                error.what());
                            }
                        }
                        result.map->owned_setup_grid = std::move(setup_plots);
                    } catch (const std::length_error &error) {
                        block(result, BnwMapScriptBlockerCode::yield_replay_budget_exceeded,
                              error.what());
                    } catch (const std::invalid_argument &error) {
                        block(result, BnwMapScriptBlockerCode::invalid_area_structure,
                              error.what());
                    } catch (const std::exception &error) {
                        block(result, BnwMapScriptBlockerCode::invalid_world_rules,
                              std::string("Selected BNW final plot yields are unresolved: ") +
                                  error.what());
                    }
                }
            }
        }
    } catch (const std::exception &error) {
        constexpr std::string_view marker = "CIVIS_UNSUPPORTED_BNW_MAP_HELPER:";
        constexpr std::string_view unresolved_rules = "CIVIS_BNW_MAP_RULES_UNRESOLVED:";
        constexpr std::string_view missing_option = "CIVIS_MISSING_BNW_MAP_OPTION:";
        constexpr std::string_view include_missing = "CIVIS_BNW_INCLUDE_MISSING:";
        constexpr std::string_view include_ambiguous = "CIVIS_BNW_INCLUDE_AMBIGUOUS:";
        constexpr std::string_view include_cycle = "CIVIS_BNW_INCLUDE_CYCLE:";
        constexpr std::string_view include_invalid = "CIVIS_BNW_INCLUDE_INVALID:";
        constexpr std::string_view include_io = "CIVIS_BNW_INCLUDE_IO:";
        constexpr std::string_view unresolved_wrap = "CIVIS_BNW_MAP_WRAP_UNRESOLVED:";
        constexpr std::string_view unresolved_grid = "CIVIS_BNW_MAP_GRID_UNRESOLVED:";
        constexpr std::string_view unresolved_plots = "CIVIS_BNW_MAP_PLOTS_UNRESOLVED:";
        constexpr std::string_view fill_budget = "CIVIS_BNW_MAP_FILL_BUDGET:";
        constexpr std::string_view fractal_budget = "CIVIS_BNW_FRACTAL_BUDGET:";
        constexpr std::string_view terrain_incomplete = "CIVIS_BNW_TERRAIN_INCOMPLETE:";
        const std::string message = error.what();
        if (message.find(unresolved_rules) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::invalid_world_rules, message);
        else if (message.find(terrain_incomplete) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::invalid_terrain_types, message);
        else if (message.find(unresolved_wrap) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::unresolved_map_wrap, message);
        else if (message.find(unresolved_grid) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::unresolved_map_dimensions, message);
        else if (message.find(unresolved_plots) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::unresolved_map_plots, message);
        else if (message.find(fill_budget) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::fill_budget_exceeded, message);
        else if (message.find(fractal_budget) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::fractal_budget_exceeded, message);
        else if (message.find(include_missing) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::include_missing, message);
        else if (message.find(include_ambiguous) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::include_ambiguous, message);
        else if (message.find(include_cycle) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::include_cycle, message);
        else if (message.find(include_invalid) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::include_invalid, message);
        else if (message.find(include_io) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::include_io_failed, message);
        else if (message.find(missing_option) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::missing_map_option, message);
        else if (message.find(marker) != std::string::npos)
            block(result, BnwMapScriptBlockerCode::unsupported_stock_helper, message);
        else
            block(result, BnwMapScriptBlockerCode::script_failed,
                  "BNW map script could not execute with available Lua APIs: " + message);
    }
    result.included_sources = std::move(includes->trace);
    return result;
}

BnwMapScriptResult generate_bnw_plot_map(std::string_view verified_source,
                                         std::string_view chunk_name,
                                         BnwMapScriptBudget budget) {
    return generate_bnw_plot_map_impl(verified_source, chunk_name, {}, {}, budget);
}

BnwMapScriptResult generate_bnw_plot_map(std::string_view verified_source,
                                         std::string_view chunk_name,
                                         SimulationRandom map_random,
                                         BnwMapScriptBudget budget) {
    return generate_bnw_plot_map_impl(verified_source, chunk_name, {}, map_random, budget);
}

BnwMapScriptResult generate_bnw_plot_map(std::string_view verified_source,
                                         std::string_view chunk_name,
                                         BnwMapSetupContext setup,
                                         std::optional<SimulationRandom> map_random,
                                         BnwMapScriptBudget budget) {
    return generate_bnw_plot_map_impl(verified_source, chunk_name, setup, map_random, budget);
}

assets::ResourceMountTable build_bnw_map_script_resources(
    const std::filesystem::path &game_root, const SavedContentProfile &profile,
    const content::Catalog &catalog) {
    const auto root = std::filesystem::canonical(game_root);
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    base->scan({"Assets/Maps", {}, false});
    base->scan({"Assets/Gameplay/Lua", {}, false});
    assets::ResourceMountTable resources(assets::resource_lookup(base),
                                        assets::resource_lookup_all(base), assets::resource_keys(base));
    std::uint64_t mount_id = 0;
    for (const auto index : profile.enabled_dlc) {
        const auto &package = catalog.packages.at(index);
        if (package.kind != content::PackageKind::dlc)
            throw std::runtime_error("Map script resource plan requires selected DLC packages");
        auto source = std::make_shared<assets::LooseResourceIndex>(
            std::filesystem::canonical(package.path.parent_path()));
        source->scan({"Maps", {}, false});
        source->scan({"Gameplay/Lua", {}, false});
        if (source->indexed_files())
            resources.mount({++mount_id, 0, 0, assets::resource_lookup(source), true, true,
                             assets::resource_lookup_all(source), assets::resource_keys(source)});
    }
    return resources;
}

BnwMapScriptResult generate_bnw_plot_map_from_resources(
    const assets::ResourceMountTable &resources, const BnwMapSourceKey &key,
    const BnwMapSetupContext &setup,
    std::optional<SimulationRandom> map_random, BnwMapScriptBudget budget,
    std::shared_ptr<const BnwFreshGameContentResult> selected_parent = {},
    std::optional<ScriptMapInitQueryResult>* selected_init = nullptr) {
    BnwMapScriptResult result;
    std::optional<assets::AssetKey> request;
    try {
        request.emplace(key.relative_path);
    } catch (const std::exception &error) {
        block(result, BnwMapScriptBlockerCode::invalid_source_key, error.what());
        return result;
    }
    if (!request->safe_relative_path() ||
        request->normalized_name().find('/') == std::string::npos ||
        !request->normalized_name().ends_with(".lua")) {
        block(result, BnwMapScriptBlockerCode::invalid_source_key,
              "Map script key must be an exact relative .lua path");
        return result;
    }
    try {
        std::vector<assets::AssetHandle> candidates;
        bool unresolved_ambiguity = false;
        for (auto &mounted : resources.resolve_all(*request)) {
            if (auto *ambiguous = std::get_if<assets::AmbiguousAsset>(&mounted.asset)) {
                unresolved_ambiguity |= !ambiguous->matches.empty();
                continue;
            }
            auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
            if (!handle) continue;
            const auto &source = handle->candidate().provenance;
            if (assets::normalize_asset_name(source.source_relative_path) != request->normalized_name() ||
                (key.source_id && source.source_id != *key.source_id))
                continue;
            candidates.push_back(std::move(*handle));
        }
        if (candidates.empty()) {
            block(result, unresolved_ambiguity ? BnwMapScriptBlockerCode::source_ambiguous
                                               : BnwMapScriptBlockerCode::source_missing,
                  "No unique active resource matches the exact map script key");
            return result;
        }
        if (!key.sha256 && (candidates.size() != 1 || unresolved_ambiguity)) {
            block(result, BnwMapScriptBlockerCode::source_ambiguous,
                  "Several active resources match the map script key; pin source ID or SHA-256");
            return result;
        }
        std::optional<assets::AssetHandle> selected;
        std::vector<std::uint8_t> bytes;
        std::string digest;
        for (auto &candidate : candidates) {
            auto data = candidate.read_bytes(budget.maximum_source_bytes);
            const auto hash = identify_bytes(data).sha256;
            if (key.sha256 && hash != *key.sha256) continue;
            if (selected) {
                block(result, BnwMapScriptBlockerCode::source_ambiguous,
                      "Several active resources satisfy the map script hash; pin source ID");
                return result;
            }
            selected = std::move(candidate);
            bytes = std::move(data);
            digest = hash;
        }
        if (!selected) {
            block(result, BnwMapScriptBlockerCode::source_hash_mismatch,
                  "No active map script source matches the pinned SHA-256");
            return result;
        }
        auto includes = std::make_shared<BnwIncludeManifest>();
        if (!discover_includes(resources, *includes, result)) return result;
        const std::string source(bytes.begin(), bytes.end());
        std::optional<BnwSelectedMapInitJob> selected_job;
        if(selected_parent){
            if(!selected_init)throw ScriptError("Selected map init job has no result storage");
            selected_job.emplace(BnwSelectedMapInitJob{std::move(selected_parent),*selected,&resources,{}});
        }
        result = generate_bnw_plot_map_impl(
            source, "@" + selected->candidate().provenance.source_relative_path,
            setup, map_random, budget, includes,selected_job?&*selected_job:nullptr);
        if(selected_job&&selected_job->init)*selected_init=selected_job->init;
        result.source = BnwMapScriptSource{key.relative_path, digest,
                                           selected->candidate().provenance};
    } catch (const std::exception &error) {
        block(result, BnwMapScriptBlockerCode::source_io_failed,
              std::string("Map script source could not be read: ") + error.what());
    }
    return result;
}

BnwMapScriptResult generate_bnw_plot_map(const assets::ResourceMountTable &resources,
                                         const BnwMapSourceKey &key,
                                         BnwMapScriptBudget budget) {
    return generate_bnw_plot_map_from_resources(resources, key, {}, {}, budget);
}

BnwMapScriptResult generate_bnw_plot_map(const assets::ResourceMountTable &resources,
                                         const BnwMapSourceKey &key,
                                         SimulationRandom map_random,
                                         BnwMapScriptBudget budget) {
    return generate_bnw_plot_map_from_resources(resources, key, {}, map_random, budget);
}

BnwMapScriptResult generate_bnw_plot_map(const assets::ResourceMountTable &resources,
                                         const BnwMapSourceKey &key,
                                         BnwMapSetupContext setup,
                                         std::optional<SimulationRandom> map_random,
                                         BnwMapScriptBudget budget) {
    return generate_bnw_plot_map_from_resources(resources, key, setup, map_random, budget);
}

BnwSelectedMapInitResult query_bnw_selected_map_init_data(
    std::shared_ptr<const BnwFreshGameContentResult> parent,const assets::ResourceMountTable& resources,
    const BnwMapSourceKey& key,BnwMapSetupContext setup,SimulationRandom random,BnwMapScriptBudget budget){
    BnwSelectedMapInitResult result;
    if(!parent||!parent->content_ready()||!parent->rules)
        throw ScriptError("Selected map constructor requires genuine current selected content");
    if(setup.selected_rules&&setup.selected_rules!=parent.get())
        throw ScriptError("Selected map constructor setup refers to a different content parent");
    setup.selected_rules=parent.get();
    result.constructor=generate_bnw_plot_map_from_resources(resources,key,setup,random,budget,
        std::move(parent),&result.init);
    return result;
}

struct BnwSelectedMapJob::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> retired{false};bool busy=false;
    BnwSelectedMapJobBoundary boundary=BnwSelectedMapJobBoundary::constructor_loaded;
    std::shared_ptr<BnwFreshGameContentResult::ContentEpoch> epoch;
    std::filesystem::path root;
    SavedContentProfile profile;
    std::vector<std::pair<std::filesystem::path,FileIdentity>> manifests;
    std::unique_ptr<assets::ResourceMountTable> resources;
    BnwMapSetupContext setup;
    SimulationRandom random;
    BnwMapScriptBudget budget;
    BnwSelectedMapInitResult result;
    std::shared_ptr<BnwIncludeManifest> includes;
    std::shared_ptr<BnwSelectedMapSourceOwner> source;
    BnwSetupPlotGrid plots; // No reset before native predecessor services.
    std::unique_ptr<LuaScriptHost> host; // Destroy VM BEFORE source/input state.
    Impl(BnwMapSetupContext s,SimulationRandom rng,BnwMapScriptBudget b)
      :setup(std::move(s)),random(std::move(rng)),budget(b){}
    void check()const {
        if(retired.load()||thread!=std::this_thread::get_id())
            throw ScriptError("Retained map job retired or foreign thread");
    }
    void retire()noexcept {retired.store(true);if(source)BnwMapScriptSelectedAccess::retire(*source);}
    static bool same_profile(const SavedContentProfile& a,const SavedContentProfile& b) {
        if(a.kind!=b.kind||a.enabled_dlc!=b.enabled_dlc||a.enabled_mods!=b.enabled_mods||
           a.all_requirements_located!=b.all_requirements_located||a.files_complete!=b.files_complete||
           a.native_mods_require_port!=b.native_mods_require_port||a.can_activate!=b.can_activate||
           a.requirements.size()!=b.requirements.size())return false;
        for(std::size_t i=0;i<a.requirements.size();++i){const auto& x=a.requirements[i];const auto& y=b.requirements[i];
            if(x.kind!=y.kind||x.id!=y.id||x.name!=y.name||x.wire_value!=y.wire_value||
               x.status!=y.status||x.package!=y.package||x.candidates!=y.candidates)return false;
        }return true;
    }
    void current()const {
        check();
        if(!epoch)throw ScriptError("Retained map genuine content epoch absent");
        epoch->require_current();
        if(!same_profile(epoch->view.profile,profile)||std::filesystem::canonical(root)!=epoch->installation)
            throw ScriptError("Retained map selected installation/profile changed");
        for(const auto& [path,identity]:manifests){const auto now=identify_file(path);
            if(now.bytes!=identity.bytes||now.sha256!=identity.sha256)
                throw ScriptError("Retained map selected manifest bytes changed");}
        if(source)BnwMapScriptSelectedAccess::same_job(*source,*resources,setup,random);
        check();
        epoch->require_current();
        if(!same_profile(epoch->view.profile,profile))
            throw ScriptError("Retained map parent changed during source callback");
    }
    struct Call {
        std::shared_ptr<Impl> held;
        explicit Call(std::shared_ptr<Impl> p):held(std::move(p)){
            held->check();if(held->busy){held->retire();throw ScriptError("Retained map job reentry");}held->busy=true;
        }
        ~Call(){held->busy=false;}
    };
    static std::shared_ptr<Impl> lock(const std::weak_ptr<Impl>& weak){
        const auto p=weak.lock();if(!p)throw ScriptError("Retained map provider expired");p->current();return p;
    }
};
BnwSelectedMapJob::BnwSelectedMapJob(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwSelectedMapJob::~BnwSelectedMapJob(){impl_->retire();}
std::shared_ptr<BnwSelectedMapJob> BnwSelectedMapJob::from_selected_installation(
    std::shared_ptr<const BnwFreshGameContentResult> parent,const std::filesystem::path& installation,
    const content::Catalog& catalog,const BnwMapSourceKey& key,BnwMapSetupContext setup,
    SimulationRandom random,BnwMapScriptBudget budget){
    const auto epoch=BnwFreshGameContentResult::retain_epoch(parent);
    epoch->require_recipe(installation,catalog);
    if(!epoch->view.profile.enabled_mods.empty()||
       !setup.world_size||*setup.world_size<0||*setup.world_size>std::numeric_limits<std::int32_t>::max()||
       !setup.runtime_defines.empty()||random.algorithm()!=RandomAlgorithm::bnw||
       (setup.selected_rules&&setup.selected_rules!=parent.get()))
        throw ScriptError("Retained map job requires actual selected stock BNW rules/world/local RNG");
    setup.selected_rules=&epoch->view;
    auto p=std::make_shared<Impl>(std::move(setup),std::move(random),budget);Impl::Call call(p);
    p->epoch=epoch;p->profile=epoch->view.profile;p->root=epoch->installation;
    p->result.constructor.map_random_before=p->random.state();
    try {
        for(const auto index:p->profile.enabled_dlc){const auto& supplied=catalog.packages.at(index);
            const auto path=std::filesystem::canonical(supplied.path);const auto actual=content::read_manifest(path);
            if(actual.kind!=supplied.kind||actual.kind!=content::PackageKind::dlc||
               content::canonical_guid(actual.id)!=content::canonical_guid(supplied.id)||actual.version!=supplied.version)
                throw ScriptError("Retained map selected Catalog differs from actual manifest");
            p->manifests.emplace_back(path,identify_file(path));
        }
        p->resources=std::make_unique<assets::ResourceMountTable>(build_bnw_map_script_resources(p->root,p->profile,epoch->catalog));
        const assets::AssetKey request(key.relative_path);
        if(!request.safe_relative_path()||request.normalized_name().find('/')==std::string::npos||
           !request.normalized_name().ends_with(".lua"))
            throw ScriptError("Retained map requires exact relative Lua key");
        std::optional<assets::AssetHandle> selected;std::string bytes,digest;bool ambiguous=false;
        for(const auto& hit:p->resources->resolve_all(request)){
            if(const auto* a=std::get_if<assets::AmbiguousAsset>(&hit.asset)){ambiguous|=!a->matches.empty();continue;}
            const auto* handle=std::get_if<assets::AssetHandle>(&hit.asset);if(!handle)continue;
            const auto& provenance=handle->candidate().provenance;
            if(assets::normalize_asset_name(provenance.source_relative_path)!=request.normalized_name()||
               (key.source_id&&provenance.source_id!=*key.source_id))continue;
            const auto data=handle->read_bytes(budget.maximum_source_bytes);const auto sha=identify_bytes(data).sha256;
            if(key.sha256&&sha!=*key.sha256)continue;
            if(selected)throw ScriptError("Retained map source remains ambiguous");
            selected=*handle;bytes.assign(data.begin(),data.end());digest=sha;
        }
        if(!selected||(!key.sha256&&ambiguous))throw ScriptError("Retained map source absent/ambiguous/hash mismatch");
        p->includes=std::make_shared<BnwIncludeManifest>();
        if(!discover_includes(*p->resources,*p->includes,p->result.constructor))
            throw ScriptError("Retained map selected include discovery failed");
        std::vector<std::pair<std::string,assets::AssetHandle>> handles;
        for(const auto& [name,handle]:p->includes->handles)handles.emplace_back(name,handle);
        p->source=BnwMapScriptSelectedAccess::make_epoch(p->epoch,*selected,std::move(handles),*p->resources,
            p->setup,p->random,budget.maximum_source_bytes);
        p->result.constructor.source=BnwMapScriptSource{key.relative_path,digest,selected->candidate().provenance};
        const std::weak_ptr<Impl> weak=p;
        ScriptSourceProvider sources;sources.strict_map_includes=true;
        sources.files=[weak](std::string_view context){const auto job=Impl::lock(weak);
            if(context!="map")throw ScriptError("Retained map include outside actual context");return job->includes->names;};
        sources.read=[weak](std::string_view context,std::string_view borrowed){const std::string name(borrowed);const auto job=Impl::lock(weak);
            if(context!="map")throw ScriptError("Retained map include outside actual context");
            const auto found=job->includes->handles.find(name);if(found==job->includes->handles.end())throw ScriptError("Retained map unselected include");
            const auto data=found->second.read_bytes(job->budget.maximum_source_bytes);
            BnwMapScriptSelectedAccess::include(*job->source,name,data);job->current();
            job->includes->trace.push_back({name,identify_bytes(data).sha256,found->second.candidate().provenance});
            return std::string(data.begin(),data.end());};
        ScriptMapProvider map;
        map.grid_size=[weak]()->std::pair<std::int64_t,std::int64_t>{const auto job=Impl::lock(weak);(void)job;
            throw ScriptError("Retained map grid requires native InitMap predecessors");};
        map.setup_world_size=[weak]{const auto job=Impl::lock(weak);return *job->setup.world_size;};
        if(p->setup.custom_options)map.setup_custom_option=[weak](std::string_view borrowed)->std::optional<std::int64_t>{
            const std::string name(borrowed);const auto job=Impl::lock(weak);const auto found=job->setup.custom_options->find(name);
            if(found==job->setup.custom_options->end())return {};return found->second;};
        std::vector<ScriptMapWorldRow> worlds;std::vector<ScriptMapClimateRow> climates;
        if(!load_selected_worlds(p->setup,p->result.constructor,worlds)||
           !load_selected_climates(p->setup,p->result.constructor,climates))throw ScriptError("Retained map actual rules rows unavailable");
        map.setup_worlds=std::move(worlds);map.setup_climates=std::move(climates);map.setup_climate=p->setup.climate;
        map.random=[weak](std::int64_t range,std::string_view){const auto job=Impl::lock(weak);
            const auto bound=static_cast<std::uint16_t>(static_cast<std::int32_t>(range));
            const auto value=job->random.draw(bound);job->check();return static_cast<std::int64_t>(value);};
        p->host=std::make_unique<LuaScriptHost>(budget.lua,std::move(sources),ScriptModdingProvider{},
            ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},std::move(map));
        p->host->enable_ui_context_sources(p->source);
        BnwMapScriptSelectedAccess::begin(*p->source,bytes);
        const auto fill_limit=std::min<std::uint64_t>(budget.maximum_plots,9'007'199'254'740'992ULL);
        p->host->execute("map", "local maximum_fill = " + std::to_string(fill_limit) + R"lua(
            local function unavailable(name)
                return setmetatable({}, {
                    __index = function() error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:' .. name, 0) end,
                    __call = function() error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:' .. name, 0) end
                })
            end
            if Fractal == nil then Fractal = unavailable('Fractal') end
            MapGenerator = unavailable('MapGenerator')
            TerrainGenerator = unavailable('TerrainGenerator')
            AssignStartingPlots = unavailable('AssignStartingPlots')
            -- Firaxis' map helpers use both table.fill(value, count) and
            -- table.fill(destination, value, count). Keep the extension in
            -- this map environment; the standard library is shared by the
            -- host's other script contexts.
            local standard_table = table
            table = setmetatable({}, { __index = standard_table })
            table.fill = function(...)
                local argc = select('#', ...)
                local destination, value, count
                if argc == 2 then
                    value, count = ...
                    destination = {}
                elseif argc == 3 then
                    destination, value, count = ...
                    if type(destination) ~= 'table' then
                        error('table.fill destination must be a table', 2)
                    end
                else
                    error('table.fill requires two or three arguments', 2)
                end
                if type(count) ~= 'number' or count ~= count or
                   count < 0 or count % 1 ~= 0 then
                    error('table.fill count must be a nonnegative integer', 2)
                end
                if count > maximum_fill then
                    error('CIVIS_BNW_MAP_FILL_BUDGET:table.fill exceeds selected plot budget', 0)
                end
                for i = 1, count do destination[i] = value end
                return destination
            end
            if GameInfo == nil then GameInfo = unavailable('GameInfo') end
            if GameDefines == nil then GameDefines = unavailable('GameDefines') end
            if Map == nil then Map = unavailable('Map') end
            setmetatable(Map, {
                __index = function(_, key)
                    error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.' .. tostring(key), 0)
                end
            })
        )lua", "=bnw-unported-map-helpers");
        p->host->execute("map",bytes,"@"+selected->candidate().provenance.source_relative_path);
        BnwMapScriptSelectedAccess::loaded(*p->source,bytes);p->current();
        p->result.constructor.included_sources=p->includes->trace;
        p->result.constructor.map_random_after=p->random.state();
        return std::shared_ptr<BnwSelectedMapJob>(new BnwSelectedMapJob(p));
    }catch(...){p->retire();throw;}
}
void BnwSelectedMapJob::advance_init_data_query(){
    const auto p=impl_;Impl::Call call(p);
    try{p->current();if(p->boundary!=BnwSelectedMapJobBoundary::constructor_loaded)
            throw ScriptError("Retained map init-data query already consumed/out of order");
        const auto value=BnwMapScriptSelectedAccess::query(*p->source,*p->host,static_cast<std::int32_t>(*p->setup.world_size));
        p->current();p->result.init=value;p->result.constructor.map_random_after=p->random.state();
        p->result.constructor.included_sources=p->includes->trace;p->boundary=BnwSelectedMapJobBoundary::init_data_returned;
    }catch(...){p->retire();throw;}
}
void BnwSelectedMapJob::advance_native_map_init(){
    const auto p=impl_;Impl::Call call(p);try{p->current();}catch(...){p->retire();throw;}
    if(p->boundary!=BnwSelectedMapJobBoundary::init_data_returned)
        throw ScriptError("Retained map native-init lacks one completed actual query");
    p->boundary=BnwSelectedMapJobBoundary::native_pregame_operands_required;
    // CvMap::init331 first reads genuine Pregame world/climate/sea-level
    // descriptions. The requested setup and detached query cannot issue them.
    // A false table_result selects init(nullptr), requiring actual WorldInfo
    // defaults; do NOT substitute the query DTO's fallback dimensions here.
    throw BnwSelectedMapNativeServicesUnavailable("CvMap::init actual Pregame World/Climate/SeaLevel logging operands required before reset; CoCreateGuid/resource span/DoMapSetup/pathfinders/InitPlots remain mandatory");
}
void BnwSelectedMapJob::require_current()const{const auto p=impl_;Impl::Call call(p);try{p->current();}catch(...){p->retire();throw;}}
BnwSelectedMapJobObservation BnwSelectedMapJob::observe()const{const auto p=impl_;Impl::Call call(p);
    try{p->current();BnwSelectedMapJobObservation value{p->boundary,p->result,p->random.state(),p->plots.size()};p->current();return value;}
    catch(...){p->retire();throw;}}
std::shared_ptr<const assets::ResourceMountTable> BnwSelectedMapJob::resource_source_view()const{
    const auto p=impl_;Impl::Call call(p);try{p->current();return {p,p->resources.get()};}catch(...){p->retire();throw;}}

} // namespace civ5::runtime
