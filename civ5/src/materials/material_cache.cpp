#include "civ5/materials/material_cache.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace civ5::materials {
namespace {
void extent(std::uint32_t n) {
    if (!n || n > 16384)
        throw std::invalid_argument("Material cache extent must be 1..16384");
}
std::uint32_t accounted_bytes(std::uint32_t width, std::uint32_t height, std::uint32_t levels,
                              MaterialCacheFormat format) {
    const std::uint32_t bits =
        format == MaterialCacheFormat::rgba16_float || format == MaterialCacheFormat::compression_intermediate
            ? 64
        : format == MaterialCacheFormat::rgba8  ? 32
        : format == MaterialCacheFormat::rgb565 ? 16
                                                : 4;
    // Original 8E8900 reports this counter using wrapping 32-bit arithmetic.
    // It is not an allocation-size estimate (autogen mips are not counted).
    auto bytes = (bits * width * height) >> 3;
    std::uint32_t result = 0;
    for (std::uint32_t mip = 0; mip < std::max(1u, levels); ++mip) {
        result += bytes;
        bytes >>= 2;
    }
    return result;
}
MaterialCacheTexturePlan texture(MaterialCacheRole role, std::uint32_t index, std::uint32_t size,
                                 std::uint32_t levels, MaterialCacheFormat format) {
    const bool compressed = format == MaterialCacheFormat::dxt1;
    return {role,
            index,
            size,
            size,
            levels,
            format,
            compressed ? 0u
            : levels   ? 1u
                       : 0x401u,
            compressed ? 1u : 0u,
            accounted_bytes(size, size, levels, format)};
}
} // namespace
MaterialCacheQuality normalize_material_cache_quality(const MaterialCacheQualityInput &input) {
    MaterialCacheQuality result{128, 128, 64, 0, input.compression, input.tessellation, input.pool_counts};
    if (input.renderer_code < 7) {
        result.pool_counts[0] = std::min(result.pool_counts[0], 2048u);
        result.pool_counts[1] = std::min(result.pool_counts[1], 256u);
        result.pool_counts[2] = std::min(result.pool_counts[2], 128u);
        result.pool_counts[3] = 0;
    }
    if (input.flags & 2)
        result.tessellation = 0;
    if (input.renderer_code < 8)
        result.compression = 0;
    if (!result.compression) {
        if (input.detail == 1 || input.detail == 2)
            result.temporary_size = result.cache_size = 256;
        else if (input.detail == 3) {
            result.temporary_size = 512;
            result.cache_size = 256;
        }
        result.pool_counts[0] = std::max(result.pool_counts[0], 16u);
    } else if (input.detail == 1) {
        result.temporary_size = result.cache_size = 256;
        result.pool_counts[0] = 16;
        result.pool_counts[2] = 256;
        result.pool_counts[3] = 0;
        result.pool_counts[4] = 4096;
    } else if (input.detail == 2 || input.detail == 3) {
        result.temporary_size = 512;
        result.cache_size = result.base_size = 256;
        result.cache_mode = 1;
        result.pool_counts = {16, 0, 256, 64, input.detail == 2 ? 4096u : 6144u};
    } else {
        result.pool_counts[0] = std::max(result.pool_counts[0], 16u);
        result.pool_counts[2] = result.pool_counts[3] = 0;
        result.pool_counts[4] = 4096;
    }
    if (!result.tessellation)
        result.cache_mode = 0;
    return result;
}
std::vector<MaterialCacheTexturePlan>
material_cache_texture_plan(std::uint32_t temporary_size, std::uint32_t cache_size, bool compression) {
    extent(temporary_size);
    extent(cache_size);
    using F = MaterialCacheFormat;
    using R = MaterialCacheRole;
    const std::array<F, 13> formats{F::rgba16_float, F::rgba16_float, F::rgba16_float, F::rgba8, F::rgba8,
                                    F::rgba8,        F::rgba8,        F::rgba8,        F::rgba8, F::rgba8,
                                    F::rgba16_float, F::rgba16_float, F::rgba8};
    std::vector<MaterialCacheTexturePlan> result;
    result.reserve(compression ? 29 : 13);
    for (std::uint32_t i = 0; i < 13; ++i)
        result.push_back(texture(static_cast<R>(i), 0, i < 7 ? temporary_size : cache_size, 0, formats[i]));
    if (compression)
        for (std::uint32_t i = 0; i < 4; ++i) {
            const auto size = 64u << i, levels = 4 + i;
            result.push_back(
                texture(R::compression_diffuse, i, size / 4, levels, F::compression_intermediate));
            result.push_back(
                texture(R::compression_specular, i, size / 4, levels, F::compression_intermediate));
            result.push_back(texture(R::compression_diffuse_color, i, size, levels, F::rgba8));
            result.push_back(texture(R::compression_specular_color, i, size, levels, F::rgba8));
        }
    return result;
}
std::vector<MaterialCacheTexturePlan> material_cache_pool_plan(std::span<const std::uint32_t, 5> counts,
                                                               bool compression, std::uint32_t base_size) {
    extent(base_size);
    const std::array<std::uint32_t, 5> capacities{8096, 1024, 1024, 1024, 8096};
    std::size_t total = 0;
    for (unsigned i = 0; i < 5; ++i) {
        if (i == 4 && !compression)
            continue;
        if (counts[i] > capacities[i])
            throw std::invalid_argument("Material texture pool exceeds original loading-owner capacity");
        total += counts[i];
    }
    std::vector<MaterialCacheTexturePlan> result;
    result.reserve(total);
    for (unsigned i = 0; i < 4; ++i)
        for (std::uint32_t n = 0; n < counts[i]; ++n)
            result.push_back(
                texture(static_cast<MaterialCacheRole>(unsigned(MaterialCacheRole::patch_64) + i), n,
                        64u << i, 0, MaterialCacheFormat::rgb565));
    if (compression) {
        const unsigned levels = base_size == 128 ? 5 : base_size == 256 ? 6 : base_size == 512 ? 7 : 4;
        for (std::uint32_t n = 0; n < counts[4]; ++n)
            result.push_back(texture(MaterialCacheRole::patch_compressed, n, base_size, levels,
                                     MaterialCacheFormat::dxt1));
    }
    return result;
}
std::array<std::array<float, 2>, 32> material_shadow_jitter(terrain::X87Precision precision) {
    if (precision != terrain::X87Precision::single && precision != terrain::X87Precision::double_precision)
        throw std::invalid_argument("Unsupported material jitter x87 precision");
    const auto round = [precision](double value) { return terrain::round_x87(value, precision); };
    std::array<std::array<float, 2>, 32> result{};
    std::uint32_t state = 7;
    for (unsigned i = 1; i < result.size(); ++i)
        for (auto &component : result[i]) {
            state = (state * 214013u + 2531011u) % 0xffffcfefu;
            // FILD itself retains the signed integer exactly even with PC24.
            double value = std::bit_cast<std::int32_t>(state);
            if (std::bit_cast<std::int32_t>(state) < 0)
                value = round(value + 0x1p32);
            value = round(value * 0x1.0000301109066p-32);
            component = static_cast<float>(round(value - .5));
        }
    return result;
}
} // namespace civ5::materials
