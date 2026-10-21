#include "civ5/saves/player_trade_culture.hpp"
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Record = PlayerTradeCultureRecord;
enum class Wire { I32, U32, I64, Bool };
struct TradeCultureReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    PlayerTradeCultureWireDimensions d;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player trade/culture exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(Wire wire) {
        if (wire == Wire::I32)
            return in.i32();
        if (wire == Wire::U32)
            return in.u32();
        if (wire == Wire::I64) {
            const auto lo = std::uint64_t(in.u32()), hi = std::uint64_t(in.u32());
            return std::bit_cast<std::int64_t>(lo | (hi << 32));
        }
        const auto value = in.u8();
        if (value > 1)
            throw std::runtime_error("Unverified player trade/culture boolean");
        return value;
    }
    std::int64_t field(Record &b, const char *name, Wire wire = Wire::I32) {
        claim(1);
        const auto n = number(wire);
        b.fields.push_back({name, n});
        return n;
    }
    void fields(Record &b, std::initializer_list<const char *> names) {
        for (auto name : names)
            field(b, name);
    }
    std::int64_t version(Record &b, std::int64_t low, std::int64_t high) {
        const auto v = field(b, "version", Wire::U32);
        if (v < low || v > high)
            throw std::runtime_error("Unverified player trade/culture version");
        return v;
    }
    // STL VP containers alone recognize unsigned -1 as an empty sentinel.
    std::uint64_t count(Record &b, const char *name, bool signed_count = false, bool stl = false) {
        const auto n = field(b, name, signed_count ? Wire::I32 : Wire::U32);
        return n <= 0 || (vp && stl && n == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(Record &b, const char *name, std::uint64_t n, std::uint64_t columns = 1) {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("Player trade/culture dimensions exceed budget");
        const auto total = n * columns;
        claim(total, 4);
        PlotArrayObservation a{name, columns, {}};
        a.values.reserve(static_cast<std::size_t>(total));
        for (std::uint64_t i = 0; i < total; ++i)
            a.values.push_back(in.i32());
        b.arrays.push_back(std::move(a));
    }
    template <class F>
    void records(Record &b, const char *name, std::uint64_t n, std::uint64_t minimum_bytes, F decode) {
        claim(n, minimum_bytes);
        for (std::uint64_t i = 0; i < n; ++i) {
            Record c{name, i, in.position(), 0, {}, {}, {}, {}};
            decode(c);
            c.end_offset = in.position();
            b.children.push_back(std::move(c));
        }
    }
    void text(Record &b, const char *name, const char *count_name) {
        const auto n = count(b, count_name);
        if (n > budget.string_bytes || n > budget.total_string_bytes || n > in.remaining())
            throw std::runtime_error("Player culture tooltip exceeds input or byte budget");
        budget.total_string_bytes -= n;
        PlayerTradeCultureBlob blob{name, std::vector<std::uint8_t>(static_cast<std::size_t>(n))};
        in.read(blob.bytes);
        b.blobs.push_back(std::move(blob));
    }
    void route(Record &b, bool plundered) {
        if (vp || !plundered)
            field(b, "m_iID");
        if (vp)
            field(b, "m_iOriginID");
        fields(b, {"m_iOriginX", "m_iOriginY"});
        if (vp)
            field(b, "m_iDestID");
        fields(b, {"m_iDestX", "m_iDestY", "m_eOriginOwner", "m_eDestOwner", "m_eDomain", "m_eConnectionType",
                   "m_iTradeUnitLocationIndex"});
        field(b, "m_bTradeUnitMovingForward", Wire::Bool);
        field(b, "m_unitID");
        if (vp)
            field(b, "m_iSpeedFactor");
        fields(b, {"m_iCircuitsCompleted", "m_iCircuitsToComplete", "m_iTurnRouteComplete"});
        if (vp) {
            field(b, "m_bTradeUnitRecalled", Wire::Bool);
            array(b, "m_aPlotList", count(b, "m_aPlotList_wire_count", false, true), 2);
            array(b, "m_aiOriginYields", d.yields);
            array(b, "m_aiDestYields", d.yields);
        }
        // BNW's history saves only the fields above, without path or yields.
    }
    void trade(Record &b) {
        const auto v = vp ? 2 : version(b, 0, 2);
        if (v >= 1)
            records(b, "expired_route", count(b, "m_aRecentlyExpiredConnections_wire_count", !vp, vp),
                    vp ? 74 : 57, [&](Record &c) { route(c, false); });
        else
            field(b, "legacy_empty_value");
        if (v >= 2)
            records(b, "plundered_route", count(b, "m_aTradeConnectionWasPlundered_wire_count", !vp, vp),
                    vp ? 78 : 57, [&](Record &c) {
                        if (!vp)
                            field(c, "m_iTurnPlundered");
                        records(c, "m_kTradeConnection", 1, vp ? 74 : 53, [&](Record &r) { route(r, true); });
                        if (vp)
                            field(c, "m_iTurnPlundered");
                    });
    }
    void trade_ai(Record &b) {
        if (!vp)
            version(b, 0, 0);
        field(b, "m_iRemovableValue");
    }
    void league_ai(Record &b) {
        const auto v = vp ? 2 : version(b, 0, 2);
        if (v >= 2)
            records(b, "vote_commitment", count(b, "m_vVoteCommitmentList_wire_count", false, vp), 17,
                    [&](Record &c) {
                        fields(c, {"eToPlayer", "iResolutionID", "iVoteChoice", "iNumVotes"});
                        field(c, "bEnact", Wire::Bool);
                    });
    }
    void culture(Record &b) {
        const auto v = vp ? 6 : version(b, 0, 6);
        const auto plots = count(b, "m_aDigCompletePlots_wire_count", !vp, vp);
        // VP plot pointers are saved as one i32 plot index (-1 is null).
        // BNW stores two i32 coordinates. No live pointers are reconstructed.
        array(b, "m_aDigCompletePlots", plots, vp ? 1 : 2);
        if (vp) {
            field(b, "m_lLastTurnLifetimeCultureTimes100", Wire::I64);
            field(b, "m_iLastTurnCPT");
            array(b, "m_aiCulturalInfluenceTimes100", d.major);
            array(b, "m_aiLastTurnCulturalInfluenceTimes100", d.major);
            array(b, "m_aiLastTurnCulturalIPTTimes100", d.major);
        } else {
            field(b, "m_iLastTurnLifetimeCulture");
            const auto n = count(b, "cultural_influence_wire_count", true);
            // The two BNW arrays are interleaved on disk.
            if (n > budget.elements / 2)
                throw std::runtime_error("BNW cultural influence exceeds budget");
            claim(n * 2, 4);
            PlotArrayObservation current{"m_aiCulturalInfluence", 1, {}},
                last{"m_aiLastTurnCulturalInfluence", 1, {}};
            current.values.reserve(static_cast<std::size_t>(n));
            last.values.reserve(static_cast<std::size_t>(n));
            for (std::uint64_t i = 0; i < n; ++i) {
                current.values.push_back(in.i32());
                last.values.push_back(in.i32());
            }
            b.arrays.push_back(std::move(current));
            b.arrays.push_back(std::move(last));
        }
        if (v >= 2) {
            field(b, "m_bReportedTwoCivsAway", Wire::Bool);
            field(b, "m_bReportedOneCivAway", Wire::Bool);
        }
        if (v >= 3) {
            fields(b, {"m_eOpinion", "m_ePreferredIdeology", "m_iOpinionUnhappiness"});
            if (vp)
                fields(b, {"m_iLastUpdate", "m_iLastThemUpdate"});
            text(b, "m_strOpinionTooltip", "m_strOpinionTooltip_wire_bytes");
        }
        if (v >= 5)
            text(b, "m_strOpinionUnhappinessTooltip", "m_strOpinionUnhappinessTooltip_wire_bytes");
        if (v >= 6) {
            field(b, "m_eOpinionBiggestInfluence");
            if (vp)
                field(b, "m_iTurnIdeologyAdopted");
            field(b, "m_iTurnIdeologySwitch");
        }
        if (v >= 4)
            fields(b, {"m_iSwappableWritingIndex", "m_iSwappableArtIndex", "m_iSwappableArtifactIndex",
                       "m_iSwappableMusicIndex"});
    }
};
} // namespace
PlayerTradeCultureObservation
observe_player_trade_culture(runtime::BinaryReader &in, GameStateLayout layout,
                             std::optional<PlayerTradeCultureWireDimensions> vp_dimensions,
                             GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (vp != vp_dimensions.has_value() || (vp && (!vp_dimensions->major || !vp_dimensions->yields)))
        throw std::runtime_error("Supply nonzero trade/culture source spans only for VP");
    PlayerTradeCultureObservation p{
        layout, vp_dimensions.value_or(PlayerTradeCultureWireDimensions{}), in.position(), 0, {}};
    TradeCultureReader r{in, budget, vp, p.dimensions};
    using Method = void (TradeCultureReader::*)(Record &);
    constexpr std::pair<const char *, Method> steps[] = {{"player_trade", &TradeCultureReader::trade},
                                                         {"trade_ai", &TradeCultureReader::trade_ai},
                                                         {"league_ai", &TradeCultureReader::league_ai},
                                                         {"player_culture", &TradeCultureReader::culture}};
    for (auto [name, method] : steps) {
        Record b{name, 0, in.position(), 0, {}, {}, {}, {}};
        try {
            (r.*method)(b);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string(name) + ": " + e.what());
        }
        b.end_offset = in.position();
        p.blocks.push_back(std::move(b));
    }
    p.end_offset = in.position();
    return p;
}
} // namespace civ5::saves
