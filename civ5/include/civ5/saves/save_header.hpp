#pragma once
#include "civ5/runtime/binary_reader.hpp"
#include <array>
#include <filesystem>
#include <vector>

namespace civ5::saves {
struct DlcRequirement {
    std::array<std::uint8_t, 16> wire_id{};
    std::string id, name;
    std::uint32_t wire_value = 0;
    bool operator==(const DlcRequirement &) const = default;
};
struct ModRequirement {
    std::string id, name;
    // Preserved opaque until the saved-version codec is verified. This must
    // not silently become a manifest version or permit a version upgrade.
    std::uint32_t wire_value = 0;
    bool operator==(const ModRequirement &) const = default;
};
struct SaveHeader {
    std::uint32_t revision = 0;
    std::string build, build_id;
    std::uint64_t turn = 0, end_offset = 0;
    std::uint8_t mode = 0;
    std::string civilization, handicap, starting_era, current_era, speed, world_size, map_source;
    std::vector<DlcRequirement> dlcs;
    std::vector<ModRequirement> mods;
    bool operator==(const SaveHeader &) const = default;
};
// Configurable memory policies, not original game/32-bit engine limits.
struct HeaderBudget {
    std::uint64_t string_bytes = 1024 * 1024;
    std::uint64_t total_bytes = 64 * 1024 * 1024;
};
std::string windows_guid(std::span<const std::uint8_t, 16> bytes);
SaveHeader read_header(runtime::BinaryReader &input, HeaderBudget budget = {});
SaveHeader read_header(const std::filesystem::path &path, HeaderBudget budget = {});
// Validates only the parsed header against its retained original bytes.
// Bytes between that header and the compressed frame remain opaque here.
void validate_source_prefix(std::span<const std::uint8_t> prefix, const SaveHeader &header,
                            HeaderBudget budget = {});
} // namespace civ5::saves
