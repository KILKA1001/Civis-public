#include "civ5/runtime/bnw_first_contact_expansion_preflight.hpp"
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
std::int64_t scalar(const auto &fields,std::string_view name) {
    using Field=typename std::remove_cvref_t<decltype(fields)>::value_type;
    const Field *found=nullptr;
    for(const auto &field:fields)
        if(field.name && name==field.name) {
            if(found) throw std::runtime_error("BNW expansion capital scalar duplicated");
            found=&field;
        }
    if(!found || found->values.size()!=1)
        throw std::runtime_error("BNW expansion capital scalar unavailable");
    if constexpr(std::is_same_v<Field,saves::PlayerCoreField>) {
        if(found->encoding!=saves::PlayerCoreEncoding::i32)
            throw std::runtime_error("BNW expansion player scalar encoding differs");
    } else {
        if(found->encoding!=saves::CityCoreEncoding::i32)
            throw std::runtime_error("BNW expansion city scalar encoding differs");
    }
    return found->values[0];
}
} // namespace

BnwFirstContactExpansionPreflight preflight_bnw_first_contact_expansion(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactExpansionPreflight result;
    const auto posture=preflight_bnw_first_contact_posture(
        source,state,request,database,types,defines,identity);
    result.source_verified=posture.source_verified;
    result.recipient=posture.recipient;
    result.target=posture.target;
    result.elapsed_turns=posture.elapsed_turns;
    result.target_city_count=posture.target_city_count;
    result.posture_gate=posture.gate;
    if(!posture.military_posture_call_completed ||
       posture.gate!=
           BnwFirstContactPostureGate::next_expansion_posture_unimplemented) {
        result.reason=posture.reason.empty()
            ? "BNW expansion posture requires completed military posture"
            : posture.reason;
        return result;
    }
    try {
        const auto &raw=state.world.all_players->players.at(
            static_cast<std::size_t>(result.recipient));
        const auto &owned=state.cities.players.at(result.recipient);
        if(raw.core.layout!=bnw || raw.cities.layout!=bnw ||
           raw.cities.header.wire_city_count<0 ||
           static_cast<std::size_t>(raw.cities.header.wire_city_count)!=
               raw.cities.cities.size() ||
           owned.cities.size()!=raw.cities.cities.size())
            throw std::runtime_error("BNW expansion recipient city roster differs");
        result.recipient_city_count=raw.cities.header.wire_city_count;
        result.recipient_capital_id=scalar(raw.core.fields,"m_iCapitalCityID");
        if(owned.capital!=result.recipient_capital_id)
            throw std::runtime_error("BNW expansion capital ID differs in owned city state");
        std::set<std::int64_t> ids;
        for(const auto &city:raw.cities.cities) {
            if(city.core.layout!=bnw)
                throw std::runtime_error("BNW expansion city profile differs");
            const auto id=scalar(city.core.fields,"m_iID");
            if(id<0 || !ids.insert(id).second ||
               !owned.cities.contains(id) || owned.cities.at(id).id!=id)
                throw std::runtime_error("BNW expansion raw and owned city IDs differ");
        }
        if(!ids.contains(result.recipient_capital_id)) {
            result.exit=BnwFirstContactExpansionExit::recipient_has_no_capital;
        } else if(result.target_city_count==0) {
            result.exit=BnwFirstContactExpansionExit::target_has_no_cities;
        } else {
            result.gate=BnwFirstContactExpansionGate::expansion_posture_unimplemented;
            result.reason="BNW expansion posture requires war and city/plot assessment";
            return result;
        }
        result.expansion_call_completed=true;
        result.gate=BnwFirstContactExpansionGate::next_target_value_unimplemented;
        result.reason="BNW expansion posture returns before effects; target value is next";
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactExpansionGate::capital_roster_inconsistent;
        result.reason=error.what();
    }
    return result;
}

} // namespace civ5::runtime
