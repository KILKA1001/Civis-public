#include "civ5/saves/player_traits.hpp"
#include <stdexcept>
#include <string>

namespace civ5::saves {
namespace {
using E = PlayerTraitsEncoding;
struct FieldSpec {
    const char *name;
    E encoding;
    unsigned minimum_version, maximum_version;
};
#include "player_traits_fields.inc"
struct TraitsReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    PlayerTraitsWireDimensions dimensions;
    std::int64_t combat_count = 0;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player traits exceed input or element budget");
        budget.elements -= n;
    }
    std::uint64_t product(std::uint64_t n, std::uint64_t columns) const {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("Player trait dimensions exceed budget");
        return n * columns;
    }
    std::int64_t number(bool flag = false) {
        if (!flag)
            return in.i32();
        const auto n = in.u8();
        if (n > 1)
            throw std::runtime_error("Unverified player trait boolean");
        return n;
    }
    std::uint64_t size(std::int64_t raw, bool stl) const {
        return raw <= 0 || (stl && vp && raw == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(raw);
    }
    void values(std::vector<std::int64_t> &out, std::uint64_t n, bool flag = false) {
        claim(n, flag ? 1 : 4);
        out.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i)
            out.push_back(number(flag));
    }
    PlayerTraitsField field(const FieldSpec &spec) {
        claim(1);
        PlayerTraitsField f{};
        f.name = spec.name;
        f.encoding = spec.encoding;
        f.start_offset = in.position();
        const auto type = spec.encoding;
        if (type == E::raw_bool8) {
            claim(1, 1);
            f.values.push_back(in.u8());
        } else if (type == E::i32 || type == E::boolean)
            values(f.values, 1, type == E::boolean);
        else if (type == E::yield_array || type == E::terrain_array)
            values(f.values, type == E::yield_array ? dimensions.yields : dimensions.terrains);
        else {
            const bool counted = type == E::counted_i32 || type == E::counted_bool ||
                                 type == E::counted_pairs || type == E::combat_i32;
            const auto raw = type == E::shared_combat_i32 ? combat_count
                             : counted                    ? std::int64_t(in.i32())
                                                          : in.u32();
            if (type != E::shared_combat_i32)
                f.wire_count = raw;
            if (type == E::combat_i32)
                combat_count = raw;
            const auto n = size(raw, !counted && type != E::hashed_i32);
            if (type == E::hashed_i32) {
                claim(n, 4);
                for (std::uint64_t i = 0; i < n; ++i) {
                    const auto h = in.u32();
                    f.hashes.push_back(h);
                    if (h) {
                        claim(1);
                        f.values.push_back(in.i32());
                    }
                }
            } else if (type == E::nested_map_ibi || type == E::nested_map_iii || type == E::map_ivec) {
                f.columns = type == E::map_ivec ? 1 : 2;
                claim(n, 8);
                for (std::uint64_t i = 0; i < n; ++i) {
                    PlayerTraitsRow row;
                    row.key = in.i32();
                    row.wire_count = in.u32();
                    const auto len = size(row.wire_count, true);
                    const auto total = product(len, f.columns);
                    if (len > in.remaining() / (type == E::nested_map_ibi ? 5 : f.columns * 4))
                        throw std::runtime_error("Truncated nested trait map");
                    claim(total);
                    row.values.reserve(static_cast<std::size_t>(total));
                    for (std::uint64_t j = 0; j < len; ++j) {
                        if (type != E::map_ivec)
                            row.values.push_back(number(type == E::nested_map_ibi));
                        row.values.push_back(in.i32());
                    }
                    f.rows.push_back(std::move(row));
                }
            } else {
                if (type == E::pairs_i32 || type == E::counted_pairs || type == E::map_ii)
                    f.columns = 2;
                else if (type == E::map_iii || type == E::map_iib || type == E::map_bii)
                    f.columns = 3;
                else if (type == E::yield_rows)
                    f.columns = dimensions.yields;
                const auto total = product(n, f.columns);
                const bool flag = type == E::vector_bool || type == E::counted_bool;
                const bool mixed = type == E::map_iib || type == E::map_bii;
                if (mixed && n > in.remaining() / 9)
                    throw std::runtime_error("Truncated mixed trait map");
                claim(total, mixed ? 0 : flag ? 1 : 4);
                f.values.reserve(static_cast<std::size_t>(total));
                for (std::uint64_t i = 0; i < n; ++i)
                    for (std::uint64_t j = 0; j < f.columns; ++j)
                        f.values.push_back(
                            number(flag || (type == E::map_iib && j == 2) || (type == E::map_bii && j == 0)));
            }
        }
        f.end_offset = in.position();
        return f;
    }
};
} // namespace
const char *player_traits_encoding_name(PlayerTraitsEncoding encoding) {
    switch (encoding) {
#define CASE(name)                                                                                           \
    case E::name:                                                                                            \
        return #name;
        // clang-format off
        CASE(i32)
        CASE(boolean)
        CASE(raw_bool8)
        CASE(yield_array)
        CASE(terrain_array)
        CASE(vector_i32)
        CASE(vector_bool)
        CASE(set_i32)
        CASE(pairs_i32)
        CASE(yield_rows)
        CASE(map_ii)
        CASE(map_iib)
        CASE(map_bii)
        CASE(map_iii)
        CASE(nested_map_ibi)
        CASE(nested_map_iii)
        CASE(map_ivec)
        CASE(counted_i32)
        CASE(counted_bool)
        CASE(counted_pairs)
        CASE(combat_i32)
        CASE(shared_combat_i32)
        CASE(hashed_i32)
        // clang-format on
#undef CASE
    }
    throw std::runtime_error("Unknown player traits encoding");
}
PlayerTraitsObservation observe_player_traits(runtime::BinaryReader &in, GameStateLayout layout,
                                              std::optional<PlayerTraitsWireDimensions> vp_dimensions,
                                              GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (vp != vp_dimensions.has_value() || (vp && (!vp_dimensions->yields || !vp_dimensions->terrains)))
        throw std::runtime_error("Supply nonzero trait enum spans only for VP");
    PlayerTraitsObservation p{
        layout, vp_dimensions.value_or(PlayerTraitsWireDimensions{}), in.position(), 0, {}, {}};
    if (!vp) {
        p.version = in.u32();
        if (!*p.version || *p.version > 19)
            throw std::runtime_error("Unverified BNW traits version");
    }
    TraitsReader reader{in, budget, vp, p.dimensions};
    const auto specs = vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
    for (const auto &spec : specs) {
        if (!vp && (*p.version < spec.minimum_version || *p.version > spec.maximum_version))
            continue;
        try {
            p.fields.push_back(reader.field(spec));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Player traits ") + spec.name + ": " + e.what());
        }
    }
    p.end_offset = in.position();
    return p;
}
} // namespace civ5::saves
