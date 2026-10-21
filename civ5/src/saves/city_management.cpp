#include "civ5/saves/city_management.hpp"
#include "civ5/saves/vp_city_management_profile.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Block = CityManagementBlock;
enum class Wire { I32, U32, Bool };
struct ManagementReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const CityManagementContext &context;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("City management exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(Wire wire) {
        if (wire == Wire::I32)
            return in.i32();
        if (wire == Wire::U32)
            return in.u32();
        const auto n = in.u8();
        if (n > 1)
            throw std::runtime_error("Unverified city management boolean");
        return n;
    }
    std::int64_t field(Block &b, const char *name, Wire wire = Wire::I32) {
        claim(1);
        const auto n = number(wire);
        b.fields.push_back({name, n});
        return n;
    }
    void version(Block &b) {
        if (field(b, "version", Wire::U32) > 1)
            throw std::runtime_error("Unverified city management version");
    }
    std::uint64_t count(Block &b, const char *name, bool stl = false) {
        const auto n = field(b, name, stl ? Wire::U32 : Wire::I32);
        return n <= 0 || (vp && stl && n == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(Block &b, const char *name, std::uint64_t n, Wire wire = Wire::I32,
               std::uint64_t columns = 1) {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("City management dimensions exceed budget");
        const auto total = n * columns;
        claim(total, wire == Wire::Bool ? 1 : 4);
        PlotArrayObservation a{name, columns, {}};
        a.values.reserve(static_cast<std::size_t>(total));
        for (std::uint64_t i = 0; i < total; ++i)
            a.values.push_back(number(wire));
        b.arrays.push_back(std::move(a));
    }
    void hashed(Block &b, const char *name, Wire wire = Wire::I32, bool signed_count = false,
                bool value_for_null = false) {
        claim(1);
        TeamHashedArray a{name, number(signed_count ? Wire::I32 : Wire::U32), {}};
        const auto n = a.wire_count > 0 ? static_cast<std::uint64_t>(a.wire_count) : 0;
        claim(n, value_for_null ? 8 : 4);
        a.rows.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) {
            TeamHashedValues row{in.u32(), {}};
            // Process AI always writes a weight, even after a null hash.
            if (row.type_hash || value_for_null) {
                claim(1, wire == Wire::Bool ? 1 : 4);
                row.values.push_back(number(wire));
            }
            a.rows.push_back(std::move(row));
        }
        b.hashes.push_back(std::move(a));
    }
    void strategy(Block &b) {
        if (vp) {
            const auto &dimensions = *vp_city_management_dimensions(context);
            array(b, "m_piLatestFlavorValues", dimensions.flavors);
            array(b, "m_pabUsingCityStrategy", dimensions.strategies, Wire::Bool);
            array(b, "m_paiTurnCityStrategyAdopted", dimensions.strategies);
        } else {
            version(b);
            array(b, "m_piLatestFlavorValues", count(b, "flavor_wire_count"));
            hashed(b, "m_pabUsingCityStrategy", Wire::Bool);
            hashed(b, "m_paiTurnCityStrategyAdopted");
        }
        field(b, "m_eSpecialization");
        field(b, "m_eDefaultSpecialization");
    }
    void weights(Block &b, const char *name, unsigned kind) {
        if (vp) {
            // CvWeightedVector has signed count and (element, weight) pairs.
            array(b, name, count(b, "weight_wire_count"), Wire::I32, 2);
        } else {
            version(b);
            if (kind == 2)
                array(b, name, *context.bnw_projects); // No count on the wire.
            else
                hashed(b, name, Wire::I32, true, kind == 3);
        }
    }
    void citizens(Block &b) {
        if (!vp)
            version(b);
        field(b, "m_bAutomated", Wire::Bool);
        field(b, "m_bNoAutoAssignSpecialists", Wire::Bool);
        if (vp)
            field(b, "m_bIsDirty", Wire::Bool);
        for (auto name : {"m_iNumUnassignedCitizens", "m_iNumCitizensWorkingPlots",
                          "m_iNumForcedWorkingPlots", "m_eCityAIFocusTypes"})
            field(b, name);
        field(b, "m_bForceAvoidGrowth", Wire::Bool);
        for (auto name : {"m_pabWorkingPlot", "m_pabForcedWorkingPlot"})
            array(b, name, vp ? 91 : 37, Wire::Bool);
        if (vp)
            array(b, "m_vBlockadedPlots", count(b, "m_vBlockadedPlots_wire_count", true));
        field(b, "m_iNumDefaultSpecialists");
        field(b, "m_iNumForcedDefaultSpecialists");
        if (vp) {
            const auto &dimensions = *vp_city_management_dimensions(context);
            for (auto name :
                 {"m_aiSpecialistCounts", "m_aiSpecialistSlots", "m_aiSpecialistGreatPersonProgressTimes100"})
                array(b, name, dimensions.promotions);
            for (auto name : {"m_aiNumSpecialistsInBuilding", "m_aiNumForcedSpecialistsInBuilding"})
                array(b, name, dimensions.buildings);
            array(b, "m_piBuildingGreatPeopleRateChanges", dimensions.promotions);
        } else {
            hashed(b, "m_aiSpecialistCounts");
            hashed(b, "m_aiSpecialistGreatPersonProgressTimes100");
            hashed(b, "m_aiNumSpecialistsInBuilding", Wire::I32, true);
            hashed(b, "m_aiNumForcedSpecialistsInBuilding", Wire::I32, true);
            hashed(b, "m_piBuildingGreatPeopleRateChanges");
        }
    }
};
} // namespace
CityManagementObservation observe_city_management(runtime::BinaryReader &in, GameStateLayout layout,
                                                  CityManagementContext context, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *dimensions = vp_city_management_dimensions(context);
    if (vp ? (!dimensions || context.bnw_projects) : (dimensions || !context.bnw_projects))
        throw std::runtime_error("City management context does not match layout");
    CityManagementObservation p{layout, context, in.position(), 0, vp ? 91u : 37u, {}};
    ManagementReader r{in, budget, vp, p.context};
    constexpr const char *names[] = {"strategy",   "building_ai", "unit_ai",
                                     "project_ai", "process_ai",  "citizens"};
    constexpr const char *weights[] = {"m_BuildingAIWeights", "m_UnitAIWeights", "m_ProjectAIWeights",
                                       "m_ProcessAIWeights"};
    for (unsigned i = 0; i < std::size(names); ++i) {
        Block b{names[i], in.position(), 0, {}, {}, {}};
        try {
            if (i == 0)
                r.strategy(b);
            else if (i == 5)
                r.citizens(b);
            else
                r.weights(b, weights[i - 1], i - 1);
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
