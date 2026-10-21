#include "civ5/saves/city_core.hpp"
#include <bit>
#include <stdexcept>
#include <string>

namespace civ5::saves {
namespace {
using E = CityCoreEncoding;
struct FieldSpec {
    const char *name;
    E encoding;
    unsigned minimum_version;
};
#include "city_core_fields.inc"
struct CityReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("City core exceeds input or element budget");
        budget.elements -= n;
    }
    std::uint64_t size(std::uint64_t n) const { return vp && n == UINT32_MAX ? 0 : n; }
    std::int64_t number(E type) {
        if (type == E::i32)
            return in.i32();
        if (type == E::hash32)
            return in.u32();
        const auto n = in.u8();
        if (n > 1)
            throw std::runtime_error("Unverified city boolean");
        return n;
    }
    void values(CityCoreField &f, std::uint64_t n, E type = E::i32) {
        claim(n, type == E::boolean ? 1 : 4);
        for (std::uint64_t i = 0; i < n; ++i)
            f.values.push_back(number(type));
    }
    void rows(CityCoreField &f, std::uint64_t n, std::uint64_t width) {
        if (n > budget.elements / width)
            throw std::runtime_error("City rows exceed element budget");
        values(f, n * width);
    }
    CityCoreField field(const FieldSpec &spec) {
        claim(1);
        CityCoreField f{};
        f.name = spec.name;
        f.encoding = spec.encoding;
        f.start_offset = in.position();
        const auto type = spec.encoding;
        switch (type) {
        case E::i32:
        case E::hash32:
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
                throw std::runtime_error("City string exceeds input or byte budget");
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
            } else if (type == E::nested_map_i_iii) {
                claim(n, 8);
                for (std::uint64_t i = 0; i < n; ++i) {
                    f.row_keys.push_back(in.i32());
                    const auto count = in.u32();
                    f.row_counts.push_back(count);
                    rows(f, size(count), 3);
                }
            } else if (type == E::map_ii || type == E::vector_fraction)
                rows(f, n, 2);
            else
                values(f, n, type == E::vector_bool ? E::boolean : E::i32);
            break;
        }
        }
        f.end_offset = in.position();
        return f;
    }
};
} // namespace
const char *city_core_encoding_name(CityCoreEncoding e) {
    switch (e) {
        // clang-format off
#define CASE(name) case E::name: return #name;
        CASE(i32) CASE(hash32) CASE(boolean) CASE(fraction) CASE(string32)
        CASE(vector_i32) CASE(vector_bool) CASE(set_i32) CASE(vector_fraction)
        CASE(map_ii) CASE(nested_map_i_iii) CASE(hashed_i32)
#undef CASE
        // clang-format on
    }
    throw std::runtime_error("Unknown city core encoding");
}
CityCollectionHeader observe_city_collection_header(runtime::BinaryReader &in, GameStateLayout layout,
                                                    GameStateBudget budget) {
    CityCollectionHeader h{};
    h.layout = layout;
    h.start_offset = in.position();
    CityReader r{in, budget, layout == GameStateLayout::vp542_corpus};
    if (!r.vp) {
        const auto scalar = [&](const char *name) {
            r.claim(1, 4);
            const auto n = in.i32();
            h.fields.push_back({name, n});
            return n;
        };
        const auto slots = scalar("num_slots");
        if (slots < 0)
            throw std::runtime_error("Negative BNW city source slot count");
        // Init rounds a nonzero non-power-of-two source capacity upward before
        // reading free indices. Use a 64-bit calculation without FLTA_MAX_BUCKETS.
        h.source_slots = slots ? std::bit_ceil(static_cast<std::uint64_t>(slots)) : 0;
        scalar("last_index");
        scalar("free_list_head");
        scalar("free_list_count");
        scalar("current_id");
        r.claim(h.source_slots, 4);
        h.next_free_indices.reserve(static_cast<std::size_t>(h.source_slots));
        for (std::uint64_t i = 0; i < h.source_slots; ++i)
            h.next_free_indices.push_back(in.i32());
    }
    r.claim(1, 4);
    h.wire_city_count = in.i32();
    // Signed negative counts execute no source loop; retain them verbatim.
    if (h.wire_city_count > 0 && static_cast<std::uint64_t>(h.wire_city_count) > r.budget.elements)
        throw std::runtime_error("City collection count exceeds element budget");
    h.end_offset = in.position();
    return h;
}
CityCoreObservation observe_city_core(runtime::BinaryReader &in, GameStateLayout layout,
                                      GameStateBudget budget) {
    CityCoreObservation c{};
    c.layout = layout;
    c.start_offset = in.position();
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (!vp) {
        c.version = in.u32();
        if (*c.version > 6)
            throw std::runtime_error("Unverified BNW city version");
    }
    CityReader r{in, budget, vp};
    const auto specs = vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
    for (const auto &spec : specs) {
        if (!vp && spec.minimum_version > *c.version)
            continue;
        try {
            c.fields.push_back(r.field(spec));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("City core ") + spec.name + ": " + e.what());
        }
    }
    c.end_offset = in.position();
    return c;
}
} // namespace civ5::saves
