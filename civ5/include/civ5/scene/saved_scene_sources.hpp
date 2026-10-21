#pragma once
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/scene/saved_fog_of_war.hpp"
#include "civ5/terrain/terrain_auxiliary.hpp"
#include "civ5/terrain/terrain_source_grid.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"

namespace civ5::scene {
// Content-resolved map inputs shared by terrain generation and FOW rendering.
// The native grid is the first loading pass; downstream feature append, height,
// material and scene-event passes still own their own stages.
struct SavedSceneSources {
    terrain::WorldLayout world;
    terrain::TerrainSourceGrid terrain_grid;
    terrain::TerrainAuxiliarySources auxiliary;
    std::vector<terrain::RiverEdge> river_edges;
    terrain::RiverNetwork river_network;
    materials::FogOfWarState fog;
    SavedFogOfWarCounts fog_counts;
};
SavedSceneSources build_saved_scene_sources(
    const saves::SavedMapTerrainSource &terrain_source,
    const saves::SavedMapTeamArtObservation &team_art,
    std::uint16_t reserved_word, const terrain::FactoryDescriptor &factory,
    const terrain::ReciprocalEstimate &reciprocal);
// Keep terrain and active-team visibility tied to the same decoded save.
SavedSceneSources build_saved_scene_sources(
    const saves::SavedSessionSnapshot &snapshot, std::uint16_t reserved_word,
    const terrain::FactoryDescriptor &factory,
    const terrain::ReciprocalEstimate &reciprocal);
} // namespace civ5::scene
