#pragma once
#include "civ5/runtime/bnw_live_city_export.hpp"
#include "civ5/runtime/bnw_live_player_state_export.hpp"

namespace civ5::runtime {

struct BnwPlayerCityPartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Prepares ID23-28 from one owned BNW world and one immutable source/rules
// identity. Existing copies of these IDs are replaced together on re-save.
// Both constituent exporters must finish before a document is returned.
// ID7, ID20 and ID22 remain explicit overlap blockers.
BnwPlayerCityPartialExport make_bnw_live_player_city_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256);

} // namespace civ5::runtime
