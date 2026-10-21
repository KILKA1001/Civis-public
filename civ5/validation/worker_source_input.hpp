#pragma once
#include "civ5/terrain/worker_field.hpp"

namespace civ5::validation {
struct WorkerCompositionSource {
    std::int32_t width, height;
    float bias;
    terrain::X87Precision precision;
    std::vector<terrain::TerrainDescriptor> descriptors;
};
// Shared validation-only source transport; no expected field or native pointer
// interpretation. Production only accepts the resulting typed owned grids.
template <class Reader, class Check>
std::vector<terrain::TerrainDescriptor> read_worker_sources(Reader &r, Check check) {
    using namespace terrain;
    const auto count = r.word();
    check(count <= 1024, "Too many source grids");
    std::vector<std::shared_ptr<const TerrainPieceGrids>> grids;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto size = r.integer();
        check(size == 128 || size == 512, "Invalid source size");
        const auto cells = std::size_t(size) * size;
        const auto h = r.take(cells * 2), m = r.take(cells), w = r.take(cells), p = r.take(cells);
        std::vector<std::uint16_t> heights(cells);
        for (std::size_t j = 0; j < cells; ++j)
            heights[j] = std::uint16_t(h[j * 2]) | std::uint16_t(h[j * 2 + 1]) << 8;
        grids.push_back(std::make_shared<const TerrainPieceGrids>(size, heights, m, w, p));
    }
    const auto descriptors = r.word();
    check(descriptors <= 65536, "Too many descriptors");
    check(std::size_t(descriptors) * 28 <= r.bytes.size() - r.at, "Truncated descriptors");
    std::vector<TerrainDescriptor> result;
    for (std::uint32_t i = 0; i < descriptors; ++i) {
        auto rectangle = r.rectangle();
        const auto flag = r.word(), gate = r.word(), source = r.word();
        check(gate <= 65535 && source < grids.size(), "Invalid descriptor gate/source");
        result.push_back({rectangle, flag, static_cast<std::uint16_t>(gate), grids[source]});
    }
    return result;
}
} // namespace civ5::validation
