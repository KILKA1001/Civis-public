#include "civ5/runtime/file_identity.hpp"
#include "civ5/platform/windows_path.hpp"
#define NOMINMAX
#include <windows.h>

#include <array>
#include <algorithm>
#include <bcrypt.h>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace civ5::runtime {
namespace {
void check(NTSTATUS status) {
    if (status < 0)
        throw std::runtime_error("File digest operation failed");
}
struct Algorithm {
    BCRYPT_ALG_HANDLE handle = nullptr;
    ~Algorithm() {
        if (handle)
            BCryptCloseAlgorithmProvider(handle, 0);
    }
};
struct Hash {
    BCRYPT_HASH_HANDLE handle = nullptr;
    ~Hash() {
        if (handle)
            BCryptDestroyHash(handle);
    }
};
struct Digest {
    std::uint64_t bytes = 0;
    std::string hex;
};
template<class Feed> Digest digest_stream(LPCWSTR name, std::size_t digest_size, Feed feed) {
    Algorithm algorithm;
    check(BCryptOpenAlgorithmProvider(&algorithm.handle, name, nullptr, 0));
    DWORD object_size = 0, returned = 0;
    check(BCryptGetProperty(algorithm.handle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size),
                            sizeof(object_size), &returned, 0));
    std::vector<UCHAR> object(object_size);
    Hash hash;
    check(BCryptCreateHash(algorithm.handle, &hash.handle, object.data(), object_size, nullptr, 0, 0));
    Digest result;
    feed([&](std::span<const std::uint8_t> bytes) {
        if (bytes.size() > UINT64_MAX - result.bytes)
            throw std::overflow_error("Digest byte count exceeds uint64");
        result.bytes += bytes.size();
        while (!bytes.empty()) {
            const auto count = std::min<std::size_t>(bytes.size(), 65536);
            check(BCryptHashData(hash.handle, const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(count), 0));
            bytes = bytes.subspan(count);
        }
    });
    std::vector<UCHAR> digest(digest_size);
    check(BCryptFinishHash(hash.handle, digest.data(), static_cast<ULONG>(digest.size()), 0));
    constexpr char hex[] = "0123456789abcdef";
    for (auto byte : digest) {
        result.hex += hex[byte >> 4];
        result.hex += hex[byte & 15];
    }
    return result;
}
Digest digest_file(const std::filesystem::path &path, LPCWSTR name, std::size_t digest_size) {
    std::ifstream stream(platform::extended_windows_path(path), std::ios::binary);
    if (!stream) {
        const auto name = path.generic_u8string();
        throw std::runtime_error("Cannot open file for digest: " +
                                 std::string(name.begin(), name.end()));
    }
    return digest_stream(name, digest_size, [&](auto consume) {
        std::array<std::uint8_t, 65536> buffer{};
        while (stream) {
            stream.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
            consume(std::span(buffer).first(static_cast<std::size_t>(stream.gcount())));
        }
        if (!stream.eof()) throw std::runtime_error("Read failed during file digest");
    });
}
} // namespace
FileIdentity identify_file(const std::filesystem::path &path) {
    const auto digest = digest_file(path, BCRYPT_SHA256_ALGORITHM, 32);
    return {digest.bytes, digest.hex};
}
FileIdentity identify_bytes(std::span<const std::uint8_t> bytes) {
    const auto digest = digest_stream(BCRYPT_SHA256_ALGORITHM, 32, [&](auto consume) { consume(bytes); });
    return {digest.bytes, digest.hex};
}
std::string file_md5(const std::filesystem::path &path) {
    return digest_file(path, BCRYPT_MD5_ALGORITHM, 16).hex;
}
std::string bytes_md5(std::span<const std::uint8_t> bytes) {
    return digest_stream(BCRYPT_MD5_ALGORITHM, 16, [&](auto consume) { consume(bytes); }).hex;
}
} // namespace civ5::runtime
