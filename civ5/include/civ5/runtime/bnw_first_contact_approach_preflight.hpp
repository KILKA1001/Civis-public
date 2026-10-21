#pragma once
#include "civ5/runtime/bnw_first_contact_relationship_preflight.hpp"

namespace civ5::runtime {

enum class BnwFirstContactApproachGate : std::uint8_t {
    earlier_relationship_unavailable,
    team_or_diplomacy_roster_inconsistent,
    neutral_rule_unavailable,
    scratchpad_unrestored,
};
struct BnwFirstContactApproachPreflight {
    bool source_verified=false;
    bool major_branch_verified=false;
    std::int64_t recipient=-1,target=-1,recipient_team=-1,target_team=-1;
    std::int64_t neutral_default=0;
    bool neutral_rule_found=false;
    BnwFirstContactRelationshipGate relationship_gate=
        BnwFirstContactRelationshipGate::earlier_target_value_unavailable;
    BnwFirstContactApproachGate gate=
        BnwFirstContactApproachGate::earlier_relationship_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:10214 calls GetBestApproachTowardsMajorCiv
// with bLookAtOtherPlayers=true and bLog=true, before optional SetWarFace and
// mandatory SetMajorCivApproach. The getter at 2744 reads target team, builds
// local weights, applies APPROACH_NEUTRAL_DEFAULT, then reads the transient
// m_paeApproachScratchPad at 2779. That array is absent from the serialized
// DiplomacyAI Read/Write and from the current owned live state. Later weights
// consume gameplay RNG at 3417 and bLog invokes LogMajorCivApproachUpdate.
// This preflight stops at the first unavailable source-order input.
BnwFirstContactApproachPreflight preflight_bnw_first_contact_approach(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
