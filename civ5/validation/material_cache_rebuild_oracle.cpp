#include "material_cache_rebuild_fixture.hpp"
#include "material_generation_fixture.hpp"
#include <bit>
namespace rv = cache_rebuild_validation;
namespace io = generation_validation;
using namespace civ5::materials;
int main(int argc, char **argv) {
    try {
        io::require(argc == 3, "Rebuild primitive input and fresh output required");
        const auto input = io::read(argv[1]);
        io::Reader in{input};
        const auto magic = in.take(8);
        io::require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5GREBI1",
                    "Rebuild input schema");
        const auto count = in.word();
        io::require(count > 0 && count <= 10000, "Rebuild case count");
        io::Bytes out{'C', '5', 'G', 'R', 'E', 'B', 'O', '1'};
        io::word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto mode = in.word(), cap = in.word(), first = in.word(), triangles = in.word();
            const auto detail = std::bit_cast<float>(in.word());
            LandPacketTextures textures;
            for (auto &v : textures.shared) v = in.word();
            for (auto &v : textures.style_diffuse) v = in.word();
            for (auto &v : textures.style_specular) v = in.word();
            const auto before = in.take(1024);
            rv::Case test({before.begin(), before.end()});
            rv::Backend backend(test, first, triangles, cap);
            const MaterialCacheRebuildSettings settings{
                rv::get(test.entering, 0xd4), rv::get(test.entering, 0xd8), rv::get(test.entering, 0x138),
                rv::get(test.entering, 0xec), detail, std::bit_cast<std::int32_t>(cap), mode != 0};
            material_cache_rebuild_geometry(test.state, settings, textures, backend);
            io::blob(out, backend.snapshot());
            io::word(out, static_cast<std::uint32_t>(backend.events.size()));
            for (const auto &event : backend.events) {
                io::word(out, event.kind);
                io::word(out, static_cast<std::uint32_t>(event.arguments.size()));
                for (const auto argument : event.arguments) {
                    io::word(out, static_cast<std::uint32_t>(argument));
                    io::word(out, static_cast<std::uint32_t>(argument >> 32));
                }
                io::blob(out, event.patch);
            }
            io::word(out, static_cast<std::uint32_t>(backend.packets.size()));
            for (const auto &[handle, packet] : backend.packets) {
                io::word(out, handle);
                io::blob(out, rv::packet_bytes(packet));
            }
            for (const auto references : backend.buffer_references) io::word(out, references);
            io::word(out, static_cast<std::uint32_t>(backend.pending_releases.size()));
            for (auto handle : backend.pending_releases) io::word(out, handle);
            io::require(backend.staging.size() == (mode ? 2u : 0u), "Rebuild staging ownership");
        }
        in.end();
        io::write(argv[2], out);
        std::cout << "PASS native geometry rebuild and packets " << count << " cases\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
