#include "civ5/saves/unit_tail.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Node = UnitTailNode;
enum class E { i32, u32, u16, i16, bool_, string32, vector_i32 };
struct FieldSpec {
    const char *name;
    E encoding;
    std::uint64_t minimum_version;
};
#include "unit_tail_fields.inc"
struct TailReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    std::uint64_t unit_version;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Unit tail exceeds input or element budget");
        budget.elements -= n;
    }
    Node node(const char *name, const char *encoding = "record") {
        claim(1);
        return {name, encoding, in.position(), 0, {}, {}, {}, {}};
    }
    void finish(Node &parent, Node child) {
        child.end_offset = in.position();
        parent.children.push_back(std::move(child));
    }
    std::int64_t number(E type = E::i32) {
        switch (type) {
        case E::i32:
            return in.i32();
        case E::u32:
            return in.u32();
        case E::u16:
            return in.u16();
        case E::i16:
            return in.i16();
        case E::bool_: {
            const auto value = in.u8();
            if (value > 1)
                throw std::runtime_error("Unverified unit tail boolean");
            return value;
        }
        default:
            throw std::runtime_error("Unit tail value is not a scalar");
        }
    }
    std::int64_t scalar(Node &parent, const char *name, E type = E::i32) {
        auto n = node(name, type == E::u32     ? "u32"
                            : type == E::u16   ? "u16"
                            : type == E::i16   ? "i16"
                            : type == E::bool_ ? "bool"
                                               : "i32");
        claim(1);
        const auto value = number(type);
        n.values.push_back(value);
        finish(parent, std::move(n));
        return value;
    }
    std::uint64_t version(Node &parent, std::uint64_t maximum) {
        const auto value = static_cast<std::uint64_t>(scalar(parent, "version", E::u32));
        if (value > maximum)
            throw std::runtime_error("Unverified unit tail version");
        return value;
    }
    std::uint64_t count(Node &n, bool signed_count = false, bool stl = false) {
        n.wire_count = number(signed_count ? E::i32 : E::u32);
        const auto raw = *n.wire_count;
        return raw <= 0 || (vp && stl && raw == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(raw);
    }
    void blob(Node &parent, const char *name, const char *encoding = "string32") {
        auto n = node(name, encoding);
        const auto size = count(n);
        if (size > budget.string_bytes || size > budget.total_string_bytes || size > in.remaining())
            throw std::runtime_error("Unit tail bytes exceed input or byte budget");
        budget.total_string_bytes -= size;
        n.bytes.resize(static_cast<std::size_t>(size));
        in.read(n.bytes);
        finish(parent, std::move(n));
    }
    void rows(Node &n, std::uint64_t count, std::initializer_list<E> types, std::uint64_t wire_width) {
        const auto columns = types.size();
        if (!columns || count > budget.elements / columns || count > in.remaining() / wire_width)
            throw std::runtime_error("Unit tail rows exceed input or element budget");
        claim(count * columns);
        for (std::uint64_t i = 0; i < count; ++i)
            for (auto type : types)
                n.values.push_back(number(type));
    }
    void promotions(Node &n) {
        if (vp) {
            blob(n, "m_kHasPromotion", "bitfield_bytes");
            blob(n, "m_kPromotionActive", "bitfield_bytes");
        } else {
            version(n, 1);
            scalar(n, "iNumPromotions");
            auto bits = node("m_kHasPromotion", "signed_hash_bool");
            const auto size = count(bits, true);
            // Every hash has a bool, including zero, unknown and OLIGARCHY hashes.
            rows(bits, size, {E::u32, E::bool_}, 5);
            finish(n, std::move(bits));
        }
    }
    void state(Node &n) {
        const auto fields =
            vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
        for (const auto &f : fields) {
            if (!vp && unit_version < f.minimum_version)
                continue;
            try {
                if (f.encoding == E::string32)
                    blob(n, f.name);
                else if (f.encoding == E::vector_i32) {
                    auto v = node(f.name, "vector_i32");
                    const auto size = count(v, false, true);
                    rows(v, size, {E::i32}, 4);
                    finish(n, std::move(v));
                } else
                    scalar(n, f.name, f.encoding);
            } catch (const std::exception &e) {
                throw std::runtime_error(std::string(f.name) + ": " + e.what());
            }
        }
    }
    void religion(Node &n) {
        if (vp) {
            scalar(n, "m_eReligion");
            scalar(n, "m_iStrength", E::u16);
            scalar(n, "m_iMaxStrength", E::u16);
            scalar(n, "m_iSpreadsUsed", E::u16);
        } else {
            const auto v = version(n, 2);
            scalar(n, "religion");
            scalar(n, "strength");
            if (v >= 2)
                scalar(n, "spreads_left");
        }
    }
    void great_work(Node &n) {
        if (vp)
            scalar(n, "m_eGreatWork");
        else {
            if (unit_version >= 6)
                scalar(n, "uiGreatWorkType", E::u32);
            else if (unit_version >= 4)
                blob(n, "strGreatWorkType");
            if (unit_version >= 7)
                scalar(n, "m_iTourismBlastStrength");
        }
    }
    void missions(Node &n) {
        n.encoding = "counted_missions";
        const auto size = count(n); // explicit loop; never an STL empty sentinel
        rows(n, size, {E::i32, E::i32, E::i32, E::i32, E::i32}, 20);
    }
    void path(Node &n) {
        auto p = node("m_kLastPath", "counted_path_i32_i32_i16_i16");
        const auto size = count(p); // original x86 size_t on disk, not host size_t
        rows(p, size, {E::i32, E::i32, E::i16, E::i16}, 12);
        finish(n, std::move(p));
        scalar(n, "m_uiLastPathCacheOrigin", E::u32);
        scalar(n, "m_uiLastPathCacheDestination", E::u32);
        scalar(n, "m_uiLastPathFlags", E::u32);
        scalar(n, "m_uiLastPathTurnSlice", E::u32);
    }
};
} // namespace
UnitTailObservation observe_unit_tail(runtime::BinaryReader &in, GameStateLayout layout,
                                      UnitTailContext context, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (vp ? bool(context.bnw_unit_version) : !context.bnw_unit_version || *context.bnw_unit_version > 9)
        throw std::runtime_error("Unit tail context does not match layout or supported BNW version");
    UnitTailObservation p{layout, context, in.position(), 0, {}};
    TailReader r{in, budget, vp, context.bnw_unit_version.value_or(0)};
    constexpr const char *names[] = {"promotions", "state",          "religion",
                                     "great_work", "m_missionQueue", "last_path"};
    using Read = void (TailReader::*)(Node &);
    constexpr Read readers[] = {&TailReader::promotions, &TailReader::state,    &TailReader::religion,
                                &TailReader::great_work, &TailReader::missions, &TailReader::path};
    for (unsigned i = 0; i < (vp ? 6u : 5u); ++i) {
        try {
            auto n = r.node(names[i]);
            (r.*readers[i])(n);
            n.end_offset = in.position();
            p.blocks.push_back(std::move(n));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Unit tail ") + names[i] + ": " + e.what());
        }
    }
    p.end_offset = in.position();
    return p;
}
} // namespace civ5::saves
