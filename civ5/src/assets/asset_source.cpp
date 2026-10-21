#include "civ5/assets/asset_source.hpp"
#ifdef _WIN32
#include "civ5/platform/windows_path.hpp"
#endif
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace civ5::assets {
namespace fs = std::filesystem;
namespace {
fs::path file_path(const fs::path &path) {
#ifdef _WIN32
    return platform::extended_windows_path(path);
#else
    return path;
#endif
}
fs::path canonical_file(const fs::path &path) {
#ifdef _WIN32
    return platform::unextended_windows_path(fs::canonical(file_path(path)));
#else
    return fs::canonical(path);
#endif
}
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::runtime_error(reason);
}
std::string slashes(std::string name) {
    std::replace(name.begin(), name.end(), '\\', '/');
    return name;
}
std::string lower(const std::string &text) {
    if (std::all_of(text.begin(), text.end(), [](unsigned char c) { return c < 128; })) {
        auto result = text;
        for (auto &c : result)
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
        return result;
    }
#ifdef _WIN32
    require(text.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()), "Asset key too long");
    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                           static_cast<int>(text.size()), nullptr, 0);
    require(count > 0, "Invalid UTF-8 asset key");
    std::wstring wide(count, L'\0');
    require(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                wide.data(), count) == count,
            "UTF-8 conversion failed");
    const auto length = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, wide.data(), count, nullptr, 0,
                                      nullptr, nullptr, 0);
    require(length > 0, "Invariant asset-key mapping failed");
    std::wstring mapped(length, L'\0');
    require(LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, wide.data(), count, mapped.data(), length,
                          nullptr, nullptr, 0) == length,
            "Invariant asset-key mapping failed");
    const auto bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, mapped.data(), length, nullptr, 0,
                                           nullptr, nullptr);
    require(bytes > 0, "UTF-8 mapping failed");
    std::string result(bytes, '\0');
    require(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, mapped.data(), length, result.data(), bytes,
                                nullptr, nullptr) == bytes,
            "UTF-8 mapping failed");
    return result;
#else
    // Never silently switch Unicode identity to the current process locale.
    throw std::runtime_error(
        "Non-ASCII asset keys need a verified invariant Unicode backend on this platform");
