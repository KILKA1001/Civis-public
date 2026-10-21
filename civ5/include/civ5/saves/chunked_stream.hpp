#pragma once
#include "civ5/runtime/binary_reader.hpp"
#include <functional>
#include <vector>

namespace civ5::saves {
struct CompressedChunk {
    std::uint64_t data_offset, bytes;
};
struct ChunkedFrame {
    std::uint64_t offset = 0, end = 0, compressed_bytes = 0;
    std::vector<CompressedChunk> chunks;
    bool sync_flush_suffix = false;
};
// Physical framing recovered from retail 403694. It does not validate zlib
// integrity or deserialize game state. A sync-flush suffix is not stream-end.
ChunkedFrame read_frame_at(runtime::BinaryReader &input, std::uint64_t offset);
ChunkedFrame find_frame(runtime::BinaryReader &input, std::uint64_t minimum_offset,
                        std::uint64_t search_budget = 64 * 1024 * 1024);
void visit_compressed_chunks(runtime::BinaryReader &input, const ChunkedFrame &frame,
                             const std::function<void(std::span<const std::uint8_t>)> &visitor);
} // namespace civ5::saves
