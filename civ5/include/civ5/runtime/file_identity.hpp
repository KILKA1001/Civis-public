#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace civ5::runtime {
struct FileIdentity {
    std::uint64_t bytes = 0;
    std::string sha256;
};
FileIdentity identify_file(const std::filesystem::path &path);
FileIdentity identify_bytes(std::span<const std::uint8_t> bytes);
// Civ V mod manifests declare MD5; SHA-256 remains the cache identity.
std::string file_md5(const std::filesystem::path &path);
// Hash the exact bytes about to be executed, avoiding a second file read.
std::string bytes_md5(std::span<const std::uint8_t> bytes);
} // namespace civ5::runtime
