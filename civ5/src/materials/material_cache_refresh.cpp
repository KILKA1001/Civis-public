#include "civ5/materials/material_cache_refresh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::invalid_argument(message);
}
std::int32_t cvtt(float value) {
    return std::isfinite(value) && double(value) >= -2147483648.0 && double(value) < 2147483648.0
        ? static_cast<std::int32_t>(value) : INT32_MIN;
}
void precision(terrain::X87Precision pc) {
    require(pc == terrain::X87Precision::single || pc == terrain::X87Precision::double_precision,
            "Invalid cache refresh precision");
}
} // namespace
MaterialCacheHeightMetrics material_cache_height_metrics(int columns, int rows,
    MaterialOrigin origin, int width, int height, const MaterialLookup &lookup, terrain::X87Precision pc) {
    precision(pc);
    require(columns >= 2 && columns <= 67 && rows >= 2 && rows <= 67 && width > 0 && height > 0 && lookup,
            "Invalid cache height metric domain");
    const auto x0 = std::int64_t(origin[0])+4, y0 = std::int64_t(origin[1])+4;
    const auto x1 = std::int64_t(origin[0])+columns, y1 = std::int64_t(origin[1])+rows+1;
    require(x0 <= INT32_MAX && y0 <= INT32_MAX && x1 <= INT32_MAX && y1 <= INT32_MAX,
            "Overflowing cache height metric traversal");
    const auto wrap = [](std::int32_t position, int extent, int next) {
        const auto value = terrain::wrap_add(terrain::wrap_add(extent, position), next);
        return static_cast<int>(std::bit_cast<std::uint32_t>(value) % static_cast<std::uint32_t>(extent));
    };
    const auto sample = [&](int x, int y) {
        const auto h = lookup(x, y);
        require(h >= 0 && h <= 65535, "Cache metric source is not U16");
        return h;
    };
    constexpr float factor = std::bit_cast<float>(0x3b808081u);
    MaterialCacheHeightMetrics result;
    for (auto y = y0; y < y1; ++y) {
        const auto sy = wrap(static_cast<std::int32_t>(y), height, 0);
        const auto ny = wrap(static_cast<std::int32_t>(y), height, 1);
        for (auto x = x0; x < x1; ++x) {
            const auto sx = wrap(static_cast<std::int32_t>(x), width, 0);
            const auto nx = wrap(static_cast<std::int32_t>(x), width, 1);
            const float center = float(sample(sx, sy)) * factor;
            const float neighbor = float(sample(nx, sy)) * factor;
            const double next = terrain::round_x87(double(sample(sx, ny)) * double(factor), pc);
            const float difference = float(std::abs(terrain::round_x87(double(center) - next, pc)));
            result.first = std::max({result.first, neighbor, difference});
            result.maximum = std::max(result.maximum, center);
        }
    }
    result.category = result.maximum < 2.f ? 1 : result.maximum < 30.f ? 2 : 3;
    return result;
}
void material_cache_refresh_region(const MaterialCacheRefreshDomain &domain,
    terrain::Point2 lower, terrain::Point2 upper, bool stamp_owner,
    terrain::X87Precision pc, std::span<MaterialCacheSchedulerPatch> patches,
    std::uint32_t &owner_revision, MaterialCacheRefreshBackend &backend) {
    precision(pc);
    require(domain.columns <= 65536 && domain.rows <= 65536 &&
            std::uint64_t(domain.columns)*domain.rows == patches.size() && patches.size() <= INT32_MAX &&
            std::isfinite(domain.scale_x) && std::isfinite(domain.scale_y) &&
            std::isfinite(domain.span_x) && std::isfinite(domain.span_y) && domain.span_x > 0 && domain.span_y > 0,
            "Invalid material refresh lattice");
    const float fx = float(terrain::round_x87(double(domain.columns)*double(domain.scale_x), pc));
    const float fy = float(terrain::round_x87(double(domain.rows)*double(domain.scale_y), pc));
    const float ix = 1.f/domain.span_x, iy = 1.f/domain.span_y;
    const int x0 = std::max(cvtt((lower.x*ix)*fx), 0), y0 = std::max(cvtt((lower.y*iy)*fy), 0);
    const int x1 = std::min(cvtt((upper.x*ix)*fx), int(domain.columns)-1);
    const int y1 = std::min(cvtt((upper.y*iy)*fy), int(domain.rows)-1);
    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y) {
            const auto index = std::uint32_t(x)*domain.rows+std::uint32_t(y);
            auto &patch = patches[index];
            patch.source_revision = backend.source_revision();
            backend.clear_selection(index);
            patch.material_parent_flag = 1;
            backend.submit_refresh(index);
        }
    backend.wait_refresh();
    if (stamp_owner) owner_revision = backend.source_revision();
}
} // namespace civ5::materials
