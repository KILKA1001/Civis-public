#pragma once
#include "civ5/runtime/research_context.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <map>
#include <vector>

namespace civ5::runtime {
// VP 5.4.2 GameCore operation data needed by the first UncoverIntrigue branch.
// Keep it separate from common player and espionage state: AI turns may later
// change these live operations independently of the saved observations.
struct VpAttackOperation {
    std::int64_t id=0, type=0, owner=0, enemy=0, state=0;
    std::int64_t target_x=0, target_y=0;
    std::size_t source_index=0;
};
struct VpAttackOperationsProfileState {
    std::map<std::int64_t,std::vector<VpAttackOperation>> players;
};
VpAttackOperationsProfileState &ensure_vp_attack_operations_profile(ResearchContext &world);
const VpAttackOperationsProfileState *find_vp_attack_operations_profile(const ResearchContext &world);
void restore_vp_attack_operations_profile(const saves::SavedSessionSnapshot &snapshot,
                                          ResearchContext &world);
// CvPlayer::getFirstAIOperationOfType as used by VP UncoverIntrigue: try
// combined, then naval, then land; each type scan retains source order.
// A null result means no matching, non-aborted attack on this target.
const VpAttackOperation *vp_first_intrigue_attack_operation(
    const ResearchContext &world,std::int64_t spy_owner,std::int64_t target_player);
} // namespace civ5::runtime
