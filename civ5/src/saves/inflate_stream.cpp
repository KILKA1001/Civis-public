#include "civ5/saves/inflate_stream.hpp"
#include <array>
#include <stdexcept>
#include <zlib.h>

namespace civ5::saves {
InflateObservation observe_inflated(runtime::BinaryReader &input, const ChunkedFrame &frame,
                                    const std::function<void(std::span<const std::uint8_t>)> &visitor,
                                    std::uint64_t maximum_output_bytes) {
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK)
        throw std::runtime_error("Cannot initialize save inflater");
    struct End {
        z_stream *stream;
        ~End() { inflateEnd(stream); }
    } cleanup{&stream};
    InflateObservation result;
    result.sync_flush_suffix = frame.sync_flush_suffix;
    std::array<std::uint8_t, 65536> output{};
    visit_compressed_chunks(input, frame, [&](std::span<const std::uint8_t> chunk) {
        if (result.stream_end)
            throw std::runtime_error("Compressed data after zlib stream end");
        stream.next_in = const_cast<Bytef *>(chunk.data());
        stream.avail_in = static_cast<uInt>(chunk.size());
        do {
            stream.next_out = output.data();
            stream.avail_out = static_cast<uInt>(output.size());
            const auto before = stream.avail_in;
            const auto code = inflate(&stream, Z_NO_FLUSH);
            const std::uint64_t produced = output.size() - stream.avail_out;
            result.compressed_bytes += before - stream.avail_in;
            if (produced > maximum_output_bytes - result.available_bytes)
                throw std::runtime_error("Inflated save exceeds configured resource budget");
            result.available_bytes += produced;
            if (code != Z_OK && code != Z_BUF_ERROR && code != Z_STREAM_END)
                throw std::runtime_error("Invalid save zlib payload: " + std::to_string(code));
            if (produced)
                visitor(std::span(output).first(static_cast<std::size_t>(produced)));
            if (code == Z_STREAM_END) {
                if (stream.avail_in)
                    throw std::runtime_error("Trailing bytes after zlib stream end");
                result.stream_end = true;
                break;
            }
            if (!produced && before == stream.avail_in) {
                if (stream.avail_in)
                    throw std::runtime_error("Save inflater made no progress");
                break;
            }
        } while (stream.avail_in || !stream.avail_out);
    });
    return result;
}
} // namespace civ5::saves
