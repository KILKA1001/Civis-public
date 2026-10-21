#pragma once
#include "civ5/runtime/bnw_live_player_core_export.hpp"
#include "civ5/runtime/bnw_player_strategy_checkpoint.hpp"

namespace civ5::runtime {

struct BnwPlayerStatePartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Composes full ID23 PlayerStrategy and ID24 PlayerCore values from the same
// detached owned world and immutable imported source. Both domains are
// encoded; this deliberately remains a partial diagnostic save. An earlier
// ID23/ID24 is replaced on re-save, while legacy ID22/ID20 overlap is refused.
// ID23's roster identity includes core m_bAlive, so changing it fails closed.
BnwPlayerStatePartialExport make_bnw_live_player_state_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256);

} // namespace civ5::runtime
