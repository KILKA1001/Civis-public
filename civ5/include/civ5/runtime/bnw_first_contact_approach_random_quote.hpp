#pragma once
#include "civ5/runtime/bnw_first_contact_approach_after_load.hpp"
#include <vector>

namespace civ5::runtime {

enum class BnwApproachRandomGate : std::uint8_t {
    deterministic_prefix_unavailable,
    source_rng_unavailable,
    random_rule_unavailable,
    native_arithmetic_unavailable,
    next_can_declare_war_unimplemented,
};

struct BnwApproachRandomDraw {
    std::size_t approach=0;
    std::uint16_t native_bound=0;
    std::uint64_t value=0;
};

struct BnwApproachRandomQuote {
    BnwApproachRandomGate gate=BnwApproachRandomGate::deterministic_prefix_unavailable;
    std::array<std::int64_t,7> weights{};
    SimulationRandomState rng_before{},rng_after{};
    std::vector<BnwApproachRandomDraw> draws;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:3407-3420. A diagnostic on a copy of
// CvGame::m_jonRand. The native int is multiplied by 100, then by the rule;
// only a positive product calls getJonRandNumVA. Its int argument narrows to
// CvRandom::get(unsigned short), including a zero bound after division.
// The seven local weights are never published, nor is the copied RNG state.
BnwApproachRandomQuote quote_bnw_approach_random_weights(
    const std::array<std::int64_t,7> &deterministic_weights,
    std::int64_t random_percent,const SimulationRandom &gameplay);

// Rechecks the entire preceding SDK prefix and the unchanged source/owned
// random streams before quoting the first JonRand loop. Later canDeclareWar,
// handicap, logging, SetWarFace and SetMajorCivApproach remain blocked.
BnwApproachRandomQuote quote_bnw_first_contact_approach_random(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad);

} // namespace civ5::runtime
