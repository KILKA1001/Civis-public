#include "civ5/saves/player_tail.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Node = PlayerTailNode;
enum class E {
    i32,
    u32,
    boolean,
    string32,
    stl_i32,
    stl_bool,
    stl_pairs,
    replay,
    strings,
    turn_sets,
    history,
    espionage,
    conquered,
    achievements,
    unit_cycle,
    requests,
    yield_rows,
    nested_map,
    progress,
    signed_i32,
    signed_bool,
    turn_data,
    request_list,
    ai_version,
    achievement_version
};
struct FieldSpec {
    const char *name;
    E encoding;
};
#include "player_tail_fields.inc"
const char *encoding_name(E e) {
#define NAME(x)                                                                                              \
    case E::x:                                                                                               \
        return #x;
    switch (e) {
        NAME(i32)
        NAME(u32) NAME(boolean) NAME(string32) NAME(stl_i32) NAME(stl_bool) NAME(stl_pairs) NAME(replay)
            NAME(strings) NAME(turn_sets) NAME(history) NAME(espionage) NAME(conquered) NAME(achievements)
                NAME(unit_cycle) NAME(requests) NAME(yield_rows) NAME(nested_map) NAME(progress)
                    NAME(signed_i32) NAME(signed_bool) NAME(turn_data) NAME(request_list) NAME(ai_version)
                        NAME(achievement_version)
    }
#undef NAME
    throw std::logic_error("Unknown player tail encoding");
}
struct Reader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const std::optional<PlayerTailVpContext> &context;
    void ensure(std::uint64_t n, std::uint64_t width = 0) const {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player tail exceeds input or element budget");
    }
    void claim(std::uint64_t n = 1, std::uint64_t width = 0) {
        ensure(n, width);
        budget.elements -= n;
    }
    Node node(const char *name, const char *enc = "record") {
        claim();
        return {name, enc, in.position(), 0, {}, {}, {}, {}};
    }
    std::int64_t number(E type) {
        if (type == E::u32 || type == E::ai_version)
            return in.u32();
        if (type != E::boolean)
            return in.i32();
        const auto v = in.u8();
        if (v > 1)
            throw std::runtime_error("Unverified player tail boolean");
        return v;
    }
    std::uint64_t count(Node &n, bool sign = false, bool stl = true) {
        claim(1, 4);
        n.wire_count = sign ? static_cast<std::int64_t>(in.i32()) : in.u32();
        const auto value = *n.wire_count;
        return value <= 0 || (vp && stl && value == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(value);
    }
    void values(Node &n, std::uint64_t rows, std::uint64_t columns, E type = E::i32) {
        if (!columns || rows > budget.elements / columns)
            throw std::runtime_error("Player tail dimensions exceed element budget");
        const auto total = rows * columns;
        claim(total, type == E::boolean ? 1 : 4);
        for (std::uint64_t i = 0; i < total; ++i)
            n.values.push_back(number(type));
    }
    void fields(Node &n, std::span<const FieldSpec> specs) {
        for (const auto &f : specs)
            n.children.push_back(read(f.name, f.encoding));
    }
    Node read(const char *name, E enc) {
        try {
            return read_impl(name, enc);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string(name) + ": " + e.what());
        }
    }
    Node read_impl(const char *name, E enc) {
        auto n = node(name, encoding_name(enc));
        switch (enc) {
        case E::i32:
        case E::u32:
        case E::boolean:
        case E::ai_version:
        case E::achievement_version: {
            claim(1, enc == E::boolean ? 1 : 4);
            const auto v = number(enc);
            if ((enc == E::ai_version || enc == E::achievement_version) && (v < 0 || v > 1))
                throw std::runtime_error("Unverified BNW player tail version");
            n.values.push_back(v);
            break;
        }
        case E::string32: {
            const auto size = count(n, false, false);
            if (size > budget.string_bytes || size > budget.total_string_bytes || size > in.remaining())
                throw std::runtime_error("Player tail string exceeds input or byte budget");
            budget.total_string_bytes -= size;
            n.bytes.resize(static_cast<std::size_t>(size));
            in.read(n.bytes);
            break;
        }
        case E::achievements:
            if (!vp)
                n.children.push_back(read("version", E::achievement_version));
            fields(n, vp ? std::span<const FieldSpec>(vp_achievements)
                         : std::span<const FieldSpec>(bnw_achievements));
            break;
        case E::conquered:
            values(n, context->players / 32 + (context->players % 32 != 0), 1, E::u32);
            break;
        case E::requests:
            n.children.push_back(read("m_ePlayer", E::i32));
            n.children.push_back(read("m_aRequests", E::request_list));
            break;
        case E::stl_i32:
        case E::stl_bool:
        case E::stl_pairs:
        case E::yield_rows:
        case E::signed_i32:
        case E::signed_bool:
        case E::unit_cycle:
        case E::turn_data: {
            const bool sign = enc == E::signed_i32 || enc == E::signed_bool || enc == E::unit_cycle;
            const auto size = count(n, sign, !sign);
            if (enc == E::turn_data) {
                if (size > budget.elements / 2)
                    throw std::runtime_error("Player turn data exceeds element budget");
                claim(size * 2, 4);
                for (std::uint64_t i = 0; i < size; ++i) {
                    n.values.push_back(in.u32());
                    n.values.push_back(in.i32());
                }
            } else
                values(n, size,
                       enc == E::stl_pairs    ? 2
                       : enc == E::yield_rows ? context->yields
                                              : 1,
                       enc == E::stl_bool || enc == E::signed_bool ? E::boolean : E::i32);
            break;
        }
        default: {
            const auto size = count(n);
            // Every row contains at least a scalar or nested count. No source
            // fixed capacity or history length is imposed by this reader.
            ensure(size, 4);
            for (std::uint64_t i = 0; i < size; ++i) {
                try {
                    if (enc == E::strings) {
                        n.children.push_back(read("name", E::string32));
                        continue;
                    }
                    if (enc == E::turn_sets) {
                        n.children.push_back(read("turns", E::turn_data));
                        continue;
                    }
                    auto row = node("entry");
                    if (enc == E::replay) {
                        row.children.push_back(read("key", E::string32));
                        row.children.push_back(read("value", E::turn_data));
                    } else if (enc == E::history) {
                        row.children.push_back(read("turn", E::i32));
                        row.children.push_back(read("values", E::stl_i32));
                    } else if (enc == E::nested_map || enc == E::progress) {
                        row.children.push_back(read("key", E::i32));
                        row.children.push_back(
                            read("value", enc == E::nested_map ? E::stl_pairs : E::string32));
                    } else if (enc == E::espionage)
                        fields(row, vp_event);
                    else if (enc == E::request_list)
                        fields(row, vp_request);
                    else
                        throw std::logic_error("Unknown player tail record");
                    row.end_offset = in.position();
                    n.children.push_back(std::move(row));
                } catch (const std::exception &e) {
                    throw std::runtime_error("Entry " + std::to_string(i) + ": " + e.what());
                }
            }
        }
        }
        n.end_offset = in.position();
        return n;
    }
};
} // namespace
PlayerTailObservation observe_player_tail(runtime::BinaryReader &in, GameStateLayout layout,
                                          std::optional<PlayerTailVpContext> context,
                                          GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (vp != bool(context) || (context && (!context->yields || !context->players || !context->major ||
                                            context->major > context->players)))
        throw std::runtime_error("Player tail context does not match layout or source dimensions");
    PlayerTailObservation p{};
    p.layout = layout;
    p.context = context;
    p.start_offset = in.position();
    Reader r{in, budget, vp, context};
    r.claim();
    const auto specs = vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
    for (const auto &f : specs) {
        if (f.encoding == E::requests && context->player_id >= 0 &&
            static_cast<std::uint64_t>(context->player_id) >= context->major)
            continue;
        p.fields.push_back(r.read(f.name, f.encoding));
    }
    p.base_player_end_offset = in.position();
    if (!vp)
        p.fields.push_back(r.read("CvPlayerAI.version", E::ai_version));
    p.end_offset = in.position();
    p.retained_elements = budget.elements - r.budget.elements;
    p.retained_bytes = budget.total_string_bytes - r.budget.total_string_bytes;
    return p;
}
} // namespace civ5::saves
