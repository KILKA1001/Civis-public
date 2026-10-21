#include "civ5/terrain/terrain_occurrences.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace civ5::terrain {
TerrainGenerationSetup prepare_terrain_generation(const TerrainPieceLibrary &library,
                                                   const FactoryDescriptor &factory) {
    TerrainGenerationSetup result{build_terrain_orientations(factory), {}, 0};
    std::uint32_t maximum = 0;
    for (const auto &record : library.records()) {
        if (!(record.raw_state & 4u)) continue;
        // The native cooker copies the serialized ordinal unchanged; the later
        // descriptor lookup uses it as a primary-vector index. Do not silently
        // conflate different markup and source grids for an unsupported library.
        if (record.serialized_ordinal != record.ordinal)
            throw std::invalid_argument("Active terrain record has nonpositional primary ordinal");
        result.cooked_to_primary.push_back(record.ordinal);
        maximum = std::max(maximum, record.generation_pass); // native unsigned JBE
    }
    result.maximum_pass = std::bit_cast<std::int32_t>(maximum);
    return result;
}
std::uint16_t TerrainRandom::next() {
    state_ = state_ * 214013u + 2531011u;
    ++calls_;
    return static_cast<std::uint16_t>((state_ >> 16) & 0x7fff);
}
std::uint32_t TerrainRandom::index(std::uint32_t bound) {
    if (bound == 0) throw std::invalid_argument("Empty terrain random index range");
    std::uint32_t maximum = 0x7fff, value = next();
    while (bound > maximum && maximum != 0xffffffffu) {
        value = (value << 15) | next();
        maximum = (maximum << 15) | 0x7fff;
    }
    return value % bound;
}
namespace {
template<class T> void shuffle(std::vector<T> &values, TerrainRandom &random) {
    for (std::size_t i = 1; i < values.size(); ++i)
        std::swap(values[i], values[random.index(static_cast<std::uint32_t>(i + 1))]);
}
} // namespace
TerrainGeneration generate_terrain_occurrences(const TerrainPieceLibrary &library,
    const TerrainPlacement &placement, std::span<const std::uint32_t> mapping,
    std::int32_t width, std::int32_t height, std::span<const std::uint8_t> cells,
    std::int32_t maximum_pass, TerrainRandom &random, const TerrainPassObserver &observer) {
    if (mapping.size() > 4096 || maximum_pass > 255)
        throw std::invalid_argument("Unsupported terrain generation record/pass count");
    std::vector<const TerrainPieceRecord *> records;
    for (auto ordinal : mapping) records.push_back(&library.record(ordinal));
    const auto base_candidates = placement.candidate_positions(width, height);
    TerrainGeneration result{{}, TerrainLogicalClaims(width, height, cells)};
    for (auto pass = maximum_pass; pass >= 0; --pass) {
        auto candidates = base_candidates;
        std::vector<std::uint32_t> order(records.size()), orientations{0, 1, 2, 3};
        std::iota(order.begin(), order.end(), 0u);
        shuffle(candidates, random);
        for (const auto &position : candidates) {
            shuffle(order, random);
            shuffle(orientations, random);
            const std::array transforms{
                placement.logical_transform({0, 0, position}), placement.logical_transform({0, 1, position}),
                placement.logical_transform({0, 2, position}), placement.logical_transform({0, 3, position})};
            for (auto index : order) {
                const auto &record = *records[index];
                if (record.generation_pass != static_cast<std::uint32_t>(pass)) continue;
                for (auto orientation : orientations)
                    if (result.claims.try_claim(record, transforms[orientation]))
                        result.occurrences.push_back({index, orientation, position});
            }
        }
        if (observer) observer(pass, result.occurrences, result.claims, random);
    }
    return result;
}
} // namespace civ5::terrain
