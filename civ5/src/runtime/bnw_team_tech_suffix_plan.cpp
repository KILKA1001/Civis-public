#include "civ5/runtime/bnw_team_tech_suffix_plan.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::optional<bool> game_flag(const saves::GameStateObservation &game,
                              std::string_view name) {
    std::optional<bool> result;
    for(const auto &flag:game.flags)
        if(flag.name && name==flag.name) {
            if(result) return std::nullopt;
            result=flag.value;
        }
    return result;
}
} // namespace

BnwTeamTechSuffixPlan plan_bnw_team_tech_suffix(
    const LiveSessionState &state,const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,std::int64_t technology) {
    BnwTeamTechSuffixPlan result;
    const auto prefix=plan_bnw_team_tech_after_lua(
        state,lua,technologies,database,types,team,technology);
    if(prefix.first_pending!=BnwTeamTechAfterLuaStep::best_route ||
       prefix.proven_no_effect.size()!=4) {
        result.blocker=prefix.blocker.empty() ?
            "BNW setHasTech suffix post-Lua prefix is incomplete" : prefix.blocker;
        return result;
    }
    const auto *previous=state.setup.extensions.find<
        BnwTeamTechShareCultureMarker>();
    if(!previous || previous->team!=team ||
       previous->technology!=technology) {
        result.blocker="BNW setHasTech suffix requires share/culture predecessor";
        return result;
    }
    // bAnnounce=false skips the research-choice block at lines 5631-5645.
    result.proven_no_effect.push_back(
        BnwTeamTechSuffixStep::announced_research_choice);
    try {
        require(state.setup.players.size()==64 && team==63 && technology==0,
                "BNW barbarian suffix roster or Agriculture identity changed");
        std::int64_t leader=-1;
        for(std::int64_t player=0;player<64;++player) {
            const auto it=state.setup.players.find(player);
            require(it!=state.setup.players.end(),
                    "BNW barbarian suffix player roster has a hole");
            if(leader<0 && it->second.alive && it->second.team==team)
                leader=player;
        }
        if(leader<0)
            for(std::int64_t player=0;player<64;++player)
                if(state.setup.players.at(player).team==team) {
                    leader=player;
                    break;
                }
        if(leader!=63) {
            result.first_pending=BnwTeamTechSuffixStep::espionage_stealable_list;
            result.blocker="BNW barbarian getLeaderID is not BARBARIAN_PLAYER";
            return result;
        }
        result.proven_no_effect.push_back(
            BnwTeamTechSuffixStep::espionage_stealable_list);

        // MAX_MAJOR_CIVS=22. If any living major shares the team,
        // DoUpdateHappiness is a mandatory world mutation.
        for(std::int64_t player=0;player<22;++player) {
            const auto &slot=state.setup.players.at(player);
            if(slot.alive && slot.team==team) {
                result.first_pending=BnwTeamTechSuffixStep::major_player_happiness;
                result.blocker="CvPlayer::DoUpdateHappiness is required for major player "+
                    std::to_string(player);
                return result;
            }
        }
        result.proven_no_effect.push_back(
            BnwTeamTechSuffixStep::major_player_happiness);

        require(state.world.game_state.layout==
                    saves::GameStateLayout::expansion2_version1 &&
                state.world.pregame &&
                state.world.pregame->layout==
                    saves::GameStateLayout::expansion2_version1,
                "BNW game suffix final-initialized state is unavailable");
        const auto final=game_flag(state.world.game_state,"final_initialized");
        require(final.has_value(),
                "BNW final-initialized flag is missing or duplicated");
        if(*final) {
            const auto active=state.world.pregame->archive.integers.find(
                "active_player");
            require(active!=state.world.pregame->archive.integers.end() &&
                    state.setup.players.contains(active->second),
                    "BNW active player is unavailable for tech UI");
            if(state.setup.players.at(active->second).team==team) {
                result.first_pending=BnwTeamTechSuffixStep::active_team_ui_dirty;
                for(const auto kind:{
                    BnwTeamTechSuffixUiKind::dirty_misc_buttons,
                    BnwTeamTechSuffixUiKind::dirty_selection_buttons,
                    BnwTeamTechSuffixUiKind::dirty_research_buttons})
                    result.ui_intents.push_back({kind,team,technology});
                result.blocker="BNW active-team tech UI dirty callbacks lack a publication owner";
                return result;
            }
        }
        result.proven_no_effect.push_back(
            BnwTeamTechSuffixStep::active_team_ui_dirty);
        if(*final) {
            // IsOptionNoRewardPopups is an external UI query even when no
            // major player would receive AddNotification and the barbarian
            // team would not receive an SP popup. Do not invent its result.
            result.first_pending=BnwTeamTechSuffixStep::reward_popup_option;
            result.ui_intents.push_back({
                BnwTeamTechSuffixUiKind::query_no_reward_popups,
                team,technology});
            result.later_engine_intent=BnwTeamTechEngineIntent{team,technology,true};
            result.blocker="DLLUI::IsOptionNoRewardPopups has no selected UI query receipt";
            return result;
        }
        result.proven_no_effect.push_back(
            BnwTeamTechSuffixStep::reward_popup_option);
        result.first_pending=BnwTeamTechSuffixStep::gameplay_tech_acquired;
        result.later_engine_intent=BnwTeamTechEngineIntent{team,technology,true};
        result.blocker="gDLL::GameplayTechAcquired requires an engine callback receipt";
    } catch(const std::exception &error) {
        if(result.proven_no_effect.size()<2)
            result.first_pending=BnwTeamTechSuffixStep::espionage_stealable_list;
        else if(result.proven_no_effect.size()<3)
            result.first_pending=BnwTeamTechSuffixStep::major_player_happiness;
        else
            result.first_pending=BnwTeamTechSuffixStep::active_team_ui_dirty;
        result.blocker=error.what();
    }
    return result;
}
} // namespace civ5::runtime
