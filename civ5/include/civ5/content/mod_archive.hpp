#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace civ5::content {
struct ArchiveBudget {
    std::uint64_t maximum_entries = 1'000'000;
    std::uint64_t maximum_expanded_bytes = 8ULL * 1024 * 1024 * 1024;
    std::uint64_t maximum_decoder_memory = 1024ULL * 1024 * 1024;
};
struct ArchiveEntry {
    std::filesystem::path relative;
    std::uint64_t bytes = 0;
    bool directory = false;
};
struct ArchiveExtraction {
    std::vector<ArchiveEntry> entries;
    std::uint64_t expanded_bytes = 0, peak_decoder_memory = 0;
};
// Windows archive paths must be portable relative names, without links/devices.
std::filesystem::path archive_relative_path(const std::wstring &name);
void require_plain_directory(const std::filesystem::path &directory);
// destination must not already exist. Errors retain an uncommitted partial
// directory for diagnosis; callers must not expose it as installed content.
ArchiveExtraction extract_mod_7z(const std::filesystem::path &archive,
                                 const std::filesystem::path &destination, ArchiveBudget budget = {});
// ZIP and ZIP64 archives from mod authors use the same path and size policy.
ArchiveExtraction extract_mod_zip(const std::filesystem::path &archive,
                                  const std::filesystem::path &destination, ArchiveBudget budget = {});
} // namespace civ5::content
