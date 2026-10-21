#include "../validation/material_cache_rebuild_fixture.hpp"
#include <limits>
namespace rv = cache_rebuild_validation;
using namespace civ5::materials;
namespace {
rv::Bytes entering() {
    rv::Bytes bytes(1024, 0);
    for (const auto [offset, value] : {std::pair{0xd4u, 65u}, {0xd8u, 65u}, {0x138u, 30u},
             {0x168u, 17u}, {0xf4u, 13u}, {0xf8u, 19u}, {0x16cu, 529u}, {0x170u, 128u},
             {0x15cu, 11u}, {0x160u, 12u}, {0x318u, 0x28000000u}, {0x31cu, 0x29000000u},
             {0x190u, 0x12345678u}, {0x194u, 0x23456789u}, {0x198u, 0x3456789au}, {0x19cu, 0x456789abu},
             {0x1c4u, 0x12345678u}, {0x1c8u, 0x23456789u}, {0x314u, 4u}, {0x1e0u, 2u}})
        rv::put(bytes, offset, value);
    bytes[0x136] = 1;
    return bytes;
}
const MaterialCacheRebuildSettings normal{65, 65, 30, 0x76543210, 2.9999998f, 6, false};
const LandPacketTextures textures{{101, 102, 103, 104, 105, 106}, {201, 202, 203, 204, 205},
                                  {301, 302, 303, 304, 305}};
struct Lifecycle final : MaterialCacheLifecycleBackend {
    rv::Backend &resources;
    bool fail_upload = false;
    explicit Lifecycle(rv::Backend &b) : resources(b) {}
    void rebuild_geometry(MaterialCacheLifecycleState &state) override {
        if (fail_upload) throw std::runtime_error("Unavailable geometry owner");
        material_cache_rebuild_geometry(state, normal, textures, resources);
    }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override {
        throw std::logic_error("Unexpected generation in requested state4");
    }
    void release_geometry_pair(std::uint32_t, std::uint32_t) override { throw std::logic_error("Unexpected release"); }
    void release_vertex_buffer(std::uint32_t) override { throw std::logic_error("Unexpected release"); }
    void release_index_buffer(std::uint32_t) override { throw std::logic_error("Unexpected release"); }
    void release_packet(std::uint32_t handle) override { resources.release_packet(handle); }
    void free_staging(std::uintptr_t token) override { resources.free_staging(token); }
};
}
int main() {
    try {
        {
            rv::Case test(entering());
            rv::Backend backend(test, 37, 8192, 6);
            Lifecycle lifecycle(backend);
            MaterialCacheFreeLists pool;
            rv::require(material_cache_dispatch(test.state, pool, lifecycle, false, 123, 0), "Missing completion tail");
            rv::require(test.state.completed_state == 4 && test.state.pending_jobs == 1 &&
                            test.state.frame_stamp == 123 && backend.staging.empty(),
                        "Rebuild did not precede actual atomic completion");
            rv::require(backend.events.size() == 5 && backend.events[0].kind == 0 &&
                            backend.events[0].arguments[3] == 384 &&
                            rv::get(backend.events[1].patch, 0x318) != 0 &&
                            rv::get(backend.events[2].patch, 0x318) != 0 &&
                            rv::get(backend.events[2].patch, 0x31c) != 0 &&
                            rv::get(backend.events[3].patch, 0x318) == 0 &&
                            test.state.geometry.secondary_index_buffer == 19,
                        "Upload count or staging clear boundary changed");
            const auto &p = backend.packets.at(37);
            rv::require(p.geometry == std::array<std::uint32_t, 5>{73, 71, 128, 0, 0} &&
                            p.pixel_textures[0] == 0x789a && p.pixel_textures[6] == 202 && !p.vertex_count &&
                            test.state.geometry.detail_levels[0] == 0x12345678,
                        "CPU packet fields or unreachable DX9 detail state changed");
        }
        {
            rv::Case test(entering());
            rv::Backend backend(test, 37, 8192, 10);
            auto settings = normal;
            settings.capability = 10;
            settings.separate_geometry_buffers = true;
            material_cache_rebuild_geometry(test.state, settings, textures, backend);
            rv::require(backend.staging.size() == 2 && backend.events[2].arguments[0] == 85 &&
                            backend.events[3].arguments[0] == 83 && backend.packets.size() == 5 &&
                            backend.pending_releases == std::vector<std::uint32_t>{11} &&
                            test.state.geometry.detail_levels == std::array<std::uint32_t, 2>{2, 3},
                        "Shared retain/release order or floor changed");
            rv::require(backend.packets.at(37).vertex_samplers[0] == 9 &&
                            backend.packets.at(39).vertex_samplers[0] == 0 &&
                            backend.packets.at(39).geometry == std::array<std::uint32_t, 5>{} &&
                            backend.packets.at(41).pixel_textures[0] == 0 &&
                            backend.packets.at(41).pixel_textures[6] == 201,
                        "Additional packet invented geometry or inherited main style");
        }
        {
            rv::Case test(entering());
            rv::Backend backend(test, 37, 8192, 6);
            backend.on_packet = [&](unsigned slot) {
                if (!slot) {
                    test.state.geometry.triangle_count = 77;
                    test.state.textures.final[0] = 999;
                    test.state.textures.higher[0] = 555;
                }
            };
            material_cache_rebuild_geometry(test.state, normal, textures, backend);
            rv::require(backend.packets.at(37).geometry[2] == 77 &&
                            backend.packets.at(37).pixel_textures[0] == 0x789a &&
                            backend.packets.at(38).pixel_textures[0] == 555,
                        "Packet constructor lost live geometry or its captured texture arguments");
        }
        {
            rv::Case test(entering());
            rv::Backend backend(test, 37, 8192, 6);
            Lifecycle lifecycle(backend);
            lifecycle.fail_upload = true;
            const auto before = backend.snapshot();
            MaterialCacheFreeLists pool;
            bool failed = false;
            try { material_cache_dispatch(test.state, pool, lifecycle, false, 123, 0); }
            catch (const std::runtime_error &) { failed = true; }
            rv::require(failed && backend.snapshot() == before && backend.events.empty(),
                        "Failed rebuild published completion or released caller staging");
            for (unsigned invalid = 0; invalid < 4; ++invalid) {
                auto settings = normal;
                if (invalid == 0) settings.columns = 1;
                if (invalid == 1) settings.detail_level = std::numeric_limits<float>::quiet_NaN();
                if (invalid == 2) settings.detail_level = std::numeric_limits<float>::infinity();
                if (invalid == 3) { settings.capability = 10; settings.detail_level = -1; }
                bool rejected = false;
                try { material_cache_rebuild_geometry(test.state, settings, textures, backend); }
                catch (const std::invalid_argument &) { rejected = true; }
                rv::require(rejected && backend.snapshot() == before && backend.events.empty(),
                            "Invalid rebuild input changed resources");
            }
        }
        std::cout << "PASS geometry rebuild ownership, packet boundaries, floor, failure and atomic completion\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
