#pragma once
#include "civ5/saves/chunked_stream.hpp"

namespace civ5::saves {
struct InflateObservation {
    std::uint64_t compressed_bytes = 0, available_bytes = 0;
    bool stream_end = false;
    bool sync_flush_suffix = false;
};
// Observation of decompressed bytes, not permission to construct a game.
// The retail corpus ends at sync flush without an RFC zlib end/checksum.
// Only an independently validated native save codec can accept that boundary.
InflateObservation observe_inflated(runtime::BinaryReader &input, const ChunkedFrame &frame,
                                    const std::function<void(std::span<const std::uint8_t>)> &visitor,
                                    std::uint64_t maximum_output_bytes = 1024ull * 1024 * 1024);
} // namespace civ5::saves
