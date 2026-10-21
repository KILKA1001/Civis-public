#include "civ5/saves/map_regions.hpp"
#include <bit>
#include <stdexcept>
#include <string_view>

namespace civ5::saves {
namespace {
struct RegionReader {
    runtime::BinaryReader &input;
    GameStateBudget budget;
    bool vp;
    RegionWireDimensions dimensions;
    void claim(std::uint64_t n, std::uint64_t bytes) {
        if (n > budget.elements || (bytes && n > input.remaining() / bytes))
            throw std::runtime_error("Map regions exceed input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(char type) {
        switch (type) {
        case 'i':
            return input.i32();
        case 'I':
            return input.u32();
        case 'b':
            return std::bit_cast<std::int8_t>(input.u8());
        case '?': {
            const auto n = input.u8();
            if (n > 1)
                throw std::runtime_error("Unverified map region boolean");
            return n;
        }
        default:
            throw std::logic_error("Invalid map region codec");
        }
    }
    void scalar(MapRegionRecord &r, const char *name, std::int64_t n) {
        claim(1, 0);
        r.fields.push_back({name, n});
    }
    void fields(MapRegionRecord &r, char type, std::initializer_list<const char *> names) {
        for (const auto name : names)
            scalar(r, name, number(type));
    }
    std::vector<std::int64_t> values(std::uint64_t rows, std::string_view types) {
        if (types.empty() || rows > budget.elements / types.size())
            throw std::runtime_error("Map region array dimensions exceed element budget");
        std::uint64_t row_bytes = 0;
        for (const auto t : types)
            row_bytes += (t == 'i' || t == 'I') ? 4 : 1;
        if (rows > input.remaining() / row_bytes)
            throw std::runtime_error("Map region array exceeds input");
        claim(rows * types.size(), 0);
        std::vector<std::int64_t> result;
        result.reserve(static_cast<std::size_t>(rows * types.size()));
        for (std::uint64_t row = 0; row < rows; ++row)
            for (const auto t : types)
                result.push_back(number(t));
        return result;
    }
    void array(MapRegionRecord &r, const char *name, std::uint64_t n, std::string_view types) {
        r.arrays.push_back({name, types.size(), values(n, types)});
    }
    void counted(MapRegionRecord &r, const char *name, const char *count_name, const char *types) {
        const auto count = input.u32();
        scalar(r, count_name, count);
        array(r, name, count == UINT32_MAX ? 0 : count, types);
    }
    std::vector<MapResourceCount> hashes(std::uint64_t n, bool improvements) {
        claim(n, 4);
        std::vector<MapResourceCount> result;
        result.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) {
            MapResourceCount item;
            item.type_hash = input.u32();
            if (*item.type_hash && (!improvements || *item.type_hash != UINT32_MAX))
                item.count = input.i32();
            result.push_back(item);
        }
        return result;
    }
    void area(MapRegionRecord &r) {
        fields(r, 'i',
               {"id", "tiles", "owned_tiles", "river_edges", "units", "cities", "population",
                "starting_plots", "natural_wonders"});
        fields(r, 'I', {"total_found_value"});
        if (vp)
            fields(r, 'i', {"bad_plots"});
        fields(r, 'i', {"north", "south", "east", "west"});
        fields(r, '?', {"water", "mountains"});
        if (vp) {
            counted(r, "units_per_player", "units_per_player_wire_count", "ii");
            counted(r, "cities_per_player", "cities_per_player_wire_count", "ii");
            counted(r, "population_per_player", "population_per_player_wire_count", "ii");
            counted(r, "free_specialists", "free_specialists_wire_count", "ii");
            counted(r, "revealed_tiles", "revealed_tiles_wire_count", "ii");
            counted(r, "target_cities", "target_cities_wire_count", "iii");
            const auto raw = input.u32();
            scalar(r, "yield_modifiers_wire_count", raw);
            const auto n = raw == UINT32_MAX ? 0 : raw;
            claim(n, 8);
            for (std::uint64_t i = 0; i < n; ++i) {
                PlotIndexedValues item;
                item.key = input.i32();
                item.wire_count = input.u32();
                item.values = values(item.wire_count == UINT32_MAX ? 0 : item.wire_count, "i");
                r.yield_modifiers.push_back(std::move(item));
            }
            counted(r, "resources", "resources_wire_count", "ii");
            counted(r, "improvements", "improvements_wire_count", "ii");
        } else {
            for (auto name :
                 {"units_per_player", "cities_per_player", "population_per_player", "free_specialists"})
                array(r, name, dimensions.fixed_players, "i");
            array(r, "revealed_tiles", dimensions.fixed_teams, "i");
            array(r, "target_cities", dimensions.players, "ii");
            // Do not construct a format string of an untrusted dimension.
            if (!dimensions.yields || dimensions.players > budget.elements / dimensions.yields)
                throw std::runtime_error("Map area yield dimensions exceed element budget");
            r.arrays.push_back(
                {"yield_modifiers", dimensions.yields, values(dimensions.players * dimensions.yields, "i")});
            r.resources = hashes(input.u32(), false);
            const auto raw = input.i32();
            scalar(r, "improvements_wire_count", raw);
            r.improvements = hashes(raw > 0 ? static_cast<std::uint64_t>(raw) : 0, true);
        }
    }
    MapRegionRecord record(std::string_view kind) {
        MapRegionRecord r;
        r.start_offset = input.position();
        if (!vp) {
            r.version = input.u32();
            if (*r.version != 1)
                throw std::runtime_error("Unverified BNW map region version");
        }
        if (kind == "areas")
            area(r);
        else if (kind == "landmasses" || kind == "continents") {
            fields(r, 'i', {"id", "tiles", "centroid_x", "centroid_y"});
            fields(r, '?', {kind == "landmasses" ? "water" : "land"});
            if (kind == "landmasses") {
                fields(r, 'b', {"continent_style"});
                if (vp)
                    counted(r, "areas", "areas_wire_count", "i");
            }
        } else {
            fields(r, 'i', {"id"});
            counted(r, "plots", "plots_wire_count", "i");
        }
        r.end_offset = input.position();
        return r;
    }
    MapRegionContainer container(const char *name) {
        MapRegionContainer c{};
        c.name = name;
        c.start_offset = input.position();
        if (!vp) {
            c.legacy_metadata = values(5, "i");
            if (c.legacy_metadata[0] < 0)
                throw std::runtime_error("Negative BNW map region slot count");
            c.next_free = values(static_cast<std::uint64_t>(c.legacy_metadata[0]), "i");
        }
        c.wire_count = input.i32();
        const auto n = c.wire_count > 0 ? static_cast<std::uint64_t>(c.wire_count) : 0;
        claim(n, 4);
        c.records.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i)
            c.records.push_back(record(name));
        c.end_offset = input.position();
        return c;
    }
};
} // namespace
MapRegionsObservation observe_map_regions(runtime::BinaryReader &input, GameStateLayout layout,
                                          RegionWireDimensions dimensions, GameStateBudget budget) {
    if (layout != GameStateLayout::vp542_corpus && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unverified map region layout");
    const bool vp = layout == GameStateLayout::vp542_corpus;
    RegionReader reader{input, budget, vp, dimensions};
    MapRegionsObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    if (!vp)
        result.dimensions = dimensions;
    result.containers.push_back(reader.container("areas"));
    result.containers.push_back(reader.container("landmasses"));
    if (vp) {
        result.containers.push_back(reader.container("continents"));
        result.containers.push_back(reader.container("rivers"));
    }
    result.ai_map_hints = input.i32();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
