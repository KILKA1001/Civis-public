#pragma once
#include "civ5/saves/inflate_stream.hpp"
#include "civ5/saves/save_header.hpp"

namespace civ5::saves {
// Host-side limits for importing one save. These limits protect the x64
// importer and are independent of the original engine's 32-bit capacities.
struct SavePayloadBudget {
    HeaderBudget header;
    std::uint64_t frame_search_bytes = 64 * 1024 * 1024;
    std::uint64_t maximum_inflated_bytes = 1024ull * 1024 * 1024;
    std::uint64_t maximum_source_prefix_bytes = 128ULL * 1024 * 1024;
};
struct SavePayload {
    SaveHeader header;
    ChunkedFrame frame;
    InflateObservation inflate;
    std::vector<std::uint8_t> bytes;
    // All original bytes [0, frame.offset), including the visible header,
    // engine metadata and CvPreGame archive. No unknown setup byte is dropped.
    std::vector<std::uint8_t> source_prefix;
};
// Read one save without executing any original load hooks. The returned bytes
// are the decompressed game-state payload and can be replayed by logic and
// graphics consumers through their own 64-bit BinaryReader instances.
SavePayload read_save_payload(const std::filesystem::path &path, SavePayloadBudget budget = {});
} // namespace civ5::saves
