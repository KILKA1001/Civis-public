#include "civ5/runtime/bnw_first_contact_posture_preflight.hpp"

namespace civ5::runtime {

BnwFirstContactPosturePreflight preflight_bnw_first_contact_posture(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactPosturePreflight result;
    const auto economic=preflight_bnw_first_contact_economic(
        source,state,request,database,types,defines,identity);
    result.source_verified=economic.source_verified;
    result.recipient=economic.recipient;
    result.target=economic.target;
    result.elapsed_turns=economic.elapsed_turns;
    result.target_city_count=economic.target_city_count;
    result.economic_gate=economic.gate;
    if(!economic.economic_call_completed ||
       economic.gate!=
           BnwFirstContactEconomicGate::next_military_posture_unimplemented) {
        result.reason=economic.reason.empty()
            ? "BNW first-contact military posture requires completed economic call"
            : economic.reason;
        return result;
    }
    if(economic.target_city_count!=0) {
        result.gate=BnwFirstContactPostureGate::military_posture_unimplemented;
        result.reason="BNW military posture for a target with cities requires prior posture, war, units and visibility";
        return result;
    }
    result.military_posture_call_completed=true;
    result.gate=BnwFirstContactPostureGate::next_expansion_posture_unimplemented;
    result.reason="BNW IsPlayerValid skips target without cities before military posture effects; expansion posture is next";
    return result;
}

} // namespace civ5::runtime
