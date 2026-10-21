#pragma once
#include "coast_assets.hpp"
#include "coast_road_network.hpp"
#include "coast_map.hpp"
#include "coast_perturbation.hpp"
#include "coast_strip.hpp"
#include "scene_mask.hpp"
#include "height_owner.hpp"
#include "land_membership.hpp"
#include "river_geometry.hpp"
#include "terrain_occurrences.hpp"
#include "terrain_marsh.hpp"
#include "worker_geometry.hpp"
#include <optional>

namespace civ5::terrain {
struct TerrainWorldRandomStates {
    // Separate entering states: consumers between these original stages are
    // not yet recovered. Equal seeds do not assert a continuous loading stream.
    std::uint32_t occurrences, coast_perturbation, coast_dispatch;
};
struct TerrainRandomCursor {
    std::uint32_t entering, final;
    std::uint64_t calls;
};
struct TerrainWorldPrecision {
    X87Precision coast, gates, worker, membership;
    Vec4Path coast_transform, gate_transform;
    MatrixPath membership_transform;
    CoastPowPath coast_power;
    CoastBoundsPrecision raster_bounds;
};
struct TerrainWorldInput {
    std::int32_t map_width, map_height, mode;
    FactoryDescriptor factory;
    std::array<IntMatrix4, 4> orientations;
    std::span<const std::uint32_t> cooked_to_primary;
    // Complete expanded source grid, x-major, eight bytes per cell. Source
    // projection from gameplay plots remains an upstream producer boundary.
    std::span<const std::uint8_t> expanded_logical_cells;
    std::int32_t maximum_pass;
    TerrainWorldRandomStates random_states;
    TerrainWorldPrecision precision;
    float worker_bias;
    bool wrap_x;
    std::uint8_t outside_gate_category;
    std::span<const std::uint32_t> coast_palette;
    std::uint32_t coast_family_prefix, coast_common_prefix;
    std::uint16_t coast_dispatch_gate;
    // Explicit image-integrity result; see unmodified_coast_integrity_return.
    std::uint32_t coast_helper_return;
};
struct TerrainWorldStorage {
    const LandStyleGrid &prior_styles;
    std::span<const std::uint32_t> initial_land_masks;
    const PageStorage *previous_height;
    const PageStorage *previous_material;
    PageAllocator allocate;
};
// Select an explicit permutation of each phase's submitted jobs. Phase barriers
// are enforced; a native scheduler/interleaving is not inferred from submission.
using CoastExecutionOrder =
    std::function<std::vector<std::size_t>(unsigned, std::span<const CoastDispatchJob>)>;
struct TerrainWorldObservers {
    TerrainPassObserver occurrence_pass;
    CoastMapGraph::StageObserver coast_smoothing;
    std::function<void(const WorkerField &)> pre_coast;
    CoastWriteObserver coast_write;
    CoastJobComplete coast_job_complete;
};
struct TerrainWorldSource {
    WorldLayout layout;
    std::int32_t map_mode;
    std::vector<std::uint8_t> initial_cells;
    std::vector<std::uint32_t> masks, cooked_to_primary;
    TerrainGeneration generation;
    std::vector<TerrainDescriptor> descriptors;
    TerrainRandomCursor random;
};
struct TerrainWorldCoast {
    CoastMapGraph graph;
    CoastSegmentGrid smoothed;
    std::vector<CoastContour> contours;
    std::vector<CoastPerturbation> perturbations;
    CoastDecalPaths decal_paths;
    TerrainRoadGrid road_network;
    // Shared scene-mask state immediately after original coast initialization.
    // Subsequent non-coast writers are not inferred from this retained phase.
    SceneMask scene_mask_after_coast;
    std::vector<CoastCurveSet> curves;
    std::vector<CoastStrip> strips;
    CoastDispatchPlan dispatch;
    std::array<std::vector<std::size_t>, 2> execution_order;
    WorkerGateGeometry geometry;
    WorkerGateRouting routing;
    std::vector<std::uint8_t> gates;
    TerrainRandomCursor perturbation_random, dispatch_random;
};
struct TerrainWorldRiverGeometry {
    std::vector<RiverEdge> input_edges;
    RiverNetwork network;
    RiverPolylines centerlines;
    std::array<RiverPolylines, 2> bank_lines;
    RiverHeights sampled_heights;
    TerrainRandomCursor random;
};
struct TerrainRiverObservers {
    RiverControlObserver controls;
    RiverCurveObserver curves;
    RiverWriteObserver write;
    RiverSampleObserver sample;
    RiverPassComplete pass_complete;
};
struct TerrainWorldRivers {
    std::vector<RiverTriangle> banks, channels;
    std::vector<std::uint32_t> material_codes;
    std::vector<std::uint8_t> bank_visited;
    float level;
    X87Precision precision;
    std::array<std::uint8_t, 4> remap_from, remap_to;
    std::uint32_t river_code;
    std::optional<TerrainWorldRiverGeometry> generated;
};
struct TerrainWorldMarshes {
    TerrainMarshStamp stamp;
    std::vector<std::uint32_t> material_codes;
    TerrainMarshState state;
    std::vector<TerrainMarshApplication> applications;
    X87Precision precision;
    Vec4Path transform;
};
// Original 4A667F flushes queued feature-2 sources before the coast dispatch
// at 4A66D4 and the river path at 4A67D8. All inputs remain caller-owned.
struct TerrainWorldLoadingMarsh {
    std::span<const TerrainFeature2Source> sources;
    const TerrainMarshStamp *stamp;
    std::span<const std::uint32_t> material_codes;
    X87Precision precision;
    Vec4Path transform;
    TerrainMarshWriteObserver write;
};

// One immutable, movable generation. Every map/worker/cache/LAND extent and
// transform comes from its own WorldLayout. All source views are copied or
// consumed during build; descriptors retain shared ownership of decoded grids.
// Construction publishes nothing on failure. The final page owner is the sole
// retained height/material field; pre-coast values are observable during build.
class TerrainWorld {
  public:
    // Prepare orientations, active record mapping and maximum pass from the
    // library and input.factory. The corresponding three input fields are
    // ignored; all other scene state remains explicit as in build().
    static TerrainWorld build_from_library(const TerrainPieceLibrary &pieces, const CoastLibrary &coasts,
                               const TerrainWorldInput &input, const TerrainWorldStorage &storage,
                               const ReciprocalEstimate &reciprocal, const CoastExecutionOrder &order,
                               const TerrainWorldObservers &observers = {},
                               const TerrainWorldLoadingMarsh *loading_marsh = nullptr);
    static TerrainWorld build(const TerrainPieceLibrary &pieces, const CoastLibrary &coasts,
                               const TerrainWorldInput &input, const TerrainWorldStorage &storage,
                               const ReciprocalEstimate &reciprocal, const CoastExecutionOrder &order,
                               const TerrainWorldObservers &observers = {},
                               const TerrainWorldLoadingMarsh *loading_marsh = nullptr);
    // Advance an owned coast-complete world through both river raster passes.
    // Consumes this world only after all computation succeeds, and retains all
    // packet/settings inputs. Use with_rivers for source-edge generation.
    TerrainWorld with_river_triangles(std::span<const RiverTriangle> banks,
                                      std::span<const RiverTriangle> channels,
                                      const RiverRasterSettings &settings,
                                      const RiverWriteObserver &write_observer = {},
                                      const RiverSampleObserver &sample_observer = {},
                                      const RiverPassComplete &pass_complete = {}) &&;
    // Generate the graph, curves, widths and triangles from original lattice
    // direction bits and an explicit entering CRT state, using this world's
    // matrices and current height owner. The coordinate cache starts fresh.
    // The supplied state continues through jitter and widths; no loading-stage
    // random cursor is inferred from a saved-game seed.
    TerrainWorld with_rivers(std::span<const RiverEdge> edges, std::uint32_t entering_random,
                              const ReciprocalSqrtEstimate &estimate, const RiverRasterSettings &settings,
                              const TerrainRiverObservers &observers = {}) &&;
    // Apply a later marsh stream to this world's current fields for explicit
    // refresh controls. Initial saved loading passes loading_marsh to build()
    // so coast and river writes follow the original pending flush.
    TerrainWorld with_marshes(std::span<const TerrainFeature2Source> sources,
                              const TerrainMarshStamp &stamp, std::span<const std::uint32_t> material_codes,
                              X87Precision precision, Vec4Path transform,
                              const TerrainMarshWriteObserver &write = {}) &&;
    // Diagnostic replay only: requires completed river and loading-marsh
    // stages, then replaces the final pages. This does not reconstruct their
    // source state or permit the writers to run again.
    TerrainWorld with_observed_final_fields(std::span<const std::uint16_t> heights,
                                            std::span<const std::uint8_t> materials) &&;
    TerrainWorld(TerrainWorld &&) = default;
    TerrainWorld &operator=(TerrainWorld &&) = default;
    TerrainWorld(const TerrainWorld &) = delete;
    TerrainWorld &operator=(const TerrainWorld &) = delete;
    const TerrainWorldSource &source() const { return source_; }
    const TerrainWorldCoast &coast() const { return coast_; }
    const std::optional<TerrainWorldRivers> &rivers() const { return rivers_; }
    const std::optional<TerrainWorldMarshes> &marshes() const { return marshes_; }
    const HeightOwnerState &owner() const { return owner_; }
    const LandLayout &land() const { return land_; }
    const LandMembership &membership() const { return membership_; }
    const LandStyleResult &land_styles() const { return styles_; }
    HeightField height_field() const { return owner_.height.height_field(); }

  private:
    TerrainWorld(TerrainWorldSource source, TerrainWorldCoast coast, HeightOwnerState owner,
                  LandLayout land, LandMembership membership, LandStyleResult styles);
    TerrainWorldSource source_;
    TerrainWorldCoast coast_;
    HeightOwnerState owner_;
    LandLayout land_;
    LandMembership membership_;
    LandStyleResult styles_;
    std::optional<TerrainWorldRivers> rivers_;
    std::optional<TerrainWorldMarshes> marshes_;
};
} // namespace civ5::terrain
