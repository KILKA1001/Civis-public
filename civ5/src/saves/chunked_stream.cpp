#include "civ5/saves/chunked_stream.hpp"
#include <algorithm>
#include <array>
#include <optional>
#include <stdexcept>

namespace civ5::saves {
ChunkedFrame read_frame_at(runtime::BinaryReader &input, std::uint64_t offset) {
    input.seek(offset);
    ChunkedFrame result;
    result.offset = offset;
    for (;;) {
        const std::uint64_t bytes = input.u32();
        if (!bytes || bytes > 65536 || bytes > input.remaining())
            throw std::runtime_error("Invalid/truncated save compressed chunk");
        const auto data_offset = input.position();
        if (result.chunks.empty() && (bytes != 65536 || input.u8() != 0x78 || input.u8() != 0x9c))
            throw std::runtime_error("Unsupported initial save compressed chunk");
        result.chunks.push_back({data_offset, bytes});
        result.compressed_bytes += bytes;
        input.seek(data_offset + bytes);
        if (bytes < 65536) {
            if (input.remaining())
                throw std::runtime_error("Trailing data after save compressed frame");
            result.end = input.position();
            // Read through the chunk view: the suffix may straddle a chunk size field.
            std::array<std::uint8_t, 4> tail{};
            std::size_t required = 4;
            for (auto it = result.chunks.rbegin(); it != result.chunks.rend() && required; ++it) {
                const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(it->bytes, required));
                input.seek(it->data_offset + it->bytes - count);
                input.read(std::span(tail).subspan(required - count, count));
                required -= count;
            }
            result.sync_flush_suffix = !required && tail == std::array<std::uint8_t, 4>{0, 0, 255, 255};
            input.seek(result.end);
            return result;
        }
    }
}
ChunkedFrame find_frame(runtime::BinaryReader &input, std::uint64_t minimum_offset,
                        std::uint64_t search_budget) {
    input.seek(minimum_offset);
    const auto bytes = std::min(input.remaining(), search_budget);
    if (bytes < 6)
        throw std::runtime_error("No space for compressed save frame");
    // Scan in fixed windows with a five-byte overlap. File size never dictates
    // an allocation; chunk traversal and the cursor use 64-bit host offsets.
    std::array<std::uint8_t, 65536 + 5> buffer{};
    std::optional<ChunkedFrame> found;
    for (std::uint64_t window = 0; window + 6 <= bytes; window += 65536) {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(bytes - window, buffer.size()));
        input.seek(minimum_offset + window);
        input.read(std::span(buffer).first(count));
        const auto header = std::span(buffer).first(count);
        for (std::size_t i = 0; i <= header.size() - 6; ++i) {
            if (header[i] || header[i + 1] || header[i + 2] != 1 || header[i + 3] || header[i + 4] != 0x78 ||
                header[i + 5] != 0x9c)
                continue;
            std::optional<ChunkedFrame> candidate;
            try {
                candidate = read_frame_at(input, minimum_offset + window + i);
            } catch (const std::runtime_error &) {
                continue;
            }
            if (found)
                throw std::runtime_error("Ambiguous compressed save frame");
            found = std::move(candidate);
        }
    }
    if (!found)
        throw std::runtime_error("No complete supported compressed frame within search budget");
    return std::move(*found);
}
void visit_compressed_chunks(runtime::BinaryReader &input, const ChunkedFrame &frame,
                             const std::function<void(std::span<const std::uint8_t>)> &visitor) {
    // Revalidate framing before using a caller-provided frame or changed file.
    const auto actual = read_frame_at(input, frame.offset);
    if (actual.end != frame.end || actual.compressed_bytes != frame.compressed_bytes ||
        actual.chunks.size() != frame.chunks.size())
        throw std::runtime_error("Save frame changed");
    std::array<std::uint8_t, 65536> buffer{};
    for (const auto &chunk : actual.chunks) {
        input.seek(chunk.data_offset);
        const auto bytes = static_cast<std::size_t>(chunk.bytes);
        input.read(std::span(buffer).first(bytes));
        visitor(std::span(buffer).first(bytes));
    }
}
} // namespace civ5::saves
