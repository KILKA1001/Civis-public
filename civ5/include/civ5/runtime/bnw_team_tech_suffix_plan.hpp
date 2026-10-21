#pragma once

#include "civ5/runtime/bnw_team_tech_share_culture.hpp"

namespace civ5::runtime {

enum class BnwTeamTechSuffixStep {
    announced_research_choice,
    espionage_stealable_list,
    major_player_happiness,
    active_team_ui_dirty,
    reward_popup_option,
    gameplay_tech_acquired,
};
enum class BnwTeamTechSuffixUiKind {
    dirty_misc_buttons, dirty_selection_buttons, dirty_research_buttons,
    query_no_reward_popups,
};
struct BnwTeamTechSuffixUiIntent {
    BnwTeamTechSuffixUiKind kind;
    std::int64_t team=63, technology=0;
};
struct BnwTeamTechEngineIntent {
    std::int64_t team=63, technology=0;
    // gDLL->GameplayTechAcquired is mandatory even for the barbarian team.
    bool gameplay_tech_acquired=true;
};
struct BnwTeamTechSuffixPlan {
    std::vector<BnwTeamTechSuffixStep> proven_no_effect;
    BnwTeamTechSuffixStep first_pending=
        BnwTeamTechSuffixStep::announced_research_choice;
    std::vector<BnwTeamTechSuffixUiIntent> ui_intents;
    std::optional<BnwTeamTechEngineIntent> later_engine_intent;
    // This read-only interval adds no checkpoint section; an actual UI or
    // engine callback requires an owner and a publication receipt.
    std::vector<saves::CivisSaveSectionId> additional_checkpoint_sections;
    std::string blocker;
    bool grant_complete=false;
};

// Source-order remainder of CvTeam::setHasTech for DoBarbarianTech's
// bNewValue=true,bAnnounce=false,ePlayer=getLeaderID(). Never invokes or
// assumes a successful external UI/engine callback.
BnwTeamTechSuffixPlan plan_bnw_team_tech_suffix(
    const LiveSessionState &detached,
    const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,
    std::int64_t technology);

} // namespace civ5::runtime
