#pragma once
#include "civ5/runtime/bnw_first_contact_economic_preflight.hpp"

namespace civ5::runtime {

enum class BnwFirstContactPostureGate : std::uint8_t {
    earlier_economic_unavailable,
    military_posture_unimplemented,
    next_expansion_posture_unimplemented,
};

struct BnwFirstContactPosturePreflight {
    bool source_verified=false;
    bool military_posture_call_completed=false;
    std::int64_t recipient=-1,target=-1,elapsed_turns=-1;
    std::int64_t target_city_count=-1;
    BnwFirstContactEconomicGate economic_gate=
        BnwFirstContactEconomicGate::earlier_military_unavailable;
    BnwFirstContactPostureGate gate=
        BnwFirstContactPostureGate::earlier_economic_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:10193-10197 orders military assessment,
// economic assessment, military aggressive posture, then expansion posture.
// At 9183-9189 the military posture call returns on !IsPlayerValid before
// reading previous posture, war state, units, visibility or rules. The only
// currently proven branch is the selected Gandhi contact with no target city;
// all earlier detached checks must pass and no live contact is published.
BnwFirstContactPosturePreflight preflight_bnw_first_contact_posture(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
