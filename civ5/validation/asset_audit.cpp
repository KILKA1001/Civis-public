#include "civ5/assets/dds.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// bcrypt.h requires the Win32 types above (including in llvm-mingw).
#include <bcrypt.h>

using namespace civ5::assets;
namespace fs = std::filesystem;
namespace {
fs::path utf8_path(const std::string &name) { return fs::path(std::u8string(name.begin(), name.end())); }
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
class Sha256 {
  public:
    Sha256() {
        try {
            check(BCryptOpenAlgorithmProvider(&algorithm_, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0,
                  "SHA provider failed");
            DWORD size = 0, returned = 0;
            check(BCryptGetProperty(algorithm_, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&size),
                                    sizeof(size), &returned, 0) >= 0,
                  "SHA object size failed");
            object_.resize(size);
            check(BCryptCreateHash(algorithm_, &hash_, object_.data(), size, nullptr, 0, 0) >= 0,
                  "SHA create failed");
        } catch (...) {
            if (hash_)
                BCryptDestroyHash(hash_);
            if (algorithm_)
                BCryptCloseAlgorithmProvider(algorithm_, 0);
            throw;
        }
    }
    ~Sha256() {
        if (hash_)
            BCryptDestroyHash(hash_);
        if (algorithm_)
            BCryptCloseAlgorithmProvider(algorithm_, 0);
    }
    Sha256(const Sha256 &) = delete;
    Sha256 &operator=(const Sha256 &) = delete;
    void add(std::span<const std::uint8_t> data) {
        check(data.size() <= 0xffffffffu, "Hash input chunk too large");
        check(BCryptHashData(hash_, const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), 0) >= 0,
              "SHA update failed");
    }
    std::array<std::uint8_t, 32> finish() {
        std::array<std::uint8_t, 32> result{};
        check(BCryptFinishHash(hash_, result.data(), static_cast<ULONG>(result.size()), 0) >= 0,
              "SHA finish failed");
        return result;
    }

