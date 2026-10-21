#pragma once
#include "civ5/assets/dds.hpp"
#include "civ5/terrain/native_math.hpp"
#include <functional>

namespace civ5::materials {
struct FogOfWarStartup {
    std::uint32_t quality, mask_extent;
    terrain::Vector4 scale;
    std::array<float, 3> layer_strength;
};
// Original DX9 6F0C00 clamps the unsigned quality to 2. Quality zero uses a
// generated 32x32 transparent black base texture and skips cloud materials.
FogOfWarStartup fog_of_war_startup(std::uint32_t requested_quality);

struct FogOfWarGeometry {
    // Original layout5: position3, batch index3, two zero UBYTE4N colors,
    // then coverage2 at byte 32 (TEXCOORD0), as produced by 8E1060.
    // Two groups of 24 cells use outer radii 1.33 and 1.4, respectively.
    std::array<std::array<float, 10>, 576> mask_vertices{};
    std::array<std::uint32_t, 2304> mask_indices{};
    // Original layout1: position3, (position.xy,batch index), UV2.
    std::array<std::array<float, 8>, 96> layer_vertices{};
    std::array<std::uint32_t, 144> layer_indices{};
};
// Only initialized and referenced bytes are owned. The original oversized
// buffer allocations also contain untouched tails which are never drawn.
FogOfWarGeometry fog_of_war_geometry();

struct FogOfWarHandles {
    std::uint32_t mask_indices, mask_vertices, layer_indices, layer_vertices;
    std::uint16_t mask_texture;
    std::array<std::uint16_t, 3> cloud_textures;
};
// Original 100-byte records: two mask meshes, then (quality > 0) three cloud
// layers and the quad used by the depth/culling pass. Handles are runtime inputs.
std::vector<std::array<std::uint8_t, 100>> fog_of_war_draw_records(std::uint32_t requested_quality,
                                                                   const FogOfWarHandles &handles);

class FogOfWarAssets {
  public:
    static constexpr std::array<const char *, 3> cloud_paths{
        "FogCloudTile_dif_02.dds", "FogCloudTile_dif_03.dds", "FogCloudTile_dif_01.dds"};
    static FogOfWarAssets load(std::uint32_t requested_quality,
                               const std::function<assets::AssetHandle(const char *)> &resolve);
    const FogOfWarStartup &state() const { return state_; }
    const std::vector<assets::DdsData> &clouds() const { return clouds_; }
    const std::vector<assets::AssetHandle> &sources() const { return sources_; }
    // Used only for quality zero: BGRA8, 32x32, one level, all channels zero.
    static constexpr std::array<std::uint8_t, 4096> fallback_base{};

  private:
    explicit FogOfWarAssets(FogOfWarStartup state) : state_(state) {}
    FogOfWarStartup state_;
    std::vector<assets::DdsData> clouds_;
    std::vector<assets::AssetHandle> sources_;
};
// Original 5A8510/7A6580/586220. Arrays use x*height+y cell indexing.
// Visibility: 0=visible, 1=explored, 2=unseen. Events mark the derived culling
// grid dirty; it changes on the next tick.
class FogOfWarState {
  public:
    FogOfWarState(std::int32_t width, std::int32_t height, std::array<std::int32_t, 3> grid_offset,
                  float minimum_x, float maximum_x);
    // Restart resets the three grids and normalized scale while retaining
    // resource animation time and the last published frame vector.
    void restart(std::int32_t width, std::int32_t height, std::array<std::int32_t, 3> grid_offset,
                 float minimum_x, float maximum_x);
    void visibility_event(std::array<std::int32_t, 3> world_cube, std::uint8_t native_type);
    void visibility_event_all(std::uint8_t native_type);
    void tick(float delta_seconds);
    std::int32_t width() const { return width_; }
    std::int32_t height() const { return height_; }
    std::array<std::int32_t, 3> grid_offset() const { return grid_offset_; }
    bool dirty() const { return dirty_; }
    float elapsed() const { return elapsed_; }
    float normalized_scale() const { return normalized_scale_; }
    const terrain::Vector4 &frame_scale() const { return frame_scale_; }
    const std::vector<std::uint32_t> &visibility() const { return visibility_; }
    const std::vector<std::uint32_t> &visibility_override() const { return visibility_override_; }
    const std::vector<std::uint32_t> &culling() const { return culling_; }

  private:
    std::int32_t width_ = 0, height_ = 0;
    std::array<std::int32_t, 3> grid_offset_{};
    bool dirty_ = false;
    float elapsed_ = 0, normalized_scale_ = 0;
    terrain::Vector4 frame_scale_ = fog_of_war_startup(0).scale;
    std::vector<std::uint32_t> visibility_, visibility_override_, culling_;
};
} // namespace civ5::materials
