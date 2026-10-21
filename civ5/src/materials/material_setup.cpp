#include "civ5/materials/material_setup.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
constexpr std::array<std::uint32_t, 5> prefixes{0x100000, 0x110000, 0x120000, 0x130000, 0x140000};
} // namespace
std::vector<MaterialRow> terrain_provider_rows(std::span<const MaterialRow> authored) {
    require(!authored.empty() && authored.size() < 64, "Terrain provider requires 1..63 authored rows");
    for (const auto &row : authored)
        require(row.continent >= 0 && row.continent < 5, "Unavailable original material continent prefix");
    std::vector<MaterialRow> result(authored.begin(), authored.end());
    // Native provider discards the parsed dominance column for every row.
    for (auto &row : result)
        row.dominance = 0.f;
    result.push_back({-1, "Checkerboard.dds", "Checkerboard.dds", std::nullopt, 0.f,
                      std::bit_cast<float>(0x3c23d70au), 0, 0xffffffffu});
    return result;
}
std::vector<std::uint32_t> terrain_material_codes(std::span<const MaterialRow> provider) {
    require(!provider.empty() && provider.size() <= 64, "Terrain material code count outside 1..64");
    std::vector<std::uint32_t> result;
    result.reserve(provider.size());
    for (std::size_t i = 0; i < provider.size(); ++i) {
        const auto &row = provider[i];
        if (row.continent == -1 && row.local_id == 0xffffffffu && i + 1 == provider.size())
            result.push_back(0xffffffffu);
        else {
            require(row.continent >= 0 && row.continent < 5,
                    "Unavailable original material continent prefix");
            result.push_back(prefixes[row.continent] | row.local_id);
        }
    }
    return result;
}
std::array<MaterialSample, 25> native_material_samples() {
    constexpr float edge = std::bit_cast<float>(0x3e000000u);
    constexpr float adjacent = std::bit_cast<float>(0x3f22d491u);
    constexpr std::array<float, 5> coefficients{edge, adjacent, 1.f, adjacent, edge};
    std::array<MaterialSample, 25> result;
    float total = 0.f;
    std::size_t at = 0;
    for (int y = -2; y <= 2; ++y) {
        const float row = coefficients[y + 2];
        const std::array<float, 5> weights{row * edge, row * adjacent, row, row * adjacent, row * edge};
        for (int x = -2; x <= 2; ++x) {
            const float weight = weights[x + 2];
            result[at++] = {x, y, weight};
            total = weight + total;
        }
    }
    const float inverse = 1.f / total;
    for (auto &sample : result)
        sample.weight = inverse * sample.weight;
    return result;
}
MaterialRiverSettings terrain_river_settings(std::span<const std::uint32_t> codes) {
    require(codes.size() <= 64, "River material palette exceeds provider capacity");
    MaterialRiverSettings result{{}, {}, std::bit_cast<float>(0x3e70fcf1u), 0x140000u};
    auto first = [&](std::uint32_t key) -> std::uint8_t {
        const auto found = std::find(codes.begin(), codes.end(), key);
        return found == codes.end() ? 255 : static_cast<std::uint8_t>(found - codes.begin());
    };
    for (std::size_t i = 0; i < 4; ++i) {
        const auto prefix = prefixes[i + 1] & 0x00ff0000u;
        result.remap_from[i] = first(prefix | 0x35u);
        result.remap_to[i] = first(prefix | 0x10u);
        if (result.remap_to[i] == 255)
            result.remap_from[i] = 255;
    }
    return result;
}
terrain::RiverRasterSettings MaterialRiverSettings::raster(std::span<const std::uint32_t> codes,
                                                           terrain::X87Precision precision) const {
    return {level, precision, remap_from, remap_to, codes, river_code};
}
MaterialCacheLayout material_cache_layout(int width, int height) {
    require(width >= 1 && width <= 65536 && height >= 1 && height <= 65536,
            "Material cache world dimensions outside 1..65536");
    MaterialCacheLayout result{(width + 62) >> 6, (height + 62) >> 6, {}};
    const auto count = static_cast<std::size_t>(result.columns) * result.rows;
    result.patches.reserve(count);
    for (int x = 0; x < result.columns; ++x)
        for (int y = 0; y < result.rows; ++y) {
            const int ox = x * 64, oy = y * 64;
            result.patches.push_back({{ox, oy},
                                      std::min(width - 1 - ox, 64) + 1,
                                      std::min(height - 1 - oy, 64) + 1,
                                      {static_cast<float>(ox) - static_cast<float>(width) * .5f,
                                       static_cast<float>(oy) - static_cast<float>(height) * .5f, 0.f, 0.f}});
        }
    return result;
}
MaterialSourceDomain material_world_domain(const terrain::HeightOwnerState &owner, int normal_x_remap) {
    const auto &p = owner.parameters;
    require(owner.height.element_bytes() == 2 && owner.material.element_bytes() == 1 &&
                p.width == owner.height.domain().width() && p.height == owner.height.domain().height() &&
                p.width == owner.material.domain().width() && p.height == owner.material.domain().height() &&
                p.width > 0 && p.height > 0 && std::isfinite(p.scale_x) && std::isfinite(p.scale_y) &&
                std::isfinite(p.scale_z),
            "Material source does not match owned world fields");
    return {p.width, p.height, {p.scale_x, p.scale_y, p.scale_z}, normal_x_remap};
}
} // namespace civ5::materials
