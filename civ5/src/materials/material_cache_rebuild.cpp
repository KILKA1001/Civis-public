#include "civ5/materials/material_cache_rebuild.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
namespace {
unsigned style(std::uint32_t mask) {
    for (unsigned i = 1; i < 5; ++i)
        if (mask & (1u << i))
            return i;
    return 0;
}
void packet(MaterialCacheLifecycleState &s, const MaterialCacheRebuildSettings &settings,
            const LandPacketTextures &textures, MaterialCacheRebuildBackend &backend,
            unsigned slot, std::uint32_t diffuse, std::uint32_t specular, unsigned selected_style,
            bool additional) {
    auto &out = backend.allocate_land_packet(s.geometry.packets[slot]);
    out = {};
    out.key = 0x49;
    if (!additional)
        out.geometry = {s.geometry.primary_index_buffer, s.geometry.vertex_buffer,
                        s.geometry.triangle_count, 0, 0};
    out.pixel_count = 10;
    const std::array<std::uint32_t, 10> sources{
        diffuse, textures.shared[0], textures.shared[1], textures.shared[2], textures.shared[3],
        textures.shared[4], textures.style_diffuse[selected_style], textures.style_specular[selected_style],
        specular, textures.shared[5]};
    for (unsigned i = 0; i < sources.size(); ++i) {
        out.pixel_textures[i] = static_cast<std::uint16_t>(sources[i]);
        out.pixel_samplers[i] = i >= 3 && i <= 5 ? 0 : 5;
    }
    if (additional || settings.separate_geometry_buffers) {
        out.vertex_count = 1;
        out.vertex_textures[0] = static_cast<std::uint16_t>(settings.height_texture);
        out.vertex_samplers[0] = additional ? 0 : 9;
    }
}
} // namespace
void material_cache_rebuild_geometry(MaterialCacheLifecycleState &s,
                                      const MaterialCacheRebuildSettings &settings,
                                      const LandPacketTextures &textures,
                                      MaterialCacheRebuildBackend &backend) {
    const auto level = std::floor(static_cast<double>(settings.detail_level));
    if (settings.columns < 2 || settings.columns > 65 || settings.rows < 2 || settings.rows > 65 ||
        !std::isfinite(level) || level < std::numeric_limits<std::int32_t>::min() ||
        level > std::numeric_limits<std::int32_t>::max() ||
        (settings.capability >= 10 && (level < 0 || level > 4)))
        throw std::invalid_argument("Invalid LAND geometry rebuild domain");
    auto &g = s.geometry;
    if (!settings.separate_geometry_buffers) {
        backend.upload_geometry_pair(s.staging[0], g.vertex_count, s.staging[1], g.triangle_count * 3u,
                                     g.vertex_buffer, g.primary_index_buffer);
        backend.free_staging(s.staging[0]);
        backend.free_staging(s.staging[1]);
        s.staging[0] = s.staging[1] = 0;
    } else {
        backend.shared_geometry(settings.columns, settings.rows, g.vertex_buffer, g.primary_index_buffer,
                                g.secondary_index_buffer, g.triangle_count);
        backend.retain_vertex_buffer(g.vertex_buffer);
        backend.retain_index_buffer(g.secondary_index_buffer);
        backend.retain_index_buffer(g.primary_index_buffer);
    }
    const auto selected_style = style(settings.style_mask);
    packet(s, settings, textures, backend, 0, s.textures.final[0], s.textures.final[1], selected_style, false);
    packet(s, settings, textures, backend, 1, s.textures.higher[0], s.textures.higher[1], selected_style, false);
    if (settings.capability < 10)
        return;
    for (unsigned i = 2; i < 5; ++i)
        backend.release_packet(g.packets[i]);
    for (unsigned i = 2; i < 5; ++i)
        g.packets[i] = 0;
    // The DX9 constructor does not read the control-stream offset or reduced
    // dimensions calculated by 4DE230. Its additional packets have no geometry
    // binding. Keep this counterfactual >=10 path without inventing DX11 draws.
    const auto detail_style = style(settings.style_mask);
    packet(s, settings, textures, backend, 2, s.textures.final[0], s.textures.final[1], detail_style, true);
    packet(s, settings, textures, backend, 3, s.textures.higher[0], s.textures.higher[1], detail_style, true);
    g.detail_levels[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(level));
    g.detail_levels[1] = std::min(g.detail_levels[0] + 1, 4u);
    packet(s, settings, textures, backend, 4, 0, 0, 0, true);
}
} // namespace civ5::materials
