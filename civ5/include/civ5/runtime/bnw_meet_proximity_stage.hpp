#pragma once
#include "civ5/runtime/bnw_player_proximity_checkpoint.hpp"
#include "civ5/runtime/bnw_sight_meet_effect_plan.hpp"

namespace civ5::runtime {

struct BnwMeetProximityReceipt {
    std::size_t event_index=0;
    std::int64_t player=-1,other=-1,old_value=-1,new_value=-1;
    std::string checkpoint_sha256;
};
struct BnwMeetProximityStage {
    bool staged=false;
    std::string reason;
    std::optional<saves::CivisSaveSection> checkpoint;
    std::vector<BnwMeetProximityReceipt> receipts;
    // DoFirstContact AI immediately follows these two calls in the SDK.
    bool can_commit() const noexcept {return false;}
};

// Bind two already calculated detached CvPlayer::DoUpdateProximityToPlayer
// calls to the first source-order meet events and ID20's atomic player-core
// checkpoint. Replays ID20 on a private copy before issuing receipts. The
// original world and source are never changed.
BnwMeetProximityStage stage_bnw_meet_proximity(
    const BnwSightMeetEffectPlan &plan,
    const BnwSightFirstContactResult &first_contact,
    const saves::SavedSessionSnapshot &source_world,
    const SessionCheckpointIdentity &identity);

// Rechecks the recorded event indices, section digest and private restore.
// Returns false for forged/stale receipts without publishing any state.
bool verify_bnw_meet_proximity_receipts(
    const BnwMeetProximityStage &stage,
    const BnwSightMeetEffectPlan &plan,
    const BnwSightFirstContactResult &first_contact,
    const saves::SavedSessionSnapshot &source_world,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
