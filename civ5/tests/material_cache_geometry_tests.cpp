#include "../validation/material_cache_geometry_fixture.hpp"
#include <cstring>
#include <iostream>
#include <limits>

using namespace cache_geometry_validation;
namespace {
const MaterialCacheGeometryDomain domain{{65, 65, 3}, {0, 0}, 80, 144, {1, .866f, .2f},
                                         civ5::terrain::X87Precision::double_precision, true};
bool filled(std::span<const std::uint8_t> bytes, std::uint8_t value) {
    return std::all_of(bytes.begin(), bytes.end(), [value](auto b) { return b == value; });
}
void prepare(Case &test, Backend &backend, const MaterialCacheGeometryDomain &d = domain) {
    observe_staging(backend);
    material_cache_prepare(test.state, d.patch, backend, false, 0x1234567812345678ull);
}
} // namespace
int main() {
    try {
        for (unsigned failure = 0; failure < 4; ++failure) {
            Case test(entering(65, 65));
            Backend backend(test);
            backend.failure_mask = failure;
            test.state.pending_jobs.store(2);
            test.state.frame_stamp = 7;
            prepare(test, backend);
            require(test.state.geometry.vertex_count == 529 && test.state.geometry.triangle_count == 968,
                    "Geometry preparation counters changed");
            MaterialCacheFreeLists pool;
            if (failure) {
                require(!test.state.prepared && !backend.queued && backend.allocations.empty(),
                        "Failed allocation submitted geometry");
                require(!material_cache_dispatch(test.state, pool, backend, false, 8, 0) &&
                            test.state.pending_jobs == 2 && test.state.frame_stamp == 7,
                        "Failed geometry preparation reached completion");
                continue;
            }
            unsigned calls = 0;
            material_cache_execute_geometry_job(*backend.queued, domain, backend, [&](int x, int y) {
                require(test.state.geometry.vertex_count == 529 && test.state.geometry.triangle_count == 968,
                        "Geometry count changed at the wrong height callback boundary");
                ++calls;
                return static_cast<std::uint16_t>(x * 17 + y * 31);
            });
            require(calls == 529 && test.state.geometry.triangle_count == 128,
                    "Geometry worker lost the original second index step");
            const auto vertices = backend.staging_memory(test.state.staging[0]);
            for (std::size_t at = 0; at < vertices.size(); at += 24)
                require(filled(vertices.subspan(at + 12, 8), 0xa5), "CPU geometry wrote normal lanes");
            require(filled(backend.staging_memory(test.state.staging[1]).subspan(768), 0xa5),
                    "CPU geometry wrote the unused index capacity");
            require(material_cache_dispatch(test.state, pool, backend, false, 8, 0) &&
                        test.state.completed_state == 3 && test.state.pending_jobs == 1 &&
                        test.state.frame_stamp == 8 && backend.events.back().kind == 0 &&
                        get(backend.events.back().patch, 0x170) == 128,
                    "Completion did not receive the actual geometry triangle count");
            material_cache_clear_staging(test.state, backend);
            require(backend.allocations.empty(), "Geometry cleanup leaked staging");
        }
        {
            auto d = domain;
            d.patch.geometry_stride = 1;
            Case test(entering(65, 65));
            Backend backend(test);
            prepare(test, backend, d);
            const auto before = backend.snapshot();
            unsigned calls = 0;
            bool rejected = false;
            try {
                material_cache_execute_geometry_job(*backend.queued, d, backend, [&](int, int) {
                    ++calls;
                    return std::uint16_t{0};
                });
            } catch (const std::length_error &) {
                rejected = true;
            }
            require(rejected && !calls && backend.snapshot() == before &&
                        filled(backend.staging_memory(test.state.staging[0]), 0xa5) &&
                        filled(backend.staging_memory(test.state.staging[1]), 0xa5),
                    "Inconsistent original step-1 capacity wrote beyond its boundary");
            material_cache_clear_staging(test.state, backend);
        }
        {
            Case test(entering(65, 65));
            Backend backend(test);
            prepare(test, backend);
            std::swap(test.state.staging[0], test.state.staging[1]);
            test.state.geometry.vertex_count = 17;
            material_cache_execute_geometry_job(*backend.queued, domain, backend, [&](int, int) {
                require(test.state.geometry.vertex_count == 529, "Worker retained stale vertex count");
                return std::uint16_t{0};
            });
            std::uint16_t first_index;
            const auto indices = backend.staging_memory(test.state.staging[1]);
            std::memcpy(&first_index, indices.data(), 2);
            require(first_index == 72 && filled(backend.staging_memory(test.state.staging[0]).subspan(12, 8), 0xa5),
                    "Geometry worker used captured rather than live owner staging");
            material_cache_clear_staging(test.state, backend);
        }
        {
            Case test(entering(65, 65));
            Backend backend(test);
            prepare(test, backend);
            unsigned calls = 0;
            bool failed = false;
            try {
                material_cache_execute_geometry_job(*backend.queued, domain, backend, [&](int, int) {
                    if (++calls == 2)
                        throw std::runtime_error("Height owner unavailable");
                    return std::uint16_t{1};
                });
            } catch (const std::runtime_error &error) {
                failed = std::string_view(error.what()) == "Height owner unavailable";
            }
            require(failed && calls == 2 && test.state.geometry.triangle_count == 968 &&
                        filled(backend.staging_memory(test.state.staging[1]), 0xa5),
                    "Failed vertex production published index data or completion count");
            material_cache_clear_staging(test.state, backend);
            require(backend.allocations.empty(), "Failed geometry job lost allocation ownership");
        }
        {
            Case test(entering(65, 65));
            Backend backend(test);
            prepare(test, backend);
            const auto before = backend.snapshot();
            for (unsigned invalid = 0; invalid < 4; ++invalid) {
                auto d = domain;
                if (invalid == 0) d.scales[0] = std::numeric_limits<float>::quiet_NaN();
                if (invalid == 1) d.origin[1] = 100;
                if (invalid == 2) d.patch.geometry_stride = 0;
                if (invalid == 3) d.origin[0] = -8001;
                bool rejected = false;
                try {
                    material_cache_execute_geometry_job(*backend.queued, d, backend,
                                                         [](int, int) { return std::uint16_t{0}; });
                } catch (const std::invalid_argument &) { rejected = true; }
                require(rejected && backend.snapshot() == before &&
                            filled(backend.staging_memory(test.state.staging[0]), 0xa5),
                        "Invalid geometry input changed staging or owner state");
            }
            material_cache_clear_staging(test.state, backend);
        }
        std::cout << "PASS CPU geometry ownership, counts, capacity, untouched lanes, failures and completion\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
