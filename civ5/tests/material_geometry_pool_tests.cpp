#include "civ5/materials/material_geometry_pool.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::materials;
namespace {
void require(bool v) { if (!v) throw std::runtime_error("Geometry pool invariant failed"); }
struct Backend final : MaterialGeometryBufferBackend {
    unsigned next = 1, writes = 0;
    std::size_t bytes = 0;
    bool fail = false;
    std::uint32_t create(MaterialGeometryBufferKind, std::uint32_t, std::span<const std::uint8_t>) override {
        if (fail) throw std::runtime_error("injected allocation failure");
        return next++;
    }
    void upload(MaterialGeometryBufferKind, std::uint32_t, std::span<const std::uint8_t> data) override {
        ++writes; bytes += data.size();
        if (fail) throw std::runtime_error("injected upload failure");
    }
};
}
int main() {
    try {
        Backend backend; MaterialGeometryPool pool;
        std::vector<std::uint8_t> vertices(32 * 24), indices(192 * 2);
        unsigned v = 0, i = 0;
        pool.upload_pair(backend, vertices, 32, indices, 192, v, i);
        require(v == 1 && i == 2);
        pool.release_pair(v, i);
        pool.upload_pair(backend, std::span(vertices).first(4 * 24), 4, std::span(indices).first(12 * 2), 12, v, i);
        require(v == 1 && i == 2 && backend.writes == 2 && backend.bytes == 120);
        require(pool.entries(MaterialGeometryBufferKind::vertex)[0].capacity == 32);
        pool.release_pair(0, 0); pool.release_pair(999, 998);
        require(pool.entries(MaterialGeometryBufferKind::vertex)[0].busy);
        bool rejected = false;
        try { pool.upload_pair(backend, vertices, 33, indices, 192, v, i); }
        catch (const std::invalid_argument &) { rejected = true; }
        require(rejected && backend.next == 3 && backend.writes == 2);
        pool.release_pair(v, i); backend.fail = true; v = i = 0;
        try { pool.upload_pair(backend, vertices, 32, indices, 192, v, i); }
        catch (const std::runtime_error &) {}
        require(v == 1 && i == 0 && pool.entries(MaterialGeometryBufferKind::vertex)[0].busy &&
                !pool.entries(MaterialGeometryBufferKind::index)[0].busy);
        pool.release_pair(v, i); backend.fail = false;
        pool.upload_pair(backend, vertices, 32, indices, 192, v, i);
        require(v == 1 && i == 2);
        std::cout << "PASS bounded geometry reuse, original capacity, failed upload and retry\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
