#include "civ5/materials/shared_land_geometry.hpp"
#include <map>
#include <stdexcept>
#include <iostream>
using namespace civ5::materials;
namespace {
void require(bool v) { if (!v) throw std::runtime_error("Shared geometry ownership test"); }
struct Backend final : SharedLandGeometryBackend {
    unsigned next = 1, calls = 0, fail = 0;
    bool null_failure = false;
    std::map<unsigned, std::vector<std::uint8_t>> buffers;
    std::uint32_t create_shared(MaterialGeometryBufferKind kind, std::uint32_t n,
                                std::span<const std::uint8_t> bytes) override {
        if (++calls == fail) {
            if (null_failure) return 0;
            throw std::runtime_error("Injected allocation failure");
        }
        require(bytes.size() == n * (kind == MaterialGeometryBufferKind::vertex ? 24u : 2u));
        buffers.emplace(next, std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
        return next++;
    }
    void discard_shared(MaterialGeometryBufferKind, std::uint32_t h) noexcept override { buffers.erase(h); }
};
}
int main() {
    try {
        for (bool null_failure : {false, true}) for (unsigned failure : {1u, 2u, 3u}) {
            Backend b; b.fail = failure; b.null_failure = null_failure; SharedLandGeometryCache cache;
            bool rejected = false;
            try { cache.get(b, 65, 65, 2, 10); } catch (const std::runtime_error &) { rejected = true; }
            require(rejected && b.buffers.empty() && cache.entries().empty());
            b.fail = 0;
            const auto value = cache.get(b, 65, 65, 2, 10);
            require(value.triangle_count == 2048 && value.control_buffer && b.buffers.size() == 3);
            const auto copies = b.buffers; const auto count = b.calls;
            const auto hit = cache.get(b, 65, 65, 0, 6);
            require(hit.index_buffer == value.index_buffer && hit.vertex_buffer == value.vertex_buffer &&
                    hit.control_buffer == value.control_buffer && hit.triangle_count == value.triangle_count &&
                    b.calls == count && b.buffers == copies && cache.entries().size() == 1);
            // Failed creation of another key must preserve the live cache entry.
            b.fail = b.calls + 2; rejected = false;
            try { cache.get(b, 7, 11, 3, 10); } catch (const std::runtime_error &) { rejected = true; }
            require(rejected && b.buffers == copies && cache.entries().size() == 1);
        }
        Backend b; SharedLandGeometryCache cache;
        for (auto dims : {std::pair{1u, 65u}, std::pair{65u, 66u}}) {
            bool rejected = false;
            try { cache.get(b, dims.first, dims.second, 1, 6); } catch (const std::invalid_argument &) { rejected = true; }
            require(rejected && b.calls == 0 && cache.entries().empty());
        }
        std::cout << "PASS shared geometry rollback, cache hits and ownership\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
