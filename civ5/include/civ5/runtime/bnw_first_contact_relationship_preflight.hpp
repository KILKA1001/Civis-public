#pragma once
#include "civ5/runtime/bnw_first_contact_target_preflight.hpp"

namespace civ5::runtime {

enum class BnwFirstContactRelationshipGate : std::uint8_t {
    earlier_target_value_unavailable,
    player_class_inconsistent,
    major_opinion_unimplemented,
    major_approach_unimplemented,
    minor_approach_unimplemented,
};
struct BnwFirstContactRelationshipPreflight {
    bool source_verified=false;
    bool target_minor=false;
    bool major_opinion_call_completed=false;
    std::int64_t recipient=-1,target=-1,elapsed_turns=-1;
    std::int64_t target_city_count=-1;
    BnwFirstContactTargetGate target_gate=
        BnwFirstContactTargetGate::earlier_expansion_unavailable;
    BnwFirstContactRelationshipGate gate=
        BnwFirstContactRelationshipGate::earlier_target_value_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvPlayer.cpp:15557 reads CvPreGame::isMinorCiv(m_eID).
// CvDiplomacyAI.cpp:10201-10229 enters the major opinion/approach branch or
// writes MINOR_CIV_APPROACH_IGNORE for a minor. Major opinion at 2261-2268
// returns on !IsPlayerValid before any diplomacy calculation or write. The
// selected Gandhi target has no cities, but the major/minor class must still
// agree with both owned mirrors; the later approach remains unavailable.
BnwFirstContactRelationshipPreflight preflight_bnw_first_contact_relationship(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