#endif
}
bool fits(std::uint64_t offset, std::uint64_t length, std::uint64_t size) {
    return offset <= size && length <= size - offset;
}
bool beneath(const fs::path &root, const fs::path &file) {
    const auto relative = file.lexically_relative(root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != fs::path("..");
}
std::vector<fs::path> files_under(const fs::path &root) {
    std::vector<fs::path> files;
    for (const auto &entry : fs::recursive_directory_iterator(root)) {
        // Do not follow directory links or index linked files outside the caller's root.
        if (entry.is_symlink() || !entry.is_regular_file())
            continue;
        const auto file = fs::canonical(entry.path());
        if (beneath(root, file))
            files.push_back(file);
    }
    std::sort(files.begin(), files.end());
    return files;
}
std::uint32_t u32(std::span<const std::uint8_t> bytes, std::size_t at) {
    require(fits(at, 4, bytes.size()), "Short U32");
    std::uint32_t v = 0;
    for (unsigned i = 0; i < 4; ++i)
        v |= std::uint32_t{bytes[at + i]} << (8 * i);
    return v;
}
Bytes read_at(std::ifstream &input, std::uint64_t size, std::uint64_t offset, std::size_t length) {
    require(fits(offset, length, size) &&
                offset <= static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()),
            "Archive read exceeds file");
    input.clear();
    input.seekg(static_cast<std::streamoff>(offset));
    require(bool(input), "Archive seek failed");
    Bytes bytes(length);
    input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(length));
    require(input.gcount() == static_cast<std::streamsize>(length), "Archive read is truncated");
    return bytes;
}
bool payload_fits(std::uint64_t offset, std::uint64_t length, std::uint64_t size) {
    return offset > 12 && offset < size && length < size && fits(offset, length, size);
}
std::string filename(Bytes bytes, std::uint32_t scrambling) {
    if (scrambling)
        for (auto &c : bytes) {
            require(c != 0, "Scrambled filename underflow");
            --c;
        }
    while (!bytes.empty() && bytes.back() == 0)
        bytes.pop_back();
    require(!bytes.empty() &&
                std::all_of(bytes.begin(), bytes.end(), [](auto c) { return c >= 32 && c <= 126; }),
            "Unsafe FPK ASCII filename");
    return {bytes.begin(), bytes.end()};
}
} // namespace
std::string path_utf8(const fs::path &path) {
    const auto text = path.generic_u8string();
    return {text.begin(), text.end()};
}
std::string normalize_asset_name(const std::string &name) {
    auto text = slashes(name);
    std::string result;
    for (std::size_t at = 0; at <= text.size();) {
        const auto end = text.find('/', at);
        const auto count = end == std::string::npos ? text.size() - at : end - at;
        const auto part = text.substr(at, count);
        if (!part.empty() && part != ".") {
            if (!result.empty())
                result += '/';
            result += part;
        }
        if (end == std::string::npos)
            break;
        at = end + 1;
    }
    return lower(result);
}
AssetKey::AssetKey(std::string name)
    : original_name_(std::move(name)), normalized_name_(normalize_asset_name(original_name_)),
      safe_relative_path_(true) {
    require(!original_name_.empty() && original_name_.find_first_not_of(" \t\n\r\v\f") != std::string::npos,
            "Blank asset key");
    auto original = slashes(original_name_);
    safe_relative_path_ = !normalized_name_.empty() && original[0] != '/' &&
                          original.find(':') == std::string::npos && original.find('\0') == std::string::npos;
    const auto guarded = '/' + original + '/';
    if (guarded.find("/../") != std::string::npos)
        safe_relative_path_ = false;
}
AssetStream::AssetStream(const fs::path &file, std::uint64_t offset, std::uint64_t length)
    : input_(file_path(file), std::ios::binary), remaining_(length) {
    require(bool(input_), "Cannot open selected asset");
    require(fits(offset, length, fs::file_size(file_path(file))) &&
                offset <= static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()),
            "Selected asset range changed or is invalid");
    input_.seekg(static_cast<std::streamoff>(offset));
    require(bool(input_), "Asset seek failed");
}
AssetStream::AssetStream(std::shared_ptr<const Bytes> memory)
    : memory_(std::move(memory)), remaining_(memory_ ? memory_->size() : 0) {
    require(bool(memory_), "Civis immutable asset storage is absent");
}
std::size_t AssetStream::read(std::span<std::uint8_t> output) {
    const auto count = std::min({remaining_, static_cast<std::uint64_t>(output.size()),
                                 static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())});
    if (count == 0)
        return 0;
    if (memory_) {
        const auto offset = memory_->size() - static_cast<std::size_t>(remaining_);
        std::memcpy(output.data(), memory_->data() + offset, static_cast<std::size_t>(count));
        remaining_ -= count;
        return static_cast<std::size_t>(count);
    }
    input_.read(reinterpret_cast<char *>(output.data()), static_cast<std::streamsize>(count));
    require(input_.gcount() == static_cast<std::streamsize>(count),
            "Selected asset ended before its declared range");
    remaining_ -= count;
    return static_cast<std::size_t>(count);
}
AssetHandle::AssetHandle(AssetKey key, AssetCandidate info, fs::path file, std::uint64_t offset,
                         std::optional<fs::path> root)
    : requested_key_(std::move(key)), candidate_(std::move(info)), file_(std::move(file)), offset_(offset),
      loose_root_(std::move(root)) {}
