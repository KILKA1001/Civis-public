#include "civ5/saves/player_espionage.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Record = PlayerEspionageRecord;
enum class Wire { I32, U32, Bool };
struct EspionageReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player espionage exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t field(Record &b, const char *name, Wire wire = Wire::I32) {
        claim(1);
        const std::int64_t n = wire == Wire::I32   ? std::int64_t(in.i32())
                               : wire == Wire::U32 ? std::int64_t(in.u32())
                                                   : in.u8();
        if (wire == Wire::Bool && n > 1)
            throw std::runtime_error("Unverified player espionage boolean");
        b.fields.push_back({name, n});
        return n;
    }
    void fields(Record &b, std::initializer_list<const char *> names) {
        for (auto name : names)
            field(b, name);
    }
    std::int64_t header(Record &b, bool spy = false) {
        const auto v = field(b, "version", Wire::U32);
        if (spy ? (vp ? v != 1 : v > 1) : v != 0)
            throw std::runtime_error("Unverified player espionage version");
        if (vp) {
            const auto mod = field(b, "mod_version", Wire::U32);
            const auto marker = field(b, "sentinel", Wire::U32);
            if (mod != 151 || marker != 0xdeadbeef)
                throw std::runtime_error("Unverified VP espionage mod version or sentinel");
        }
        return v;
    }
    // These are explicit source loops, not STL operators. Unsigned -1 is
    // never an empty sentinel, including inside VP espionage.
    std::uint64_t count(Record &b, const char *name, bool signed_count = false) {
        const auto n = field(b, name, signed_count ? Wire::I32 : Wire::U32);
        return n <= 0 ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(Record &b, const char *name, std::uint64_t n, std::uint64_t columns = 1) {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("Player espionage dimensions exceed budget");
        const auto total = n * columns;
        claim(total, 4);
        PlotArrayObservation a{name, columns, {}};
        a.values.reserve(static_cast<std::size_t>(total));
        for (std::uint64_t i = 0; i < total; ++i)
            a.values.push_back(in.i32());
        b.arrays.push_back(std::move(a));
    }
    void vector(Record &b, const char *name, const char *count_name, bool signed_count = false) {
        array(b, name, count(b, count_name, signed_count));
    }
    template <class F>
    void records(Record &b, const char *name, std::uint64_t n, std::uint64_t minimum_bytes, F decode) {
        claim(n, minimum_bytes);
        for (std::uint64_t i = 0; i < n; ++i) {
            Record child{name, i, in.position(), 0, {}, {}, {}, {}};
            decode(child);
            child.end_offset = in.position();
            b.children.push_back(std::move(child));
        }
    }
    void text(Record &b, const char *name, const char *count_name) {
        const auto n = count(b, count_name);
        if (n > budget.string_bytes || n > budget.total_string_bytes || n > in.remaining())
            throw std::runtime_error("Player espionage name exceeds input or byte budget");
        budget.total_string_bytes -= n;
        PlayerEspionageBlob blob{name, std::vector<std::uint8_t>(static_cast<std::size_t>(n))};
        in.read(blob.bytes);
        b.blobs.push_back(std::move(blob));
    }
    void spy(Record &b) {
        const auto v = header(b, true);
        if (vp)
            text(b, "m_sName", "m_sName_wire_bytes");
        else
            field(b, "m_iName");
        field(b, "m_eRank");
        if (vp)
            field(b, "m_iExperience");
        fields(b, {"m_iCityX", "m_iCityY", "m_eSpyState"});
        if (vp)
            field(b, "m_eSpyFocus");
        field(b, "m_iReviveCounter");
        if (vp || v >= 1)
            field(b, "m_bIsDiplomat", Wire::Bool);
        if (vp)
            field(b, "m_eVassalDiplomatPlayer");
        // BNW Read forces this to true after reading. Preserve the saved byte.
        field(b, "m_bEvaluateReassignment", Wire::Bool);
        if (vp) {
            field(b, "m_bPassive", Wire::Bool);
            fields(b, {"m_iTurnCounterspyMissionChanged", "m_iTurnActiveMissionConducted"});
        }
    }
    void intrigue(Record &b) {
        fields(b, {"m_eDiscoveringPlayer", "m_eSourcePlayer", "m_eTargetPlayer"});
        if (vp)
            field(b, "m_eDiplomacyPlayer");
        fields(b, {"m_eBuilding", "m_eProject"});
        if (vp)
            field(b, "m_eUnit");
        fields(b, {"m_iIntrigueType", "m_iTurnNum", "m_iCityX", "m_iCityY"});
        text(b, "m_strSpyName", "m_strSpyName_wire_bytes");
        field(b, "m_bShared", Wire::Bool);
        if (vp)
            field(b, "iSpyID");
    }
    void espionage(Record &b) {
        header(b);
        records(b, "spy", count(b, "m_aSpyList_wire_count", true), vp ? 59 : 29, [&](Record &c) { spy(c); });
        if (!vp) {
            vector(b, "m_aiSpyListNameOrder", "m_aiSpyListNameOrder_wire_count", true);
            field(b, "m_iSpyListNameOrderIndex");
        }
        records(b, "stealable_techs", count(b, "m_aaPlayerStealableTechList_wire_count"), 4,
                [&](Record &c) { vector(c, "techs", "techs_wire_count"); });
        vector(b, "m_aiNumTechsToStealList", "m_aiNumTechsToStealList_wire_count");
        if (vp)
            vector(b, "m_aiNumSpyActionsDone", "m_aiNumSpyActionsDone_wire_count");
        else {
            vector(b, "m_aiMaxTechCost", "m_aiMaxTechCost_wire_count", true);
            records(b, "heist_locations", count(b, "m_aHeistLocations_wire_count", true), 4,
                    [&](Record &c) { array(c, "locations", count(c, "locations_wire_count", true), 2); });
        }
        records(b, "spy_notification", count(b, "m_aSpyNotificationMessages_wire_count", true), vp ? 32 : 20,
                [&](Record &c) {
                    fields(c, {"m_iCityX", "m_iCityY", "m_eAttackingPlayer", "m_iSpyResult"});
                    if (vp)
                        field(c, "m_iAmountStolen");
                    field(c, "m_eStolenTech");
                    if (vp)
                        fields(c, {"m_iMission", "m_iGWID"});
                });
        records(b, "intrigue", count(b, "m_aIntrigueNotificationMessages_wire_count", true), vp ? 53 : 41,
                [&](Record &c) { intrigue(c); });
    }
    void ai(Record &b) {
        header(b);
        constexpr std::pair<const char *, const char *> lists[] = {
            {"m_aiCivOutOfTechTurn", "m_aiCivOutOfTechTurn_wire_count"},
            {"m_aiNumSpiesCaught", "m_aiNumSpiesCaught_wire_count"},
            {"m_aiNumSpiesKilled", "m_aiNumSpiesKilled_wire_count"},
            {"m_aiNumSpiesDied", "m_aiNumSpiesDied_wire_count"},
            {"m_aiTurnLastSpyCaught", "m_aiTurnLastSpyCaught_wire_count"},
            {"m_aiTurnLastSpyKilled", "m_aiTurnLastSpyKilled_wire_count"},
            {"m_aiTurnLastSpyDied", "m_aiTurnLastSpyDied_wire_count"}};
        for (auto [name, count_name] : lists)
            vector(b, name, count_name);
        field(b, "m_bUNCountdownStarted", Wire::Bool);
        field(b, "m_iTurnEspionageStarted");
    }
};
} // namespace
PlayerEspionageObservation observe_player_espionage(runtime::BinaryReader &in, GameStateLayout layout,
                                                    GameStateBudget budget) {
    PlayerEspionageObservation result{layout, in.position(), 0, {}};
    EspionageReader r{in, budget, layout == GameStateLayout::vp542_corpus};
    using Method = void (EspionageReader::*)(Record &);
    constexpr std::pair<const char *, Method> steps[] = {{"player_espionage", &EspionageReader::espionage},
                                                         {"espionage_ai", &EspionageReader::ai}};
    for (auto [name, method] : steps) {
        Record b{name, 0, in.position(), 0, {}, {}, {}, {}};
        try {
            (r.*method)(b);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string(name) + ": " + e.what());
        }
        b.end_offset = in.position();
        result.blocks.push_back(std::move(b));
    }
    result.end_offset = in.position();
    return result;
}
} // namespace civ5::saves
