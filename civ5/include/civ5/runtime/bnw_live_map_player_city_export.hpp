#pragma once
#include "civ5/runtime/bnw_live_player_city_export.hpp"
#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"

namespace civ5::runtime {

struct BnwMapPlayerCityPartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Composes changed ID17 map plots (including SDK-validated land score) with
// ID23-28 player/city state. Active rule fingerprint and ID17/ID24 score
// agreement are checked before returning any document. Existing ID18/19 unit
// relocation and ID21 first-contact sections need separate action contracts.
BnwMapPlayerCityPartialExport make_bnw_live_map_player_city_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

struct BnwMapPlayerCityClockPartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Composes the committed BNW game turn and elapsed turn mirrors with the
// map/player/city document and both authoritative BNW RNG streams (ID13).
// Pending clock publication, incompatible streams and saved-history regression
// are rejected. The original payload remains an immutable import baseline.
BnwMapPlayerCityClockPartialExport make_bnw_live_map_player_city_clock_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

} // namespace civ5::runtime
