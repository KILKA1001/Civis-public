#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/treasury_checkpoint.hpp"

namespace civ5::runtime {

// Proposed common CivisSave ID; shared enum/import/LiveSession registration is
// separate. v1 covers m_iGold; v2 covers balance, lifetime gross gold and both
// history arrays. v3 adds authenticated native wire counts and validates
// count/vector equality. v1/v2 decode remains compatible; legacy v2 counts
// are reconstructed from its authenticated arrays. This does not authorize DoGold.
inline constexpr std::uint32_t bnw_treasury_balance_checkpoint_section_id=30;

saves::CivisSaveSection make_bnw_treasury_balance_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);
void restore_bnw_treasury_balance_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

struct BnwTreasuryBalanceCheckpointPair {
    saves::CivisSaveSection raw,owned;
};
struct BnwTreasuryBalanceRestored {
    saves::SavedSessionSnapshot world;
    TreasuryContext treasury;
};

// The baseline treasury must be restored from the immutable source. All
// non-checkpoint owned values must remain unchanged, and the four ID10 values
// must agree with raw administration. Finish UI publication before export.
BnwTreasuryBalanceCheckpointPair make_bnw_treasury_balance_checkpoint_pair(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    const TreasuryContext &baseline_treasury,
    const SessionCheckpointIdentity &identity);
BnwTreasuryBalanceRestored restore_bnw_treasury_balance_checkpoint_pair(
    const BnwTreasuryBalanceCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const TreasuryContext &baseline_treasury,
    const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,
    // If v2/v3 appends a turn sample, pass a world whose ID16 clock overlay has
    // already been validated/restored. The section still binds to source.
    const saves::SavedSessionSnapshot *world_after_clock=nullptr);

} // namespace civ5::runtime
