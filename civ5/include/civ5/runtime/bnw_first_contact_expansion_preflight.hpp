#pragma once
#include "civ5/runtime/bnw_first_contact_posture_preflight.hpp"

namespace civ5::runtime {

enum class BnwFirstContactExpansionExit : std::uint8_t {
    none,
    recipient_has_no_capital,
    target_has_no_cities,
};
enum class BnwFirstContactExpansionGate : std::uint8_t {
    earlier_posture_unavailable,
    capital_roster_inconsistent,
    expansion_posture_unimplemented,
    next_target_value_unimplemented,
};
struct BnwFirstContactExpansionPreflight {
    bool source_verified=false;
    bool expansion_call_completed=false;
    std::int64_t recipient=-1,target=-1,elapsed_turns=-1;
    std::int64_t recipient_city_count=-1,recipient_capital_id=-1;
    std::int64_t target_city_count=-1;
    BnwFirstContactExpansionExit exit=BnwFirstContactExpansionExit::none;
    BnwFirstContactPostureGate posture_gate=
        BnwFirstContactPostureGate::earlier_economic_unavailable;
    BnwFirstContactExpansionGate gate=
        BnwFirstContactExpansionGate::earlier_posture_unavailable;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:10197 calls expansion posture after military
// posture and before target value. At 8222-8226 it reads getCapitalCity first,
// then IsPlayerValid(target); both can return before war/city/plot/diplomacy
// effects. Resolve the capital ID against the exact raw and owned city roster
// so the selected no-op follows the source order rather than an assumed city
// count. Any earlier first-contact blocker still prevents this call.
BnwFirstContactExpansionPreflight preflight_bnw_first_contact_expansion(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
