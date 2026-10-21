#include "civ5/runtime/bnw_deficit_scrap.hpp"
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
std::int64_t field(const saves::UnitCoreObservation &core,
                   std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for (const auto &item:core.fields)
        if (item.name && std::string_view(item.name)==name) {
            require(!found,"Duplicate BNW scrap unit field");found=&item;
        }
    require(found && found->values.size()==1,
            "BNW scrap unit field is unavailable");
    return found->values.front();
}
saves::UnitCoreField &mutable_field(saves::UnitCoreObservation &core,
                                    std::string_view name) {
    saves::UnitCoreField *found=nullptr;
    for (auto &item:core.fields)
        if (item.name && std::string_view(item.name)==name) {
            require(!found,"Duplicate BNW scrap unit field");found=&item;
        }
    require(found && found->values.size()==1,
            "BNW scrap unit field is unavailable");
    return *found;
}
std::int64_t checked_mul(std::int64_t left,std::int64_t right) {
    require(left>=0 && right>=0 &&
            (right==0 || left<=INT32_MAX/right),
            "BNW scrap refund exceeds source int range");
    return left*right;
}
std::int64_t refund(std::int64_t cost,std::int64_t percent,
                    std::int64_t hp,std::int64_t damage,
                    std::int64_t train) {
    require(cost>=INT32_MIN && cost<=INT32_MAX &&
            percent>=0 && percent<=INT32_MAX &&
            hp>0 && hp<=INT32_MAX && damage>=0 && damage<=hp &&
            train>=0 && train<=INT32_MAX,
            "BNW scrap refund rules or health unavailable");
    if (cost<=0) return 0;
    // CvUnit::GetScrapGold performs these integer operations in this order.
    // Refuse source-int overflow rather than emulate undefined C++ behavior.
    auto gold=checked_mul(cost,percent)/100;
    const auto health_percent=checked_mul(100,hp-damage)/hp;
    gold=checked_mul(gold,health_percent)/100;
    return checked_mul(gold,train)/100;
}
} // namespace

BnwDeficitScrapDraft stage_bnw_deficit_scrap_draft(
    LiveSessionState &state,std::int64_t player,
    const BnwDeficitCandidate &selected,const BnwDeficitScrapRules &rules,
    const BnwDeficitScrapPlot &plot) {
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.treasury.profile==content::TechnologyProfile::bnw &&
            state.treasury.players.contains(player),
            "BNW scrap requires an owned BNW world and treasury");
    require(plot.x && plot.y && plot.owner && plot.unit_fighting &&
            !*plot.unit_fighting &&
            !rules.unit_delete_disabled,
            "BNW scrap requires live plot and canScrap state");
    require(selected.unit_id>=0 && selected.power>=0 &&
            (selected.domain==BnwDeficitDomain::land ||
             selected.domain==BnwDeficitDomain::sea),
            "BNW scrap selection is invalid");
    saves::SavedPlayerObservation *owner=nullptr;
    for (auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!owner,"Duplicate BNW scrap player");owner=&candidate;
        }
    require(owner && owner->units.layout==bnw,
            "BNW scrap player units unavailable");
    saves::SavedUnitObservation *unit=nullptr;
    for (auto &candidate:owner->units.units)
        if (candidate.index==selected.source_index) {
            require(!unit,"Duplicate BNW scrap source index");unit=&candidate;
        }
    require(unit && unit->core.layout==bnw &&
            field(unit->core,"m_iID")==selected.unit_id &&
            field(unit->core,"m_eOwner")==player &&
            field(unit->core,"m_iX")==*plot.x && *plot.x>=0 &&
            field(unit->core,"m_iY")==*plot.y && *plot.y>=0 &&
            field(unit->core,"m_iMoves")>0 &&
            field(unit->core,"m_iCargo")==0 &&
            field(unit->core,"m_bDeathDelay")==0,
            "BNW scrap candidate changed, carries cargo or cannot move");
    const auto hash=field(unit->core,"idUnitType");
    require(hash>0 && hash<=UINT32_MAX &&
            rules.production_cost_by_unit_hash.contains(
                static_cast<std::uint32_t>(hash)),
            "BNW scrap production cost is not bound to unit Info hash");
    // A saved cargo count is not enough to prove no current transport link.
    // Every other owned unit must have a decoded transport handle.
    for (const auto &saved_player:state.world.all_players->players)
        for (const auto &other:saved_player.units.units) {
            if (&other==unit) continue;
            const saves::UnitTailNode *tail_state=nullptr;
            for (const auto &block:other.tail.blocks)
                if (block.name && std::string_view(block.name)=="state") {
                    require(!tail_state,"Duplicate BNW scrap unit tail state");
                    tail_state=&block;
                }
            require(tail_state,"BNW scrap transport links unavailable");
            const saves::UnitTailNode *transport_owner=nullptr,*transport_id=nullptr;
            for (const auto &child:tail_state->children) {
                if (child.name && std::string_view(child.name)=="m_transportUnit.eOwner")
                    transport_owner=&child;
                if (child.name && std::string_view(child.name)=="m_transportUnit.iID")
                    transport_id=&child;
            }
            require(transport_owner && transport_id &&
                    transport_owner->values.size()==1 &&
                    transport_id->values.size()==1,
                    "BNW scrap transport handle unavailable");
            require(transport_owner->values.front()!=player ||
                    transport_id->values.front()!=selected.unit_id,
                    "BNW scrap cargo refund and death effects unimplemented");
        }
    const auto amount=*plot.owner==player ?
        refund(rules.production_cost_by_unit_hash.at(
                   static_cast<std::uint32_t>(hash)),rules.refund_percent,
               rules.max_hit_points,field(unit->core,"m_iDamage"),
               rules.train_percent) : 0;
    auto &gold=state.treasury.players.at(player).balance_times100;
    require(amount<=INT32_MAX/100 && gold>=INT32_MIN && gold<=INT32_MAX &&
            gold<=INT32_MAX-amount*100,
            "BNW scrap treasury change exceeds source int range");
    gold+=amount*100;
    mutable_field(unit->core,"m_bDeathDelay").values.front()=1;
    return {amount,selected.unit_id};
}

LiveActionResult try_bnw_deficit_scrap_draft(
    LiveSession &session,std::int64_t player,
    const BnwDeficitCandidate &selected,const BnwDeficitScrapRules &rules,
    const BnwDeficitScrapPlot &plot) {
    try {
        return apply_live_action(session,[&](LiveSessionState &detached) {
            (void)stage_bnw_deficit_scrap_draft(
                detached,player,selected,rules,plot);
            return LiveActionResult{LiveActionOutcome::rejected,
                "BNW scrap requires DoDeficit notification, CanSaveUnit and "
                "UnitPrekill Lua, delayed-death finalization and cache services"};
        });
    } catch (const std::exception &error) {
        return {LiveActionOutcome::rejected,error.what()};
    }
}

} // namespace civ5::runtime
