#include "civ5/saves/map_state.hpp"
#include "civ5/saves/vp_plot_projection.hpp"
#include "civ5/saves/pregame.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::saves {
std::uint64_t MapGrid::size() const {
    if (!width || !height || width > std::numeric_limits<std::uint64_t>::max() / height)
        throw std::runtime_error("Invalid or overflowing map grid");
    return width * height;
}
std::uint64_t MapGrid::index(std::uint64_t x, std::uint64_t y) const {
    (void)size();
    if (x >= width || y >= height)
        throw std::runtime_error("Map coordinate outside grid");
    return y * width + x;
}
std::array<std::uint64_t, 2> MapGrid::coordinates(std::uint64_t i) const {
    if (i >= size())
        throw std::runtime_error("Map index outside grid");
    return {i % width, i / width};
}
namespace {
struct MapReader {
    runtime::BinaryReader &input;
    GameStateBudget budget;
    void claim(std::uint64_t n, std::uint64_t minimum) {
        if (n > budget.elements || (minimum && n > input.remaining() / minimum))
            throw std::runtime_error("Map data exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(char type) {
        switch (type) {
        case 'b':
            return std::bit_cast<std::int8_t>(input.u8());
        case 'B':
            return input.u8();
        case 'h':
            return input.i16();
        case 'H':
            return input.u16();
        case 'i':
            return input.i32();
        case 'I':
            return input.u32();
        case '?': {
            const auto value = input.u8();
            if (value > 1)
                throw std::runtime_error("Unverified map boolean");
            return value;
        }
        default:
            throw std::logic_error("Invalid map field codec");
        }
    }
    std::uint64_t count() { return input.u32(); }
    void fields(PlotObservation &plot, char type, std::initializer_list<const char *> names) {
        claim(names.size(), 1);
        for (const auto name : names)
            plot.fields.push_back({name, number(type)});
    }
    void scalar(PlotObservation &plot, const char *name, std::int64_t value) {
        claim(1, 0);
        plot.fields.push_back({name, value});
    }
    std::vector<std::uint8_t> bytes() {
        const auto n = count();
        if (n > budget.string_bytes || n > budget.total_string_bytes || n > input.remaining())
            throw std::runtime_error("Map string exceeds input or string budget");
        budget.total_string_bytes -= n;
        std::vector<std::uint8_t> data(static_cast<std::size_t>(n));
        input.read(data);
        return data;
    }
    void array(PlotObservation &plot, const char *name, std::uint64_t rows, const char *types) {
        const std::string_view format(types);
        if (rows > budget.elements / format.size() || rows > input.remaining() / format.size())
            throw std::runtime_error("Map array exceeds input or element budget");
        claim(rows * format.size(), 1);
        PlotArrayObservation a{name, format.size(), {}};
        a.values.reserve(static_cast<std::size_t>(rows * format.size()));
        for (std::uint64_t row = 0; row < rows; ++row)
            for (auto type : format)
                a.values.push_back(number(type));
        plot.arrays.push_back(std::move(a));
    }
    void counted(PlotObservation &plot, const char *name, const char *count_name, const char *types) {
        const auto raw = count();
        scalar(plot, count_name, static_cast<std::int64_t>(raw));
        array(plot, name, raw == UINT32_MAX ? 0 : raw, types);
    }
    std::vector<MapResourceCount> hashed(bool short_values = false) {
        const auto n = count();
        claim(n, 4);
        std::vector<MapResourceCount> result;
        result.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) {
            MapResourceCount row;
            row.type_hash = input.u32();
            if (*row.type_hash)
                row.count = number(short_values ? 'h' : 'i');
            result.push_back(row);
        }
        return result;
    }
    void vp_plot(PlotObservation &p, PlotWireDimensions d) {
        fields(p, 'h', {"area", "ownership_duration", "improvement_duration", "upgrade_progress"});
        fields(p, 'b',
               {"major_civs_revealed", "city_radius_count", "recon_count", "river_crossing_count",
                "resource_count"});
        fields(p, 'h', {"landmass", "continent"});
        counted(p, "rivers", "rivers_wire_count", "i");
        fields(p, 'H', {"packed_flags"});
        fields(p, 'b',
               {"unit_plot_experience", "unit_plot_ga_experience", "change_moves", "restore_moves",
                "free_move_across", "owner", "plot_type", "terrain_type"});
        fields(p, 'i', {"feature_type", "resource_type", "improvement_type"});
        fields(p, 'b',
               {"improvement_builder", "improvement_responsible", "route_builder", "route_responsible",
                "barbarian_camp_clearer", "landmark_credit_minor", "dig_clearer", "city_destroyer",
                "route_type"});
        fields(p, 'h', {"unit_increment"});
        fields(p, 'b',
               {"world_anchor", "world_anchor_data", "river_e_flow", "river_se_flow", "river_sw_flow"});
        fields(p, '?', {"is_city"});
        fields(
            p, 'i',
            {"owning_city_owner", "owning_city_id", "owning_city_override_owner", "owning_city_override_id"});
        array(p, "yields", d.yields, "B");
        // radius, visibility, maximum visibility, revealed owner, forced
        // resource reveal, improvement, route, impassable, strategic, planned.
        array(p, "teams", d.teams, "BBBb?ii??b");
        const auto bits = count();
        scalar(p, "revealed_bit_count", static_cast<std::int64_t>(bits));
        if (bits != d.players)
            throw std::runtime_error("VP revealed bitfield does not match source player slots");
        array(p, "revealed_words", bits / 32 + (bits % 32 != 0), "I");
        fields(p, 'b', {"river_crossing_bits"});
        if (number('?'))
            p.script_data = bytes();
        counted(p, "build_progress", "build_progress_wire_count", "ii");
        counted(p, "invisible_unit_visibility", "invisible_unit_wire_count", "ii");
        const auto raw = count();
        scalar(p, "invisible_visibility_wire_count", static_cast<std::int64_t>(raw));
        const auto n = raw == UINT32_MAX ? 0 : raw;
        claim(n, 8);
        p.invisible_visibility.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) {
            PlotIndexedValues row;
            row.key = input.i32();
            const auto raw_values = count();
            // Keep the nested sentinel distinct from a zero-length vector.
            row.wire_count = raw_values;
            const auto values = raw_values == UINT32_MAX ? 0 : raw_values;
            claim(values, 4);
            for (std::uint64_t j = 0; j < values; ++j)
                row.values.push_back(input.i32());
            p.invisible_visibility.push_back(std::move(row));
        }
        array(p, "units", count(), "ii");
        fields(p, 'b', {"continent_style"});
        fields(p, 'i',
               {"artifact_type", "artifact_era", "artifact_player1", "artifact_player2", "artifact_work",
                "trade_route_units"});
        fields(p, 'h', {"last_build_change_turn", "spawned_resource_x", "spawned_resource_y"});
    }
    void bnw_plot(PlotObservation &p, PlotWireDimensions d) {
        p.version = count();
        if (*p.version < 1 || *p.version > 7)
            throw std::runtime_error("Unverified BNW plot version");
        fields(p, 'h', {"source_x", "source_y"});
        fields(p, 'i', {"area"});
        fields(p, 'b', {"feature_variety"});
        fields(p, 'h', {"ownership_duration", "improvement_duration", "upgrade_progress", "culture"});
        fields(p, 'b',
               {"major_civs_revealed", "city_radius_count", "recon_count", "river_crossing_count",
                "resource_count", "builder_scratch_player"});
        fields(p, 'h', {"builder_scratch_turn", "builder_scratch_value"});
        fields(p, 'i', {"builder_scratch_route", "landmass"});
        fields(p, 'I', {"trade_route_flags"});
        fields(p, '?',
               {"starting_plot", "hills", "ne_of_river", "w_of_river", "nw_of_river", "potential_city_work",
                "improvement_pillaged", "route_pillaged", "barbarian_camp_not_converting", "rough_feature",
                "resource_linked_city_active", "improved_by_gift"});
        fields(p, 'b', {"owner", "plot_type", "terrain_type"});
        if (*p.version >= 3)
            fields(p, 'I', {"feature_type_hash"});
        else
            fields(p, 'b', {"feature_type"});
        fields(p, 'I', {"resource_type_hash"});
        if (*p.version >= 5)
            fields(p, 'I', {"improvement_type_hash"});
        else
            fields(p, 'b', {"improvement_type"});
        if (*p.version >= 7)
            fields(p, 'I', {"improvement_under_construction_hash"});
        else
            scalar(p, "improvement_under_construction", -1);
        if (*p.version >= 2)
            fields(p, 'b', {"improvement_builder"});
        else
            scalar(p, "improvement_builder", -1);
        fields(p, 'b',
               {"improvement_responsible", "route_responsible", "barbarian_camp_clearer", "route_type",
                "world_anchor", "world_anchor_data", "river_e_flow", "river_se_flow", "river_sw_flow"});
        fields(p, 'i',
               {"plot_city_owner", "plot_city_id", "working_city_owner", "working_city_id",
                "working_override_owner", "working_override_id", "resource_linked_city_owner",
                "resource_linked_city_id", "purchase_city_owner", "purchase_city_id"});
        array(p, "yields", d.yields, "h");
        array(p, "found_values", d.players, "i");
        array(p, "city_radius", d.players, "b");
        array(p, "visibility", d.teams, "h");
        array(p, "revealed_owners", d.teams, "b");
        fields(p, 'b', {"river_crossing_bits"});
        array(p, "revealed_words", 4, "I");
        array(p, "resource_force_reveal", d.teams, "?");
        array(p, *p.version >= 6 ? "revealed_improvement_hashes" : "revealed_improvements", d.teams,
              *p.version >= 6 ? "I" : "h");
        array(p, "revealed_routes", d.teams, "h");
        array(p, "no_settling", d.major_civilizations, "?");
        if (number('?'))
            p.script_data = bytes();
        const auto builds = input.i32();
        scalar(p, "build_progress_declared_count", builds);
        if (builds > 0)
            p.legacy_build_progress = hashed(true);
        if (d.invisible_types && d.teams > std::numeric_limits<std::uint64_t>::max() / d.invisible_types)
            throw std::runtime_error("Invisible visibility dimensions overflow");
        array(p, "invisible_visibility", d.teams * d.invisible_types, "h");
        array(p, "units", count(), "ii");
        fields(p, 'b', {"continent_style"});
        const auto version = count();
        scalar(p, "archaeology_version", static_cast<std::int64_t>(version));
        if (version < 1 || version > 2)
            throw std::runtime_error("Unverified archaeology version");
        fields(p, 'i', {"artifact_type", "artifact_era", "artifact_player1", "artifact_player2"});
        if (version >= 2)
            fields(p, 'i', {"artifact_work"});
        else
            scalar(p, "artifact_work", -1);
    }
};
} // namespace
MapHeaderObservation observe_map_header(runtime::BinaryReader &input, GameStateLayout layout,
                                        std::optional<std::uint64_t> vp_resource_slots,
                                        GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if ((!vp && layout != GameStateLayout::expansion2_version1) || vp != vp_resource_slots.has_value())
        throw std::runtime_error("Map dimensions do not match layout");
    MapHeaderObservation h{};
    h.layout = layout;
    h.start_offset = input.position();
    MapReader r{input, budget};
    if (!vp) {
        h.version = input.u32();
        if (*h.version != 1)
            throw std::runtime_error("Unverified BNW map version");
    }
    const auto width = input.i32(), height = input.i32();
    if (width <= 0 || height <= 0)
        throw std::runtime_error("Nonpositive saved map dimensions");
    h.grid = {static_cast<std::uint64_t>(width), static_cast<std::uint64_t>(height)};
    h.land_plots = input.i32();
    h.owned_plots = input.i32();
    h.natural_wonders = input.i32();
    h.top_latitude = input.i32();
    h.bottom_latitude = input.i32();
    h.wrap_x = r.number('?') != 0;
    h.wrap_y = r.number('?') != 0;
    input.read(h.guid);
    if (vp)
        h.map_generated = r.number('?') != 0;
    h.resources_offset = input.position();
    for (auto &rows : h.resources) {
        if (!vp)
            rows = r.hashed();
        else {
            r.claim(*vp_resource_slots, 4);
            rows.reserve(static_cast<std::size_t>(*vp_resource_slots));
            for (std::uint64_t i = 0; i < *vp_resource_slots; ++i)
                rows.push_back({{}, input.i32()});
        }
    }
    h.end_offset = input.position();
    return h;
}
MapPlotsObservation observe_map_plots(runtime::BinaryReader &input, const MapHeaderObservation &h,
                                      PlotWireDimensions d, MapPlotBudget budget) {
    const bool vp = h.layout == GameStateLayout::vp542_corpus;
    if ((!vp && h.layout != GameStateLayout::expansion2_version1) || !d.players || !d.teams || !d.yields ||
        (!vp && (!d.major_civilizations || !d.invisible_types)))
        throw std::runtime_error("Invalid plot dimensions or layout");
    const auto size = h.grid.size();
    if (size > budget.plots || size > input.remaining() / 100)
        throw std::runtime_error("Map plots exceed input or plot budget");
    MapPlotsObservation result{h.layout, d, input.position(), 0, {}};
    result.plots.reserve(static_cast<std::size_t>(size));
    MapReader r{input, budget.data};
    for (std::uint64_t i = 0; i < size; ++i) {
        PlotObservation p;
        p.index = i;
        const auto xy = h.grid.coordinates(i);
        p.x = xy[0];
        p.y = xy[1];
        p.start_offset = input.position();
        if (vp)
            r.vp_plot(p, d);
        else
            r.bnw_plot(p, d);
        p.end_offset = input.position();
        result.plots.push_back(std::move(p));
    }
    result.end_offset = input.position();
    return result;
}
SavedPlotTeamArtObservation observe_saved_plot_team_art(
    const PlotObservation &plot, GameStateLayout layout, std::uint64_t active_team) {
    const auto array = [&](const char *name, std::uint64_t columns,
                           bool team_indexed = true) -> const PlotArrayObservation & {
        for (const auto &candidate : plot.arrays) {
            if (candidate.name && std::string_view(candidate.name) == name) {
                if (candidate.columns != columns || candidate.values.size() % columns != 0 ||
                    (team_indexed && active_team >= candidate.values.size() / columns))
                    throw std::runtime_error("Saved team art array shape or team index is invalid");
                return candidate;
            }
        }
        throw std::runtime_error("Saved team art array is missing");
    };
    SavedPlotTeamArtObservation result;
    result.plot_index = plot.index;
    const auto revealed_for_team = [&]() {
        const auto &words = array("revealed_words", 1, false);
        const auto word = active_team / 32;
        if (word >= words.values.size() || words.values[static_cast<std::size_t>(word)] < 0 ||
            words.values[static_cast<std::size_t>(word)] > UINT32_MAX)
            throw std::runtime_error("Saved team reveal bitfield is invalid");
        return (static_cast<std::uint32_t>(words.values[static_cast<std::size_t>(word)]) &
                (std::uint32_t{1} << (active_team % 32))) != 0;
    };
    if (layout == GameStateLayout::vp542_corpus) {
        // VP's ten-value team tuple: radius, visibility, maximum visibility,
        // owner, resource force reveal, improvement, route, impassable,
        // strategic route, planned route.
        const auto &teams = array("teams", 10);
        const auto base = static_cast<std::size_t>(active_team * 10);
        result.visibility_count = teams.values[base + 1];
        result.revealed_improvement = teams.values[base + 5];
        result.revealed_route = teams.values[base + 6];
        // VP serializes this bitfield with player-slot capacity, but
        // CvPlot::isRevealed/setRevealed address its bits by TeamTypes.
        const auto bit_count = std::find_if(plot.fields.begin(), plot.fields.end(), [](const auto &field) {
            return field.name && std::string_view(field.name) == "revealed_bit_count";
        });
        if (bit_count == plot.fields.end() || bit_count->value < 0 ||
            active_team >= static_cast<std::uint64_t>(bit_count->value))
            throw std::runtime_error("Saved VP reveal bitfield does not cover active team");
        result.revealed = revealed_for_team();
    } else if (layout == GameStateLayout::expansion2_version1) {
        if (!plot.version || *plot.version < 1 || *plot.version > 7)
            throw std::runtime_error("Saved BNW team art plot version is invalid");
        result.visibility_count = array("visibility", 1).values[static_cast<std::size_t>(active_team)];
        result.revealed = revealed_for_team();
        if (*plot.version >= 6) {
            const auto hash = array("revealed_improvement_hashes", 1)
                                  .values[static_cast<std::size_t>(active_team)];
            if (hash < 0 || hash > UINT32_MAX)
                throw std::runtime_error("Saved BNW improvement hash is invalid");
            result.revealed_improvement_hash = static_cast<std::uint64_t>(hash);
        } else
            result.revealed_improvement =
                array("revealed_improvements", 1).values[static_cast<std::size_t>(active_team)];
        result.revealed_route =
            array("revealed_routes", 1).values[static_cast<std::size_t>(active_team)];
    } else
        throw std::runtime_error("Unsupported saved team art layout");
    if (result.visibility_count < 0)
        throw std::runtime_error("Saved team visibility count is negative");
    return result;
}
SavedMapTeamArtObservation observe_saved_map_team_art(
    const PregameObservation &setup, const MapPlotsObservation &map) {
    if (setup.layout != map.layout || !map.dimensions.players || !map.dimensions.teams)
        throw std::runtime_error("Saved setup and map art layouts or dimensions differ");
    const auto player_it = setup.archive.integers.find("active_player");
    const auto teams_it = setup.archive.integer_arrays.find("team_types");
    if (player_it == setup.archive.integers.end() || teams_it == setup.archive.integer_arrays.end() ||
        player_it->second < 0 || static_cast<std::uint64_t>(player_it->second) >= map.dimensions.players ||
        static_cast<std::uint64_t>(player_it->second) >= teams_it->second.size())
        throw std::runtime_error("Saved active player cannot be mapped to a team");
    const auto player = static_cast<std::uint64_t>(player_it->second);
    const auto team = teams_it->second[static_cast<std::size_t>(player)];
    if (team < 0 || static_cast<std::uint64_t>(team) >= map.dimensions.teams)
        throw std::runtime_error("Saved active team is outside map dimensions");
    SavedMapTeamArtObservation result;
    result.active_player = player;
    result.active_team = static_cast<std::uint64_t>(team);
    result.plots.reserve(map.plots.size());
    for (const auto &plot : map.plots)
        result.plots.push_back(observe_saved_plot_team_art(plot, map.layout, result.active_team));
    return result;
}
namespace {
template <class T> std::optional<T> named(const PlotObservation &plot, const char *name) {
    for (const auto &field : plot.fields)
        if (field.name && std::string_view(field.name) == name)
            return static_cast<T>(field.value);
    return std::nullopt;
}
std::optional<std::uint64_t> named_hash(const PlotObservation &plot, const char *name) {
    for (const auto &field : plot.fields)
        if (field.name && std::string_view(field.name) == name)
            return static_cast<std::uint64_t>(field.value);
    return std::nullopt;
}
std::optional<SavedRiverPlot> project_rivers(const PlotObservation &plot, GameStateLayout layout) {
    const auto count = named<std::int64_t>(plot, "river_crossing_count");
    if (!count || *count < -128 || *count > 127) return {};
    SavedRiverPlot result;
    result.is_river = *count > 0;
    if (!result.is_river) return result;
    if (layout == GameStateLayout::vp542_corpus) {
        const auto flags = named<std::int64_t>(plot, "packed_flags");
        if (!flags || *flags < 0 || *flags > 65535) return {};
        // VP CvPlot::Serialize PlotSaveStructBitFlags (not class bitfield order).
        result.northeast = (*flags & (1 << 6)) != 0;
        result.west = (*flags & (1 << 7)) != 0;
        result.northwest = (*flags & (1 << 8)) != 0;
    } else {
        const auto ne = named<std::int64_t>(plot, "ne_of_river"),
                   w = named<std::int64_t>(plot, "w_of_river"),
                   nw = named<std::int64_t>(plot, "nw_of_river");
        if (!ne || !w || !nw || *ne < 0 || *ne > 1 || *w < 0 || *w > 1 || *nw < 0 || *nw > 1) return {};
        result.northeast = *ne != 0;
        result.west = *w != 0;
        result.northwest = *nw != 0;
    }
    const auto flow = [&](const char *name, bool used, std::int32_t &destination) {
        if (!used) return true;
        const auto value = named<std::int64_t>(plot, name);
        if (!value || *value < -128 || *value > 127) return false;
        destination = static_cast<std::int32_t>(*value);
        return true;
    };
    if (!flow("river_e_flow", result.west, result.flow_e) ||
        !flow("river_se_flow", result.northwest, result.flow_se) ||
        !flow("river_sw_flow", result.northeast, result.flow_sw)) return {};
    return result;
}
SavedMapCellProjection project_cell(const PlotObservation &plot, const SavedTerrainFeatureResolver &features,
                                    GameStateLayout layout) {
    SavedMapCellProjection result;
    result.index = plot.index;
    result.x = plot.x;
    result.y = plot.y;
    result.rivers = project_rivers(plot, layout);
    const auto route = named<std::int64_t>(plot, "route_type");
    const auto responsible = named<std::int64_t>(plot, "route_responsible");
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (const auto flags=named<std::int64_t>(plot,vp?"packed_flags":"potential_city_work"))
        result.potential_city_work=vp?(*flags & (1<<9))!=0:*flags!=0;
    if(vp)ensure_vp_plot_projection(result);
    for(const auto &a:plot.arrays)if(a.name){
        const std::string_view name(a.name);
        if(name=="yields"){
            if(result.cached_yields||a.columns!=1)throw std::runtime_error("Invalid or duplicate plot yields");
            result.cached_yields.emplace();
            for(std::size_t i=0;i<a.values.size();++i){
                if(i>INT64_MAX)throw std::overflow_error("Plot yield ID exceeds int64");
                // Keep zero entries too: a missing active yield is unresolved.
                result.cached_yields->emplace(static_cast<std::int64_t>(i),a.values[i]);}
        }else if(vp&&name=="teams"){
            auto &state=ensure_vp_plot_projection(result);
            if(state.revealed_types||a.columns!=10||a.values.size()%10)throw std::runtime_error("Invalid or duplicate revealed plot types");
            state.revealed_types.emplace();
            for(std::size_t i=0;i<a.values.size()/10;++i){
                if(i>INT64_MAX)throw std::overflow_error("Plot reveal team exceeds int64");
                state.revealed_types->emplace(static_cast<std::int64_t>(i),VpPlotRevealedTypes{a.values[i*10+3],a.values[i*10+5],a.values[i*10+6]});}
        }
    }
    const auto pillaged = named<std::int64_t>(plot, vp ? "packed_flags" : "route_pillaged");
    const auto builder = vp ? named<std::int64_t>(plot, "route_builder") : std::nullopt;
    if (route && responsible && pillaged && (!vp || builder))
        result.route = SavedRoutePlot{*route, *responsible, builder, vp ? (*pillaged & 8) != 0 : *pillaged != 0};
    result.landmass = named<std::int64_t>(plot, "landmass");
    result.artifact_era = named<std::int64_t>(plot, "artifact_era");
    result.trade_route_units = named<std::int64_t>(plot, "trade_route_units");
    result.river_crossing_bits = named<std::int64_t>(plot, "river_crossing_bits");
    const auto city_reference = [&](const char *owner_name, const char *id_name) -> std::optional<SavedPlotCityReference> {
        const auto owner = named<std::int64_t>(plot, owner_name), id = named<std::int64_t>(plot, id_name);
        if (owner && id) return SavedPlotCityReference{*owner, *id};
        return {};
    };
    result.yield_city = city_reference(vp ? "owning_city_owner" : "working_city_owner", vp ? "owning_city_id" : "working_city_id");
    result.plot_city = vp ? result.yield_city : city_reference("plot_city_owner", "plot_city_id");
    result.yield_city_override = city_reference(vp ? "owning_city_override_owner" : "working_override_owner", vp ? "owning_city_override_id" : "working_override_id");
    for (const auto &array : plot.arrays) if (array.name) {
        const std::string_view name(array.name);
        if (name == "units") {
            if (result.units || array.columns != 2 || array.values.size() % 2)
                throw std::runtime_error("Invalid or duplicate plot unit handles");
            result.units.emplace();
            for (std::size_t i=0;i<array.values.size();i+=2)
                result.units->push_back({array.values[i],array.values[i+1]});
        } else if (name == "revealed_words") {
            if (result.revealed_teams || array.columns != 1)
                throw std::runtime_error("Invalid or duplicate plot revealed words");
            const auto bits=vp?named<std::int64_t>(plot,"revealed_bit_count"):std::optional<std::int64_t>{};
            if (vp && (!bits || *bits<0 || static_cast<std::uint64_t>(*bits)/32+(*bits%32!=0)!=array.values.size()))
                throw std::runtime_error("Invalid VP revealed bit span");
            result.revealed_teams.emplace();
            for (std::size_t word=0;word<array.values.size();++word) {
                const auto value=array.values[word];
                if (value<0 || value>UINT32_MAX || word>static_cast<std::uint64_t>(INT64_MAX)/32)
                    throw std::runtime_error("Invalid saved plot reveal word");
                for (std::int64_t bit=0;bit<32;++bit) {
                    const auto team=static_cast<std::int64_t>(word*32)+bit;
                    if ((!bits || team<*bits) && (static_cast<std::uint64_t>(value)&(std::uint64_t{1}<<bit)))
                        result.revealed_teams->insert(team);
                }
            }
        }
    }
    if (const auto flags = named<std::int64_t>(plot, vp ? "packed_flags" : "improvement_pillaged")) {
        result.improvement_pillaged = vp ? (*flags & 4) != 0 : *flags != 0;
        if (vp) result.improvement_passable = (*flags & 2) != 0;
    }
    for (const auto &array : plot.arrays) if (array.name && std::string_view(array.name) == (vp ? "teams" : "resource_force_reveal")) {
        const std::size_t columns = vp ? 10 : 1, offset = vp ? 4 : 0;
        if (result.force_resource_reveal || array.columns != columns || array.values.size() % columns)
            throw std::runtime_error("Invalid or duplicate plot resource reveal flags");
        result.force_resource_reveal.emplace();
        for (std::size_t i = 0; i < array.values.size() / columns; ++i) {
            const auto flag = array.values[i * columns + offset];
            if ((flag != 0 && flag != 1) || i > INT64_MAX)
                throw std::runtime_error("Invalid plot resource reveal team/flag");
            if (flag) result.force_resource_reveal->insert(static_cast<std::int64_t>(i));
        }
    }
    result.owner = named<std::int64_t>(plot, "owner");
    result.plot_type = named<std::int64_t>(plot, "plot_type");
    result.terrain_type = named<std::int64_t>(plot, "terrain_type");
    result.feature_type = named<std::int64_t>(plot, "feature_type");
    result.resource_type = named<std::int64_t>(plot, "resource_type");
    result.improvement_type = named<std::int64_t>(plot, "improvement_type");
    result.continent_type = named<std::int64_t>(plot, "continent_style");
    result.feature_type_hash = named_hash(plot, "feature_type_hash");
    result.resource_type_hash = named_hash(plot, "resource_type_hash");
    result.improvement_type_hash = named_hash(plot, "improvement_type_hash");
    if (const auto city = named<std::int64_t>(plot, "is_city"))
        result.is_city = *city != 0;
    if (!result.terrain_type)
        return result;
    const auto terrain = *result.terrain_type;
    if (terrain == 5)
        result.native_terrain_mask = 0x00002000;
    else if (terrain == 6)
        result.native_terrain_mask = 0x00004000;
    else if (result.plot_type && terrain >= INT32_MIN && terrain <= INT32_MAX) {
        // SDK PlotTypes: MOUNTAIN=0, HILLS=1, LAND=2, OCEAN=3.
        const auto plot_type = *result.plot_type;
        if (plot_type == 0)
            result.native_terrain_mask = 0x00000400;
        else {
            // The retail producer sends every non-water terrain other than
            // Grass/Desert/Tundra/Snow through its Plains fallback.
            const auto base = terrain == 0 ? 0u : terrain == 2 ? 2u : terrain == 3 ? 3u : terrain == 4 ? 4u : 1u;
            result.native_terrain_mask = std::uint32_t{1} << (base + (plot_type == 1 ? 5u : 0u));
        }
    } else
        return result;
    const bool no_feature = (result.feature_type && *result.feature_type == -1) ||
                            (result.feature_type_hash && *result.feature_type_hash == 0);
    std::optional<SavedTerrainFeature> feature;
    if (!no_feature) {
        if (!features || !(feature = features(result)))
            return result;
    }
    const auto feature_id = no_feature ? std::optional<std::int64_t>(-1) :
        result.feature_type ? result.feature_type : feature->id;
    if (!(result.feature_type && result.feature_type_hash) && feature_id &&
        *feature_id >= INT32_MIN && *feature_id <= INT32_MAX &&
        terrain >= INT32_MIN && terrain <= INT32_MAX && result.plot_type &&
        *result.plot_type >= INT32_MIN && *result.plot_type <= INT32_MAX &&
        result.continent_type && *result.continent_type >= -128 && *result.continent_type <= 255)
        result.auxiliary = SavedTerrainAuxiliaryPlot{static_cast<std::int32_t>(terrain),
            static_cast<std::int32_t>(*result.plot_type), static_cast<std::int32_t>(*result.continent_type),
            static_cast<std::int32_t>(*feature_id)};
    if (result.continent_type && *result.continent_type >= -128 && *result.continent_type <= 255)
        result.native_category = static_cast<std::uint8_t>(*result.continent_type);
    if (terrain == 5 || terrain == 6)
        result.native_category = 0;
    if (feature && feature->natural_wonder) {
        // Natural wonders replace the base mask. Ordinary features never enter
        // this string table. The original restores the raw continent byte here.
        result.native_category.reset();
        if (result.continent_type && *result.continent_type >= -128 && *result.continent_type <= 255)
            result.native_category = static_cast<std::uint8_t>(*result.continent_type);
        static constexpr std::pair<std::string_view, std::uint32_t> wonders[] = {
            {"FEATURE_GEYSER", 0x8000}, {"FEATURE_CRATER", 0x10000},
            {"FEATURE_GIBRALTAR", 0x4000000}, {"FEATURE_FUJI", 0x40000},
            {"FEATURE_MESA", 0x80000}, {"FEATURE_REEF", 0x100000},
            {"FEATURE_VOLCANO", 0x200000}, {"FEATURE_POTOSI", 0x20000},
            {"FEATURE_ULURU", 0x400000}, {"FEATURE_MT_SINAI", 0x800000},
            {"FEATURE_MT_KAILASH", 0x1000000}, {"FEATURE_SRI_PADA", 0x2000000},
            {"FEATURE_KILIMANJARO", 0x800}, {"FEATURE_LAKE_VICTORIA", 0x1000},
            {"FEATURE_SOLOMONS_MINES", 4}};
        if (feature->type != "FEATURE_EL_DORADO" && feature->type != "FEATURE_FOUNTAIN_YOUTH") {
            const auto found = std::find_if(std::begin(wonders), std::end(wonders), [&](const auto &entry) {
                return entry.first == feature->type;
            });
            result.native_terrain_mask = found == std::end(wonders) ? 0x40000 : found->second;
        }
    }
    result.native_terrain_mask_known = true;
    return result;
}
} // namespace
SavedMapProjection project_saved_map(const MapHeaderObservation &header,
                                     const MapPlotsObservation &plots, const SavedTerrainFeatureResolver &features) {
    if (header.layout != plots.layout)
        throw std::runtime_error("Saved map projection layout mismatch");
    const auto size = header.grid.size();
    if (plots.plots.size() != size)
        throw std::runtime_error("Saved map projection plot count mismatch");
    SavedMapProjection result{header.grid, header.wrap_x, header.wrap_y, {}};
    result.cells.reserve(plots.plots.size());
    std::vector<bool> seen(static_cast<std::size_t>(size));
    for (const auto &plot : plots.plots) {
        if (plot.index >= size || seen[static_cast<std::size_t>(plot.index)] ||
            header.grid.index(plot.x, plot.y) != plot.index)
            throw std::runtime_error("Saved map projection has invalid or duplicate plot index");
        seen[static_cast<std::size_t>(plot.index)] = true;
        result.cells.push_back(project_cell(plot, features, plots.layout));
    }
    if (std::find(seen.begin(), seen.end(), false) != seen.end())
        throw std::runtime_error("Saved map projection has a missing plot");
    std::sort(result.cells.begin(), result.cells.end(), [](const auto &left, const auto &right) {
        return left.index < right.index;
    });
    return result;
}

