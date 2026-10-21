#include "civ5/runtime/bnw_first_contact_economic_preflight.hpp"

namespace civ5::runtime {

BnwFirstContactEconomicPreflight preflight_bnw_first_contact_economic(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactEconomicPreflight result;
    const auto military=preflight_bnw_first_contact_military(
        source,state,request,database,types,defines,identity);
    result.source_verified=military.source_verified;
    result.recipient=military.recipient;
    result.target=military.target;
    result.elapsed_turns=military.elapsed_turns;
    result.target_city_count=military.target_city_count;
    result.military_gate=military.gate;
    if(!military.first_call_completed ||
       military.gate!=BnwFirstContactMilitaryGate::next_economic_ai_unimplemented) {
        result.reason=military.reason.empty()
            ? "BNW first-contact economic assessment requires completed military call"
            : military.reason;
        return result;
    }
    if(military.target_city_count!=0) {
        result.gate=BnwFirstContactEconomicGate::economic_assessment_unimplemented;
        result.reason="BNW economic assessment for a target with cities requires live might and diplomacy fields";
        return result;
    }
    result.economic_call_completed=true;
    result.gate=BnwFirstContactEconomicGate::next_military_posture_unimplemented;
    result.reason="BNW IsPlayerValid skips a target without cities; military aggressive posture is next";
    return result;
}

} // namespace civ5::runtime