AssetHandle::AssetHandle(AssetKey key, AssetCandidate info,
                         std::shared_ptr<const Bytes> memory)
    : requested_key_(std::move(key)), candidate_(std::move(info)), offset_(0),
      memory_(std::move(memory)) {
    require(bool(memory_) && memory_->size() == candidate_.payload_size,
            "Civis immutable asset size disagrees with its selected descriptor");
}
AssetStream AssetHandle::open_stream() const {
    if (memory_) return AssetStream(memory_);
    if (loose_root_) {
        require(beneath(*loose_root_, canonical_file(file_)), "Loose asset escaped its configured root");
        require(fs::file_size(file_path(file_)) == candidate_.payload_size,
                "Loose asset size changed after lookup");
    }
    return AssetStream(file_, offset_, candidate_.payload_size);
}
Bytes AssetHandle::read_bytes(std::uint64_t maximum) const {
    require(candidate_.payload_size <= maximum &&
                candidate_.payload_size <= std::numeric_limits<std::size_t>::max(),
            "Asset exceeds materialization budget; use bounded stream");
    Bytes result(static_cast<std::size_t>(candidate_.payload_size));
    auto input = open_stream();
    std::size_t at = 0;
    while (at < result.size())
        at += input.read(std::span{result}.subspan(at));
    return result;
}
AssetResolver::AssetResolver(std::vector<std::shared_ptr<const AssetSource>> sources)
    : sources_(std::move(sources)) {
    require(std::all_of(sources_.begin(), sources_.end(), [](const auto &source) { return bool(source); }),
            "Null asset source");
}
std::vector<AssetHandle> AssetSource::find_all(const AssetKey &key) const {
    auto result = find(key);
    if (auto *handle = std::get_if<AssetHandle>(&result))
        return {*handle};
    return {};
}
std::vector<std::string> AssetSource::keys(std::string_view) const { return {}; }
AssetLookup AssetResolver::resolve(const AssetKey &key) const {
    for (const auto &source : sources_) {
        auto result = source->find(key);
        if (!std::holds_alternative<std::monostate>(result))
            return result;
    }
    return {};
}
FpkOpenResult FpkSource::open(const fs::path &archive, const std::string &relative_path) {
    try {
        auto source = std::shared_ptr<FpkSource>(new FpkSource);
        source->archive_ = fs::canonical(archive);
        source->relative_path_ = slashes(relative_path);
        require(fs::is_regular_file(source->archive_), "Archive is not a regular file");
        const auto size = fs::file_size(source->archive_);
        std::ifstream input(source->archive_, std::ios::binary);
        require(bool(input), "Cannot read archive");
        const auto header = read_at(input, size, 0, 14);
        require(u32(header, 0) == 6 && std::string(header.begin() + 4, header.begin() + 8) == "FPK_",
                "Unsupported FPK version/header");
        source->scrambling_ = header[8];
        source->extra_ = header[9];
        require(source->scrambling_ <= 1, "Unknown filename scrambling");
        const auto count = u32(header, 10);
        require(count <= 1000000, "FPK entry count exceeds safety bound");
        std::uint64_t cursor = 14;
        for (std::uint32_t ordinal = 0; ordinal < count; ++ordinal) {
            const auto record = cursor;
            const auto name_length = u32(read_at(input, size, cursor, 4), 0);
            cursor += 4;
            require(name_length >= 1 && name_length <= 4096, "Unsafe FPK filename length");
            const auto name = filename(read_at(input, size, cursor, name_length), source->scrambling_);
            cursor += name_length;
            auto metadata = read_at(input, size, cursor, 16);
            const bool direct = fits(cursor + 16, 4, size) &&
                                u32(read_at(input, size, cursor + 16, 4), 0) <= 255 &&
                                payload_fits(u32(metadata, 12), u32(metadata, 8), size);
            std::uint32_t padding = 0;
            if (!direct) {
                padding = read_at(input, size, cursor, 1)[0];
                cursor += padding;
                metadata = read_at(input, size, cursor, 16);
                require(payload_fits(u32(metadata, 12), u32(metadata, 8), size),
                        "Unsafe FPK dummy payload range");
            }
            source->entries_.push_back({ordinal, name, normalize_asset_name(name), record, cursor, padding,
                                        u32(metadata, 0), u32(metadata, 4), u32(metadata, 12),
                                        u32(metadata, 8), direct});
            source->index_[source->entries_.back().normalized_name].push_back(ordinal);
            cursor += 16;
        }
        source->directory_end_ = cursor;
        return source;
    } catch (const std::exception &e) {
        return FpkDiagnostic{slashes(relative_path), e.what()};
    }
}
Provenance FpkSource::provenance() const { return {SourceKind::fpk, relative_path_, ".", relative_path_}; }
AssetLookup FpkSource::find(const AssetKey &key) const {
    if (!key.safe_relative_path())
        return {};
    const auto found = index_.find(key.normalized_name());
    if (found == index_.end())
        return {};
    auto candidate = [&](const FpkEntry &e) {
        auto p = provenance();
        p.source_relative_path = e.original_name;
        return AssetCandidate{e.original_name, e.normalized_name, p, e.payload_length};
    };
    if (found->second.size() > 1) {
        AmbiguousAsset result{key, provenance(), {}};
        for (auto index : found->second)
            result.matches.push_back(candidate(entries_[index]));
        return result;
    }
    const auto &entry = entries_[found->second[0]];
    return AssetHandle(key, candidate(entry), archive_, entry.payload_offset, {});
}
std::vector<AssetHandle> FpkSource::find_all(const AssetKey &key) const {
    std::vector<AssetHandle> result;
    if (!key.safe_relative_path())
        return result;
    const auto found = index_.find(key.normalized_name());
    if (found == index_.end())
        return result;
    result.reserve(found->second.size());
    for (const auto ordinal : found->second) {
        const auto &entry = entries_[ordinal];
        auto provenance = this->provenance();
        provenance.source_relative_path = entry.original_name;
        result.push_back(AssetHandle(
            key, AssetCandidate{entry.original_name, entry.normalized_name, provenance, entry.payload_length},
            archive_, entry.payload_offset, std::nullopt));
    }
    return result;
}
std::vector<std::string> FpkSource::keys(std::string_view prefix) const {
    const auto normalized_prefix = normalize_asset_name(std::string(prefix));
    std::vector<std::string> result;
    for (const auto &entry : entries_)
        if (normalized_prefix.empty() || entry.normalized_name == normalized_prefix ||
            entry.normalized_name.starts_with(normalized_prefix + '/'))
            result.push_back(entry.original_name);
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        const auto l = normalize_asset_name(left), r = normalize_asset_name(right);
        return l == r ? left < right : l < r;
    });
    result.erase(std::unique(result.begin(), result.end(), [](const auto &left, const auto &right) {
                     return normalize_asset_name(left) == normalize_asset_name(right);
                 }),
                 result.end());
    return result;
}
LooseSource::LooseSource(const fs::path &root) : root_(fs::canonical(root)) {
    require(fs::is_directory(root_), "Loose root is not a directory");
    for (const auto &file : files_under(root_)) {
        auto relative = path_utf8(file.lexically_relative(root_));
        auto normalized = normalize_asset_name(relative);
        index_[normalized].push_back({file, relative, normalized});
    }
}
Provenance LooseSource::provenance() const { return {SourceKind::loose, path_utf8(root_), ".", {}}; }
AssetLookup LooseSource::find(const AssetKey &key) const {
    if (!key.safe_relative_path())
        return {};
    const auto found = index_.find(key.normalized_name());
    if (found == index_.end())
        return {};
    auto candidate = [&](const Entry &e) {
        auto p = provenance();
        p.source_relative_path = e.original_name;
        return AssetCandidate{e.original_name, e.normalized_name, p, fs::file_size(e.file)};
    };
    if (found->second.size() > 1) {
        AmbiguousAsset result{key, provenance(), {}};
        for (const auto &e : found->second)
            result.matches.push_back(candidate(e));
        return result;
    }
    const auto &entry = found->second[0];
    return AssetHandle(key, candidate(entry), entry.file, 0, root_);
}
std::vector<AssetHandle> LooseSource::find_all(const AssetKey &key) const {
    std::vector<AssetHandle> result;
    if (!key.safe_relative_path())
        return result;
    const auto found = index_.find(key.normalized_name());
    if (found == index_.end())
        return result;
    result.reserve(found->second.size());
    for (const auto &entry : found->second) {
        auto provenance = this->provenance();
        provenance.source_relative_path = entry.original_name;
        result.push_back(AssetHandle(
            key, AssetCandidate{entry.original_name, entry.normalized_name, provenance,
                                fs::file_size(entry.file)},
            entry.file, 0, root_));
    }
    return result;
}
std::vector<std::string> LooseSource::keys(std::string_view prefix) const {
    const auto normalized_prefix = normalize_asset_name(std::string(prefix));
    std::vector<std::string> result;
    for (const auto &[normalized, entries] : index_)
        if (normalized_prefix.empty() || normalized == normalized_prefix ||
            normalized.starts_with(normalized_prefix + '/'))
            for (const auto &entry : entries)
                result.push_back(entry.original_name);
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        const auto l = normalize_asset_name(left), r = normalize_asset_name(right);
        return l == r ? left < right : l < r;
    });
    result.erase(std::unique(result.begin(), result.end(), [](const auto &left, const auto &right) {
                     return normalize_asset_name(left) == normalize_asset_name(right);
                 }),
                 result.end());
    return result;
}
AssetEnvironment::AssetEnvironment(const fs::path &root) : root_(fs::canonical(root)) {
    require(fs::is_directory(root_), "Asset root is not a directory");
    for (const auto &file : files_under(root_))
        if (lower(path_utf8(file.extension())) == ".fpk")
            archives_.emplace_back(path_utf8(file.lexically_relative(root_)), file);
    std::sort(archives_.begin(), archives_.end(), [](const auto &a, const auto &b) {
        const auto na = normalize_asset_name(a.first), nb = normalize_asset_name(b.first);
        return na == nb ? a.first < b.first : na < nb;
    });
}
FpkOpenResult AssetEnvironment::open_fpk_archive(const std::string &relative_path) const {
    const AssetKey key(relative_path);
    if (!key.safe_relative_path())
        return FpkDiagnostic{slashes(relative_path), "Unsafe archive-relative path"};
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i < archives_.size(); ++i)
        if (normalize_asset_name(archives_[i].first) == key.normalized_name()) {
            if (found)
                return FpkDiagnostic{slashes(relative_path), "Ambiguous archive-relative path"};
            found = i;
        }
    if (!found)
        return FpkDiagnostic{slashes(relative_path), "Archive is missing"};
    return FpkSource::open(archives_[*found].second, archives_[*found].first);
}
FpkDiscovery AssetEnvironment::discover_fpk_sources() const {
    FpkDiscovery result;
    for (const auto &archive : archives_) {
        auto opened = open_fpk_archive(archive.first);
        if (auto *source = std::get_if<std::shared_ptr<FpkSource>>(&opened))
            result.supported.push_back(*source);
        else
            result.unresolved.push_back(std::get<FpkDiagnostic>(opened));
    }
    return result;
}
std::shared_ptr<LooseSource> AssetEnvironment::loose_source() const {
    return std::make_shared<LooseSource>(root_);
}
} // namespace civ5::assets