SavedMapTerrainSource project_saved_map_terrain(const SavedMapProjection &projection) {
    const auto size = projection.grid.size();
    if (projection.cells.size() != size)
        throw std::runtime_error("Saved terrain projection cell count mismatch");
    SavedMapTerrainSource result{projection.grid, projection.wrap_x, projection.wrap_y, {}, 0, false};
    result.cells.resize(static_cast<std::size_t>(size));
    std::vector<bool> seen(static_cast<std::size_t>(size));
    for (const auto &cell : projection.cells) {
        if (cell.index >= size || cell.x >= projection.grid.width || cell.y >= projection.grid.height ||
            projection.grid.index(cell.x, cell.y) != cell.index || seen[static_cast<std::size_t>(cell.index)])
            throw std::runtime_error("Saved terrain projection has an invalid or duplicate cell");
        seen[static_cast<std::size_t>(cell.index)] = true;
        if (cell.x > std::numeric_limits<std::uint64_t>::max() / projection.grid.height)
            throw std::runtime_error("Saved terrain projection x-major index overflow");
        const auto x_major = cell.x * projection.grid.height + cell.y;
        if (x_major >= size)
            throw std::runtime_error("Saved terrain projection x-major index outside grid");
        auto &destination = result.cells[static_cast<std::size_t>(x_major)];
        destination.native_terrain_mask = cell.native_terrain_mask;
        destination.native_terrain_mask_known = cell.native_terrain_mask_known;
        destination.native_category = cell.native_category;
        destination.rivers = cell.rivers;
        destination.auxiliary = cell.auxiliary;
        if (destination.native_terrain_mask_known)
            ++result.known_terrain_masks;
    }
    if (std::find(seen.begin(), seen.end(), false) != seen.end())
        throw std::runtime_error("Saved terrain projection has a missing cell");
    // Final cell classification alone does not construct the transformed and
    // padded native grid or its claim/reserved storage. Keep this false even
    // when every mask is known; TerrainLogicalClaims needs that complete buffer.
    return result;
}
} // namespace civ5::saves
