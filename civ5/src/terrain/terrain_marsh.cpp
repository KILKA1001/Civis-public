#include "civ5/terrain/terrain_marsh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::invalid_argument(message);
}
std::int32_t cvtt(float f) {
    return std::isfinite(f) && double(f) >= -2147483648.0 && double(f) < 2147483648.0
        ? static_cast<std::int32_t>(f) : std::numeric_limits<std::int32_t>::min();
}
struct Clip {
    std::int32_t first, last, lower, upper;
};
Clip clip(std::int32_t origin, std::int32_t extent) {
    const auto end = wrap_add(origin, 128);
    const auto excess = std::max(wrap_add(end, wrap_neg(extent)), 0);
    const auto first = std::max(wrap_neg(origin), 0);
    return {first, wrap_add(128, wrap_neg(excess)), wrap_add(origin, first),
            wrap_add(end, wrap_neg(excess))};
}
} // namespace
TerrainMarshStamp TerrainMarshStamp::prepare(const assets::DdsData &dds, Storage prior) {
    require(dds.width() == 128 && dds.height() == 128 && dds.face_count() == 1 &&
            dds.format() == assets::DdsFormat::luminance && dds.bits_per_pixel() == 8 &&
            dds.channel_masks() == std::array<std::uint32_t, 4>{255, 0, 0, 0},
            "Marsh stamp requires the recovered 128x128 L8 layout");
    const auto image = dds.payload(0, 0);
    require(image.size() == 16384, "Marsh stamp base image size differs");
    std::copy(image.begin(), image.end(), prior.begin() + 1);
    return TerrainMarshStamp(std::move(prior));
}
TerrainMarshResult apply_terrain_marshes(HeightDomain domain,
    std::span<const std::uint16_t> heights, std::span<const std::uint8_t> materials,
    const TerrainMarshState &prior, std::span<const TerrainFeature2Source> sources,
    bool activate, const TerrainMarshStamp &stamp, const TerrainMarshSettings &settings,
    const TerrainMarshWriteObserver &write) {
    const auto count = std::size_t(domain.width()) * domain.height();
    require(heights.size() == count && materials.size() == count, "Incomplete marsh fields");
    TerrainMarshResult result{{materials.begin(), materials.end()}, prior, {}};
    const auto apply = [&](const TerrainFeature2Source &source) {
        require(settings.raster_width > 0 && settings.raster_height > 0, "Invalid marsh raster extent");
        require(settings.material_codes.size() <= 256, "Marsh palette exceeds byte ordinals");
        // B89658 has five established continent codes. Never index beyond
        // that recovered table using an unchecked saved/modded identifier.
        require(source.continent >= 0 && source.continent < 5, "Unsupported marsh continent code");
        const auto &p = source.position;
        const auto v = vector4_transform({float(p.x), float(p.y), float(p.z), 1.f},
            settings.cache_matrix, settings.precision, settings.transform);
        const auto x = wrap_add(cvtt(v[0]), -64), y = wrap_add(cvtt(v[1]), -64);
        const auto cx = clip(x, settings.raster_width), cy = clip(y, settings.raster_height);
        const auto code = 0x100000u + static_cast<std::uint32_t>(source.continent) * 0x10000u;
        const auto match = std::find(settings.material_codes.begin(), settings.material_codes.end(), code);
        const auto material = match == settings.material_codes.end() ? std::uint8_t(255)
            : static_cast<std::uint8_t>(match - settings.material_codes.begin());
        // Ordinary bounds reduce to at most one 128x128 stamp. Reject an
        // overflowing rectangle that would read outside the original mask.
        if (cx.first < cx.last && cy.first < cy.last) {
            require(cx.first >= 0 && cx.last <= 128 && cy.first >= 0 && cy.last <= 128,
                    "Marsh rectangle exceeds mask storage");
            for (auto u = cx.first; u < cx.last; ++u)
                for (auto v = cy.first; v < cy.last; ++v) {
                    if (!stamp.storage()[std::size_t(u)*128+v]) continue;
                    auto dx = wrap_add(x, u);
                    const auto dy = wrap_add(y, v);
                    if (settings.map_mode == 1)
                        dx = wrap_add(wrap_mul(100, domain.width()), dx) % domain.width();
                    require(dx >= 0 && dx < domain.width() && dy >= 0 && dy < domain.height(),
                            "Marsh sample outside height owner");
                    const auto index = dy * domain.width() + dx;
                    if (heights[index] > 13000 && heights[index] < 18000) {
                        result.materials[index] = material;
                        if (write) write(result.applications.size(), index, material);
                    }
                }
        }
        result.applications.push_back({source, x, y, code, material,
            {float(cx.lower), float(cy.lower)}, {float(cx.upper), float(cy.upper)}});
        result.state.retained.push_back(source);
    };
    for (const auto &source : sources) {
        if (result.state.enabled) apply(source);
        else result.state.pending.push_back(source);
    }
    if (activate) {
        result.state.enabled = true;
        for (const auto &source : result.state.pending) apply(source);
        result.state.pending.clear();
    }
    return result;
}
} // namespace civ5::terrain
