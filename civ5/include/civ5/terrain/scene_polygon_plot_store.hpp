#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace civ5::terrain {
using ScenePolygonPlotBytes=std::array<std::uint8_t,144>;

// Source event consumed by retail 5415F0. Offsets refer to its input record,
// not the retained 144-byte plot. The eight selector values index the scene's
// material ranges; material resolution itself is supplied by the scene owner.
struct ScenePolygonPlotSourceEvent {
    std::uint32_t identity=0;        // +04, compared with scene owner +4A8
    std::uint32_t gate=0;            // +08, compared with scene owner +4AC
    std::uint8_t style=0;            // +0C, retained plot +08
    std::uint32_t first_flags=0;     // +10, low nibble -> plot flags 4..7
    std::uint32_t second_flags=0;    // +14, low nibble -> plot flags 0..3
    std::int16_t x=0,y=0;           // +18/+1A, retained plot +10/+12
    std::uint8_t selector=0;         // +20, retained plot +07
    std::uint8_t variant=0;          // +24, retained plot +04
};
using ScenePolygonMaterialResolve=std::function<std::uint32_t(
    std::uint32_t material_row,std::uint8_t selector,std::uint8_t variant)>;
using ScenePolygonRandomDraw=std::function<std::uint32_t()>;
// Registry fields loaded from ArtDefine_Landmarks and read by retail 628920
// from each 0x1800-byte source record.
// The resulting material ID is the record's zero-based registry index.
struct ScenePolygonMaterialSource {
    std::uint32_t improvement=0; // +1754, art-definition ID
    std::uint32_t resource=0;    // +1758, 0xFFFFFFFF for RESOURCE_NONE
    std::uint32_t era=0;         // +175C, Ancient..Modern = 1..4
    std::uint32_t state=0;       // +1760, Any=1, UnderConstruction=2, Constructed=4, Pillaged=8
};
// Owner words +4B0..+4FC: three five-word material rows followed by five
// variation counters. Preserve the supplied initial words outside writes.
void populate_scene_polygon_material_rows(std::span<std::uint32_t> words,
    std::uint32_t farm_improvement,std::span<const ScenePolygonMaterialSource> sources);
// Retail 70D500's three seven-word groups for Moai, Pontoon Bridge and
// Feitoria. Each group is [art ID, three state keys, three registry IDs].
// Matching registry records overwrite the registry-ID slot in source order.
void populate_scene_special_improvement_material_rows(std::span<std::uint32_t> words,
    std::array<std::uint32_t,3> improvement_ids,
    std::span<const ScenePolygonMaterialSource> sources);
// Retail 637F30 scans backward from a selector over contiguous material-ID
// words starting at the chosen row pointer. This may cross the row's nominal
// five-word stride. A missing ID is 0xFFFFFFFF; the source event's variant
// argument is ignored by this lookup.
std::uint32_t resolve_scene_polygon_material_id(
    std::span<const std::uint32_t> row_words,std::int32_t selector);
// Build the source record before 681700 inserts it. The caller first applies
// 5415F0's grid/gate policy; this record builder does not inspect event.gate.
// 637F30 material selection is the explicit callback; a nonempty variation
// range consumes one RNG draw.
ScenePolygonPlotBytes make_scene_polygon_plot_record(
    const ScenePolygonPlotSourceEvent &event,std::uint32_t scene_identity,
    std::uint32_t variation_count,const ScenePolygonRandomDraw &random,
    const ScenePolygonMaterialResolve &resolve_material);

// Original +2079C plot free list and +207A4 vector. Source tile objects are
// supplied by the scene event producer; this store does not invent them from
// the rendered terrain. The first word of a freed record is its next index.
class ScenePolygonPlotStore {
  public:
    static constexpr std::uint32_t absent=0x0fffffffu;
    explicit ScenePolygonPlotStore(std::span<const std::uint8_t> initial={});
    std::uint32_t insert(const ScenePolygonPlotBytes &source);
    void release(std::uint32_t index);
    std::array<std::int16_t,2> plot_coordinates(std::uint32_t index) const;
    std::span<const ScenePolygonPlotBytes> slots() const {return slots_;}
    std::uint32_t free_head() const {return free_head_;}
    std::size_t active_count() const {return active_count_;}

  private:
    std::vector<ScenePolygonPlotBytes> slots_;
    std::vector<bool> active_;
    std::uint32_t free_head_=absent;
    std::size_t active_count_=0;
};
} // namespace civ5::terrain
