#pragma once
#include "civ5/runtime/bnw_first_contact_approach_random_quote.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"

namespace civ5::runtime {

struct BnwFirstContactRngReceipt {
    SimulationRandomState map_before{},gameplay_before{},gameplay_after{};
    std::vector<BnwApproachRandomDraw> draws;
    std::array<std::int64_t,7> local_weights{};
    std::string checkpoint_sha256;
};
struct BnwFirstContactRngStage {
    bool staged=false;
    std::string reason;
    std::optional<saves::CivisSaveSection> checkpoint;
    std::optional<BnwFirstContactRngReceipt> receipt;
    // CanDeclareWar Lua and the subsequent approach/AI/UI effects are pending.
    bool can_commit() const noexcept {return false;}
};

// Completes only CvDiplomacyAI::GetBestApproachTowardsMajorCiv's first
// JonRand loop after all selected source-order no-city gates. Draws are
// replayed on a private RNG stream and persisted as an ID13 diagnostic.
BnwFirstContactRngStage stage_bnw_first_contact_approach_rng(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad);

// Replays the receipt and restores ID13 to a private stream. No live RNG is
// advanced, including on a failed validation.
bool verify_bnw_first_contact_rng_receipt(
    const BnwFirstContactRngStage &stage,
    const SessionRandomStreams &source_streams,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
