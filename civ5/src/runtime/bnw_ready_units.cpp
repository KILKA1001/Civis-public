#include "civ5/runtime/bnw_ready_units.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
bool delayed_death(const saves::SavedUnitObservation &unit) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)if(field.name && std::string_view(field.name)=="m_bDeathDelay") {
        need(!found,"Duplicate ready-unit delayed-death flag");found=&field;
    }
    need(found && found->encoding==saves::UnitCoreEncoding::boolean && found->values.size()==1 &&
         (found->values[0]==0 || found->values[0]==1),"Unknown ready-unit delayed-death flag");
    return found->values[0]!=0;
}
}
BnwReadyPlayerObservation observe_bnw_ready_player(
    const LiveSessionState &state,const BnwUpdateMovesImportOwner &moves,
    const BnwBusyStateOwner &busy,const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(moves.identity().payload_sha256==identity.payload_sha256 && moves.identity().rules_sha256==identity.rules_sha256,
         "Ready-unit original world owner identity differs");
    const auto evidence=observe_bnw_update_moves_units(state,moves,player);
    const auto busy_now=observe_bnw_busy_player(state,busy,identity,player);
    const auto &source=state.world.all_players->players.at(player).units.units;
    need(source.size()==evidence.size(),"Ready-unit source evidence order differs");
    BnwReadyPlayerObservation result;
    for(std::size_t index=0;index<evidence.size();++index) {
        const auto &unit=evidence.at(index);
        const bool is_busy=std::find(busy_now.busy_units.begin(),busy_now.busy_units.end(),unit.id)!=busy_now.busy_units.end();
        // CvUnit.cpp:18594-18631. Source Activity AWAKE is0, NO_AUTOMATE=-1.
        // ReadyToAuto does not require awake/not-busy/not-automated/not-dying.
        const bool ready=unit.can_move && unit.mission_count==0 && unit.activity==0 && unit.automate==-1 && !is_busy;
        const bool automatic=unit.can_move && unit.mission_count!=0;
        const bool delayed=delayed_death(source.at(index));
        result.units.push_back({unit.id,ready,automatic,delayed});
        result.has_auto_unit=result.has_auto_unit || automatic;
        // CvPlayer.cpp:4676-4706: dying units are excluded from the PLAYER
        // count/getter, not from the UNIT ReadyToMove predicate itself.
        if(ready && !delayed)result.counted_ready_ids.push_back(unit.id);
    }
    need(result.counted_ready_ids.size()<=INT32_MAX,"Native ready-unit count exceeds source signed int");
    result.count_ready=static_cast<std::int64_t>(result.counted_ready_ids.size());
    result.has_ready_unit=!result.counted_ready_ids.empty();return result;
}
BnwReadyPlayerObservation observe_bnw_ready_player(
    const BnwEndTurnRequestDraft &request,const BnwUpdateMovesImportOwner &moves,std::int64_t player) {
    need(request.owner.busy_state.has_value(),"Ready-unit same retained busy owner is unavailable");
    return observe_bnw_ready_player(request.state,moves,*request.owner.busy_state,request.owner.identity,player);
}
} // namespace civ5::runtime
