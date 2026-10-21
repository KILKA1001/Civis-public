#include "civ5/runtime/vp_anarchy_turn.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <stdexcept>

namespace civ5::runtime {
VpAnarchyTurnResult do_vp_anarchy_after_policy_cost(
    ResearchContext &world,TreasuryTurnContext &turn,std::int64_t player,
    const VpPlayerAnarchyEvent &event) {
    constexpr auto vp=content::TechnologyProfile::vp542;
    if(world.profile!=vp||turn.state.profile!=vp||&turn.world.setup!=&world)
        throw std::runtime_error("VP anarchy turn requires one VP GameCore world");
    auto *history=world.extensions.find<VpCultureHistoryProfileState>();
    if(!history||!world.players.contains(player)||!history->players.contains(player)||
       !turn.state.players.contains(player))
        throw std::runtime_error("Unresolved VP anarchy player");
    auto &state=history->players.at(player);
    auto &treasury=turn.state.players.at(player);
    if(treasury.anarchy!=(state.anarchy_turns>0))
        throw std::runtime_error("VP anarchy state differs between player and treasury");
    VpAnarchyTurnResult result;
    result.before=result.after=state.anarchy_turns;
    if(result.before<=0)return result;
    result.after=wide::subtract(result.before,1);
    if(result.after==0) {
        if(!event)throw std::runtime_error("VP PlayerAnarchy event host is unavailable");
        event(player,false,0);
        result.event_ended=true;
    }
    state.anarchy_turns=result.after;
    treasury.anarchy=result.after>0;
    result.changed=true;
    if(player==turn.active_player) {
        turn.services.treasury_dirty(player,TreasuryDirty::game_data);
        result.ui_dirty=true;
    }
    return result;
}
} // namespace civ5::runtime
