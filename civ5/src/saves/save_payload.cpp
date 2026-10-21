#include "civ5/saves/save_payload.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>

namespace civ5::saves {
SavePayload read_save_payload(const std::filesystem::path &path, SavePayloadBudget budget) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open save payload");
    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    if (end < 0)
        throw std::runtime_error("Cannot size save payload");
    const auto length = static_cast<std::uint64_t>(end);
    if (length > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        throw std::runtime_error("Save payload exceeds stream offset range");
    runtime::BinaryReader reader(input, length);
    SavePayload result;
    result.header = read_header(reader, budget.header);
    result.frame = find_frame(reader, result.header.end_offset, budget.frame_search_bytes);
    if (result.frame.offset > budget.maximum_source_prefix_bytes || result.frame.offset > result.source_prefix.max_size())
        throw std::runtime_error("Original save prefix exceeds host byte budget");
    result.source_prefix.resize(static_cast<std::size_t>(result.frame.offset));
    reader.seek(0);
    reader.read(result.source_prefix);
    validate_source_prefix(result.source_prefix, result.header, budget.header);
    result.inflate = observe_inflated(
        reader, result.frame,
        [&](std::span<const std::uint8_t> bytes) {
            if (bytes.size() > budget.maximum_inflated_bytes - result.bytes.size())
                throw std::runtime_error("Inflated save exceeds payload budget");
            result.bytes.insert(result.bytes.end(), bytes.begin(), bytes.end());
        },
        budget.maximum_inflated_bytes);
    return result;
}
} // namespace civ5::saves
