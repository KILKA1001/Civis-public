#pragma once
#include "civ5/content/custom_mod_options.hpp"
#include "civ5/content/research_rules.hpp"
#include "civ5/runtime/research.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <set>

namespace civ5::runtime {
struct ResearchPlayerState {
    std::int64_t team = -1, handicap = -1, slot_status = 2, slot_claim = 0;
    bool alive = false;
    // Shared policy ownership. Policy adoption effects remain a separate step.
    std::set<std::int64_t> policies;
    bool policies_resolved=false;
    // Civ V base, expansions and GameCore profiles all serialize this flag.
    // A defeated player can be dead now while having lived earlier.
    bool ever_alive=false;
};
// Owned inputs restored from the same snapshot used by graphics. Public state
// is intentionally live: later roster/era changes must affect the next query.
// Map keys are saved object IDs, never the serialized collection's positions.
struct ResearchContext {
    content::TechnologyProfile profile = content::TechnologyProfile::bnw;
    ProfileExtensions extensions;
    std::int64_t game_type = 0, speed = -1, start_era = -1, current_era = 0, game_handicap = -1;
    std::int64_t game_turn = 0;
    // Pregame mapRandomSeed is shared Civ V game state. VP uses it for
    // CvSeeder; other profiles may use it for their own map/game rules.
    std::uint64_t map_random_seed = 0;
    std::int64_t world_percent = 0;
    std::map<std::int64_t, ResearchPlayerState> players;
    std::map<std::int64_t, std::int64_t> team_members;
    std::vector<saves::PregameOption> game_options;
    // A missing DB default means false. nullopt means the lookup was not needed
    // during restoration. Resolve it before adding an open non-hotseat player.
    std::optional<bool> pitboss;
    bool is_human(const ResearchPlayerState &player) const;
    // First saved C-string match; otherwise native lazy GameOptions.Default.
    // A found default is retained in the owned options after turn zero.
    std::int64_t game_option_value(const content::RulesDatabase &rules, std::string_view name);
};
ResearchContext restore_research_context(const saves::SavedSessionSnapshot &snapshot,
                                          const content::RulesDatabase &rules);
struct BoundTeamResearch {
    TeamResearchFactors factors;
    std::int64_t handicap_id = -1;
    std::uint64_t alive_players = 0;
    // Source missing-Info branches are retained (VP empty Info defaults to
    // zero). Keep them visible instead of claiming a fully resolved binding.
    std::vector<std::string> missing_infos;
};
BoundTeamResearch bind_team_research(const ResearchContext &context, const content::ResearchRules &rules,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *custom_options,
    const TeamTechnologies &team);
} // namespace civ5::runtime
