// Input-only, pointer-free comparison transport. Legacy packet layout is
// serialized ONLY in this test executable, never used as the x64 runtime ABI.
#include "civ5/materials/lighting_sources.hpp"
#include "material_blend_fixture.hpp"
#include <iostream>
using namespace blend_validation;
namespace {
using generation_validation::blob;
using generation_validation::word;
template <std::size_t N> std::array<std::uint32_t, N> words(Reader &r) {
    std::array<std::uint32_t, N> result;
    for (auto &v : result)
        v = r.word();
    return result;
}
Bytes run(std::span<const std::uint8_t> data) {
    Reader r{data};
    const auto magic = r.take(8);
    require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5LGTI01",
            "Lighting input schema");
    Bytes out{'C', '5', 'L', 'G', 'T', 'O', '0', '1'};
    const auto sources = r.word();
    require(sources > 0 && sources <= 256, "Lighting source count");
    word(out, sources);
    for (unsigned i = 0; i < sources; ++i) {
        const auto supplied = words<11>(r);
        unsigned call = 0;
        const auto handles = resolve_lighting_handles([&](const LightingSource &s) {
            blob(out, {reinterpret_cast<const std::uint8_t *>(s.native_path.data()), s.native_path.size()});
            word(out, s.loader == LightingLoader::cube ? 1 : 0);
            return supplied[call++];
        });
        word(out, handles.spec_screen);
        for (auto v : handles.renderer_words())
            word(out, v);
    }
    const auto packets = r.word();
    require(packets > 0 && packets <= 256, "Lighting packet count");
    word(out, packets);
    for (unsigned i = 0; i < packets; ++i) {
        MaterialLightingResources p{};
        p.mask = r.word();
        p.normal = r.word();
        p.albedo = r.word();
        p.specular = r.word();
        p.shadow = r.word();
        p.sunlight = words<5>(r);
        p.shadowlight = words<5>(r);
        p.color_correct = words<5>(r);
        p.land_color_key = words<5>(r);
        p.global_texture9 = r.word();
        p.parent_texture11 = r.word();
        p.world_height = r.word();
        const auto draw = words<5>(r);
        const auto prior = r.take(100);
        Bytes packet(prior.begin(), prior.end());
        const auto b = material_lighting_bindings(p);
        const auto put = [&](unsigned at, std::uint32_t v, unsigned n) {
            for (unsigned j = 0; j < n; ++j)
                packet.at(at + j) = static_cast<std::uint8_t>(v >> (8 * j));
        };
        // Draw identifiers are explicit serialized fixture words, not addresses.
        put(4, draw[1], 4);
        put(8, draw[0], 4);
        put(12, draw[2], 4);
        put(16, draw[3], 4);
        put(20, draw[4], 4);
        for (unsigned j = 0; j < 12; ++j) {
            put(0x18 + 2 * j, b.pixel_textures[j], 2);
            put(0x38 + 2 * j, b.pixel_samplers[j], 2);
        }
        put(0x58, b.vertex_texture, 2);
        put(0x5c, b.vertex_sampler, 2);
        put(0x60, 12, 1);
        put(0x61, 1, 1);
        out.insert(out.end(), packet.begin(), packet.end());
    }
    r.end();
    return out;
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_lighting_source_oracle input fresh-output");
        require(!std::filesystem::exists(argv[2]), "Fresh lighting output required");
        const auto result = run(read(argv[1]));
        write(argv[2], result);
        std::cout << "PASS C++ lighting source and binding producers\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
