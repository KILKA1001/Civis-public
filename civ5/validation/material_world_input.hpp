#pragma once
// Original archives and primitive map/edge state -> owned material world.
#include "civ5/materials/lighting_mesh.hpp"
#include "civ5/materials/material_world.hpp"
#include "terrain_river_output.hpp"
#include <numeric>
#include <xmmintrin.h>
namespace {
using namespace civ5::materials;
struct Estimates {
    std::vector<std::array<std::uint32_t, 2>> values;
    std::size_t at = 0;
    float take(float squared) {
        check(at < values.size() && std::bit_cast<std::uint32_t>(squared) == values[at][0],
              "World material RSQRT input/count differs");
        return std::bit_cast<float>(values[at++][1]);
    }
    void end() const { check(at == values.size(), "Unused world material RSQRT observations"); }
};
Estimates estimates(Reader &r, std::uint32_t maximum) {
    Estimates result;
    const auto count = r.word();
    check(count <= maximum && std::size_t(count) * 8 <= r.bytes.size() - r.at,
          "Invalid material estimate count");
    for (unsigned i = 0; i < count; ++i) {
        const auto squared = r.word(), seed = r.word();
        result.values.push_back({squared, seed});
    }
    return result;
}
struct PatchRequest {
    std::uint32_t index;
    MaterialPatchSettings settings;
    Estimates scores, vertices;
};
struct MaterialRequest {
    int map_width, map_height, mode;
    std::uint32_t seed, constants_fill;
    X87Precision precision;
    std::vector<RiverEdge> edges;
    Estimates rivers;
    std::vector<PatchRequest> patches;
};
MaterialRequest material_request(Reader &r) {
    MaterialRequest result;
    result.map_width = r.integer();
    result.map_height = r.integer();
    result.mode = r.integer();
    result.seed = r.word();
    result.precision = precision(r);
    const auto count = r.word();
    check(count <= 1048576 && std::size_t(count) * 16 <= r.bytes.size() - r.at,
          "Invalid material river edges");
    for (unsigned i = 0; i < count; ++i) {
        const auto x = r.integer(), y = r.integer(), z = r.integer(), direction = r.integer();
        result.edges.push_back({x, y, z, direction});
    }
    for (unsigned phase = 0; phase < 3; ++phase) {
        auto e = estimates(r, 1048576);
        result.rivers.values.insert(result.rivers.values.end(), e.values.begin(), e.values.end());
    }
    result.constants_fill = r.word();
    const auto patches = r.word();
    check(patches <= 128, "Excessive original material patch requests");
    for (unsigned i = 0; i < patches; ++i) {
        PatchRequest q;
        q.index = r.word();
        q.settings.cache_size = r.integer();
        q.settings.cache_mode = r.integer();
        q.settings.parent_flag = r.integer();
        q.settings.normal_x_remap = r.integer();
        q.settings.precision = precision(r);
        q.scores = estimates(r, 4761);
        q.vertices = estimates(r, 4489);
        result.patches.push_back(std::move(q));
    }
    r.end();
    return result;
}
Bytes input_bytes(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size >= 24 && size <= 64 * 1024 * 1024, "Invalid material world input size");
    Bytes out(static_cast<std::size_t>(size));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char *>(out.data()), out.size());
    check(bool(in), "Material world input read failed");
    return out;
}
std::shared_ptr<civ5::assets::FpkSource> archive(const civ5::assets::AssetEnvironment &environment,
                                                 const char *path) {
    auto result = environment.open_fpk_archive(path);
    check(std::holds_alternative<std::shared_ptr<civ5::assets::FpkSource>>(result),
          "Unavailable material world archive");
    return std::get<std::shared_ptr<civ5::assets::FpkSource>>(result);
}
void cache_layout(Writer &out, const MaterialCacheLayout &cache) {
    out.word(cache.columns);
    out.word(cache.rows);
    out.word(static_cast<std::uint32_t>(cache.patches.size()));
    for (const auto &patch : cache.patches) {
        for (auto v : {patch.origin[0], patch.origin[1], patch.interior_columns, patch.interior_rows})
            out.word(v);
        for (auto f : patch.position)
            out.scalar(f);
    }
}
Bytes material_patch_bytes(const WorldMaterialPatch &patch) {
    // Both GPU consumers must accept the full original edge/interior domain.
    const auto material_mesh = patch.geometry.mesh();
    const LightingPatchMesh lighting_mesh(patch.geometry.columns, patch.geometry.rows,
                                          patch.geometry.streams.normal24, patch.geometry.indices);
    check(material_mesh.vertices().size() == patch.geometry.streams.material48.size() &&
              lighting_mesh.vertices().size() == patch.geometry.streams.normal24.size(),
          "Material consumer extent differs");
    Writer out;
    out.word(patch.geometry.columns);
    out.word(patch.geometry.rows);
    out.word(static_cast<std::uint32_t>(patch.scores.normals.size()));
    for (const auto &normal : patch.scores.normals)
        for (auto f : normal)
            out.scalar(f);
    for (auto score : patch.scores.scores)
        out.word(score);
    const auto &selected = patch.scores.selected;
    out.word(static_cast<std::uint32_t>(selected.sources().size()));
    for (auto source : selected.owner_words())
        out.word(source);
    out.word(selected.specular());
    out.u16(patch.resources);
    out.blob(patch.geometry.streams.material48);
    out.blob(patch.geometry.streams.normal24);
    out.word(static_cast<std::uint32_t>(patch.geometry.indices.size()));
    out.u16(patch.geometry.indices);
    return std::move(out.bytes);
}
void output_patch(const WorldMaterialPatch &patch, const fs::path &folder) {
    save(folder / "patch.bin", material_patch_bytes(patch));
}
} // namespace
