#pragma once
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/live_session.hpp"
#include <string_view>

namespace civ5::runtime {

struct BnwPlayerCorePartialExport {
    saves::CivisSaveDocument document;
    // Domains outside this ID24 export are deliberately reported even when a
    // standalone codec exists. The document is a diagnostic partial save.
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Exports the owned LiveSessionState::world PlayerCore against the immutable
// source payload. Replaces an earlier ID24 on re-save, rejects overlap with
// legacy ID20, and retains all other imported sections unchanged. It does not
// certify a complete BNW turn/save or alter LiveSession::readiness(). For an
// original Civ5Save, the caller must supply its independently measured file
// SHA-256; the decoded payload alone cannot prove that outer-file hash.
BnwPlayerCorePartialExport make_bnw_live_player_core_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256);

} // namespace civ5::runtime
