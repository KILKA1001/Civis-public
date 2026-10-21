#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct LeagueVoteObservation {
    std::optional<std::uint64_t> version;
    std::int64_t player = 0, votes = 0, choice = 0;
};
struct LeagueVoterDecisionObservation {
    std::optional<std::uint64_t> base_version, version;
    std::int64_t type = 0;
    std::vector<LeagueVoteObservation> votes;
};
struct LeagueProposerDecisionObservation {
    std::optional<std::uint64_t> base_version, version;
    std::int64_t type = 0;
    LeagueVoteObservation vote;
};
struct ResolutionEffectsObservation {
    std::optional<std::uint64_t> version;
    std::vector<NamedSaveInteger> integers;
    std::vector<NamedSaveFlag> flags;
};
// Group membership identifies enact/repeal/active. No resolution is executed.
struct ResolutionObservation {
    std::optional<std::uint64_t> version, proposal_version, kind_version;
    // Version 1 has no saved ID. The original generates one during loading;
    // keep that operation separate from observation and retain the absence.
    std::optional<std::int64_t> id;
    std::int64_t type = 0, league = 0;
    ResolutionEffectsObservation effects;
    LeagueVoterDecisionObservation voter_decision;
    LeagueProposerDecisionObservation proposer_decision;
    std::optional<std::int64_t> proposal_player, legacy_active_id, turn_enacted, target_resolution_id;
    std::optional<LeagueVoterDecisionObservation> repeal_decision;
};
struct LeagueMemberObservation {
    std::int64_t player = 0, extra_votes = 0, proposals = 0, votes = 0, abstained_votes = 0;
    std::vector<std::uint8_t> vote_sources;
    bool may_propose = false, ever_been_host = false, always_been_host = true;
};
struct LeagueProjectObservation {
    std::int64_t type = 0;
    std::vector<std::int64_t> production;
    bool complete = false, progress_warning_sent = false;
};
struct LeagueObservation {
    std::optional<std::uint64_t> version;
    std::int64_t id = 0, turns_until_session = 0, resolutions_ever_enacted = 0;
    bool united_nations = false, in_session = false;
    std::optional<bool> host_proposing;
    std::vector<ResolutionObservation> enact_proposals, repeal_proposals, active_resolutions;
    std::vector<LeagueMemberObservation> members;
    std::int64_t host = -1;
    std::vector<LeagueProjectObservation> projects;
    std::int64_t consecutive_hosted_sessions = 0, assigned_name = -1;
    // 128 raw bytes in the original format; empty in BNW versions before 7.
    std::vector<std::uint8_t> custom_name;
    std::optional<std::int64_t> last_session_ended_turn;
    std::int64_t last_special_session = -1, current_special_session = -1;
    std::vector<ResolutionObservation> enact_proposals_on_hold, repeal_proposals_on_hold;
    std::vector<ResolutionObservation> last_turn_enact_proposals, last_turn_repeal_proposals;
};
struct GameLeaguesObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    std::int64_t generated_id_count = 0, leagues_ever_founded = 0;
    std::int64_t diplomatic_victor = -1, last_era_trigger = -1;
    std::vector<LeagueObservation> leagues;
};
// Starts after CvGameCulture; stops before CvGameTrade. Preserves all source
// IDs, effect values, votes and project contributions without applying them.
GameLeaguesObservation observe_game_leagues(runtime::BinaryReader &input, GameStateLayout layout,
                                            GameStateBudget budget = {});
} // namespace civ5::saves
