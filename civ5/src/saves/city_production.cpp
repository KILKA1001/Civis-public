#include "civ5/saves/city_production.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Record = CityProductionRecord;
enum class Wire { I32, U32, Bool };
struct ProductionReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const CityProductionContext &context;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("City production exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(Wire wire) {
        if (wire == Wire::I32)
            return in.i32();
        if (wire == Wire::U32)
            return in.u32();
        const auto n = in.u8();
        if (n > 1)
            throw std::runtime_error("Unverified city production boolean");
        return n;
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
    std::uint64_t count(Record &b, const char *name, bool signed_count = false, bool stl = false) {
        const auto n = field(b, name, signed_count ? Wire::I32 : Wire::U32);
        return n <= 0 || (vp && stl && n == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(Record &b, const char *name, std::uint64_t n, std::uint64_t columns = 1,
               Wire wire = Wire::I32) {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("City production dimensions exceed budget");
        const auto total = n * columns;
        claim(total, wire == Wire::Bool ? 1 : 4);
        PlotArrayObservation a{name, columns, {}};
        a.values.reserve(static_cast<std::size_t>(total));
        for (std::uint64_t i = 0; i < total; ++i)
            a.values.push_back(number(wire));
        b.arrays.push_back(std::move(a));
    }
    template <class F>
    void records(Record &b, const char *name, std::uint64_t n, std::uint64_t width, F decode) {
        claim(n, width);
        for (std::uint64_t i = 0; i < n; ++i) {
            Record c{name, i, in.position(), 0, {}, {}, {}, {}};
            decode(c);
            c.end_offset = in.position();
            b.children.push_back(std::move(c));
        }
    }
    void hashed(Record &b, const char *name, bool signed_count = false, std::uint64_t columns = 1) {
        Record a{name, 0, in.position(), 0, {}, {}, {}, {}};
        records(a, "row", count(a, "wire_count", signed_count), 4, [&](Record &row) {
            // Null hash has no payload. Unknown nonzero hashes still have data.
            if (field(row, "hash", Wire::U32))
                array(row, "values", 1, columns);
        });
        a.end_offset = in.position();
        b.children.push_back(std::move(a));
    }
    void buildings(Record &b) {
        if (!vp && field(b, "version", Wire::U32) > 1)
            throw std::runtime_error("Unverified CityBuildings version");
        fields(b, {"m_iNumBuildings", "m_iBuildingProductionModifier"});
        if (vp)
            field(b, "m_iBuildingProductionModifierPotentialFromMinorTrade");
        fields(b, {"m_iBuildingDefense", "m_iBuildingDefenseMod", "m_iMissionaryExtraSpreads",
                   "m_iLandmarksTourismPercent", "m_iGreatWorksTourismModifier"});
        field(b, "m_bSoldBuildingThisTurn", Wire::Bool);
        for (auto name :
             {"m_paiBuildingProduction", "m_paiBuildingProductionTime", "m_paiBuildingOriginalOwner",
              "m_paiBuildingOriginalTime", "m_paiNumRealBuilding", "m_paiNumFreeBuilding"}) {
            if (vp)
                array(b, name, *vp_city_production_building_slots(context));
            else
                hashed(b, name, true);
        }
        if (vp) {
            array(b, "m_paiFirstTimeBuilding", *vp_city_production_building_slots(context));
            array(b, "m_paiThemingBonusIndex", *vp_city_production_building_slots(context));
        }
        array(b, "m_aBuildingYieldChange", count(b, "m_aBuildingYieldChange_wire_count", false, true), 3);
        array(b, "m_aBuildingGreatWork", count(b, "m_aBuildingGreatWork_wire_count", false, true), 3);
    }
    void before_queue(Record &b) {
        hashed(b, "m_paiUnitProduction");
        hashed(b, "m_paiUnitProductionTime");
        for (auto name : {"m_paiSpecialistCount", "m_paiMaxSpecialistCount", "m_paiForceSpecialistCount",
                          "m_paiFreeSpecialistCount", "m_paiImprovementFreeSpecialists",
                          "m_paiUnitCombatFreeExperience", "m_paiUnitCombatProductionModifier"}) {
            Record a{name, 0, in.position(), 0, {}, {}, {}, {}};
            array(a, "values", count(a, "wire_count"));
            a.end_offset = in.position();
            b.children.push_back(std::move(a));
        }
        hashed(b, "m_paiFreePromotionCount");
    }
    void queue(Record &b) {
        // Explicit unsigned count in both sources; not a VP STL sentinel.
        records(b, "order", count(b, "wire_count"), 14, [&](Record &order) {
            const auto type = field(order, "eOrderType");
            const bool hash = !vp && *context.bnw_city_version >= 5 && type >= 0 && type <= 4;
            // BNW type 3 is PREPARE, 4 MAINTAIN. VP type 3 is MAINTAIN.
            field(order, hash ? "iData1_hash" : "iData1", hash ? Wire::U32 : Wire::I32);
            field(order, "iData2");
            field(order, "bSave", Wire::Bool);
            field(order, "bRush", Wire::Bool);
        });
    }
    void before_strategy(Record &b) {
        for (auto name :
             {"m_ppaiResourceYieldChange", "m_ppaiFeatureYieldChange", "m_ppaiTerrainYieldChange"})
            hashed(b, name, false, 6); // BNW NUM_YIELD_TYPES wire span, no row length.
        field(b, "m_iPopulationRank");
        field(b, "m_bPopulationRankValid", Wire::Bool);
        array(b, "m_aiBaseYieldRank", count(b, "m_aiBaseYieldRank_wire_count"));
        array(b, "m_abBaseYieldRankValid", count(b, "m_abBaseYieldRankValid_wire_count"), 1, Wire::Bool);
        array(b, "m_aiYieldRank", count(b, "m_aiYieldRank_wire_count"));
        array(b, "m_abYieldRankValid", count(b, "m_abYieldRankValid_wire_count"), 1, Wire::Bool);
        fields(b, {"m_iGameTurnLastExpanded", "m_iBaseHappinessFromBuildings",
                   "m_iUnmoddedHappinessFromBuildings"});
        field(b, "m_bRouteToCapitalConnectedLastTurn", Wire::Bool);
        field(b, "m_bRouteToCapitalConnectedThisTurn", Wire::Bool);
        const auto n = count(b, "m_strName_wire_bytes");
        if (n > budget.string_bytes || n > budget.total_string_bytes || n > in.remaining())
            throw std::runtime_error("City production name exceeds input or byte budget");
        budget.total_string_bytes -= n;
        CityProductionBlob name{"m_strName", std::vector<std::uint8_t>(static_cast<std::size_t>(n))};
        in.read(name.bytes);
        b.blobs.push_back(std::move(name));
        field(b, "m_bOwedCultureBuilding", Wire::Bool);
    }
};
} // namespace
CityProductionObservation observe_city_production(runtime::BinaryReader &in, GameStateLayout layout,
                                                  CityProductionContext context, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto slots = vp_city_production_building_slots(context);
    if (vp ? (!slots || !*slots || context.bnw_city_version)
           : (slots || !context.bnw_city_version || *context.bnw_city_version > 6))
        throw std::runtime_error("City production requires matching building slots or BNW city version");
    CityProductionObservation p{layout, context, in.position(), 0, {}};
    ProductionReader r{in, budget, vp, p.context};
    using Method = void (ProductionReader::*)(Record &);
    constexpr std::pair<const char *, Method> steps[] = {
        {"buildings", &ProductionReader::buildings},
        {"before_queue", &ProductionReader::before_queue},
        {"queue", &ProductionReader::queue},
        {"before_strategy", &ProductionReader::before_strategy}};
    for (std::size_t i = 0; i < std::size(steps); ++i) {
        if (vp && (i == 1 || i == 3))
            continue;
        auto [name, method] = steps[i];
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
