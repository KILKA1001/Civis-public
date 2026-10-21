#include "civ5/saves/unit_core.hpp"
#include <bit>
#include <stdexcept>
#include <string>

namespace civ5::saves {
namespace {
using E = UnitCoreEncoding;
struct FieldSpec {
    const char *name;
    E encoding;
};
#include "unit_core_fields.inc"
struct UnitReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Unit core exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(E type) {
        if (type == E::i32)
            return in.i32();
        if (type == E::hash32)
            return in.u32();
        const auto v = in.u8();
        if (v > 1)
            throw std::runtime_error("Unverified unit boolean");
        return v;
    }
    void values(UnitCoreField &f, std::uint64_t n, E type = E::i32) {
        claim(n, type == E::boolean ? 1 : 4);
        for (std::uint64_t i = 0; i < n; ++i)
            f.values.push_back(number(type));
    }
    UnitCoreField field(const FieldSpec &spec) {
        claim(1);
        UnitCoreField f{};
        f.name = spec.name;
        f.encoding = spec.encoding;
        f.start_offset = in.position();
        const auto type = spec.encoding;
        if (type == E::i32 || type == E::hash32 || type == E::boolean)
            values(f, 1, type);
        else if (type == E::string32) {
            const auto n = in.u32();
            f.wire_count = n;
            if (n > budget.string_bytes || n > budget.total_string_bytes || n > in.remaining())
                throw std::runtime_error("Unit string exceeds input or byte budget");
            budget.total_string_bytes -= n;
            f.bytes.resize(static_cast<std::size_t>(n));
            in.read(f.bytes);
        } else {
            const auto raw = in.u32();
            f.wire_count = raw;
            const std::uint64_t n = vp && raw == UINT32_MAX ? 0 : raw;
            if (type == E::plagues) {
                if (n > budget.elements / 5 || n > in.remaining() / 14)
                    throw std::runtime_error("Unit plague rows exceed input or element budget");
                for (std::uint64_t i = 0; i < n; ++i) {
                    values(f, 2);
                    values(f, 2, E::boolean);
                    values(f, 1);
                }
            } else {
                const std::uint64_t width = type == E::map_triple                             ? 3
                                            : (type == E::map_pair || type == E::vector_pair) ? 2
                                                                                              : 1;
                if (n > budget.elements / width)
                    throw std::runtime_error("Unit rows exceed element budget");
                values(f, n * width, type == E::vector_bool ? E::boolean : E::i32);
            }
        }
        f.end_offset = in.position();
        return f;
    }
};
} // namespace
const char *unit_core_encoding_name(UnitCoreEncoding e) {
    switch (e) {
        // clang-format off
        case E::i32: return "i32";
        case E::hash32: return "hash32";
        case E::boolean: return "boolean";
        case E::string32: return "string32";
        case E::vector_i32: return "vector_i32";
        case E::vector_bool: return "vector_bool";
        case E::set_i32: return "set_i32";
        case E::vector_pair: return "vector_pair";
        case E::map_pair: return "map_pair";
        case E::map_triple: return "map_triple";
        case E::plagues: return "plagues";
        // clang-format on
    }
    throw std::runtime_error("Unknown unit core encoding");
}
UnitCollectionHeader observe_unit_collection_header(runtime::BinaryReader &in, GameStateLayout layout,
                                                    GameStateBudget budget) {
    UnitCollectionHeader h{};
    h.layout = layout;
    h.start_offset = in.position();
    UnitReader r{in, budget, layout == GameStateLayout::vp542_corpus};
    if (!r.vp) {
        const auto scalar = [&](const char *name) {
            r.claim(1, 4);
            const auto n = in.i32();
            h.fields.push_back({name, n});
            return n;
        };
        const auto slots = scalar("num_slots");
        if (slots < 0)
            throw std::runtime_error("Negative BNW unit source slot count");
        h.source_slots = slots ? std::bit_ceil(static_cast<std::uint64_t>(slots)) : 0;
        scalar("last_index");
        scalar("free_list_head");
        scalar("free_list_count");
        scalar("current_id");
        r.claim(h.source_slots, 4);
        for (std::uint64_t i = 0; i < h.source_slots; ++i)
            h.next_free_indices.push_back(in.i32());
    }
    r.claim(1, 4);
    h.wire_unit_count = in.i32();
    if (h.wire_unit_count > 0 && static_cast<std::uint64_t>(h.wire_unit_count) > r.budget.elements)
        throw std::runtime_error("Unit collection count exceeds element budget");
    h.end_offset = in.position();
    return h;
}
UnitCoreObservation observe_unit_core(runtime::BinaryReader &in, GameStateLayout layout,
                                      GameStateBudget budget) {
    UnitCoreObservation c{};
    c.layout = layout;
    c.start_offset = in.position();
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (!vp) {
        c.version = in.u32();
        if (*c.version > 9)
            throw std::runtime_error("Unverified BNW unit version");
    }
    UnitReader r{in, budget, vp};
    const auto fields = vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
    for (const auto &spec : fields) {
        try {
            c.fields.push_back(r.field(spec));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Unit core ") + spec.name + ": " + e.what());
        }
    }
    c.end_offset = in.position();
    return c;
}
} // namespace civ5::saves
