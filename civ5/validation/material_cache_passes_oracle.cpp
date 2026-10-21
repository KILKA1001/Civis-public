#include "civ5/materials/material_cache_passes.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>

namespace {
using namespace civ5::materials;
using namespace generation_validation;
template <class T, std::size_t N> void read_array(Reader &r, std::array<T, N> &values) {
    for (auto &v : values)
        v = std::bit_cast<T>(r.word());
}
Bytes packet_bytes(const MaterialCachePacket &p) {
    Bytes result(100, 0);
    const auto write = [&]<class T>(unsigned at, T v) {
        for (unsigned n = 0; n < sizeof(T); ++n)
            result[at + n] = static_cast<std::uint8_t>(v >> (8 * n));
    };
    write(0, p.key);
    for (unsigned i = 0; i < 5; ++i)
        write(4 + 4 * i, p.geometry[i]);
    for (unsigned i = 0; i < 16; ++i) {
        write(0x18 + 2 * i, p.pixel_textures[i]);
        write(0x38 + 2 * i, p.pixel_samplers[i]);
    }
    for (unsigned i = 0; i < 2; ++i) {
        write(0x58 + 2 * i, p.vertex_textures[i]);
        write(0x5c + 2 * i, p.vertex_samplers[i]);
    }
    write(0x60, p.pixel_count);
    write(0x61, p.vertex_count);
    return result;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        require(argc == 3, "Usage: material_cache_passes_oracle input.bin fresh-output");
        const std::filesystem::path output = argv[2];
        require(!std::filesystem::exists(output), "Fresh cache pass output required");
        const auto input = read(argv[1]);
        Reader r{input};
        const auto magic = r.take(8);
        const std::string schema(magic.begin(),magic.end());
        const bool with_decals=schema=="C5CPIN02";
        require(with_decals || schema=="C5CPIN01", "Wrong cache pass input schema");
        MaterialCachePassSource source{};
        read_array(r, source.textures);
        for (auto &v : source.compression_textures)
            read_array(r, v);
        source.depth_surface = r.word();
        source.fallback_texture = r.word();
        source.normal_vertex_buffer = r.word();
        source.normal_index_buffer = r.word();
        source.lighting_vertex_buffer = r.word();
        source.material_vertex_buffer = r.word();
        source.patch_index_buffer = r.word();
        read_array(r, source.shader_bindings);
        read_array(r, source.lighting.sunlight);
        read_array(r, source.lighting.shadowlight);
        read_array(r, source.lighting.color_correct);
        read_array(r, source.lighting.land_color_key);
        source.lighting.global_texture9 = r.word();
        source.lighting.parent_texture11 = r.word();
        std::array<float, 64> scales;
        std::array<std::int32_t, 64> rows, modes;
        std::array<std::uint32_t, 64> flags, diffuse, specular, height;
        read_array(r, scales);
        read_array(r, rows);
        read_array(r, modes);
        read_array(r, flags);
        read_array(r, diffuse);
        read_array(r, specular);
        read_array(r, height);
        const auto minimum = r.floating(), maximum = r.floating();
        const auto count = r.word();
        require(count > 0 && count <= 1024, "Invalid cache pass case count");
        Bytes out{'C', '5', 'C', 'P', 'O', 'U', '0', '1'};
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            MaterialCachePassSettings s{};
            s.temporary_size = r.word();
            s.patch_size = r.word();
            s.shadow_samples = r.word();
            s.view = r.word();
            for (bool *value : {&s.compression, &s.cache_mode, &s.alternate, &s.specular}) {
                const auto v = r.word();
                require(v <= 1, "Invalid cache pass Boolean");
                *value = v != 0;
            }
            s.precision = pc(r);
            const auto prior = r.word();
            require(prior <= 255, "Invalid cache pass prior byte");
            MaterialCachePassPatch p{};
            p.columns = r.word();
            p.rows = r.word();
            read_array(r, p.world_rebase);
            p.continent_mask = r.word();
            p.world_height = r.word();
            p.alternate_height = r.word();
            read_array(r, p.higher_cache);
            read_array(r, p.final_cache);
            p.material_base_vertex = r.word();
            p.lighting_base_vertex = r.word();
            p.start_index = r.word();
            const auto selected = r.values<std::int32_t>(8);
            const MaterialCachePassPalette palette{selected, rows, modes, scales, flags, diffuse,
                                                    specular, height, minimum, maximum};
            std::optional<MaterialCacheDecalLayers> decals;
            if(with_decals) {
                const auto x=std::bit_cast<std::int32_t>(r.word()),y=std::bit_cast<std::int32_t>(r.word());
                require(x>=-800&&x<=800&&y>=-800&&y<=800,"Decal fixture origin outside provider");
                const auto state=r.word();require(state<=4,"Invalid decal fixture state");
                TerrainDecalStore store({-1024,-1024,1024,1024},8,8,37);
                for(unsigned j=0;j<(state==1?0u:6u);++j) {
                    const float px=x-5+static_cast<int>(j)*7,py=y-6+static_cast<int>(j)*5,w=20+j*5;
                    const civ5::terrain::TerrainDecalGeometry geometry{
                        {{px,py,0,0},{px+w,py,1,0},{px,py+24,0,1},{px+w,py+24,1,1}},
                        {2,1,0,2,3,1}};
                    store.submit({.125f*j,13+j,29+j,37},geometry,j%2);
                }
                struct Backend : TerrainDecalBufferBackend {
                    std::uint32_t count=100;
                    std::uint32_t create() override {return ++count;}
                    void upload(std::uint32_t,std::span<const std::array<std::uint16_t,4>>) override {}
                } backend;
                if(state!=2)store.upload_pending(backend);
                const std::uint32_t available=state==3?1003:131072;
                if(state!=4)decals=prepare_material_cache_decals(s,p,{x,y},store,77,143,{available,available});
            }
            const auto *layers=decals?&*decals:nullptr;
            const Bytes entering(material_cache_constant_bytes(s,layers), static_cast<std::uint8_t>(prior));
            std::array<float, 8> normal;
            normal.fill(std::bit_cast<float>(prior * 0x01010101u));
            const auto plan = material_cache_pass_plan(s, p, source, palette, entering, normal,layers);
            word(out, static_cast<std::uint32_t>(plan.passes.size()));
            for (const auto &pass : plan.passes) {
                word(out, std::uint32_t(pass.kind));
                raw(out, std::span<const std::uint32_t>(pass.targets));
                for (auto value : {pass.mip, pass.depth_surface, pass.blend_state, pass.depth_state, pass.flags})
                    word(out, value);
                raw(out, std::span<const float>(pass.clear_color));
                word(out, std::bit_cast<std::uint32_t>(pass.clear_depth));
                word(out, pass.copy_source);
                word(out, pass.copy_destination);
                word(out, static_cast<std::uint32_t>(pass.draws.size()));
                for (const auto &draw : pass.draws)
                    for (auto value : {draw.packet, draw.shader_binding, draw.selector, draw.constant_offset,
                                       draw.constant_bytes})
                        word(out, value);
            }
            word(out, static_cast<std::uint32_t>(plan.packets.size()));
            for (const auto &packet : plan.packets) {
                word(out, packet.key);
                const auto bytes = packet_bytes(packet);
                out.insert(out.end(), bytes.begin(), bytes.end());
            }
            blob(out, plan.constants);
            raw(out, std::span<const float>(plan.normal_constants));
        }
        r.end();
        require(std::filesystem::create_directories(output), "Cannot create cache pass output");
        write(output / "passes.bin", out);
        std::cout << "PASS " << count << " complete material cache pass plans\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
