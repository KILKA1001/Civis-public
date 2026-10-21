#include "civ5/saves/city_tail.hpp"
#include "civ5/saves/vp_city_tail_profile.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Node = CityTailNode;
enum class Wire { I32, U32, Bool, I16 };
struct TailReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const CityTailContext &context;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("City tail exceeds input or element budget");
        budget.elements -= n;
    }
    Node node(const char *name, const char *encoding = "record") {
        claim(1);
        return {name, encoding, in.position(), 0, {}, {}, {}};
    }
    void finish(Node &parent, Node child) {
        child.end_offset = in.position();
        parent.children.push_back(std::move(child));
    }
    std::int64_t number(Wire wire = Wire::I32) {
        if (wire == Wire::I32)
            return in.i32();
        if (wire == Wire::U32)
            return in.u32();
        if (wire == Wire::I16)
            return in.i16();
        const auto n = in.u8();
        if (n > 1)
            throw std::runtime_error("Unverified city tail boolean");
        return n;
    }
    std::int64_t scalar(Node &parent, const char *name, Wire wire = Wire::I32) {
        auto n = node(name, wire == Wire::Bool ? "bool" : wire == Wire::U32 ? "u32" : "i32");
        const auto value = number(wire);
        n.values.push_back(value);
        finish(parent, std::move(n));
        return value;
    }
    std::int64_t version(Node &b, std::uint64_t maximum) {
        const auto v = scalar(b, "version", Wire::U32);
        if (static_cast<std::uint64_t>(v) > maximum)
            throw std::runtime_error("Unverified city tail version");
        return v;
    }
    std::uint64_t count(Node &n, bool signed_count = false, bool stl = false) {
        n.wire_count = number(signed_count ? Wire::I32 : Wire::U32);
        const auto raw = *n.wire_count;
        return raw <= 0 || (vp && stl && raw == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(raw);
    }
    void values(Node &n, std::uint64_t rows, std::uint64_t columns = 1, Wire wire = Wire::I32) {
        if (!columns || rows > budget.elements / columns)
            throw std::runtime_error("City tail dimensions exceed budget");
        const auto total = rows * columns;
        claim(total, wire == Wire::Bool ? 1 : wire == Wire::I16 ? 2 : 4);
        n.values.reserve(static_cast<std::size_t>(total));
        for (std::uint64_t i = 0; i < total; ++i)
            n.values.push_back(number(wire));
    }
    void raw(Node &b, const char *name, std::uint64_t size, Wire wire = Wire::I32) {
        auto n = node(name, wire == Wire::Bool ? "span_bool" : "span_i32");
        values(n, size, 1, wire);
        finish(b, std::move(n));
    }
    void pairs(Node &b, const char *name, std::uint64_t columns = 2) {
        auto n = node(name, columns == 3 ? "stl_triples_i32" : "stl_pairs_i32");
        const auto size = count(n, false, true);
        values(n, size, columns);
        finish(b, std::move(n));
    }
    void religion(Node &b) {
        if (!vp) {
            const auto v = version(b, 3);
            if (v >= 2)
                scalar(b, "paid_adoption_bonus", Wire::Bool);
            if (v >= 3)
                scalar(b, "religious_pressure_modifier");
        }
        auto status = node("m_ReligionStatus", vp ? "stl_records" : "signed_records");
        const auto size = count(status, !vp, vp);
        claim(size, vp ? 16 : 17);
        for (std::uint64_t i = 0; i < size; ++i) {
            auto row = node("religion");
            const auto v = vp ? 2 : version(row, 2);
            scalar(row, "m_eReligion");
            if (!vp)
                scalar(row, "m_bFoundedHere", Wire::Bool);
            scalar(row, "m_iFollowers");
            scalar(row, "m_iPressure");
            if (v == 2)
                scalar(row, "m_iNumTradeRoutesApplyingPressure");
            finish(status, std::move(row));
        }
        finish(b, std::move(status));
        if (vp)
            scalar(b, "m_eMajorityCityReligion");
    }
    void emphases(Node &b) {
        if (!vp)
            version(b, 1);
        scalar(b, "m_iEmphasizeAvoidGrowthCount");
        scalar(b, "m_iEmphasizeGreatPeopleCount");
        raw(b, "m_aiEmphasizeYieldCount", vp ? vp_city_tail_dimensions(context)->yields : 6);
        if (vp)
            raw(b, "m_pbEmphasize", vp_city_tail_dimensions(context)->emphases, Wire::Bool);
        else {
            auto flags = node("m_pbEmphasize", "hashed_bool");
            const auto size = count(flags);
            claim(size, 4);
            for (std::uint64_t i = 0; i < size; ++i) {
                auto row = node("entry");
                if (scalar(row, "hash", Wire::U32))
                    scalar(row, "value", Wire::Bool);
                finish(flags, std::move(row));
            }
            finish(b, std::move(flags));
        }
    }
    void espionage(Node &b) {
        version(b, 0);
        if (vp) {
            const auto mod = scalar(b, "mod_version", Wire::U32);
            const auto sentinel = scalar(b, "sentinel", Wire::U32);
            if (mod != 151 || sentinel != 0xdeadbeef)
                throw std::runtime_error("Unverified VP city espionage mod header");
        }
        constexpr const char *names[] = {"m_aiSpyAssignment",
                                         "m_aiAmount",
                                         "m_aiMaxAmount",
                                         "m_aiNextPassiveBonus",
                                         "m_aiRate",
                                         "m_aiGoal",
                                         "m_aiLastProgress",
                                         "m_aiLastPotential",
                                         "m_aiLastBasePotential",
                                         "m_aiResult",
                                         "m_aiNumTimesCityRobbed",
                                         "m_aiPendingEventsForPlayer",
                                         "m_aiDiplomatTradeBonus",
                                         "m_abDiplomatReceiveIntrigues",
                                         "m_abDiplomatRevealTrueApproaches",
                                         "m_aiDiplomatVoteTradePercent",
                                         "m_aiSciencePassivePerTurn",
                                         "m_abRevealCityScreen",
                                         "m_aiVisionBonus"};
        for (unsigned i = 0; i < std::size(names); ++i) {
            if (!vp && (i == 2 || i == 3 || i > 10))
                continue;
            const bool boolean = i == 13 || i == 14 || i == 17;
            auto n = node(names[i], boolean ? "counted_bool" : "counted_i32");
            // Explicit loops: UINT32_MAX is never an STL empty sentinel here.
            const auto size = count(n, i >= 6);
            values(n, size, 1, boolean ? Wire::Bool : Wire::I32);
            finish(b, std::move(n));
        }
    }
    void yield_records(Node &b, bool events) {
        auto n = node(events ? "m_eventYields" : "m_yieldChanges", "stl_records");
        const auto size = count(n, false, true);
        claim(size, events ? 28 : 80);
        constexpr const char *extra[] = {"forTerrain",
                                         "forXTerrain",
                                         "forTerrainFromBuildings",
                                         "forTerrainFromReligion",
                                         "forFeature",
                                         "forXFeature",
                                         "forFeatureFromBuildings",
                                         "forFeatureFromReligion",
                                         "forFeatureUnimproved",
                                         "forImprovement",
                                         "fromImprovement",
                                         "fromImprovementLocal",
                                         "fromImprovementGlobal",
                                         "forSpecialist",
                                         "forResource",
                                         "forPlot",
                                         "forYield",
                                         "forActualYield",
                                         "forLocalBuilding",
                                         "forReligionBuilding"};
        constexpr const char *event[] = {"forBuilding", "forBuildingModifier", "forFeature", "forImprovement",
                                         "forResource", "forSpecialist",       "forTerrain"};
        for (std::uint64_t i = 0; i < size; ++i) {
            auto row = node("yield");
            for (unsigned j = 0; j < (events ? std::size(event) : std::size(extra)); ++j)
                pairs(row, events ? event[j] : extra[j], !events && j >= 10 && j <= 12 ? 3 : 2);
            finish(n, std::move(row));
        }
        finish(b, std::move(n));
    }
    void remainder(Node &b) {
        if (!vp) {
            if (*context.bnw_city_version >= 3)
                scalar(b, "m_iExtraHitPoints");
            return;
        }
        yield_records(b, false);
        yield_records(b, true);
        auto gp = node("m_ppiGreatPersonProgressFromConstruction", "stl_pair_i32_i16");
        const auto count_gp = count(gp, false, true);
        claim(count_gp, 10);
        for (std::uint64_t i = 0; i < count_gp; ++i) {
            claim(3);
            gp.values.push_back(number());
            gp.values.push_back(number());
            gp.values.push_back(number(Wire::I16));
        }
        finish(b, std::move(gp));
        pairs(b, "m_aiYieldPerPopInEmpire");
        auto nested = node("m_miTechEnhancedYields", "stl_map_i32_pairs");
        const auto size = count(nested, false, true);
        claim(size, 8);
        for (std::uint64_t i = 0; i < size; ++i) {
            auto row = node("entry");
            scalar(row, "key");
            pairs(row, "value");
            finish(nested, std::move(row));
        }
        finish(b, std::move(nested));
        pairs(b, "m_miGreatPersonPointFromConstruction", 3);
        scalar(b, "m_iUnhappinessFromBuildings");
    }
    void ai(Node &b) {
        if (!vp)
            version(b, 1);
        scalar(b, "m_bChooseProductionDirty", Wire::Bool);
        scalar(b, "m_iCachePlayerClosenessTurn");
        scalar(b, "m_iCachePlayerClosenessDistance");
        if (vp)
            pairs(b, "m_mapPlotsAcquiredByOtherPlayers");
        else {
            // Source fixed arrays, never the capacity of the native player roster.
            raw(b, "m_aiPlayerCloseness", 80);
            raw(b, "m_aiNumPlotsAcquiredByOtherPlayers", 80);
        }
    }
};
} // namespace
CityTailObservation observe_city_tail(runtime::BinaryReader &in, GameStateLayout layout,
                                      CityTailContext context, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *dimensions = vp_city_tail_dimensions(context);
    if (vp ? (!dimensions || context.bnw_city_version)
           : (dimensions || !context.bnw_city_version || *context.bnw_city_version > 6))
        throw std::runtime_error("City tail context does not match layout");
    CityTailObservation p{layout, context, in.position(), 0, {}};
    TailReader r{in, budget, vp, p.context};
    constexpr const char *names[] = {"religions", "emphases", "espionage", "remainder", "city_ai"};
    for (unsigned i = 0; i < std::size(names); ++i) {
        auto b = r.node(names[i]);
        try {
            if (i == 0)
                r.religion(b);
            else if (i == 1)
                r.emphases(b);
            else if (i == 2)
                r.espionage(b);
            else if (i == 3)
                r.remainder(b);
            else
                r.ai(b);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string(names[i]) + ": " + e.what());
        }
        b.end_offset = in.position();
        p.blocks.push_back(std::move(b));
    }
    p.end_offset = in.position();
    return p;
}
} // namespace civ5::saves
