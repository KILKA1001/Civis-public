#include "civ5/saves/player_diplomacy.hpp"
#include <algorithm>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>

namespace civ5::saves {
namespace {
enum class Dimension {
    one,
    major,
    minor,
    civ_players,
    players,
    approaches,
    minor_approaches,
    statements,
    log_entries
};
struct Spec {
    const char *name;
    DiplomacyEncoding encoding;
    Dimension rows, columns;
    unsigned minimum, group;
};
#define DIPLO_VP(name, type, rows, cols, minimum, group)                                                     \
    {#name, DiplomacyEncoding::type, Dimension::rows, Dimension::cols, minimum, group},
#define DIPLO_BNW(...)
constexpr Spec vp_fields[] = {
#include "player_diplomacy_fields.inc"
};
#undef DIPLO_VP
#undef DIPLO_BNW
#define DIPLO_VP(...)
#define DIPLO_BNW(name, type, rows, cols, minimum, group)                                                    \
    {#name, DiplomacyEncoding::type, Dimension::rows, Dimension::cols, minimum, group},
constexpr Spec bnw_fields[] = {
#include "player_diplomacy_fields.inc"
};
#undef DIPLO_VP
#undef DIPLO_BNW
std::uint64_t dimension(Dimension key, const PlayerDiplomacyWireDimensions &d) {
    switch (key) {
    case Dimension::one:
        return 1;
    case Dimension::major:
        return d.major;
    case Dimension::minor:
        return d.minor;
    case Dimension::civ_players:
        return d.civ_players;
    case Dimension::players:
        return d.players;
    case Dimension::approaches:
        return d.approaches;
    case Dimension::minor_approaches:
        return d.minor_approaches;
    case Dimension::statements:
        return d.statements;
    case Dimension::log_entries:
        return d.log_entries;
    }
    throw std::runtime_error("Unknown diplomacy dimension");
}
std::uint64_t width(DiplomacyEncoding t) {
    switch (t) {
    case DiplomacyEncoding::i8:
    case DiplomacyEncoding::u8:
    case DiplomacyEncoding::boolean:
        return 1;
    case DiplomacyEncoding::i16:
    case DiplomacyEncoding::u16:
        return 2;
    case DiplomacyEncoding::i32:
        return 4;
    case DiplomacyEncoding::diplo_log:
        return 8;
    case DiplomacyEncoding::declaration:
        return 21;
    }
    throw std::runtime_error("Unknown diplomacy encoding");
}
std::uint64_t components(DiplomacyEncoding t) {
    return t == DiplomacyEncoding::diplo_log ? 2 : t == DiplomacyEncoding::declaration ? 6 : 1;
}
std::int64_t scalar(runtime::BinaryReader &in, DiplomacyEncoding t) {
    switch (t) {
    case DiplomacyEncoding::i8: {
        const auto v = in.u8();
        return v < 128 ? v : std::int64_t(v) - 256;
    }
    case DiplomacyEncoding::u8:
        return in.u8();
    case DiplomacyEncoding::i16:
        return in.i16();
    case DiplomacyEncoding::u16:
        return in.u16();
    case DiplomacyEncoding::i32:
        return in.i32();
    case DiplomacyEncoding::boolean: {
        const auto v = in.u8();
        if (v > 1)
            throw std::runtime_error("Unverified diplomacy boolean");
        return v;
    }
    default:
        throw std::runtime_error("Compound diplomacy value needs its record codec");
    }
}
void read_rows(runtime::BinaryReader &in, PlayerDiplomacyField &f, std::uint64_t n) {
    if (!n)
        return;
    const auto start = in.position();
    for (std::uint64_t j = 0; j < n; ++j) {
        for (std::uint64_t k = 0; k < components(f.encoding); ++k) {
            auto t = f.encoding;
            if (t == DiplomacyEncoding::diplo_log)
                t = DiplomacyEncoding::i32;
            else if (t == DiplomacyEncoding::declaration)
                t = k == 4 ? DiplomacyEncoding::boolean : DiplomacyEncoding::i32;
            f.values.push_back(scalar(in, t));
        }
    }
    f.spans.push_back({start, in.position()});
}
} // namespace
const char *diplomacy_encoding_name(DiplomacyEncoding t) {
    switch (t) {
    case DiplomacyEncoding::i8:
        return "i8";
    case DiplomacyEncoding::u8:
        return "u8";
    case DiplomacyEncoding::i16:
        return "i16";
    case DiplomacyEncoding::u16:
        return "u16";
    case DiplomacyEncoding::i32:
        return "i32";
    case DiplomacyEncoding::boolean:
        return "boolean";
    case DiplomacyEncoding::diplo_log:
        return "diplo_log";
    case DiplomacyEncoding::declaration:
        return "declaration";
    }
    return "unknown";
}
PlayerDiplomacyObservation observe_player_diplomacy(runtime::BinaryReader &in, GameStateLayout layout,
                                                    PlayerDiplomacyWireDimensions dimensions,
                                                    GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    PlayerDiplomacyObservation out{layout, dimensions, in.position(), 0, {}, {}};
    if (!vp) {
        out.version = in.u32();
        if (*out.version < 1 || *out.version > 4)
            throw std::runtime_error("Unverified BNW diplomacy version");
    }
    const std::span<const Spec> specs =
        vp ? std::span<const Spec>(vp_fields) : std::span<const Spec>(bnw_fields);
    for (std::size_t i = 0; i < specs.size();) {
        const auto begin = i++;
        while (specs[begin].group && i < specs.size() && specs[i].group == specs[begin].group)
            ++i;
        const auto first = out.fields.size();
        for (auto k = begin; k < i; ++k) {
            const auto &s = specs[k];
            if (out.version && s.minimum > *out.version)
                continue;
            const auto rows = dimension(s.rows, dimensions), cols = dimension(s.columns, dimensions);
            if (cols && rows > std::numeric_limits<std::uint64_t>::max() / cols)
                throw std::runtime_error(std::string(s.name) + ": diplomacy dimension overflow");
            const auto n = rows * cols;
            if (n > budget.elements / components(s.encoding) || n > in.remaining() / width(s.encoding))
                throw std::runtime_error(std::string(s.name) + ": diplomacy exceeds input or element budget");
            budget.elements -= n * components(s.encoding);
            PlayerDiplomacyField f{s.name, s.encoding, rows, cols, {}, {}};
            f.values.reserve(static_cast<std::size_t>(n * components(s.encoding)));
            out.fields.push_back(std::move(f));
        }
        if (first == out.fields.size())
            continue;
        const bool interleaved = specs[begin].group != 0;
        const auto count = interleaved ? out.fields[first].rows : 1;
        // Empty columns must not cause a loop over a potentially huge row count.
        if (interleaved && std::all_of(out.fields.begin() + first, out.fields.end(),
                                       [](const auto &f) { return !f.columns; }))
            continue;
        for (std::uint64_t row = 0; row < count; ++row) {
            for (auto k = first; k < out.fields.size(); ++k) {
                auto &f = out.fields[k];
                try {
                    read_rows(in, f, interleaved ? f.columns : f.rows * f.columns);
                } catch (const std::exception &e) {
                    throw std::runtime_error(std::string(f.name) + ": " + e.what());
                }
            }
        }
    }
    out.end_offset = in.position();
    return out;
}
} // namespace civ5::saves
