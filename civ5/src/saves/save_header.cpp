#include "civ5/saves/save_header.hpp"
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace civ5::saves {
std::string windows_guid(std::span<const std::uint8_t, 16> bytes) {
    constexpr unsigned order[]{3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (unsigned i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            result += '-';
        result += hex[bytes[order[i]] >> 4];
        result += hex[bytes[order[i]] & 15];
    }
    return result;
}
SaveHeader read_header(runtime::BinaryReader &input, HeaderBudget budget) {
    const auto start = input.position();
    const auto available = [&]() {
        const auto used = input.position() - start;
        if (used > budget.total_bytes)
            throw std::runtime_error("Save header exceeds resource budget");
        return budget.total_bytes - used;
    };
    const auto string = [&]() {
        if (available() < 4)
            throw std::runtime_error("Save header exceeds resource budget");
        return input.string32(std::min(budget.string_bytes, available() - 4));
    };
    std::array<std::uint8_t, 4> magic{};
    input.read(magic);
    if (magic != std::array<std::uint8_t, 4>{'C', 'I', 'V', '5'})
        throw std::runtime_error("Missing CIV5 signature");
    SaveHeader result;
    result.revision = input.u32();
    if (result.revision != 8)
        throw std::runtime_error("Unsupported save header revision " + std::to_string(result.revision));
    result.build = string();
    result.build_id = string();
    if (result.build != "1.0.3.279 (403694)" || result.build_id != "403694")
        throw std::runtime_error("Unverified save build: " + result.build);
    result.turn = input.u32();
    result.mode = input.u8();
    if (result.mode != 0)
        throw std::runtime_error("Unverified multiplayer/hotseat header layout");
    result.civilization = string();
    result.handicap = string();
    result.starting_era = string();
    result.current_era = string();
    result.speed = string();
    result.world_size = string();
    result.map_source = string();
    const std::uint64_t dlc_count = input.u32();
    if (dlc_count > std::min(available(), input.remaining()) / 24)
        throw std::runtime_error("Invalid DLC count");
    std::set<std::string> ids;
    for (std::uint64_t i = 0; i < dlc_count; ++i) {
        DlcRequirement record;
        input.read(record.wire_id);
        record.id = windows_guid(record.wire_id);
        record.wire_value = input.u32();
        record.name = string();
        if (!ids.insert(record.id).second)
            throw std::runtime_error("Duplicate saved DLC identity");
        result.dlcs.push_back(std::move(record));
    }
    const std::uint64_t mod_count = input.u32();
    if (mod_count > std::min(available(), input.remaining()) / 12)
        throw std::runtime_error("Invalid mod count");
    ids.clear();
    for (std::uint64_t i = 0; i < mod_count; ++i) {
        ModRequirement record;
        record.id = string();
        record.wire_value = input.u32();
        record.name = string();
        if (record.id.empty() || !ids.insert(runtime::ascii_lower(record.id)).second)
            throw std::runtime_error("Empty or duplicate saved mod identity");
        result.mods.push_back(std::move(record));
    }
    available();
    result.end_offset = input.position();
    return result;
}
SaveHeader read_header(const std::filesystem::path &path, HeaderBudget budget) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("Cannot open save");
    runtime::BinaryReader reader(stream, std::filesystem::file_size(path));
    return read_header(reader, budget);
}
void validate_source_prefix(std::span<const std::uint8_t> prefix, const SaveHeader &header, HeaderBudget budget) {
    if (header.end_offset == 0 || header.end_offset > prefix.size() || header.end_offset > budget.total_bytes)
        throw std::runtime_error("Original prefix has no complete header within budget");
    const std::string bytes(reinterpret_cast<const char *>(prefix.data()), static_cast<std::size_t>(header.end_offset));
    std::istringstream stream(bytes, std::ios::binary);
    runtime::BinaryReader reader(stream, bytes.size());
    if (read_header(reader, budget) != header)
        throw std::runtime_error("Original prefix disagrees with saved metadata");
}
} // namespace civ5::saves
