#include "civ5/saves/player_administration.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Record = PlayerAdministrationRecord;
enum class Wire { I32, U32, Bool };
struct AdministrationReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player administration exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(Wire wire) {
        if (wire == Wire::I32)
            return in.i32();
        if (wire == Wire::U32)
            return in.u32();
        const auto value = in.u8();
        if (value > 1)
            throw std::runtime_error("Unverified player administration boolean");
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
            throw std::runtime_error("Unverified player administration version");
        return v;
    }
    // STL VP containers alone recognize unsigned -1 as an empty sentinel.
    std::uint64_t count(Record &b, const char *name, bool signed_count = false, bool stl = false) {
        const auto n = field(b, name, signed_count ? Wire::I32 : Wire::U32);
        return n <= 0 || (vp && stl && n == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(Record &b, const char *name, std::uint64_t n, std::uint64_t columns = 1) {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("Player administration dimensions exceed budget");
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
            throw std::runtime_error("Player administration string exceeds input or byte budget");
        budget.total_string_bytes -= n;
        PlayerAdministrationBlob blob{name, std::vector<std::uint8_t>(static_cast<std::size_t>(n))};
        in.read(blob.bytes);
        b.blobs.push_back(std::move(blob));
    }
    void notifications(Record &b) {
        if (!field(b, "has_notifications", Wire::Bool))
            return;
        if (!vp)
            version(b, 0, 2);
        fields(b, {"m_ePlayer", "m_iCurrentLookupIndex", "m_iNotificationsBeginIndex",
                   "m_iNotificationsEndIndex"});
        // The source writes every slot without a count, including inactive ones.
        records(b, "notification", vp ? 150 : 100, 41, [&](Record &c) {
            // Hashes may set bit 31; NO_NOTIFICATION_TYPE is all bits set.
            // BNW versions 0/1 use legacy indices. Preserve bits in both cases.
            field(c, "m_eNotificationType_bits", Wire::U32);
            text(c, "m_strMessage", "m_strMessage_wire_bytes");
            text(c, "m_strSummary", "m_strSummary_wire_bytes");
            fields(c, {"m_iX", "m_iY", "m_iGameDataIndex", "m_iExtraGameData", "m_iTurn", "m_iLookupIndex"});
            field(c, "m_bDismissed", Wire::Bool);
            field(c, "m_ePlayerID");
        });
    }
    void treasury(Record &b) {
        if (!vp)
            version(b, 0, 1);
        fields(b, {"m_iGold", "m_iGoldPerTurnFromDiplomacy", "m_iExpensePerTurnUnitMaintenance"});
        if (!vp)
            field(b, "m_iExpensePerTurnUnitSupply");
        fields(b, {"m_iCityConnectionGoldTimes100", "m_iCityConnectionTradeRouteGoldModifier",
                   "m_iCityConnectionTradeRouteGoldChange", "m_iBaseBuildingGoldMaintenance",
                   "m_iBaseImprovementGoldMaintenance"});
        array(b, "m_GoldBalanceForTurnTimes100",
              count(b, "m_GoldBalanceForTurnTimes100_wire_count", false, true));
        array(b, "m_GoldChangeForTurnTimes100",
              count(b, "m_GoldChangeForTurnTimes100_wire_count", false, true));
        field(b, "m_iLifetimeGrossGoldIncome");
        if (vp)
            fields(b, {"m_iInternalTradeGoldBonus", "m_iExpensePerTurnFromVassalTax"});
    }
    void pre_city(Record &b) {
        if (!vp) {
            // Each BNW vector row is a fixed Firaxis::Array<int,6>, no row count.
            array(b, "m_ppaaiSpecialistExtraYield", count(b, "m_ppaaiSpecialistExtraYield_wire_count"), 6);
            array(b, "m_ppaaiImprovementYieldChange", count(b, "m_ppaaiImprovementYieldChange_wire_count"),
                  6);
            // CvUnitCycler saves only its CLinkList<int>, no object version.
            array(b, "m_UnitCycle", count(b, "m_UnitCycle_wire_count", true));
        }
        // CLinkList lengths are signed in both layouts; negative means no items.
        array(b, "m_researchQueue", count(b, "m_researchQueue_wire_count", true));
        if (!vp) {
            field(b, "m_bEverPoppedGoody", Wire::Bool);
            field(b, "m_bEverTrainedBuilder", Wire::Bool);
        }
        fields(b, {"m_eEndTurnBlockingType", "m_iEndTurnBlockingNotificationIndex"});
        if (vp)
            field(b, "m_iEmpireSizeModifierPerCityMod");
        records(b, "city_name", count(b, "m_cityNames_wire_count", true), 4,
                [&](Record &c) { text(c, "value", "value_wire_bytes"); });
    }
};
} // namespace
PlayerAdministrationObservation
observe_player_administration(runtime::BinaryReader &in, GameStateLayout layout, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    PlayerAdministrationObservation p{layout, in.position(), 0, {}};
    AdministrationReader r{in, budget, vp};
    using Method = void (AdministrationReader::*)(Record &);
    constexpr std::pair<const char *, Method> steps[] = {
        {"notifications", &AdministrationReader::notifications},
        {"treasury", &AdministrationReader::treasury},
        {"pre_city", &AdministrationReader::pre_city}};
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
