#pragma once
#include "civ5/runtime/bnw_first_contact_military_preflight.hpp"

namespace civ5::runtime {

enum class BnwFirstContactEconomicGate : std::uint8_t {
    earlier_military_unavailable,
    economic_assessment_unimplemented,
    next_military_posture_unimplemented,
};

struct BnwFirstContactEconomicPreflight {
    bool source_verified=false;
    bool economic_call_completed=false;
    std::int64_t recipient=-1,target=-1,elapsed_turns=-1;
    std::int64_t target_city_count=-1;
    BnwFirstContactMilitaryGate military_gate=
        BnwFirstContactMilitaryGate::earlier_contact_unavailable;
    BnwFirstContactEconomicGate gate=
        BnwFirstContactEconomicGate::earlier_military_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:10193-10197 calls military strength, then
// economic strength, then military aggressive posture. The economic call at
// 6484 first runs IsPlayerValid; a living target without cities returns before
// either GetEconomicMight, any rule lookup, or a diplomacy-array write.
// All prior detached contact/clock/roster checks must pass before using this
// branch. A target with cities remains blocked by the earlier military call.
BnwFirstContactEconomicPreflight preflight_bnw_first_contact_economic(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
