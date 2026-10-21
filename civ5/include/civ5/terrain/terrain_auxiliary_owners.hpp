#pragma once
#include "terrain_auxiliary.hpp"
#include <cstdint>
#include <span>

namespace civ5::terrain {
struct HeightOwnerParameters;
// Fixed native strides, independently owned storage. Caller supplies every
// prior byte: initialization of unrelated fields is not an assumed zero fill.
template <std::size_t Stride, std::size_t Bytes, bool HasFallback = true> struct TerrainAuxiliaryGrid {
    static constexpr std::size_t stride = Stride, record_bytes = Bytes, capacity = Stride * Stride;
    WorldGrid coordinates;
    bool wrap_x;
    std::vector<std::array<std::uint8_t, Bytes>> cells;
    std::array<std::uint8_t, HasFallback ? Bytes : 0> fallback;
};
using TerrainIceGrid = TerrainAuxiliaryGrid<255, 1, false>;
using TerrainAuxiliaryTwoByteGrid = TerrainAuxiliaryGrid<255, 2>;
using TerrainAdjacencyGrid = TerrainAuxiliaryGrid<255, 12>;
struct TerrainAuxiliaryInitializedTables {
    TerrainAuxiliaryTwoByteGrid two_byte;
    TerrainAdjacencyGrid adjacency;
};
struct TerrainAuxiliaryOwnerState {
    bool relief_enabled, water_enabled, flood_state_flag; // original flood owner +2B1A0
    TerrainAuxiliaryGrid<255, 8> relief;
    TerrainAdjacencyGrid adjacency;
    TerrainAuxiliaryGrid<256, 4, false> water;
    TerrainIceGrid ice;
    TerrainAuxiliaryGrid<200, 4, false> flood;
};
// Original 6DC370 initialization on the verified global factory path. Rebuilds
// the header and clears only active ice cells. Retains wrap flag and all other
// storage, including inactive rows/columns. Nonzero global
// factories remain unsupported, as in the existing world initializer.
TerrainIceGrid initialize_terrain_ice_grid(const TerrainIceGrid &prior,
    std::int32_t width, std::int32_t height, const FactoryDescriptor &factory);
struct TerrainIceInput {
    TerrainIceGrid grid;
    std::uint32_t ice_cells = 0, exposed_sides = 0;
};
// Original 87B290: mark occupied ice bytes with bit 6 and absent neighbors
// with bits 0..5, in C7FB08 cube-direction order. This is the input topology
// for ice mesh construction, not the final 20-byte minimap region table.
TerrainIceInput build_terrain_ice_input(const TerrainIceGrid &prior);
struct TerrainIceGroup {
    std::uint32_t first_vertex = 0, vertex_count = 0;
};
struct TerrainIceGrouping {
    TerrainIceGrid grid;
    std::vector<TerrainIceGroup> groups;
    std::vector<LogicalHex> vertices;
    std::vector<std::uint8_t> boundary_flags;
};
// Original 87B4E0: scan marked cells, emit depth-first cube vertices in
// C7FB08 direction order, and clear bit 6 on discovery. These connected
// groups precede the later ice-region splitting and mesh generation stages.
TerrainIceGrouping group_terrain_ice_input(const TerrainIceInput &input);
struct TerrainIceContourGroup {
    std::uint32_t first_segment = 0, first_vertex = 0;
    std::uint32_t segment_count = 0, vertex_count = 0;
};
struct TerrainIceContourSegment {
    std::uint32_t first_edge = 0, edge_count = 0;
};
struct TerrainIceContourEdge {
    LogicalHex cube{};
    std::uint8_t side = 0;
};
struct TerrainIceContours {
    TerrainIceGrid grid;
    std::vector<TerrainIceContourGroup> groups;
    std::vector<LogicalHex> vertices;
    std::vector<std::uint8_t> boundary_flags;
    std::vector<TerrainIceContourSegment> segments;
    std::vector<TerrainIceContourEdge> edges;
};
// Original 87BBA0 / 87B9C0 world-mode-1 branch for horizontally wrapped maps.
// Resolves exposed sides into ordered contours and translates closed groups
// across the wrap seam. Other world modes are not represented by this entry.
TerrainIceContours build_terrain_ice_contours_mode1(const TerrainIceGrouping &input);
struct TerrainIceAllocation {
    std::uint32_t vertices = 0, in_bounds_edges = 0;
};
// Original 87AAC0 prepass: count contour edges whose adjacent plot is in
// bounds, then reserve lattice vertices for those edges and one for each
// edge outside the map. The caller supplies the loaded ice-settings value.
TerrainIceAllocation count_terrain_ice_allocation(const TerrainIceContours &contours,
    std::uint32_t lattice_vertices_per_hex);
struct TerrainIceMeshSegment {
    std::uint32_t first_vertex = 0, vertex_count = 0;
};
struct TerrainIceMeshFlagPlan {
    std::vector<TerrainIceMeshSegment> segments;
    std::vector<std::uint8_t> flags;
};
// Original 87AC40's contour rotation and 0/1 vertex flag stream. Geometry
// is built by the separate control-point, spline and exterior paths below.
TerrainIceMeshFlagPlan plan_terrain_ice_mesh_flags(const TerrainIceContours &contours,
    std::uint32_t lattice_vertices_per_hex);
struct TerrainIceProjection {
    std::array<float, 16> world_matrix{};
    float scale_x = 1, scale_y = 1, center_x = 0, center_y = 0;
};
// The original ice builder reads global D80E80 (world transform 16..31) and
// the current height owner's +A8/+AC scales and +B4/+B8 centers.
TerrainIceProjection terrain_ice_projection(const WorldLayout &world,
                                            const HeightOwnerParameters &owner);
// Original 87A580 visits the contour owner's connected-group vertex spans,
// projects each cube position through 6A8BC0, and fills the separate XY
// stream used after contour/mesh construction.
std::vector<std::array<float, 2>> project_terrain_ice_group_vertices(
    const TerrainIceContours &contours, const TerrainIceProjection &projection);
struct TerrainIceCurveParameters {
    float radius = 0;
    std::uint32_t lattice_vertices_per_hex = 0;
    std::array<float, 24> controls{};
    std::array<float, 12> corners{};
};
static_assert(sizeof(TerrainIceCurveParameters) == 38 * 4);
// Original 449740 builds the six ice-corner offsets and the 24 spline control
// coordinates from a world grid origin and the terrain projection owner.
TerrainIceCurveParameters build_terrain_ice_curve_parameters(const WorldGrid &world,
    const TerrainIceProjection &projection, std::uint32_t lattice_vertices_per_hex);
struct TerrainIceRandomState {
    std::uint32_t value = 0, draws = 0, initial_value = 0, modulus = 0;
    double inverse_modulus = 0;
};
static_assert(sizeof(TerrainIceRandomState) == 24);
// Normal original settings-load path: 618CA0 initializes the settings seed
// to zero and 593A20 copies it into the contour-mesh RNG before 87AC40.
TerrainIceRandomState initial_terrain_ice_random_state();
// Original 62CA72: one region-pruning LCG draw, normalized to Float32 by
// x87 before the SSE kill-rate comparison. This does not mutate region data.
float draw_terrain_ice_region_prune_random(TerrainIceRandomState &random,
    std::uint32_t x87_precision = 53);
bool should_prune_terrain_ice_region_record(TerrainIceRandomState &random,
    std::uint32_t vertex_count, bool boundary, float boundary_kill_rate,
    float interior_kill_rate,
    std::uint32_t x87_precision = 53);
struct TerrainIceIntermediateRegionCounts {
    std::uint32_t active_source24 = 0, records20 = 0, links12 = 0;
    // Inactive native map slots are untouched; this plan marks them UINT32_MAX.
    std::vector<std::uint32_t> source24_to_active;
};
// Original 42CFB6..42D047 prepass for 24-byte source records and 36-byte
// weighted edge records, before allocation and output-record construction.
TerrainIceIntermediateRegionCounts count_terrain_ice_intermediate_regions(
    std::span<const std::uint8_t> source24_flags,
    std::span<const std::array<float, 2>> source36_weights);
struct TerrainIceSourceAdjacency {
    // Original 0x873210: contiguous point slices and reverse edge insertion.
    std::vector<std::array<std::uint32_t, 2>> ranges;
    std::vector<std::uint32_t> edge_indices;
};
TerrainIceSourceAdjacency build_terrain_ice_source_adjacency(
    std::uint32_t point_count,
    std::span<const std::array<std::uint32_t, 2>> edges);
struct TerrainIceSourceWeightPoint {
    std::array<float, 2> xy{};
    std::uint8_t active = 0;
    float potential = 0;
};
struct TerrainIceSourceWeightEdge {
    std::uint32_t first = 0, second = 0;
    std::uint8_t boundary = 0;
    std::array<float, 2> weights{};
    std::array<float, 2> propagation{};
    std::array<float, 2> pending{};
};
struct TerrainIceSourceWeightSettings {
    float crack_cost = 0, initial_potential = 0;
    float min_clip = 0, max_clip = 0, source_scale = 0;
};
// Original 732970 and 8760B0/875EE0 source-graph propagation before 42CF50.
void weight_terrain_ice_source_graph(
    std::span<TerrainIceSourceWeightPoint> points,
    std::span<TerrainIceSourceWeightEdge> edges,
    const TerrainIceSourceAdjacency &adjacency,
    TerrainIceSourceWeightSettings settings);
struct TerrainIceIntermediateSourcePoint {
    std::array<float, 2> xy{};
    std::uint8_t active = 0;
};
struct TerrainIceIntermediateSourceEdge {
    std::uint32_t first = 0, second = 0;
    std::uint8_t boundary = 0;
    std::array<float, 2> weights{};
};
struct TerrainIceIntermediatePoint {
    std::array<float, 2> xy{};
    std::uint32_t adjacency_first = 0;
    std::uint32_t link_count = 0;
};
struct TerrainIceIntermediateLink {
    std::uint32_t first = 0, second = 0;
    std::uint8_t boundary = 0;
};
struct TerrainIceIntermediateFill {
    std::vector<TerrainIceIntermediatePoint> points;
    std::vector<TerrainIceIntermediateLink> links;
    std::vector<std::uint32_t> source24_to_active;
    std::vector<std::uint32_t> adjacency_link_indices;
    std::vector<std::uint8_t> closed_walk_flags;
};
// Original 42D04F..42D2C6 fill: active source XY, interpolated edge XY and
// endpoint/flag links. Original record padding and adjacency pointers are
// deliberately absent until later native stages produce them.
TerrainIceIntermediateFill build_terrain_ice_intermediate_regions(
    std::span<const TerrainIceIntermediateSourcePoint> points,
    std::span<const TerrainIceIntermediateSourceEdge> edges);
// Original 875AC0: count incident links and place each link index into a
// contiguous per-point range in reverse source-link order.
void build_terrain_ice_intermediate_adjacency(TerrainIceIntermediateFill &fill);
// Original 875B50: arrange each point's incident links by their neighboring
// XY direction, starting in the upper half-plane and continuing clockwise.
void sort_terrain_ice_intermediate_adjacency(TerrainIceIntermediateFill &fill);
// Original 712CF0 loop: follow sorted, boundary-marked adjacency around each
// point and mark incident slots whose walk returns to its starting slot.
void mark_terrain_ice_closed_walks(TerrainIceIntermediateFill &fill);
struct TerrainIceIntermediateRegion {
    std::vector<std::array<std::uint32_t, 2>> directed_edges;
};
// Original 5737D0 traversal, before 508E00/785690 shape refinement. Consumes
// the unmarked incidence slots and records their ordered directed edges.
std::vector<TerrainIceIntermediateRegion> trace_terrain_ice_intermediate_regions(
    TerrainIceIntermediateFill &fill);
struct TerrainIceIntermediateRegionRecord {
    std::uint32_t first_vertex = 0, first_index = 0, vertex_count = 0;
    std::uint8_t boundary = 0;
};
struct TerrainIceIntermediateRegionGeometry {
    std::vector<TerrainIceIntermediateRegionRecord> regions;
    std::vector<std::array<float, 2>> vertices;
    std::vector<std::array<std::uint32_t, 3>> indices;
    // Active graph-edge identifiers in original 876CA0 insertion order.
    std::vector<std::vector<std::uint32_t>> incidence;
};
// Original 5737D0 with 508E00, 785690 and 628CA0: append oriented region
// edges and pull each eligible corner using the authored ice edge amount.
TerrainIceIntermediateRegionGeometry build_terrain_ice_intermediate_region_geometry(
    const TerrainIceIntermediateFill &fill,
    std::span<const TerrainIceIntermediateRegion> regions,
    float edge_pull, std::uint32_t x87_precision = 53);
// Original 5306D0 before pruning: the component box spans every smoothed
// region vertex, including those belonging to regions later removed.
std::array<float, 4> terrain_ice_region_component_box(
    const TerrainIceIntermediateRegionGeometry &geometry);
// Original 62CAFA..62CCDB keep branch for an eligible (at most 16 vertices)
// region. The centroid follows the first vertex of each region index record.
void shrink_terrain_ice_intermediate_region(
    TerrainIceIntermediateRegionGeometry &geometry, std::size_t region_index,
    float shrink_amount, std::uint32_t x87_precision = 53);
// Original 496640 and caller 62CAED for a region selected by the prune draw.
// The graph keeps its allocated index slots; deleted triplets become UINT32_MAX
// and each endpoint's ordered active incidence list is compacted.
void prune_terrain_ice_intermediate_region(
    TerrainIceIntermediateRegionGeometry &geometry, std::size_t region_index);
// Original 62CA3F..62CCF8 pass: process region records in order, drawing one
// prune value for each region of at most 16 vertices. Larger regions bypass
// both mutation paths and do not advance the independent settings RNG.
void process_terrain_ice_intermediate_regions(
    TerrainIceIntermediateRegionGeometry &geometry, TerrainIceRandomState &random,
    float boundary_kill_rate, float interior_kill_rate, float shrink_amount,
    std::uint32_t x87_precision = 53);
// Original 546DA0, 688870 and 415F90: remove marked graph edges by swapping
// with the last active slot, compact surviving points in source order, remap
// endpoints and rebuild sorted adjacency. Region records retain source ranges.
TerrainIceIntermediateRegionGeometry finalize_terrain_ice_intermediate_regions(
    const TerrainIceIntermediateRegionGeometry &geometry);
// Original 4D2DF0 after crossing repair: resize the face vector, preserve
// boundary flags, then recount every edge in source order. The final edge
// supplies each face's first_vertex and first_index fields.
void recount_terrain_ice_region_faces(
    TerrainIceIntermediateRegionGeometry &geometry, std::uint32_t face_count);
// Candidate segment pairs for original 4BEFE0 crossing repair. A proper
// interior intersection appears once, with ascending edge identifiers.
std::vector<std::array<std::uint32_t, 2>> find_terrain_ice_edge_intersections(
    const TerrainIceIntermediateRegionGeometry &geometry);
struct TerrainIceDirectedIntersection {
    std::uint32_t edge = 0, other = 0;
    float position = 0.0f;
};
// Original 496DE0 Float32 intersection parameter and 59D030 ordering:
// two directed records per proper crossing, sorted by edge then position.
std::vector<TerrainIceDirectedIntersection> build_terrain_ice_directed_intersections(
    const TerrainIceIntermediateRegionGeometry &geometry);
struct TerrainIceCrossingVertex {
    std::uint32_t first_edge = 0, second_edge = 0;
    std::array<float, 2> xy{};
};
// Original 4BF1D6..4BF28F: emit one Float32-interpolated point for each
// sorted directed event whose first edge identifier is smaller.
std::vector<TerrainIceCrossingVertex> build_terrain_ice_crossing_vertices(
    const TerrainIceIntermediateRegionGeometry &geometry,
    std::span<const TerrainIceDirectedIntersection> events);
// Original 4BF2F4..4BF3D0: append ordered edge chains through each crossing
// point and mark the replaced source edges deleted. If any edges are split,
// incidence is cleared and must be rebuilt by the later graph finalizer.
TerrainIceIntermediateRegionGeometry split_terrain_ice_intersecting_edges(
    const TerrainIceIntermediateRegionGeometry &geometry);
// Original 8764D0 after crossing-edge compaction: trace the newly formed
// directed cycles from each appended vertex in angular incidence order,
// assigning one new face ID to each cycle. The input must be finalized.
TerrainIceIntermediateRegionGeometry rebuild_terrain_ice_crossing_faces(
    const TerrainIceIntermediateRegionGeometry &geometry);
// Original 876420: repeatedly remove the smallest face still paired with
// another live face at an inserted crossing vertex. Deleted edges retain
// their slots until the subsequent graph finalizer.
void prune_terrain_ice_crossing_faces(
    TerrainIceIntermediateRegionGeometry &geometry,
    std::uint32_t first_new_vertex, std::uint32_t first_new_face);
// Compose crossing repair, edge/vertex compaction, adjacency rebuilding,
// and face recount for a finalized ice graph.
TerrainIceIntermediateRegionGeometry postprocess_terrain_ice_crossings(
    const TerrainIceIntermediateRegionGeometry &geometry);
struct TerrainIceSmallMeshSettings {
    float scale = 1.0f;
    float height_min = 0.0f, height_max = 0.0f;
    float depth_min = 0.0f, depth_max = 0.0f;
    float skirt_slope = 0.0f;
    std::uint32_t max_polygon_vertices = 16;
};
struct TerrainIceRawMesh {
    std::vector<std::array<std::uint8_t, 16>> vertices;
    std::vector<std::uint32_t> indices;
    std::uint32_t local_random_end = 0;
};
struct TerrainIceTessellationPoint {
    std::array<float, 2> position{};
    std::uint32_t halfedge_count = 0, first_halfedge = 0;
};
struct TerrainIceTessellationEdge {
    std::uint32_t first_point = 0, second_point = 0;
    std::uint8_t active = 0;
};
struct TerrainIceTessellationGraph {
    std::vector<TerrainIceTessellationPoint> points;
    std::vector<TerrainIceTessellationEdge> edges;
    std::vector<std::uint32_t> halfedge_edges;
};
struct TerrainIceLargeSourceGraph {
    std::vector<std::array<float, 2>> points;
    // The third word retains the source region identifier.
    std::vector<std::array<std::uint32_t, 3>> edges;
};
// The small-mesh pass removes contours of at most max_polygon_vertices.
// Original surviving points compact stably; removed edges swap with the last.
TerrainIceLargeSourceGraph select_terrain_ice_large_source_graph(
    const TerrainIceIntermediateRegionGeometry &graph,
    std::uint32_t max_polygon_vertices);
struct TerrainIceContourIntersection {
    float position = 0.0f;
    std::uint32_t point = 0, state = 0;
    std::uint32_t source_edge = UINT32_MAX;
    float position_on_edge = 0.0f;
};
struct TerrainIceRefinementFrontierPoint;
// Retail 0x79D8B0/0x8778E0 contour events, evaluated directly from the
// selected source edges instead of the original spatial-search structure.
// axis 0 scans a vertical X line; axis 1 scans a horizontal Y line.
std::vector<TerrainIceContourIntersection> produce_terrain_ice_contour_intersections(
    const TerrainIceLargeSourceGraph &source,
    std::uint32_t axis, float coordinate,
    std::uint32_t first_new_point);
struct TerrainIceVerticalEvent {
    float y = 0.0f;
    std::uint32_t point = 0, state = 0;
};
struct TerrainIceVerticalSweepLine {
    float x = 0.0f;
    std::vector<TerrainIceVerticalEvent> events;
};
struct TerrainIceVerticalInsertionGraph {
    std::vector<std::array<float, 2>> points;
    std::vector<TerrainIceTessellationEdge> edges;
};
// Retail 0x877ED0 after 0x8778E0 has supplied the ordered contour
// intersections: append those point records, then the regular vertical
// lattice points and edges inside each entering/exiting pair.
TerrainIceVerticalInsertionGraph insert_terrain_ice_vertical_lattice(
    const TerrainIceLargeSourceGraph &source,
    std::span<const TerrainIceVerticalSweepLine> lines,
    float spacing, const std::array<float, 4> *component_box = nullptr);
// Derive each contour line from the selected graph as the lattice advances.
TerrainIceVerticalInsertionGraph insert_terrain_ice_vertical_lattice(
    const TerrainIceLargeSourceGraph &source, float spacing,
    std::vector<TerrainIceRefinementFrontierPoint> *frontier = nullptr,
    const std::array<float, 4> *component_box = nullptr);
struct TerrainIceHorizontalEvent {
    float x = 0.0f;
    std::uint32_t point = 0, state = 0;
};
struct TerrainIceHorizontalSweepLine {
    float y = 0.0f;
    std::vector<TerrainIceHorizontalEvent> events;
};
// Retail 0x878920 samples each horizontal lattice row through its ordered
// contour intervals, connecting the existing vertical-grid points between
// each entering/exiting intersection pair.
TerrainIceVerticalInsertionGraph sample_terrain_ice_horizontal_lattice(
    TerrainIceVerticalInsertionGraph graph,
    std::span<const TerrainIceHorizontalSweepLine> lines);
TerrainIceVerticalInsertionGraph sample_terrain_ice_horizontal_lattice(
    TerrainIceVerticalInsertionGraph graph,
    const TerrainIceLargeSourceGraph &source, float spacing,
    std::vector<TerrainIceRefinementFrontierPoint> *frontier = nullptr,
    const std::array<float, 4> *component_box = nullptr);
// Retail 0x876EF0 and 0x8773A0: build incident edge spans and order each
// point's neighbors clockwise from the positive-Y half-plane.
TerrainIceTessellationGraph assemble_terrain_ice_refined_graph(
    std::span<const std::array<float, 2>> point_positions,
    std::span<const TerrainIceTessellationEdge> edges);
// The 0x878730 boundary pass connects every point whose sampled-grid
// incidence has fewer than four edges, once incoming and once outgoing.
std::vector<std::uint8_t> select_terrain_ice_refinement_boundary_points(
    std::uint32_t point_count,
    std::span<const TerrainIceTessellationEdge> sampled_edges);
struct TerrainIceRefinementFrontierPoint {
    std::uint32_t point = 0, source_edge = 0;
    float position_on_edge = 0.0f;
};
// Retail 0x878730: sort the pending boundary points by source-edge index and
// position, then replace each source edge with a directed chain through them.
std::vector<TerrainIceTessellationEdge> stitch_terrain_ice_refinement_boundary(
    std::uint32_t point_count,
    std::span<const std::array<std::uint32_t, 2>> source_edges,
    std::span<const TerrainIceRefinementFrontierPoint> frontier);
// Build the complete large-ice refinement graph from the selected contour.
TerrainIceTessellationGraph construct_terrain_ice_refined_graph(
    const TerrainIceLargeSourceGraph &source, float spacing,
    const std::array<float, 4> *component_box = nullptr);
// Original 427BC0: follow each active directed edge through the next
// incident edge in adjacency order, marking only closed boundary walks.
std::vector<std::uint8_t> select_terrain_ice_exterior_halfedges(
    const TerrainIceTessellationGraph &graph);
struct TerrainIceTessellationPreparation {
    std::vector<std::uint8_t> flags;
    std::uint32_t max_polygon_vertices = 0;
    std::uint32_t polygon_indices = 0;
};
// Original 0x48D2C0: visit the remaining directed sides, temporarily mark
// them 2, and count the polygon indices reserved by the following mesh pass.
TerrainIceTessellationPreparation prepare_terrain_ice_tessellation(
    const TerrainIceTessellationGraph &graph);
// Retail 0x62A150/0x75B400/0x6BA100 and 0x879C50: triangulate each
// unselected face, then append both skirt triangles per active boundary edge.
std::vector<std::uint32_t> build_terrain_ice_large_indices(
    const TerrainIceTessellationGraph &graph, std::uint32_t first_vertex);
struct TerrainIceLargeTopSettings {
    float scale = 1.0f;
    float displacement_frequency = 0.0f;
    float displacement_min = 0.0f, displacement_max = 0.0f;
    float height = 0.0f, height_ceiling = 0.0f;
};
// Retail 0x878EE0 and 0x879D6F–0x879F0E with DisplaceExponent=1 and
// zero large-edge height range. Pixels include the following DDS mip bytes.
std::vector<std::array<float, 4>> build_terrain_ice_large_top_vertices(
    const TerrainIceTessellationGraph &graph,
    std::span<const std::uint8_t> displacement_pixels,
    std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
    const TerrainIceLargeTopSettings &settings);
// Retail 0x879F24–0x87A106: one extruded vertex per closed exterior point,
// ordered by refined point index. Top vertices retain graph XY separately.
std::vector<std::array<float, 4>> build_terrain_ice_large_border_vertices(
    const TerrainIceTessellationGraph &graph, float scale,
    float skirt_slope, float depth);
// Retail 0x879680 and 0x6BA100: walk each directed contour of 3–16 vertices,
// triangulate the cap, and extrude the two-triangle skirt.
TerrainIceRawMesh build_terrain_ice_small_mesh(
    const TerrainIceIntermediateRegionGeometry &graph,
    const TerrainIceSmallMeshSettings &settings, std::uint32_t random_seed);
struct TerrainIceControlPoint {
    std::array<float, 4> base{}, displaced{};
};
// Original 87A650: one four-float control-point pair from three neighboring
// contour edges. Advances the original ice RNG by exactly four draws.
TerrainIceControlPoint build_terrain_ice_control_point(
    const TerrainIceContourEdge &previous, const TerrainIceContourEdge &center,
    const TerrainIceContourEdge &following, const TerrainIceCurveParameters &curve,
    const TerrainIceProjection &projection, TerrainIceRandomState &random,
    std::uint32_t x87_precision = 53);
// Original 4435F0's three-point interpolation after its caller has selected
// the triplet and local parameter. Each coordinate is evaluated in SSE float.
struct TerrainIceSplineSelection {
    std::uint32_t first_point = 0;
    float local_t = 0;
};
TerrainIceSplineSelection select_terrain_ice_spline_triplet(
    std::uint32_t point_count, float parameter, std::int32_t boundary = 0);
std::array<float, 2> interpolate_terrain_ice_spline_triplet(
    const std::array<float, 6> &points, float local_t);
struct TerrainIceExteriorVertex {
    std::uint32_t index = 0;
    std::uint32_t source_edge = 0;
    std::array<float, 2> position{};
};
// Original 87AC40's out-of-bounds one-vertex path. Indices refer to the
// complete mesh stream, leaving interior spline positions for its other path.
std::vector<TerrainIceExteriorVertex> build_terrain_ice_exterior_vertices(
    const TerrainIceContours &contours, const TerrainIceCurveParameters &curve,
    const TerrainIceProjection &projection);
struct TerrainIceMeshGeometry {
    std::vector<TerrainIceMeshSegment> segments;
    std::vector<std::array<float, 2>> vertices;
    std::vector<std::uint8_t> flags;
};
// Original 87AC40 mode-1 mesh: randomized control points, spline runs,
// exterior corners and the final ordered XY/flag streams.
TerrainIceMeshGeometry build_terrain_ice_mesh_geometry(
    const TerrainIceContours &contours, const TerrainIceCurveParameters &curve,
    const TerrainIceProjection &projection, TerrainIceRandomState &random,
    std::uint32_t x87_precision = 53);
// Original 87A4C0 bounds over the complete ordered mesh XY stream. The
// extremal values also cover the native empty-mesh return path.
std::array<float, 4> bound_terrain_ice_mesh_geometry(
    std::span<const std::array<float, 2>> vertices);
// Retail 0x878DE0: pack an ice vertex against the component-wide bounds used
// by the kind-9 shader, preserving its three source bytes and quantized Z.
std::array<std::uint8_t, 8> pack_terrain_ice_vertex(
    const std::array<float, 4> &component_box,
    const std::array<std::uint8_t, 16> &source,
    float height_scale, float height_offset);
// Retail 0x8791F0 / 0x8790A0: compute angle-weighted triangle normals and
// encode the resulting three vertex-normal bytes in the prepared mesh.
void prepare_terrain_ice_vertex_normals(
    std::span<std::array<std::uint8_t, 16>> vertices,
    std::span<const std::uint32_t> indices);
struct TerrainIcePackedRegion {
    std::array<float, 4> bounds{};
    std::vector<std::array<std::uint8_t, 8>> vertices;
    std::vector<std::uint16_t> indices;
};
// Retail 0x7240B0: recursively divide prepared ice triangles until each
// region has fewer than 65,535 distinct source vertices, then pack vertices
// in first-index-use order and emit local 16-bit indices.
std::vector<TerrainIcePackedRegion> partition_terrain_ice_mesh(
    const std::array<float, 4> &root_box,
    const std::array<float, 4> &component_box,
    std::span<const std::array<std::uint8_t, 16>> vertices,
    std::span<const std::uint32_t> indices,
    float height_scale, float height_offset);
// Prepare the combined mesh's normal bytes in place and partition it into
// the kind-9 shader's packed vertex and local-index regions.
std::vector<TerrainIcePackedRegion> build_terrain_ice_gpu_regions(
    const std::array<float, 4> &component_box,
    std::span<std::array<std::uint8_t, 16>> combined_vertices,
    std::span<const std::uint32_t> indices,
    float height_scale, float height_offset);
struct TerrainIceCompleteMesh {
    std::array<float, 4> component_box{};
    TerrainIceRawMesh combined;
};
// Join the small ice contours and refined large-ice surface into the original
// 16-byte vertex and global-index streams before normal preparation. The
// component box comes from the pre-prune owner, not the selected large graph.
TerrainIceCompleteMesh build_terrain_ice_complete_mesh(
    const TerrainIceIntermediateRegionGeometry &geometry,
    const std::array<float, 4> &component_box,
    const TerrainIceSmallMeshSettings &small_settings,
    const TerrainIceLargeTopSettings &large_settings,
    std::span<const std::uint8_t> displacement_pixels,
    std::uint32_t displacement_width, std::uint32_t displacement_height,
    std::uint32_t displacement_pitch,
    float large_skirt_slope, float large_depth,
    std::uint32_t random_seed);
// Complete integer mutation paths 4D57C0 / 5D3730 / 6F35A0 / 76D2F0.
// Relief runs before second-pass ice/flood records. Feature-2 marsh material
// work is a separate consumer; its records are not consumed by this function.
// Owner headers, enable flags and prior storage are explicit. No inferred
// owner initialization, random state, GPU resources or model integration.
// Returns new owned state; invalid physical addresses never mutate prior.
TerrainAuxiliaryOwnerState apply_terrain_auxiliary_flags(const TerrainAuxiliaryOwnerState &prior,
    const TerrainAuxiliarySources &sources);
// Original 5B2000, called by the relief-owner reset path 558240. Computes
// corner result bytes 4 and 5 from the retained bytes 6..8 and neighboring
// records. The caller supplies the already populated owner table and header.
TerrainAdjacencyGrid finalize_terrain_auxiliary_adjacency(const TerrainAdjacencyGrid &prior);
// Original 5490F0. The first and last dwords of each adjacency record read
// caller-stack scratch bytes; expose all 12 prior scratch bytes rather than
// assuming that the original loader supplied zeroed stack memory.
TerrainAuxiliaryInitializedTables initialize_terrain_auxiliary_tables(
    const TerrainAuxiliaryTwoByteGrid &two_byte, const TerrainAdjacencyGrid &adjacency,
    std::array<std::uint8_t, 12> scratch);
} // namespace civ5::terrain
