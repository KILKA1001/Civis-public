#include "civ5/runtime/bnw_team_tech_reward_ui.hpp"

namespace civ5::runtime {

BnwTechRewardUiPlan plan_bnw_team_tech_reward_ui(
    const BnwTechRewardUiInput &input) {
    BnwTechRewardUiPlan out;
    if(input.team<0 || input.technology<0) {
        out.blocker="Team and technology must be resolved before reward UI";
        return out;
    }
    if(!input.final_initialized || !input.new_value) {
        out.status=BnwTechRewardUiStatus::planned;
        return out;
    }
    out.native_option_query_required=true;
    if(!input.observed_no_reward_popups) {
        out.blocker="DLLUI::IsOptionNoRewardPopups lacks a live query receipt";
        return out;
    }
    out.native_option_query_observed=true;

    if(input.major_players_in_slot_order.size()!=22) {
        out.blocker="BNW major-player notification roster is incomplete";
        return out;
    }
    for(std::int64_t slot=0;slot<22;++slot)
        if(input.major_players_in_slot_order[slot].id!=slot) {
            out.blocker="BNW major-player notification roster is out of SDK slot order";
            return out;
        }

    // CvTeam::AddNotification (Expansion2 CvTeam.cpp:7094-7113) scans major
    // player slots in order and ignores dead, other-team, or ownerless slots.
    std::vector<std::int64_t> recipients;
    for(const auto &player:input.major_players_in_slot_order) {
        if(player.alive && player.team==input.team &&
           player.has_notification_owner)
            recipients.push_back(player.id);
    }
    const bool popup_possible=input.team==input.active_team;
    if(!*input.observed_no_reward_popups && !input.network_multiplayer &&
       (!recipients.empty() || popup_possible)) {
        out.blocker="Network mode is needed to choose notification or popup";
        return out;
    }
    out.network_mode_observed=input.network_multiplayer.has_value();
    const bool notify=*input.observed_no_reward_popups ||
        input.network_multiplayer.value_or(false);
    if(notify) {
        for(auto player:recipients)
            out.intents.push_back({BnwTechRewardUiKind::notification,
                                   player,input.team,input.technology});
    } else if(popup_possible) {
        // The execution owner must supply GC.getGame().getActivePlayer().
        out.intents.push_back({BnwTechRewardUiKind::popup,-1,
                               input.team,input.technology});
    }
    out.status=BnwTechRewardUiStatus::planned;
    return out;
}

} // namespace civ5::runtime
