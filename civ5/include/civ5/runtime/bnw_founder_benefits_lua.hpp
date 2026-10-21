#pragma once
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
namespace civ5::runtime {
enum class BnwFounderBenefitsLuaStatus { blocked, native_fallback_required, override_quoted };
struct BnwFounderBenefitsLuaQuote {
    BnwFounderBenefitsLuaStatus status=BnwFounderBenefitsLuaStatus::blocked;
    std::optional<std::int32_t> religion;
    // The quote owns an isolated VM/world. No effects are published here.
    // The enclosing happiness/religion transaction must publish it through
    // the same epoch owner together with all subsequent native effects.
    std::unique_ptr<BnwTeamTechLuaEpoch> epoch;
    std::string blocker;
};
// CvGameReligions::GetFounderBenefitsReligion: CallAccumulator(player,0).
// An absent return requires GetReligionCreatedByPlayer/eligibility next;
// an explicit -1 is an override and must not take that native fallback.
// The owner, not an empty host or filesystem scan, attests active listeners.
BnwFounderBenefitsLuaQuote quote_bnw_founder_benefits_lua(
    const LiveSessionState &,const SavedContentProfile &,
    const SessionCheckpointIdentity &,BnwGameplayLuaEpochOwner &,
    std::int64_t player);
}
