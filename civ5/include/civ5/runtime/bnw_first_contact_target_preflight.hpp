#pragma once
#include "civ5/runtime/bnw_first_contact_expansion_preflight.hpp"

namespace civ5::runtime {

enum class BnwFirstContactTargetGate : std::uint8_t {
    earlier_expansion_unavailable,
    target_value_unimplemented,
    next_relationship_branch_unimplemented,
};
struct BnwFirstContactTargetPreflight {
    bool source_verified=false;
    bool target_value_call_completed=false;
    std::int64_t recipient=-1,target=-1,elapsed_turns=-1;
    std::int64_t target_city_count=-1;
    BnwFirstContactExpansionGate expansion_gate=
        BnwFirstContactExpansionGate::earlier_posture_unavailable;
    BnwFirstContactTargetGate gate=
        BnwFirstContactTargetGate::earlier_expansion_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:10191-10204 orders target value after
// expansion posture. DoUpdateOnePlayerTargetValue at 6567-6573 returns on
// !IsPlayerValid before getPower, city iteration, war state, rules or writing
// m_paePlayerTargetValue. Earlier first-contact checks must prove the selected
// target's city roster is empty. Opinion/approach or minor approach follows.
BnwFirstContactTargetPreflight preflight_bnw_first_contact_target(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
