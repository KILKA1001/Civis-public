#pragma once
#include "civ5/runtime/bnw_founder_benefits_lua.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <set>
namespace civ5::runtime {
// Immutable native query snapshot of one current BNW world. It is derived
// from the decoded religion list and agreeing raw/projected plot owners;
// it is not a separately mutable cache or a continuation readiness receipt.
class BnwFounderReligionReadModel {
 public:
    static BnwFounderReligionReadModel bind(const LiveSessionState &);
    std::int32_t religion_created_by_player(std::int64_t player) const;
    const saves::ReligionObservation *religion(
        std::int32_t religion,std::int64_t player=-1) const;
    bool eligible_for_founder_benefits(std::int32_t religion,
                                      std::int64_t player) const;
    std::size_t religion_count() const noexcept {return religions_.size();}
 private:
    BnwFounderReligionReadModel()=default;
    std::vector<saves::ReligionObservation> religions_;
    MapGeometry map_;
    std::vector<std::int64_t> plot_owners_;
    std::set<std::int64_t> players_;
};
struct BnwReligionHappinessQuote {
    bool founder_resolved=false,happiness_quoted=false;
    bool native_fallback_used=false;
    std::optional<std::int32_t> founder_religion,happiness;
    std::unique_ptr<BnwTeamTechLuaEpoch> epoch;
    std::string blocker;
};
// Executes the mandatory founder Lua quote for this player INSIDE this
// operation, then resolves its native fallback on that same detached world.
// A quote from another player therefore cannot be substituted. NO_RELIGION
// or the native GetReligion-null branch completes GetHappinessFromReligion
// with zero. Founded-belief effects still require their full native owner.
// No VM/world publication or aggregate happiness update happens here.
BnwReligionHappinessQuote quote_bnw_happiness_from_religion(
    const LiveSessionState &,const SavedContentProfile &,
    const SessionCheckpointIdentity &,BnwGameplayLuaEpochOwner &,
    std::int64_t player);
}
