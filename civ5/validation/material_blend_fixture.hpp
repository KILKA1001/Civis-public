#pragma once
// Validation-only input transport and explicit texture controls. Not runtime.
#include "civ5/materials/material_assets.hpp"
#include "civ5/materials/material_mesh.hpp"
#include "material_generation_fixture.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace blend_validation {
using namespace civ5::assets;
using namespace civ5::materials;
inline void require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
inline Bytes read(const std::filesystem::path &path) {
    const auto size = std::filesystem::file_size(path);
    require(size <= 4 * 1024 * 1024, "Oversized blend fixture");
    Bytes result(static_cast<std::size_t>(size));
    std::ifstream stream(path, std::ios::binary);
    stream.read(reinterpret_cast<char *>(result.data()), static_cast<std::streamsize>(size));
    require(bool(stream), "Fixture read failed");
    return result;
}
inline void write(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    require(!std::filesystem::exists(path), "Refusing output overwrite");
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    require(bool(stream), "Blend write failed");
}
inline void write(const std::filesystem::path &path, std::span<const float> values) {
    write(path, {reinterpret_cast<const std::uint8_t *>(values.data()), values.size_bytes()});
}
inline void text(const std::filesystem::path &path, std::string_view value) {
    write(path, {reinterpret_cast<const std::uint8_t *>(value.data()), value.size()});
}
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t at = 0;
    auto take(std::size_t n) {
        require(n <= data.size() - at, "Truncated blend fixture");
        auto out = data.subspan(at, n);
        at += n;
        return out;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return b[0] | (std::uint32_t(b[1]) << 8) | (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
    }
    auto blob() { return take(word()); }
    void end() { require(at == data.size(), "Trailing blend fixture bytes"); }
};
struct Patch {
    MaterialPatchMesh mesh;
    std::vector<std::int32_t> selected;
};
inline bool is_generation_input(std::span<const std::uint8_t> data) {
    return data.size() >= 8 && std::string_view(reinterpret_cast<const char *>(data.data()), 8) == "C5MATG01";
}
inline std::vector<Patch> patches(std::span<const std::uint8_t> data) {
    if (is_generation_input(data)) {
        std::vector<Patch> result;
        for (const auto &generated : generation_validation::gpu_patches(data)) {
            const auto selected = generated.selected.sources();
            result.push_back({generated.patch.mesh(), {selected.begin(), selected.end()}});
        }
        return result;
    }
    Reader r{data};
    const auto magic = r.take(8);
    require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5MBIN01",
            "Wrong computed blend input schema");
    require(r.word() == 3, "Expected three computed native patches");
    std::vector<Patch> result;
    for (int i = 0; i < 3; ++i) {
        const int w = static_cast<int>(r.word()), h = static_cast<int>(r.word());
        const auto n = r.word();
        require(n <= 8, "Invalid material selection extent");
        std::vector<std::int32_t> selected;
        for (unsigned j = 0; j < n; ++j) {
            const auto source = r.word();
            require(source < 64, "Invalid material source");
            selected.push_back(static_cast<std::int32_t>(source));
        }
        const auto vertices = r.blob(), raw_indices = r.blob();
        require(raw_indices.size() % 2 == 0, "Partial U16 index");
        std::vector<std::uint16_t> indices(raw_indices.size() / 2);
        std::memcpy(indices.data(), raw_indices.data(), raw_indices.size());
        result.push_back({MaterialPatchMesh(w, h, vertices, indices), std::move(selected)});
    }
    r.end();
    return result;
}
inline Bytes computed_patches(std::span<const Patch> patches) {
    using generation_validation::blob;
    using generation_validation::word;
    const std::string_view magic = "C5MBIN01";
    Bytes output(magic.begin(), magic.end());
    word(output, static_cast<std::uint32_t>(patches.size()));
    for (const auto &patch : patches) {
        word(output, patch.mesh.columns());
        word(output, patch.mesh.rows());
        word(output, static_cast<std::uint32_t>(patch.selected.size()));
        for (auto selected : patch.selected)
            word(output, selected);
        blob(output, patch.mesh.vertices());
        const auto indices = patch.mesh.indices();
        blob(output, {reinterpret_cast<const std::uint8_t *>(indices.data()), indices.size_bytes()});
    }
    return output;
}
inline std::array<float, 384> float_coefficients() {
    std::array<float, 384> result{};
    for (int i = 0; i < 24; ++i) {
        result[i * 4] =
            material_texture_multiplier(static_cast<float>((i / 3 + 1) * .031337), i % 3, 2070.375f);
        result[i * 4 + 1] = static_cast<float>(i % 3);
    }
    return result;
}
inline std::array<float, 64> float_texture(int layer, bool pattern) {
    std::array<float, 64> result{};
    for (int i = 0; i < 64; ++i) {
        const int cell = i / 4, component = i % 4;
        result[i] =
            layer >= 8 ? ((layer - 7) * .0625f + (pattern ? (cell % 4 + cell / 4) * .015625f : 0.f))
            : component == 3
                ? .375f + (layer % 5) * .125f
                : .0625f + ((layer + component * 3 + (pattern ? cell * (component + 1) : 0)) % 13) * .0625f;
    }
    return result;
}
inline MaterialAssets authored(const std::filesystem::path &root) {
    const AssetEnvironment env(root);
    auto archive = [&](const char *path) {
        auto found = env.open_fpk_archive(path);
        require(std::holds_alternative<std::shared_ptr<FpkSource>>(found), "Missing exact material archive");
        return std::get<std::shared_ptr<FpkSource>>(found);
    };
    const auto models = archive("Resource/Common/TerrainModels.fpk"),
               textures = archive("Resource/DX9/TerrainTextures.fpk");
    auto found = models->find(AssetKey("TerrainDef.csv"));
    require(std::holds_alternative<AssetHandle>(found), "Missing exact TerrainDef");
    auto result = MaterialAssets::load(std::get<AssetHandle>(found), *textures);
    require(result.rows().size() == 41 && result.textures().size() == 72 &&
                result.expansion().source_rows().size() == 53,
            "Changed material baseline");
    return result;
}
} // namespace blend_validation
