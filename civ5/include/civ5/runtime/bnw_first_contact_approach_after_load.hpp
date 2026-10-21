#pragma once
#include "civ5/runtime/bnw_diplomacy_scratchpad_load.hpp"
#include "civ5/runtime/bnw_first_contact_approach_preflight.hpp"
#include <array>

namespace civ5::runtime {

enum class BnwFirstContactApproachAfterLoadGate : std::uint8_t {
    earlier_approach_unavailable,
    scratchpad_foreign_or_stale,
    current_bias_rule_unavailable,
    war_state_unavailable,
    conquest_strategy_unavailable,
    personality_unavailable,
    opinion_unavailable,
    opinion_rule_unavailable,
    friendship_unavailable,
    friendship_rule_unavailable,
    denounced_unavailable,
    denounced_rule_unavailable,
    friend_betrayal_unavailable,
    reciprocal_denouncement_unavailable,
    protected_minor_unavailable,
    demand_unavailable,
    promise_unavailable,
    military_threat_unavailable,
    nuke_inputs_unavailable,
    at_war_loop_unavailable,
    other_players_unavailable,
    trade_value_unavailable,
    war_projection_unavailable,
    reckless_expander_unavailable,
    dogpile_unavailable,
    proximity_weight_unavailable,
    peace_treaty_unavailable,
    duel_unavailable,
    coop_war_unavailable,
    random_factor_unimplemented,
};

struct BnwFirstContactApproachAfterLoad {
    BnwFirstContactApproachAfterLoadGate gate=
        BnwFirstContactApproachAfterLoadGate::earlier_approach_unavailable;
    std::int64_t recipient=-1,target=-1,target_team=-1,war_state=-2,opinion=-2;
    std::int64_t neutral_default=0,current_bias=0;
    std::int64_t own_nuke_units=-1,target_nuke_units=-1;
    std::int64_t target_nuke_personality=-1,target_nuke_grand_mod=0;
    std::int64_t target_nuke_likelihood=-1;
    bool world_conquest=false,friendship_accepted=false,denounced_target=false;
    bool friend_betrayal=false,reciprocal_denouncement=false;
    bool nuke_inputs_verified=false;
    std::uint32_t other_wars=0,other_valid_players=0;
    std::int64_t trade_value=0;
    // SDK approach order: war, hostile, deceptive, guarded, afraid,
    // friendly, neutral. A deterministic prefix is computed; see gate for
    // the first unverified source-order input.
    std::array<std::int64_t,7> weights{};
    std::array<std::int64_t,7> pre_opinion_weights{};
    std::array<std::int64_t,7> pre_friendship_weights{};
    std::array<std::int64_t,7> pre_denouncement_weights{};
    std::array<std::int64_t,7> pre_betrayal_weights{};
    std::array<std::int64_t,7> pre_reciprocal_weights{};
    std::array<std::int64_t,7> pre_protected_minor_weights{};
    std::array<std::int64_t,7> pre_demand_weights{};
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvDiplomacyAI.cpp:2744-2830. This is an after-CvPlayer::Read
// diagnostic, never an AI decision. At 2779 Reset's -1 scratchpad becomes
// NEUTRAL (6); at 2793 GetWarState reads serialized m_paeWarState; at 2802
// IsGoingForWorldConquest checks active GrandStrategy; at 2811 personality
// biases are read. At 2830-2874 GetMajorCivOpinion adds only the selected
// opinion branch. At 2870-2876 IsDoFAccepted reads the saved boolean row;
// if true four active defines adjust weights in SDK order. At 2879-2887
// IsDenouncedPlayer reads m_pabDenouncedPlayer[ePlayer] and applies five
// active defines. Subsequent deterministic branches are evaluated in source
// order until the first unverified input. No RNG, logging or write runs here.
// Later the seven random weights may consume JonRand at 3417, bLog=true
// invokes LogMajorCivApproachUpdate, and the caller may SetWarFace before
// mandatory SetMajorCivApproach at 10223. None is executed here.
BnwFirstContactApproachAfterLoad preflight_bnw_first_contact_approach_after_load(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad);

} // namespace civ5::runtime
