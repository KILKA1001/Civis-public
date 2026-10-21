#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/runtime/treasury.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {
struct TreasuryCheckpointBudget {
    std::uint64_t players=1'000'000,history_entries=100'000'000;
    std::uint64_t bytes=1024ULL*1024*1024;
};
// Shared BNW/VP balance, lifetime gross income and both per-turn histories.
// VP-only income inputs live behind the GameCore profile boundary and are not
// part of this common section. Reopening reimports them from source payload.
saves::CivisSaveSection make_treasury_checkpoint(
    const TreasuryContext&,const ResearchContext&,
    const SessionCheckpointIdentity&,TreasuryCheckpointBudget={});
// Validate the entire section and exact imported roster/offset binding before
// replacing any live player. The original payload remains immutable.
void restore_treasury_checkpoint(
    const saves::CivisSaveSection&,TreasuryContext&,const ResearchContext&,
    const SessionCheckpointIdentity&,TreasuryCheckpointBudget={});
} // namespace civ5::runtime
