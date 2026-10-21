#pragma once
#include "civ5/runtime/bnw_sight_first_contact.hpp"

namespace civ5::runtime {

enum class BnwFirstContactMilitaryGate : std::uint8_t {
    earlier_contact_unavailable,
    clock_or_roster_inconsistent,
    military_rules_unavailable,
    own_might_lazy_recompute,
    target_might_lazy_recompute,
    military_assessment_unimplemented,
    next_economic_ai_unimplemented,
};
struct BnwFirstContactMilitaryPreflight {
    bool source_verified=false;
    bool first_call_completed=false;
    std::int64_t recipient=-1,target=-1,elapsed_turns=-1;
    std::int64_t own_might_cache_turn=-1,target_might_cache_turn=-1;
    std::int64_t target_city_count=-1;
    BnwFirstContactMilitaryGate gate=
        BnwFirstContactMilitaryGate::earlier_contact_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:10191 starts with
// DoUpdateOnePlayerMilitaryStrength, then economic strength, military and
// expansion postures, target value, opinion/approach. The military call reads
// MILITARY_STRENGTH_BASE and GetMilitaryMight(own) before IsPlayerValid(target).
// A stale might cache lazily recomputes both military/economic might from
// units, treasury and yields. This read-only gate proves the no-city skip when
// possible and never silently treats that lazy write as a cache read.
BnwFirstContactMilitaryPreflight preflight_bnw_first_contact_military(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
