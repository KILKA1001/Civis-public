#include "civ5/materials/material_geometry_pool.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>
namespace io = generation_validation;
using namespace civ5::materials;
namespace {
struct Backend final : MaterialGeometryBufferBackend {
    MaterialGeometryPool pool;
    std::uint32_t vertex = 0, index = 0, next = 1;
    io::Bytes events;
    unsigned count = 0;
    io::Bytes snapshot() const {
        io::Bytes out;
        io::word(out, vertex); io::word(out, index);
        for (auto kind : {MaterialGeometryBufferKind::vertex, MaterialGeometryBufferKind::index}) {
            const auto entries = pool.entries(kind);
            io::word(out, static_cast<std::uint32_t>(entries.size()));
            for (const auto &e : entries) { io::word(out, e.handle); io::word(out, e.capacity); io::word(out, e.busy); }
        }
        return out;
    }
    void record(unsigned kind, unsigned handle, unsigned elements) {
        ++count; io::word(events, kind); io::word(events, handle); io::word(events, elements);
        const auto state = snapshot(); events.insert(events.end(), state.begin(), state.end());
    }
    std::uint32_t create(MaterialGeometryBufferKind kind, std::uint32_t elements,
                         std::span<const std::uint8_t> bytes) override {
        io::require(bytes.size() == std::size_t(elements) * (kind == MaterialGeometryBufferKind::vertex ? 24 : 2),
                    "Created geometry input extent");
        record(kind == MaterialGeometryBufferKind::vertex ? 0 : 1, next, elements);
        return next++;
    }
    void upload(MaterialGeometryBufferKind kind, std::uint32_t handle, std::span<const std::uint8_t>) override {
        record(kind == MaterialGeometryBufferKind::vertex ? 2 : 3, handle, 0);
    }
};
}
int main(int argc, char **argv) {
    try {
        io::require(argc == 3, "Pool input/output required");
        const auto data = io::read(argv[1]); io::Reader in{data};
        const auto magic = in.take(8);
        io::require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5GPOOLI", "Pool input schema");
        const auto cases = in.word(); io::require(cases <= 10000, "Pool case limit");
        io::Bytes out{'C','5','G','P','O','O','L','O'}; io::word(out, cases);
        for (unsigned c = 0; c < cases; ++c) {
            Backend backend; const auto count = in.word(); io::word(out, count);
            for (unsigned i = 0; i < count; ++i) {
                const auto op = in.word(), x = in.word(), y = in.word();
                if (op == 0) {
                    io::require(x <= 65535 && y <= 65535, "Pool fixture extent");
                    io::Bytes vb(x * 24, 0xa5), ib(y * 2, 0x5a);
                    backend.pool.upload_pair(backend, vb, x, ib, y, backend.vertex, backend.index);
                } else {
                    io::require(op == 1, "Pool operation"); backend.pool.release_pair(x, y);
                }
                const auto state = backend.snapshot(); out.insert(out.end(), state.begin(), state.end());
            }
            io::word(out, backend.count); out.insert(out.end(), backend.events.begin(), backend.events.end());
        }
        in.end(); io::write(argv[2], out);
        std::cout << "PASS geometry pool " << cases << " original sequences\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
