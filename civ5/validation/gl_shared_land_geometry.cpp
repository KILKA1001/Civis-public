#include "civ5/graphics/gl_land_geometry_pool.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>
namespace io = generation_validation;
using namespace civ5::graphics;
using Kind = civ5::materials::MaterialGeometryBufferKind;
int main(int argc, char **argv) {
    try {
        io::require(argc == 3, "Shared cache input/output required");
        const auto bytes = io::read(argv[1]); io::Reader in{bytes};
        const auto magic = in.take(8);
        io::require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5SHARIN", "Shared cache schema");
        auto context = std::make_shared<GlContext>();
        io::Bytes out{'C','5','S','H','A','R','O','U'};
        const auto groups = in.word(); io::require(groups <= 24, "Shared cache group limit"); io::word(out, groups);
        unsigned cases = 0, guards = 0;
        for (unsigned group = 0; group < groups; ++group) {
            auto pool = std::make_shared<GlLandGeometryPool>(context);
            const auto step = in.word(), capability = in.word(), count = in.word();
            io::require(count <= 64, "Shared cache record limit"); io::word(out, count);
            for (unsigned n = 0; n < count; ++n) {
                const auto columns = in.word(), rows = in.word();
                const auto first = pool->shared_grid(columns, rows, step, capability);
                const auto references = [&] {
                    for (auto handle : {first.vertex_buffer(), first.index_buffer(), first.control_buffer()})
                        io::word(out, pool->shared_references(handle));
                };
                references();
                const auto again = pool->shared_grid(columns, rows, 1 + step % 3, capability == 10 ? 6 : 10);
                io::require(first.vertex_buffer() == again.vertex_buffer() && first.index_buffer() == again.index_buffer() &&
                            first.control_buffer() == again.control_buffer() && first.index_count() == again.index_count(),
                            "Dimension-only cache hit changed geometry");
                references();
                pool->release(first); references();
                bool rejected = false;
                try { pool->buffers(first, *context); } catch (const std::invalid_argument &) { rejected = true; }
                io::require(rejected, "Returned shared lease accepted"); ++guards;
                pool->buffers(again, *context);
                for (const auto &data : {pool->read_buffer(again, Kind::vertex), pool->read_buffer(again, Kind::index),
                                         pool->read_control_buffer(again)}) {
                    io::word(out, static_cast<std::uint32_t>(data.size())); out.insert(out.end(), data.begin(), data.end());
                }
                pool->release(again); references();
                rejected = false;
                try { pool->release(again); } catch (const std::invalid_argument &) { rejected = true; }
                io::require(rejected, "Double shared release accepted"); ++guards;
                const auto entries = pool->shared_entries(); io::word(out, static_cast<std::uint32_t>(entries.size()));
                for (const auto &e : entries)
                    for (auto word : {e.columns, e.rows, e.triangle_count, e.vertex_buffer, e.index_buffer, e.control_buffer})
                        io::word(out, word);
                io::require(pool->entries(Kind::vertex).empty() && pool->entries(Kind::index).empty(),
                            "Shared geometry leaked into first-fit pool");
                ++cases;
            }
        }
        in.end(); io::write(argv[2], out);
        std::cout << "PASS actual GPU shared cache cases=" << cases << " rejectedLeases=" << guards << '\n';
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
