#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace civ5::saves { struct SavedSession; }

namespace civ5::runtime {

struct LiveSessionState;

struct BnwDiplomacyRequestsAfterRead {
    std::int64_t player=-1;
    std::int64_t next_ai_player=-1;
    bool request_active=false;
    std::int64_t active_from_player=-1;
    std::size_t request_count=0;
};
struct BnwPlayerReadTailTransients {
    // SDK MAX_MAJOR_CIVS (22); minor/barbarian slots do not run this branch.
    std::map<std::int64_t,BnwDiplomacyRequestsAfterRead> requests;
    // CvGame::getNumGameTurnActive recomputes, rather than loading a counter.
    std::vector<std::int64_t> active_player_ids;
    bool current=false;
};

// Expansion2 CvPlayer::Read tail: Init(GetID()) is called for every major
// DiplomacyRequests object and its Read is commented out. A subsequent
// changeNumGameTurnActive merely emits a gDLL debug log; its query counts
// alive && turnActive over all 64 players. This is a detached read-only
// projection, not a diplomacy request executor or an external log publisher.
BnwPlayerReadTailTransients rebuild_bnw_player_read_tail_transients(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
