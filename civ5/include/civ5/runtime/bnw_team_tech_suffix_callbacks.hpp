#pragma once

#include "civ5/runtime/bnw_team_tech_suffix_plan.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <functional>

namespace civ5::runtime {

struct BnwTeamTechSuffixCallbackOwner {
    const LiveSessionState *getter_world=nullptr;
    SessionCheckpointIdentity identity;
    // The selected UI/options owner and engine owner must attest their real
    // source epoch. A preview frame or empty callback is not such an owner.
    bool selected_ui_option_owner=false;
    bool selected_engine_owner=false;
    bool external_writes_escrowed=false;
    // nullopt means the native-equivalent UI query was not confirmed.
    std::function<std::optional<bool>(const LiveSessionState &)>
        query_no_reward_popups;
    // True only after GameplayTechAcquired(team,tech) was observed complete.
    std::function<bool(std::int64_t,std::int64_t,const LiveSessionState &)>
        gameplay_tech_acquired;
};

struct BnwTeamTechSuffixCallbackMarker {
    std::int64_t team=63, technology=0;
    bool ui_query_attempted=false, ui_query_confirmed=false;
    std::optional<bool> no_reward_popups;
    bool engine_attempted=false, engine_confirmed=false;
    bool uncertain=false;
};
enum class BnwTeamTechSuffixCallbackStatus { blocked, quoted, uncertain };
struct BnwTeamTechSuffixCallbackResult {
    BnwTeamTechSuffixCallbackStatus status=
        BnwTeamTechSuffixCallbackStatus::blocked;
    bool ui_query_confirmed=false, engine_confirmed=false;
    std::optional<bool> no_reward_popups;
    std::size_t notifications=0, popups=0;
    std::string blocker;
    bool grant_complete=false;
};

// Only the already-proven selected branch (no living major player on team 63,
// active team !=63) is supported: both reward-option outcomes produce zero
// notification/popup effects. The native-equivalent query still runs before
// the mandatory engine callback. Any unconfirmed call is uncertain, one-shot.
BnwTeamTechSuffixCallbackResult quote_bnw_team_tech_suffix_callbacks(
    LiveSessionState &detached,
    const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity,
    BnwTeamTechSuffixCallbackOwner *owner,
    std::int64_t team,
    std::int64_t technology);

} // namespace civ5::runtime
