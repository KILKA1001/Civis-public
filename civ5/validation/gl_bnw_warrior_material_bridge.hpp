#pragma once
#include "civ5/graphics/gl_distance_fog.hpp"
#include "civ5/graphics/bnw_warrior_team_color_rules.hpp"
#include "civ5/graphics/bnw_warrior_retained_material_packets.hpp"
namespace civ5::graphics {
// Caller borrows the current SAME renderer's camera/fog and stable scene epoch.
// No new terrain, roster, framebuffer, camera, or normalized pose is supplied.
struct GlBnwWarriorMaterialView {
    std::int32_t unit;
    std::uint64_t scene_revision;
    float renderer_clock;
    terrain::Vector3 camera_world;
    const GlDistanceFog* fog_owner;
    float normalized_zoom;
    terrain::X87Precision fog_precision;
    bool world_visibility_gate;
};
struct GlBnwWarriorMaterialState {
    bool retained=false;
    std::size_t visible_members=0,visibility_flushes=0;
    std::array<std::uint16_t,12> packet_family{};
    bool source_ps_sdk_four_diagnostic=false,native_optimized_vertex_buffer=false;
};
} // namespace civ5::graphics
