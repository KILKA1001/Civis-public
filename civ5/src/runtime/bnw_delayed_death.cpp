#include "civ5/runtime/bnw_delayed_death.hpp"
#include <algorithm>
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
            require(!found,"Duplicate BNW delayed-death core field");
            found=&item;
        }
    require(found && found->values.size()==1,
            "BNW delayed-death core field unavailable");
    return found->values.front();
}
std::int64_t tail_field(const saves::UnitTailObservation &tail,
                        std::string_view name) {
    const saves::UnitTailNode *state=nullptr,*found=nullptr;
    for (const auto &block:tail.blocks)
        if (block.name && std::string_view(block.name)=="state") {
            require(!state,"Duplicate BNW delayed-death tail state");
            state=&block;
        }
    require(state,"BNW delayed-death tail state unavailable");
    for (const auto &child:state->children)
        if (child.name && std::string_view(child.name)==name) {
            require(!found,"Duplicate BNW delayed-death tail field");
            found=&child;
        }
    require(found && found->values.size()==1,
            "BNW delayed-death tail field unavailable");
    return found->values.front();
}
} // namespace

BnwDelayedDeathDraft stage_bnw_delayed_death_draft(
    LiveSessionState &state,std::int64_t player,
    const BnwDeficitCandidate &selected,
    const BnwDelayedDeathLiveChecks &checks) {
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.maintenance.profile==content::TechnologyProfile::bnw &&
            checks.fighting && checks.busy && !*checks.fighting &&
            !*checks.busy && selected.unit_id>=0,
            "BNW delayed death requires a ready owned world and live checks");
    saves::SavedPlayerObservation *owner=nullptr;
    for (auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!owner,"Duplicate BNW delayed-death player");
            owner=&candidate;
        }
    require(owner && owner->units.layout==bnw &&
            state.maintenance.players.contains(player) &&
            state.maintenance.players.at(player).units.contains(selected.unit_id),
            "BNW delayed-death unit owner or maintenance view unavailable");
    auto &units=owner->units.units;
    const auto iter=std::find_if(units.begin(),units.end(),[&](const auto &unit) {
        return unit.index==selected.source_index;
    });
    require(iter!=units.end() &&
            std::count_if(units.begin(),units.end(),[&](const auto &unit) {
                return unit.index==selected.source_index;
            })==1 && iter->core.layout==bnw && iter->tail.layout==bnw &&
            field(iter->core,"m_eOwner")==player &&
            field(iter->core,"m_iID")==selected.unit_id &&
            field(iter->core,"m_bDeathDelay")==1 &&
            field(iter->core,"m_iX")>=0 && field(iter->core,"m_iY")>=0,
            "BNW delayed-death candidate is stale or not death-delayed");
    const auto &core=iter->core;
    require(field(core,"m_iArmyId")==-1 &&
            field(core,"m_iCargo")==0 &&
            field(core,"m_iReconX")==-1 &&
            field(core,"m_iReconY")==-1 &&
            field(core,"m_iAttackPlotX")==-1 &&
            field(core,"m_iAttackPlotY")==-1 &&
            field(core,"m_iGarrisonCityID")==-1 &&
            field(core,"m_eCapturingPlayer")==-1 &&
            field(core,"m_eLeaderUnitType")==-1 &&
            tail_field(iter->tail,"m_combatUnit.eOwner")==-1 &&
            tail_field(iter->tail,"m_combatUnit.iID")==-1 &&
            tail_field(iter->tail,"m_transportUnit.eOwner")==-1 &&
            tail_field(iter->tail,"m_transportUnit.iID")==-1,
            "BNW delayed-death unit has linked army, plot or transport effects");
    for (const auto &saved_player:state.world.all_players->players) {
        require(saved_player.units.layout==bnw,
                "BNW delayed-death player unit profile mismatch");
        for (const auto &other:saved_player.units.units) {
            if (&other==&*iter) continue;
            require(other.core.layout==bnw && other.tail.layout==bnw,
                    "BNW delayed-death other unit profile mismatch");
            require(tail_field(other.tail,"m_transportUnit.eOwner")!=player ||
                    tail_field(other.tail,"m_transportUnit.iID")!=selected.unit_id,
                    "BNW delayed-death transport cargo still linked");
            if (saved_player.saved_player_id==player)
                require(field(other.core,"m_iID")!=selected.unit_id,
                        "Duplicate BNW delayed-death unit ID");
        }
    }
    // CvPlayer::deleteUnit removes the live unit by ID. The saved collection
    // header remains diagnostic; its allocator/free-index rules are not ported.
    state.maintenance.players.at(player).units.erase(selected.unit_id);
    units.erase(iter);
    return {selected.unit_id,selected.source_index};
}

LiveActionResult try_bnw_delayed_death_draft(
    LiveSession &session,std::int64_t player,
    const BnwDeficitCandidate &selected,
    const BnwDelayedDeathLiveChecks &checks) {
    try {
        return apply_live_action(session,[&](LiveSessionState &copy) {
            (void)stage_bnw_delayed_death_draft(copy,player,selected,checks);
            return LiveActionResult{LiveActionOutcome::rejected,
                "BNW kill(false) requires Lua CanSaveUnit/UnitPrekill, plot, "
                "army, cargo, unit-class, resource, capture, cache and UI effects"};
        });
    } catch (const std::exception &error) {
        return {LiveActionOutcome::rejected,error.what()};
    }
}

} // namespace civ5::runtime
