#include "civ5/runtime/bnw_first_contact_target_preflight.hpp"

namespace civ5::runtime {

BnwFirstContactTargetPreflight preflight_bnw_first_contact_target(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactTargetPreflight result;
    const auto expansion=preflight_bnw_first_contact_expansion(
        source,state,request,database,types,defines,identity);
    result.source_verified=expansion.source_verified;
    result.recipient=expansion.recipient;
    result.target=expansion.target;
    result.elapsed_turns=expansion.elapsed_turns;
    result.target_city_count=expansion.target_city_count;
    result.expansion_gate=expansion.gate;
    if(!expansion.expansion_call_completed ||
       expansion.gate!=BnwFirstContactExpansionGate::next_target_value_unimplemented) {
        result.reason=expansion.reason.empty()
            ? "BNW first-contact target value requires completed expansion posture"
            : expansion.reason;
        return result;
    }
    if(expansion.target_city_count!=0) {
        result.gate=BnwFirstContactTargetGate::target_value_unimplemented;
        result.reason="BNW target value requires live power, city defense, war and diplomacy inputs";
        return result;
    }
    result.target_value_call_completed=true;
    result.gate=BnwFirstContactTargetGate::next_relationship_branch_unimplemented;
    result.reason="BNW IsPlayerValid skips target without cities before target value effects; relationship branch is next";
    return result;
}

} // namespace civ5::runtime
