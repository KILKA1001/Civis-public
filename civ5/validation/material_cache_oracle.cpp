#include "civ5/materials/material_cache.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>

namespace {
using namespace civ5::assets;
using namespace civ5::materials;
using namespace generation_validation;
void textures(Bytes &output, const std::vector<MaterialCacheTexturePlan> &plans) {
    word(output, static_cast<std::uint32_t>(plans.size()));
    for (const auto &p : plans)
        for (auto n : {std::uint32_t(p.role), p.index, p.width, p.height, p.requested_levels,
                       std::uint32_t(p.format), p.native_usage, p.native_pool, p.native_accounted_bytes})
            word(output, n);
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        require(argc == 3, "Usage: material_cache_oracle input.bin fresh-output");
        const std::filesystem::path output = argv[2];
        require(!std::filesystem::exists(output), "Fresh material cache output required");
        const auto input = read(argv[1]);
        Reader r{input};
        const auto magic = r.take(8);
        require(std::string(magic.begin(), magic.end()) == "C5MCIN01", "Wrong material cache schema");
        Bytes out{'C', '5', 'M', 'C', 'O', 'U', '0', '1'};
        auto count = r.word();
        require(count <= 1024, "Too many material quality controls");
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            MaterialCacheQualityInput q;
            q.renderer_code = r.integer();
            q.detail = r.word();
            q.tessellation = r.word();
            q.compression = r.word();
            q.flags = r.word();
            for (auto &n : q.pool_counts)
                n = r.word();
            const auto s = normalize_material_cache_quality(q);
            for (auto n :
                 {s.temporary_size, s.cache_size, s.base_size, s.cache_mode, s.compression, s.tessellation})
                word(out, n);
            raw(out, std::span<const std::uint32_t>(s.pool_counts));
        }
        count = r.word();
        require(count <= 1024, "Too many material texture controls");
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto temporary = r.word(), cache = r.word(), compression = r.word();
            require(compression <= 1, "Invalid texture compression control");
            textures(out, material_cache_texture_plan(temporary, cache, compression != 0));
        }
        count = r.word();
        require(count <= 1024, "Too many material pool controls");
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            std::array<std::uint32_t, 5> counts;
            for (auto &n : counts)
                n = r.word();
            const auto compression = r.word(), size = r.word();
            require(compression <= 1, "Invalid pool compression control");
            textures(out, material_cache_pool_plan(counts, compression != 0, size));
        }
        count = r.word();
        require(count <= 16, "Too many jitter controls");
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto pc = r.word();
            require(pc == 24 || pc == 53, "Invalid material jitter precision");
            for (const auto &p :
                 material_shadow_jitter(pc == 24 ? civ5::terrain::X87Precision::single
                                                 : civ5::terrain::X87Precision::double_precision))
                raw(out, std::span<const float>(p));
        }
        r.end();
        require(std::filesystem::create_directories(output), "Cannot create material cache output");
        write(output / "resources.bin", out);
        std::cout << "PASS material cache quality, texture plans, pools and shadow jitter\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
