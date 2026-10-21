// Direct original archives -> complete provider/filter/cache-layout setup.
#include "civ5/materials/material_setup.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>
namespace {
using namespace civ5::assets;
using namespace civ5::materials;
using namespace generation_validation;
std::shared_ptr<FpkSource> archive(const AssetEnvironment &environment, const char *path) {
    auto found = environment.open_fpk_archive(path);
    require(std::holds_alternative<std::shared_ptr<FpkSource>>(found), "Unavailable setup source archive");
    return std::get<std::shared_ptr<FpkSource>>(found);
}
template <class T> void values(Bytes &out, std::span<const T> v) { vector(out, v); }
void string(Bytes &out, const std::string &value) {
    blob(out, {reinterpret_cast<const std::uint8_t *>(value.data()), value.size()});
}
void settings(Bytes &out, const MaterialRiverSettings &s) {
    raw(out, std::span<const std::uint8_t>(s.remap_from));
    raw(out, std::span<const std::uint8_t>(s.remap_to));
    word(out, std::bit_cast<std::uint32_t>(s.level));
    word(out, s.river_code);
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        require(argc == 4, "Usage: material_setup_oracle asset-root input.bin fresh-output");
        const std::filesystem::path root = argv[1], output = argv[3];
        require(!std::filesystem::exists(output), "Setup output must be fresh");
        const auto input = read(argv[2]);
        Reader r{input};
        const auto magic = r.take(8);
        require(std::string(magic.begin(), magic.end()) == "C5MSIN01", "Wrong setup input schema");
        const auto count = r.word();
        require(count >= 1 && count <= 128, "Invalid setup world count");
        std::vector<std::array<int, 2>> dimensions;
        for (unsigned i = 0; i < count; ++i) {
            const auto w = r.integer(), h = r.integer();
            dimensions.push_back({w, h});
        }
        const auto controls = r.word();
        require(controls <= 128, "Too many setup remap controls");
        std::vector<std::vector<std::uint32_t>> palettes;
        for (unsigned i = 0; i < controls; ++i)
            palettes.push_back(r.values<std::uint32_t>(64));
        r.end();
        AssetEnvironment environment(root);
        const auto models = archive(environment, "Resource/Common/TerrainModels.fpk");
        const auto textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
        const auto lookup = models->find(AssetKey("TerrainDef.csv"));
        require(std::holds_alternative<AssetHandle>(lookup), "Unresolved original TerrainDef");
        auto assets = MaterialAssets::load_terrain_provider(std::get<AssetHandle>(lookup), *textures);
        Bytes out;
        string(out, "C5MSET01");
        blob(out, assets.definition().read_bytes());
        word(out, static_cast<std::uint32_t>(assets.rows().size()));
        for (const auto &row : assets.rows()) {
            word(out, row.continent);
            for (const auto &name : {row.diffuse, row.height, row.specular})
                string(out, name.value_or(""));
            word(out, std::bit_cast<std::uint32_t>(row.dominance));
            word(out, std::bit_cast<std::uint32_t>(row.scale));
            word(out, row.projection);
            word(out, row.local_id);
        }
        const auto codes = terrain_material_codes(assets.rows());
        values(out, std::span<const std::uint32_t>(codes));
        values(out, assets.expansion().base_slots());
        values(out, assets.expansion().source_rows());
        values(out, assets.expansion().source_modes());
        values(out, assets.specular_resources());
        word(out, static_cast<std::uint32_t>(assets.textures().size()));
        for (const auto &texture : assets.textures()) {
            string(out, texture.asset.requested_key().normalized_name());
            blob(out, texture.asset.read_bytes());
        }
        const auto sources = assets.expansion().source_rows().size();
        word(out, static_cast<std::uint32_t>(sources));
        for (std::size_t i = 0; i < sources; ++i) {
            const auto selected = assets.selected_resources(std::array{static_cast<std::int32_t>(i)});
            for (auto id : selected)
                word(out, id);
        }
        const auto samples = native_material_samples();
        word(out, static_cast<std::uint32_t>(samples.size()));
        for (const auto &sample : samples) {
            word(out, sample.x);
            word(out, sample.y);
            word(out, std::bit_cast<std::uint32_t>(sample.weight));
        }
        settings(out, terrain_river_settings(codes));
        word(out, count);
        for (auto [w, h] : dimensions) {
            const auto p = civ5::terrain::HeightOwnerParameters::native_call(w, h);
            word(out, p.width);
            word(out, p.height);
            for (auto f : {p.center_x, p.center_y, p.center_z, p.world_span_x, p.world_span_y, p.scale_x,
                           p.scale_y, p.scale_z})
                word(out, std::bit_cast<std::uint32_t>(f));
            const auto cache = material_cache_layout(w, h);
            word(out, cache.columns);
            word(out, cache.rows);
            word(out, static_cast<std::uint32_t>(cache.patches.size()));
            for (const auto &patch : cache.patches) {
                for (auto v : {patch.origin[0], patch.origin[1], patch.interior_columns, patch.interior_rows})
                    word(out, v);
                raw(out, std::span<const float>(patch.position));
            }
        }
        word(out, controls);
        for (const auto &palette : palettes)
            settings(out, terrain_river_settings(palette));
        require(std::filesystem::create_directories(output), "Cannot create setup output");
        write(output / "setup.bin", out);
        std::cout << "PASS complete terrain provider, factor kernel and cache layout\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