  private:
    BCRYPT_ALG_HANDLE algorithm_ = nullptr;
    BCRYPT_HASH_HANDLE hash_ = nullptr;
    Bytes object_;
};
auto hash(std::span<const std::uint8_t> bytes) {
    Sha256 h;
    h.add(bytes);
    return h.finish();
}
auto hash(AssetStream stream) {
    Sha256 h;
    // Small entries must not zero a full MiB each (30,557 entries in the corpus).
    Bytes buffer(static_cast<std::size_t>(std::min<std::uint64_t>(stream.remaining(), 1024 * 1024)));
    while (stream.remaining()) {
        const auto count = stream.read(buffer);
        h.add(std::span{buffer}.first(count));
    }
    return h.finish();
}
struct Writer {
    std::ofstream output;
    explicit Writer(const fs::path &path) : output(path, std::ios::binary) {
        check(bool(output), "Output open failed");
    }
    void raw(std::span<const std::uint8_t> bytes) {
        output.write(reinterpret_cast<const char *>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        check(bool(output), "Output write failed");
    }
    void word(std::uint32_t v) {
        const std::array<std::uint8_t, 4> b{static_cast<std::uint8_t>(v), static_cast<std::uint8_t>(v >> 8),
                                            static_cast<std::uint8_t>(v >> 16),
                                            static_cast<std::uint8_t>(v >> 24)};
        raw(b);
    }
    void wide(std::uint64_t v) {
        word(static_cast<std::uint32_t>(v));
        word(static_cast<std::uint32_t>(v >> 32));
    }
    void text(const std::string &v) {
        check(v.size() <= 0xffffffffu, "Oversized label");
        word(static_cast<std::uint32_t>(v.size()));
        raw({reinterpret_cast<const std::uint8_t *>(v.data()), v.size()});
    }
};
std::uint64_t dds_supported = 0, dds_rejected = 0, dds_levels = 0, dds_payload_bytes = 0;
void dds(Writer &out, const AssetHandle &handle) {
    auto bytes = handle.read_bytes();
    std::optional<DdsData> parsed;
    try {
        parsed = DdsData::parse(std::move(bytes), NumericDdsProfile::legacy_d3d9,
                                DdsHeaderProfile::civ5_d3dx_size24);
    } catch (const std::invalid_argument &) {
        ++dds_rejected;
        out.word(0);
        return;
    }
    const auto &data = *parsed;
    ++dds_supported;
    out.word(1);
    for (auto v : {data.width(), data.height(), data.face_count(), data.mip_count(), data.bits_per_pixel(),
                   data.fourcc(), static_cast<std::uint32_t>(data.format())})
        out.word(v);
    for (auto mask : data.channel_masks())
        out.word(mask);
    out.word(static_cast<std::uint32_t>(data.levels().size()));
    for (const auto &level : data.levels()) {
        for (auto v : {level.face, level.mip, level.width, level.height})
            out.word(v);
        out.wide(level.offset);
        out.wide(level.length);
        out.wide(level.row_pitch);
        out.raw(hash(data.payload(level.face, level.mip)));
        ++dds_levels;
        dds_payload_bytes += level.length;
    }
}
bool is_dds(const std::string &name) { return name.size() >= 4 && name.substr(name.size() - 4) == ".dds"; }
AssetHandle found(AssetLookup result) {
    check(std::holds_alternative<AssetHandle>(result), "Real-installation exact lookup missing/ambiguous");
    return std::get<AssetHandle>(std::move(result));
}
void audit(const fs::path &root, const fs::path &output) {
    const AssetEnvironment environment(root);
    auto discovery = environment.discover_fpk_sources();
    check(discovery.unresolved.empty(), "Unresolved archive in owned corpus");
    Writer out(output);
    out.text("C5ASSET01");
    out.word(static_cast<std::uint32_t>(discovery.supported.size()));
    std::uint64_t entries = 0, payload_bytes = 0;
    for (const auto &source : discovery.supported) {
        const auto archive = source->provenance().source_id;
        const auto path = root / utf8_path(archive);
        out.text(archive);
        out.wide(fs::file_size(path));
        out.raw(hash(AssetStream(path, 0, fs::file_size(path))));
        out.word(source->filename_scrambling());
        out.word(source->extra_header_byte());
        out.wide(source->directory_end());
        out.word(static_cast<std::uint32_t>(source->entries().size()));
        for (const auto &e : source->entries()) {
            out.word(e.ordinal);
            out.text(e.original_name);
            out.text(e.normalized_name);
            out.wide(e.directory_record_offset);
            out.wide(e.metadata_offset);
            out.word(e.padding_length);
            out.word(e.file_time_raw);
            out.word(e.unknown_raw);
            out.wide(e.payload_offset);
            out.wide(e.payload_length);
            out.word(e.direct_metadata_was_valid);
            const auto handle = found(source->find(AssetKey(e.original_name)));
            check(handle.candidate().payload_size == e.payload_length &&
                      handle.candidate().original_name == e.original_name &&
                      handle.candidate().provenance.source_id == archive &&
                      handle.candidate().provenance.archive_relative_path == archive,
                  "FPK handle provenance");
            out.raw(hash(handle.open_stream()));
            out.word(is_dds(e.normalized_name));
            if (is_dds(e.normalized_name))
                dds(out, handle);
            ++entries;
            payload_bytes += e.payload_length;
        }
        std::cout << "PASS raw FPK " << archive << " / " << source->entries().size() << " entries\n";
    }
    std::vector<std::string> loose;
    for (const auto &entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_symlink() || !entry.is_regular_file())
            continue;
        const auto name = path_utf8(fs::relative(entry.path(), root));
        if (is_dds(normalize_asset_name(name)))
            loose.push_back(name);
    }
    std::sort(loose.begin(), loose.end());
    const auto source = environment.loose_source();
    out.word(static_cast<std::uint32_t>(loose.size()));
    for (const auto &name : loose) {
        const auto handle = found(source->find(AssetKey(name)));
        out.text(name);
        out.wide(handle.candidate().payload_size);
        out.raw(hash(handle.open_stream()));
        dds(out, handle);
    }
    out.output.close();
    check(bool(out.output), "Final audit write failed");
    std::cout << "PASS 64-bit raw asset audit: archives=" << discovery.supported.size()
              << " entries=" << entries << " rawPayloadBytes=" << payload_bytes
              << " looseDDS=" << loose.size() << " DDSsupported=" << dds_supported
              << " DDSrejected=" << dds_rejected << " authoredLevels=" << dds_levels
              << " levelBytes=" << dds_payload_bytes << '\n';
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 3, "Usage: civ5_asset_audit asset-root fresh-output.bin");
        auto root = fs::canonical(utf8_path(argv[1]));
        auto output = utf8_path(argv[2]);
        check(!fs::exists(output), "Refusing output overwrite");
        audit(root, output);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
