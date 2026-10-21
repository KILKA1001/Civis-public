#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace civ5::assets {
static_assert(sizeof(void *) == 8, "Civ V assets require the 64-bit engine");
using Bytes = std::vector<std::uint8_t>;
std::string path_utf8(const std::filesystem::path &path);
std::string normalize_asset_name(const std::string &name);
class AssetKey {
  public:
    explicit AssetKey(std::string name);
    const std::string &original_name() const { return original_name_; }
    const std::string &normalized_name() const { return normalized_name_; }
    bool safe_relative_path() const { return safe_relative_path_; }

  private:
    std::string original_name_, normalized_name_;
    bool safe_relative_path_;
};
enum class SourceKind { loose, fpk };
struct Provenance {
    SourceKind kind;
    std::string source_id, source_relative_path;
    std::optional<std::string> archive_relative_path;
};
struct AssetCandidate {
    std::string original_name, normalized_name;
    Provenance provenance;
    std::uint64_t payload_size;
};
// Fresh, bounded, seek-based stream: no archive preload or extraction. Offsets
// and lengths are 64-bit even though v6 archive fields are serialized U32.
class CivisResourceBankOwner;
class AssetStream {
  public:
    AssetStream(const std::filesystem::path &file, std::uint64_t offset, std::uint64_t length);
    std::size_t read(std::span<std::uint8_t> destination);
    std::uint64_t remaining() const { return remaining_; }

  private:
    friend class AssetHandle;
    explicit AssetStream(std::shared_ptr<const Bytes> memory);
    std::ifstream input_;
    std::shared_ptr<const Bytes> memory_;
    std::uint64_t remaining_;
};
class AssetHandle {
  public:
    const AssetKey &requested_key() const { return requested_key_; }
    const AssetCandidate &candidate() const { return candidate_; }
    AssetStream open_stream() const;
    // Callers may impose a memory budget; materialization has no inherited
    // signed-32-bit array ceiling. Large resources should use open_stream().
    Bytes read_bytes(std::uint64_t maximum = std::numeric_limits<std::size_t>::max()) const;

  private:
    friend class FpkSource;
    friend class LooseSource;
    friend class LooseResourceIndex;
    friend class CivisResourceBankOwner;
    AssetHandle(AssetKey key, AssetCandidate candidate, std::filesystem::path file, std::uint64_t offset,
                std::optional<std::filesystem::path> root);
    // Only the own Civis bank can publish its already verified immutable bytes.
    // This constructor grants no Original bank/activation capability.
    AssetHandle(AssetKey key, AssetCandidate candidate, std::shared_ptr<const Bytes> memory);
    AssetKey requested_key_;
    AssetCandidate candidate_;
    std::filesystem::path file_;
    std::uint64_t offset_;
    std::optional<std::filesystem::path> loose_root_;
    std::shared_ptr<const Bytes> memory_;
};
struct AmbiguousAsset {
    AssetKey requested_key;
    Provenance source;
    std::vector<AssetCandidate> matches;
};
using AssetLookup = std::variant<std::monostate, AssetHandle, AmbiguousAsset>;
class AssetSource {
  public:
    virtual ~AssetSource() = default;
    virtual AssetLookup find(const AssetKey &key) const = 0;
    // TextData has a native all-matching branch. Sources that do not expose
    // duplicates retain the ordinary single-result behavior by default.
    virtual std::vector<AssetHandle> find_all(const AssetKey &key) const;
    // Enumerate logical names below an optional normalized prefix. Sources
    // without an exposed index return an empty list.
    virtual std::vector<std::string> keys(std::string_view prefix = {}) const;
    virtual Provenance provenance() const = 0;
};
class AssetResolver {
  public:
    explicit AssetResolver(std::vector<std::shared_ptr<const AssetSource>> sources);
    AssetLookup resolve(const AssetKey &key) const;

  private:
    std::vector<std::shared_ptr<const AssetSource>> sources_;
};
struct FpkEntry {
    std::uint32_t ordinal;
    std::string original_name, normalized_name;
    std::uint64_t directory_record_offset, metadata_offset;
    std::uint32_t padding_length, file_time_raw, unknown_raw;
    std::uint64_t payload_offset, payload_length;
    bool direct_metadata_was_valid;
};
struct FpkDiagnostic {
    std::string archive_relative_path, reason;
};
class FpkSource;
using FpkOpenResult = std::variant<std::shared_ptr<FpkSource>, FpkDiagnostic>;
class FpkSource final : public AssetSource {
  public:
    static FpkOpenResult open(const std::filesystem::path &archive, const std::string &relative_path);
    AssetLookup find(const AssetKey &key) const override;
    std::vector<AssetHandle> find_all(const AssetKey &key) const override;
    std::vector<std::string> keys(std::string_view prefix = {}) const override;
    Provenance provenance() const override;
    const std::vector<FpkEntry> &entries() const { return entries_; }
    std::uint32_t filename_scrambling() const { return scrambling_; }
    std::uint32_t extra_header_byte() const { return extra_; }
    std::uint64_t directory_end() const { return directory_end_; }

  private:
    FpkSource() = default;
    std::filesystem::path archive_;
    std::string relative_path_;
    std::uint32_t scrambling_ = 0, extra_ = 0;
    std::uint64_t directory_end_ = 0;
    std::vector<FpkEntry> entries_;
    std::unordered_map<std::string, std::vector<std::size_t>> index_;
};
class LooseSource final : public AssetSource {
  public:
    explicit LooseSource(const std::filesystem::path &root);
    AssetLookup find(const AssetKey &key) const override;
    std::vector<AssetHandle> find_all(const AssetKey &key) const override;
    std::vector<std::string> keys(std::string_view prefix = {}) const override;
    Provenance provenance() const override;

  private:
    struct Entry {
        std::filesystem::path file;
        std::string original_name, normalized_name;
    };
    std::filesystem::path root_;
    std::unordered_map<std::string, std::vector<Entry>> index_;
};
struct FpkDiscovery {
    std::vector<std::shared_ptr<FpkSource>> supported;
    std::vector<FpkDiagnostic> unresolved;
};
class AssetEnvironment {
  public:
    // Root is explicit, not a legacy-host setting or an assumed Steam installation.
    explicit AssetEnvironment(const std::filesystem::path &root);
    FpkOpenResult open_fpk_archive(const std::string &relative_path) const;
    FpkDiscovery discover_fpk_sources() const;
    std::shared_ptr<LooseSource> loose_source() const;

  private:
    std::filesystem::path root_;
    std::vector<std::pair<std::string, std::filesystem::path>> archives_;
};
} // namespace civ5::assets
