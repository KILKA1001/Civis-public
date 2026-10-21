#pragma once

#include "civ5/runtime/bnw_team_tech_player_research.hpp"

namespace civ5::runtime {

enum class BnwTeamTechShareCultureStep {
    first_to_research,
    announce,
    tech_share,
    owed_culture_buildings,
    later_set_has_tech,
};
enum class BnwTeamTechUiIntentKind {
    message, notification, replay,
};
struct BnwTeamTechUiIntent {
    BnwTeamTechUiIntentKind kind=BnwTeamTechUiIntentKind::message;
    std::int64_t player=-1, technology=-1;
};
struct BnwTeamTechShareCheck {
    std::int64_t recipient=-1;
    std::int64_t best_share=-1, met_owner_count=0;
    bool already_owned=false, would_grant=false;
};
struct BnwTeamTechShareCultureMarker {
    std::int64_t team=63, technology=0;
    std::vector<BnwTeamTechShareCheck> checked_teams;
};
struct BnwTeamTechShareCultureResult {
    bool applied=false, grant_complete=false;
    std::vector<BnwTeamTechShareCultureStep> proven_no_effect;
    BnwTeamTechShareCultureStep first_pending=
        BnwTeamTechShareCultureStep::first_to_research;
    std::vector<BnwTeamTechShareCheck> checked_teams;
    // Empty for the selected bFirst=false/bAnnounce=false/no-grant path.
    // A future active branch must stage typed intents before publication.
    std::vector<BnwTeamTechUiIntent> ui_intents;
    // This interval adds no saved change on its safe path. Prior tech/player
    // effects still require their companion checkpoint sections.
    std::vector<saves::CivisSaveSectionId> additional_checkpoint_sections;
    std::string blocker;
};

// DoBarbarianTech calls setHasTech with bFirst=false,bAnnounce=false. For the
// selected Agriculture branch this only proves skipped first/announce work,
// no updateTechShare grant, and no owed major-team culture buildings. Any
// recursive grant or owed city rejects before mutation and before UI effects.
BnwTeamTechShareCultureResult run_bnw_team_tech_share_culture(
    LiveSessionState &detached,
    const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,
    std::int64_t technology);

} // namespace civ5::runtime

namespace civ5::runtime::detail {
// Pure actual native reads only. No receipt/readiness or mutation authority.
std::vector<BnwTeamTechShareCheck> quote_bnw_team_tech_share_checks(
 const LiveSessionState&,const content::TechnologyRules&,const content::InfoTypeCatalog&,
 std::int64_t team,std::int64_t technology);
std::optional<std::int64_t> quote_bnw_team_tech_first_owed_culture_player(const LiveSessionState&,std::int64_t team);
}
