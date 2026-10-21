#include "civ5/saves/game_leagues.hpp"
#include <stdexcept>
#include <type_traits>

namespace civ5::saves {
namespace {
enum class ResolutionKind { enact, repeal, active };
class LeaguesReader {
  public:
    runtime::BinaryReader &input;
    GameStateBudget remaining;
    bool vp;
    std::optional<std::uint64_t> version(std::uint64_t maximum) {
        if (vp)
            return {};
        const auto value = input.u32();
        if (!value || value > maximum)
            throw std::runtime_error("Unverified league structure version");
        return value;
    }
    bool since(const std::optional<std::uint64_t> &v, std::uint64_t first) const { return vp || *v >= first; }
    bool flag() {
        const auto value = input.u8();
        if (value > 1)
            throw std::runtime_error("Unverified league boolean");
        return value != 0;
    }
    template <class Read> auto list(Read read, std::uint64_t minimum_bytes) {
        const std::int64_t count = vp ? std::int64_t(input.u32()) : input.i32();
        if (count < 0 || static_cast<std::uint64_t>(count) > remaining.elements ||
            static_cast<std::uint64_t>(count) > input.remaining() / minimum_bytes)
            throw std::runtime_error("League collection exceeds input or element budget");
        remaining.elements -= static_cast<std::uint64_t>(count);
        std::vector<std::invoke_result_t<Read>> result;
        result.reserve(static_cast<std::size_t>(count));
        for (std::int64_t i = 0; i < count; ++i)
            result.push_back(read());
        return result;
    }
    std::vector<std::uint8_t> bytes(std::uint64_t size) {
        if (size > remaining.string_bytes || size > remaining.total_string_bytes || size > input.remaining())
            throw std::runtime_error("League string exceeds input or string budget");
        remaining.total_string_bytes -= size;
        std::vector<std::uint8_t> result(static_cast<std::size_t>(size));
        input.read(result);
        return result;
    }
    LeagueVoteObservation vote() {
        LeagueVoteObservation result;
        result.version = version(1);
        result.player = input.i32();
        result.votes = input.i32();
        result.choice = input.i32();
        return result;
    }
    LeagueVoterDecisionObservation voter() {
        LeagueVoterDecisionObservation result;
        result.base_version = version(1);
        result.type = input.i32();
        result.version = version(1);
        result.votes = list([&] { return vote(); }, vp ? 12 : 16);
        return result;
    }
    LeagueProposerDecisionObservation proposer() {
        LeagueProposerDecisionObservation result;
        result.base_version = version(1);
        result.type = input.i32();
        result.version = version(1);
        result.vote = vote();
        return result;
    }
    ResolutionEffectsObservation effects() {
        ResolutionEffectsObservation result;
        result.version = version(9);
        // Wire order matters: flags and integers are interleaved. first == 0
        // denotes a VP-only effect; absent old BNW effects use SDK defaults.
        struct Field {
            const char *name;
            bool boolean;
            unsigned first;
            std::int64_t fallback = 0;
        };
        constexpr Field fields[] = {{"diplomatic_victory", true, 1},
                                    {"change_league_host", true, 2},
                                    {"one_time_gold", false, 1},
                                    {"one_time_gold_percent", false, 1},
                                    {"raise_city_state_influence_to_neutral", true, 1},
                                    {"league_project_enabled", false, 3, -1},
                                    {"gold_per_turn", false, 1},
                                    {"resource_quantity", false, 1},
                                    {"embargo_city_states", true, 1},
                                    {"embargo_player", true, 1},
                                    {"no_resource_happiness", true, 1},
                                    {"unit_maintenance_gold_percent", false, 1},
                                    {"member_discovered_tech_mod", false, 1},
                                    {"culture_per_wonder", false, 4},
                                    {"culture_per_natural_wonder", false, 4},
                                    {"no_training_nuclear_weapons", true, 5},
                                    {"votes_for_following_religion", false, 6},
                                    {"holy_city_tourism", false, 6},
                                    {"religion_spread_strength_mod", false, 9},
                                    {"votes_for_following_ideology", false, 6},
                                    {"other_ideology_rebellion_mod", false, 6},
                                    {"artsy_great_person_rate_mod", false, 7},
                                    {"sciencey_great_person_rate_mod", false, 7},
                                    {"great_person_tile_improvement_culture", false, 8},
                                    {"landmark_culture", false, 8},
                                    {"open_door", true, 0},
                                    {"sphere_of_influence", true, 0},
                                    {"decolonization", true, 0},
                                    {"limit_spaceship_production", false, 0},
                                    {"limit_spaceship_purchase", false, 0},
                                    {"is_world_war", false, 0},
                                    {"embargo_ideology", true, 0},
                                    {"change_tourism", false, 0},
                                    {"vassal_maintenance_gold_percent", false, 0},
                                    {"end_all_current_vassals", true, 0}};
        for (const auto &field : fields) {
            if (!vp && !field.first)
                continue;
            const auto value = since(result.version, field.first)
                                   ? (field.boolean ? std::int64_t(flag()) : input.i32())
                                   : field.fallback;
            if (field.boolean)
                result.flags.push_back({field.name, value != 0});
            else
                result.integers.push_back({field.name, value});
        }
        return result;
    }
    ResolutionObservation resolution(ResolutionKind kind) {
        ResolutionObservation result;
        result.version = version(2);
        if (since(result.version, 2))
            result.id = input.i32();
        result.type = input.i32();
        result.league = input.i32();
        result.effects = effects();
        result.voter_decision = voter();
        result.proposer_decision = proposer();
        if (kind != ResolutionKind::active) {
            result.proposal_version = version(1);
            result.proposal_player = input.i32();
        }
        result.kind_version = version(kind == ResolutionKind::active ? 2 : 1);
        if (kind == ResolutionKind::active) {
            if (!vp && *result.kind_version < 2)
                result.legacy_active_id = input.i32();
            result.turn_enacted = input.i32();
        } else if (kind == ResolutionKind::repeal) {
            result.target_resolution_id = input.i32();
            result.repeal_decision = voter();
        }
        return result;
    }
    auto resolutions(ResolutionKind kind) {
        return list([&] { return resolution(kind); }, 32);
    }
    LeagueMemberObservation member(const std::optional<std::uint64_t> &v) {
        LeagueMemberObservation result;
        result.player = input.i32();
        if (since(v, 9))
            result.extra_votes = input.i32();
        if (since(v, 10))
            result.vote_sources = bytes(input.u32());
        result.may_propose = flag();
        if (since(v, 12))
            result.proposals = input.i32();
        result.votes = input.i32();
        if (since(v, 13))
            result.abstained_votes = input.i32();
        if (since(v, 14)) {
            result.ever_been_host = flag();
            result.always_been_host = flag();
        }
        return result;
    }
    LeagueProjectObservation project(const std::optional<std::uint64_t> &v) {
        LeagueProjectObservation result;
        result.type = input.i32();
        result.production = list([&] { return std::int64_t(input.i32()); }, 4);
        if (since(v, 6))
            result.complete = flag();
        if (since(v, 11))
            result.progress_warning_sent = flag();
        return result;
    }
    LeagueObservation league() {
        LeagueObservation result;
        const auto v = result.version = version(14);
        result.id = input.i32();
        if (since(v, 4))
            result.united_nations = flag();
        if (since(v, 2))
            result.in_session = flag();
        if (vp)
            result.host_proposing = flag();
        result.turns_until_session = input.i32();
        result.resolutions_ever_enacted = input.i32();
        result.enact_proposals = resolutions(ResolutionKind::enact);
        result.repeal_proposals = resolutions(ResolutionKind::repeal);
        result.active_resolutions = resolutions(ResolutionKind::active);
        result.members = list([&] { return member(v); }, vp ? 27 : 9);
        if (since(v, 3))
            result.host = input.i32();
        if (since(v, 5))
            result.projects = list([&] { return project(v); }, 8);
        if (since(v, 7)) {
            result.consecutive_hosted_sessions = input.i32();
            result.assigned_name = input.i32();
            result.custom_name = bytes(128);
        }
        if (vp)
            result.last_session_ended_turn = input.i32();
        if (since(v, 8)) {
            result.last_special_session = input.i32();
            result.current_special_session = input.i32();
            result.enact_proposals_on_hold = resolutions(ResolutionKind::enact);
            result.repeal_proposals_on_hold = resolutions(ResolutionKind::repeal);
        }
        if (vp) {
            result.last_turn_enact_proposals = resolutions(ResolutionKind::enact);
            result.last_turn_repeal_proposals = resolutions(ResolutionKind::repeal);
        }
        return result;
    }
};
} // namespace
GameLeaguesObservation observe_game_leagues(runtime::BinaryReader &input, GameStateLayout layout,
                                            GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (!vp && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown game leagues layout");
    GameLeaguesObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    LeaguesReader reader{input, budget, vp};
    const auto v = result.version = reader.version(5);
    if (reader.since(v, 4))
        result.generated_id_count = input.i32();
    result.leagues = reader.list([&] { return reader.league(); }, 32);
    if (reader.since(v, 2))
        result.leagues_ever_founded = input.i32();
    if (reader.since(v, 3))
        result.diplomatic_victor = input.i32();
    if (reader.since(v, 5))
        result.last_era_trigger = input.i32();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
