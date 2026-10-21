#pragma once
#include "civ5/runtime/bnw_live_map_player_city_export.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"

namespace civ5::runtime {

struct BnwRngPartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Appends authoritative ID13 map/gameplay RNG streams to the validated
// ID17+ID23-28 partial document. The raw progression RNG stays an immutable
// source baseline, as in LiveSession::restore_bnw. Only draw-only BNW stream
// advances whose final seed follows the source LCG are accepted; reseeds,
// profile changes and raw progression edits are blocked.
BnwRngPartialExport make_bnw_live_rng_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

struct BnwTurnBasePartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// One document for committed BNW clock, supporting classes, religion cache,
// and paired raw/owned treasury ID30/10 plus ID34 expense caches when
// changed. Unpublished gold effects,
// orphan companions, unsupported history and previous-save regression reject.
// draw-only map/gameplay RNG and validated map/player/city trees. An unchanged RNG is valid when no draw was
// consumed; a prior ID13 must be an ancestor of the exported streams.
BnwTurnBasePartialExport make_bnw_live_turn_base_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

} // namespace civ5::runtime
