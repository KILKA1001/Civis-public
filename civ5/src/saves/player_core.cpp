#include "civ5/saves/player_core.hpp"
#include <bit>
#include <stdexcept>
#include <string>

namespace civ5::saves {
namespace {
using E = PlayerCoreEncoding;
struct FieldSpec {
    const char *name;
    E encoding;
    unsigned minimum_version;
};
#include "player_core_fields.inc"
struct CoreReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    std::uint64_t yields;
    void claim(std::uint64_t count, std::uint64_t bytes = 0) {
        if (count > budget.elements || (bytes && count > in.remaining() / bytes))
            throw std::runtime_error("Player core exceeds input or element budget");
        budget.elements -= count;
    }
    std::int64_t number(E type) {
        if (type == E::i32)
            return in.i32();
        if (type == E::u32)
            return in.u32();
        if (type == E::i64) {
            const auto low = static_cast<std::uint64_t>(in.u32());
            const auto high = static_cast<std::uint64_t>(in.u32());
            return std::bit_cast<std::int64_t>(low | (high << 32));
        }
        const auto value = in.u8();
        if (type == E::boolean && value > 1)
            throw std::runtime_error("Unverified player core boolean");
        return value;
    }
    void values(PlayerCoreField &f, std::uint64_t count, E type = E::i32) {
        const auto width = type == E::i64 ? 8 : (type == E::i32 || type == E::u32 ? 4 : 1);
        claim(count, width);
        for (std::uint64_t i = 0; i < count; ++i)
            f.values.push_back(number(type));
    }
    std::uint64_t size(std::uint64_t count) const { return vp && count == UINT32_MAX ? 0 : count; }
    PlayerCoreField field(const FieldSpec &spec) {
        claim(1);
        PlayerCoreField f{};
        f.name = spec.name;
        f.encoding = spec.encoding;
        f.start_offset = in.position();
        const auto type = spec.encoding;
        switch (type) {
        case E::i32:
        case E::u32:
        case E::i64:
        case E::boolean:
            values(f, 1, type);
            break;
        case E::fraction:
            values(f, 2);
            break;
        case E::string32: {
            const auto n = in.u32();
            f.wire_count = n;
            if (n > budget.string_bytes || n > budget.total_string_bytes || n > in.remaining())
                throw std::runtime_error("Player core string exceeds input or byte budget");
            budget.total_string_bytes -= n;
            f.bytes.resize(static_cast<std::size_t>(n));
            in.read(f.bytes);
            break;
        }
        default: {
            const auto raw = in.u32();
            f.wire_count = raw;
            const auto n = type == E::hashed_i32 ? raw : size(raw);
            if (type == E::hashed_i32) {
                claim(n, 4);
                for (std::uint64_t i = 0; i < n; ++i) {
                    const auto hash = in.u32();
                    f.hashes.push_back(hash);
                    if (hash)
                        values(f, 1);
                }
            } else if (type == E::nested_i32) {
                claim(n, 4);
                for (std::uint64_t i = 0; i < n; ++i) {
                    const auto row = in.u32();
                    f.row_counts.push_back(row);
                    values(f, size(row));
                }
            } else if (type == E::yield_rows || type == E::options) {
                const auto columns = type == E::options ? 2 : yields;
                if (!columns || n > budget.elements / columns)
                    throw std::runtime_error("Player core row dimensions exceed budget");
                if (type == E::yield_rows)
                    values(f, n * columns);
                else {
                    for (std::uint64_t i = 0; i < n; ++i) {
                        values(f, 1, E::u32);
                        values(f, 1);
                    }
                }
            } else
                values(f, n,
                       type == E::vector_bool ? E::boolean
                       : type == E::vector_u8 ? E::vector_u8
                                              : E::i32);
            break;
        }
        }
        f.end_offset = in.position();
        return f;
    }
};
} // namespace
const char *player_core_encoding_name(PlayerCoreEncoding encoding) {
    switch (encoding) {
#define CASE(name)                                                                                           \
    case PlayerCoreEncoding::name:                                                                           \
        return #name;
        CASE(i32)
        CASE(u32) CASE(i64) CASE(boolean) CASE(fraction) CASE(string32) CASE(vector_i32) CASE(vector_bool)
            CASE(vector_u8) CASE(set_i32) CASE(nested_i32) CASE(options) CASE(hashed_i32) CASE(yield_rows)
#undef CASE
    }
    throw std::runtime_error("Unknown player core encoding");
}
PlayerCoreObservation observe_player_core(runtime::BinaryReader &input, GameStateLayout layout,
                                          std::optional<std::uint64_t> vp_yield_slots,
                                          GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if ((vp && (!vp_yield_slots || !*vp_yield_slots)) || (!vp && vp_yield_slots))
        throw std::runtime_error("Supply source yield slots only for VP player core");
    PlayerCoreObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    result.yield_slots = vp ? *vp_yield_slots : 6;
    if (!vp) {
        result.version = input.u32();
        if (!*result.version || *result.version > 16)
            throw std::runtime_error("Unverified BNW player version");
    }
    CoreReader r{input, budget, vp, result.yield_slots};
    const auto specs = vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
    for (const auto &spec : specs) {
        if (!vp && spec.minimum_version > *result.version)
            continue;
        try {
            result.fields.push_back(r.field(spec));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Player core ") + spec.name + ": " + e.what());
        }
    }
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
