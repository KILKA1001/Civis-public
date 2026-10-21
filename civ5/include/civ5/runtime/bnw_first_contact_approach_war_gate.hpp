#pragma once
#include "civ5/runtime/bnw_first_contact_approach_random_quote.hpp"

namespace civ5::runtime {

enum class BnwApproachWarGate : std::uint8_t {
    random_prefix_unavailable,
    team_state_unavailable,
    game_option_unavailable,
    lua_can_declare_war_unavailable,
    next_human_difficulty_unimplemented,
};

struct BnwApproachWarGateQuote {
    BnwApproachWarGate gate=BnwApproachWarGate::random_prefix_unavailable;
    std::array<std::int64_t,7> weights{};
    SimulationRandomState rng_before{},rng_after{};
    std::int64_t recipient_team=-1,target_team=-1;
    bool can_declare_without_lua=false,at_war=false,war_weight_zeroed=false;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:3421-3447, CvTeam.cpp:938-1025.
// Quotes source-order deterministic early returns from canDeclareWar after
// the first JonRand loop. If the native call reaches Lua CanDeclareWar, stops
// there. The copied RNG state and local weights are never published.
BnwApproachWarGateQuote quote_bnw_first_contact_approach_war_gate(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad);

} // namespace civ5::runtime
