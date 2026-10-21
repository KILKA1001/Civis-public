#include "civ5/content/mod_acquisition.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/content/inno_archive.hpp"
#include "civ5/platform/windows_path.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include "civ5/runtime/file_identity.hpp"
#define NOMINMAX
#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <limits>
#include <map>
#include <objbase.h>
#include <set>
#include <stdexcept>
#include <windows.h>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error(message);
}
std::string field(const XmlNode &node, const std::string &name) {
    const auto *child = node.child(name);
    require(child && child->children.empty() && !child->text.empty(), "Missing metadata field: " + name);
    return child->text;
}
bool digest(const std::string &text, std::size_t length) {
    return text.size() == length && std::all_of(text.begin(), text.end(), [](unsigned char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}
std::string escaped(const std::string &value) {
    std::string result;
    for (const char c : value) {
        if (c == '&')
            result += "&amp;";
        else if (c == '<')
            result += "&lt;";
        else if (c == '>')
            result += "&gt;";
        else if (c == '"')
            result += "&quot;";
        else if (c == '\'')
            result += "&apos;";
        else
            result += c;
    }
    return result;
}
std::string text_file(const fs::path &path, std::uint64_t budget = 16 * 1024 * 1024) {
    const auto size = fs::file_size(path);
    require(size <= budget, "Metadata exceeds byte budget");
    std::ifstream input(path, std::ios::binary);
    std::string text(static_cast<std::size_t>(size), '\0');
    input.read(text.data(), static_cast<std::streamsize>(text.size()));
    require(static_cast<bool>(input), "Cannot read metadata file");
    return text;
}
void write_new_text(const fs::path &path, const std::string &text) {
    const auto handle =
        CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    require(handle != INVALID_HANDLE_VALUE, "Cannot create metadata file");
    try {
        std::size_t done = 0;
        while (done < text.size()) {
            const auto count = static_cast<DWORD>(std::min<std::size_t>(65536, text.size() - done));
            DWORD written = 0;
            require(WriteFile(handle, text.data() + done, count, &written, nullptr) && written == count,
                    "Cannot write metadata file");
            done += written;
        }
        require(FlushFileBuffers(handle), "Cannot flush metadata file");
    } catch (...) {
        CloseHandle(handle);
        throw;
    }
    CloseHandle(handle);
}
fs::path fresh_stage(const fs::path &cache) {
    initialize_mod_cache(cache);
    GUID guid;
    require(SUCCEEDED(CoCreateGuid(&guid)), "Cannot allocate staging identifier");
    wchar_t text[40]{};
    require(StringFromGUID2(guid, text, 40) != 0, "Cannot format staging identifier");
    const auto path = fs::absolute(cache).lexically_normal() / "staging" / std::wstring(text + 1, 36);
    require_plain_directory(path.parent_path());
    require(fs::create_directory(path), "Cannot create staging directory");
    return path;
}
struct ManifestMismatch {
    std::string path, expected, actual;
    bool operator==(const ManifestMismatch &) const = default;
};
PackageManifest validate_target(const fs::path &content, const ModTarget &target,
                                bool allow_stale_md5, std::vector<ManifestMismatch> &mismatches,
                                bool allow_missing_assets,
                                std::vector<AcquiredMod::MissingManifestAsset> &missing_assets) {
    const auto catalog = discover_packages({content});
    if(!catalog.diagnostics.empty())
        throw std::runtime_error("Downloaded package contains invalid manifests or paths: "+
                                 catalog.diagnostics.front());
    std::optional<PackageManifest> selected;
    for (const auto &package : catalog.packages) {
        if (package.kind != PackageKind::mod || package.id != runtime::ascii_lower(target.id) ||
            package.version != target.version)
            continue;
        require(!selected, "Archive has multiple exact mod manifests");
        selected = package;
    }
    require(selected.has_value(), "Archive does not contain the saved mod ID and exact version");
    std::map<std::string,std::string> declared_hashes;
    for (const auto &file : selected->files) {
        const auto declared=runtime::ascii_lower(file.md5);
        const auto [it,inserted]=declared_hashes.emplace(file.path,declared);
        require(inserted||it->second==declared,
                "Conflicting duplicate manifest MD5 declarations: "+file.path);
        const auto candidate=selected->path.parent_path() /
            fs::path(std::u8string(file.path.begin(),file.path.end()));
        if (!fs::exists(platform::extended_windows_path(candidate))) {
            const auto extension=runtime::ascii_lower(candidate.extension().string());
            static const std::set<std::string> asset_extensions={
                ".dds",".gr2",".fxsxml",".ggxml",".wav",".ogg",".mp3",
                ".bnk",".wem",".ttf",".otf",".png",".jpg",".jpeg",".bmp",".tga"};
            require(allow_missing_assets && asset_extensions.contains(extension),
                "Missing package file: " + file.path);
            require(declared.empty() || digest(declared,32),
                "Malformed mod manifest MD5: " + file.path);
            missing_assets.push_back({file.path,declared});
            continue;
        }
        const auto path = package_file(selected->path.parent_path(), file.path);
        if (!file.md5.empty()) {
            const auto expected = runtime::ascii_lower(file.md5);
            require(digest(expected, 32), "Malformed mod manifest MD5: " + file.path);
            const auto actual = runtime::file_md5(path);
            if (actual != expected) {
                require(allow_stale_md5, "Mod manifest MD5 mismatch: " + file.path);
                mismatches.push_back({file.path, expected, actual});
            }
        }
    }
    return *selected;
}
std::optional<AcquiredMod> cached_package(const fs::path &cache, const ModTarget &target,
                                          const std::string &sha, bool require_isolated = false,
                                          bool allow_stale_md5 = false,
                                          bool allow_missing_assets = false) {
    if (sha.empty())
        return {};
    require(digest(sha, 64), "Malformed expected archive SHA-256");
    const auto root = fs::absolute(cache).lexically_normal() / "packages" / sha;
    if (!fs::exists(root))
        return {};
    require_plain_directory(root);
    require_plain_directory(root / "content");
    const auto archive = runtime::identify_file(package_file(root, "archive.civ5mod"));
    require(archive.sha256 == sha, "Cached archive identity changed");
    const auto receipt = read_xml(package_file(root, "receipt.xml"));
    require(receipt.name == "CivisAcquiredMod" && receipt.attribute("schema") == "1" &&
                receipt.attribute("sha256") == sha,
            "Invalid cache receipt");
    const auto policy = receipt.attribute("manifest_md5");
    require(policy.empty() || policy == "archive", "Unknown cached manifest MD5 policy");
    require(policy.empty() || allow_stale_md5, "Cached source requires explicit stale-MD5 policy");
    const auto assets_policy=receipt.attribute("manifest_assets");
    require(assets_policy.empty()||assets_policy=="archive",
            "Unknown cached missing-asset policy");
    require(assets_policy.empty()||allow_missing_assets,
            "Cached source requires explicit missing-asset policy");
    const auto *files = receipt.child("Files");
    require(files != nullptr, "Cache receipt has no files");
    std::set<fs::path> expected_paths;
    for (const auto &file : files->children) {
        require(file.name == "File" && file.children.empty(), "Invalid receipt file");
        // Use package_file for existing path validation; no receipt path can escape
        // content.
        const auto path = package_file(root / "content", file.attribute("path"));
        const auto identity = runtime::identify_file(path);
        require(identity.sha256 == file.attribute("sha256") &&
                    identity.bytes == decimal(file.attribute("bytes")),
                "Cached mod file changed");
        require(expected_paths.insert(fs::canonical(platform::extended_windows_path(path))).second,
                "Duplicate cache receipt path");
    }
    const auto content_root=root / "content";
    const auto extended_content_root=platform::extended_windows_path(content_root);
    for (fs::recursive_directory_iterator it(extended_content_root), end; it != end; ++it) {
        const auto relative=it->path().lexically_relative(extended_content_root);
        require(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=fs::path(".."),
                "Cache entry escaped content root");
        const auto entry=content_root/relative;
        const auto attributes = GetFileAttributesW(platform::extended_windows_path(entry).c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES)
            throw std::runtime_error("Cannot inspect installed mod cache path (Win32 "+
                std::to_string(GetLastError())+"): "+assets::path_utf8(entry));
        require(!(attributes & FILE_ATTRIBUTE_REPARSE_POINT),
                "Link in installed mod cache");
        if (it->is_directory())
            require_plain_directory(entry);
        else
            require(it->is_regular_file() &&
                        expected_paths.contains(fs::canonical(platform::extended_windows_path(entry))),
                    "Unexpected file in installed mod cache");
    }
    std::vector<ManifestMismatch> mismatches;
    std::vector<AcquiredMod::MissingManifestAsset> missing_assets;
    auto manifest = validate_target(root / "content", target, policy == "archive", mismatches,
                                    assets_policy=="archive",missing_assets);
    const auto *declared_mismatches = receipt.child("ManifestMismatches");
    if (policy == "archive") {
        require(declared_mismatches != nullptr &&
                    declared_mismatches->children.size() == mismatches.size(),
                "Cached manifest MD5 mismatch list changed");
        for (std::size_t i = 0; i < mismatches.size(); ++i) {
            const auto &entry = declared_mismatches->children[i];
            const auto &actual = mismatches[i];
            require(entry.name == "File" && entry.children.empty() &&
                        entry.attribute("path") == actual.path &&
                        entry.attribute("expected") == actual.expected &&
                        entry.attribute("actual") == actual.actual,
                    "Cached manifest MD5 mismatch list changed");
        }
    } else {
        require(declared_mismatches == nullptr, "Strict cache has manifest mismatch exceptions");
    }
    const auto *declared_missing=receipt.child("ManifestAssetOmissions");
    if(assets_policy=="archive"){
        require(declared_missing&&declared_missing->children.size()==missing_assets.size(),
                "Cached missing-asset list changed");
        for(std::size_t i=0;i<missing_assets.size();++i){
            const auto &entry=declared_missing->children[i];
            require(entry.name=="File"&&entry.children.empty()&&
                    entry.attribute("path")==missing_assets[i].path&&
                    entry.attribute("md5")==missing_assets[i].declared_md5,
                    "Cached missing-asset list changed");
        }
    }else require(!declared_missing,"Strict cache has missing-asset exceptions");
    if (require_isolated)
        require(fs::canonical(platform::extended_windows_path(manifest.path.parent_path())) ==
                    fs::canonical(root / "content"),
                "Cached archive contains an unisolated package");
    AcquiredMod result{std::move(manifest), root, sha, archive.bytes, expected_paths.size(), true, {}, {}, {}};
    for (const auto &mismatch : mismatches) {
        const auto duplicate=std::find_if(result.stale_manifest_hashes.begin(),
            result.stale_manifest_hashes.end(),[&](const auto &entry){
                return entry.path==mismatch.path;});
        if(duplicate!=result.stale_manifest_hashes.end()){
            require(duplicate->declared_md5==mismatch.expected&&
                    duplicate->actual_md5==mismatch.actual,
                    "Conflicting duplicate manifest MD5 declarations");
            continue;
        }
        result.stale_manifest_md5_files.push_back(mismatch.path);
        result.stale_manifest_hashes.push_back({mismatch.path, mismatch.expected, mismatch.actual});
    }
    result.missing_manifest_assets=std::move(missing_assets);
    return result;
}
AcquiredMod publish_stage(const fs::path &stage, const ModTarget &target, const fs::path &cache,
                          const std::string &expected_sha256, ArchiveBudget budget,
                          const std::string &origin, const std::string &package_root = {},
                          bool allow_stale_md5 = false,bool allow_missing_assets = false) {
    require((!allow_stale_md5&&!allow_missing_assets)||digest(expected_sha256,64),
            "Manifest exceptions require a pinned archive SHA-256");
    const auto identity = runtime::identify_file(stage / "archive.civ5mod");
    require(expected_sha256.empty() || identity.sha256 == expected_sha256,
            "Downloaded archive SHA-256 mismatch");
    if (auto existing = cached_package(cache, target, identity.sha256, !package_root.empty(),
                                       allow_stale_md5,allow_missing_assets))
        return *existing;
    std::array<unsigned char,6> signature{};
    std::ifstream archive(stage / "archive.civ5mod",std::ios::binary);
    archive.read(reinterpret_cast<char*>(signature.data()),signature.size());
    require(archive.gcount()>=4,"Unsupported mod archive format");
    const bool seven_zip=signature==std::array<unsigned char,6>{'7','z',0xbc,0xaf,0x27,0x1c};
    const bool zip=signature[0]=='P'&&signature[1]=='K'&&
        ((signature[2]==3&&signature[3]==4)||(signature[2]==5&&signature[3]==6));
    require(seven_zip||zip,"Unsupported mod archive format");
    archive.close(); // Windows cannot publish the stage while this file is open.
    const auto extraction=seven_zip
        ?extract_mod_7z(stage / "archive.civ5mod",stage / "content",budget)
        :extract_mod_zip(stage / "archive.civ5mod",stage / "content",budget);
    const auto requested = package_root.empty()
        ? fs::path{}
        : archive_relative_path(fs::path(std::u8string(package_root.begin(),
                                                       package_root.end())).native());
    // A provider bundle can carry other mods. Their manifests do not govern
    // the selected package, although the extractor still validates every path.
    std::vector<ManifestMismatch> mismatches;
    std::vector<AcquiredMod::MissingManifestAsset> missing_assets;
    auto manifest = validate_target(stage / "content" / requested, target, allow_stale_md5,
                                    mismatches,allow_missing_assets,missing_assets);
    std::vector<ArchiveEntry> entries = extraction.entries;
    if (!package_root.empty()) {
        const auto content = fs::canonical(stage / "content");
        const auto actual_root = manifest.path.parent_path().lexically_relative(content);
        require(actual_root == requested,
                "Selected package root does not contain the target manifest: " +
                    assets::path_utf8(actual_root));
        require(content.parent_path() == fs::canonical(stage), "Archive content escaped staging directory");
        const auto selected = fs::canonical(platform::extended_windows_path(content / requested));
        require(selected == fs::canonical(platform::extended_windows_path(manifest.path.parent_path())),
                "Selected package root changed after validation");
        fs::rename(platform::extended_windows_path(selected),
                   platform::extended_windows_path(stage / "selected_content"));
        fs::remove_all(platform::extended_windows_path(content));
        fs::rename(platform::extended_windows_path(stage / "selected_content"),
                   platform::extended_windows_path(stage / "content"));
        manifest.path = stage / "content" / manifest.path.filename();
        entries.clear();
        const auto selected_content = platform::extended_windows_path(stage / "content");
        for (fs::recursive_directory_iterator it(selected_content), end; it != end; ++it)
            if (it->is_regular_file())
                entries.push_back({it->path().lexically_relative(selected_content), 0, false});
        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return assets::path_utf8(a.relative) < assets::path_utf8(b.relative);
        });
        mismatches.clear();
        missing_assets.clear();
        manifest = validate_target(stage / "content", target, allow_stale_md5,mismatches,
                                   allow_missing_assets,missing_assets);
    }
    std::string receipt = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><CivisAcquiredMod "
                          "schema=\"1\" sha256=\"" +
                          identity.sha256 + "\" id=\"" + escaped(manifest.id) + "\" version=\"" +
                          std::to_string(manifest.version) + "\"" +
                          (allow_stale_md5 ? " manifest_md5=\"archive\"" : "") +
                          (allow_missing_assets ? " manifest_assets=\"archive\"" : "") +
                          "><Origin>" + escaped(origin) +
                          "</Origin><Files>";
    std::uint64_t files = 0;
    for (const auto &entry : entries)
        if (!entry.directory) {
            const auto file = runtime::identify_file(stage / "content" / entry.relative);
            receipt += "<File path=\"" + escaped(assets::path_utf8(entry.relative)) + "\" bytes=\"" +
                       std::to_string(file.bytes) + "\" sha256=\"" + file.sha256 + "\"/>";
            ++files;
        }
    receipt += "</Files>";
    if (allow_stale_md5) {
        receipt += "<ManifestMismatches>";
        for (const auto &mismatch : mismatches)
            receipt += "<File path=\"" + escaped(mismatch.path) + "\" expected=\"" +
                       mismatch.expected + "\" actual=\"" + mismatch.actual + "\"/>";
        receipt += "</ManifestMismatches>";
    }
    if(allow_missing_assets){
        receipt+="<ManifestAssetOmissions>";
        for(const auto &missing:missing_assets)
            receipt+="<File path=\""+escaped(missing.path)+"\" md5=\""+
                     escaped(missing.declared_md5)+"\"/>";
        receipt+="</ManifestAssetOmissions>";
    }
    receipt += "</CivisAcquiredMod>";
    write_new_text(stage / "receipt.xml", receipt);
    const auto relative_manifest = fs::relative(manifest.path, stage);
    const auto canonical_cache = fs::canonical(cache);
    require_plain_directory(canonical_cache / "staging");
    require_plain_directory(canonical_cache / "packages");
    require(fs::canonical(stage.parent_path()) == canonical_cache / "staging", "Stage is outside this cache");
    const auto destination = canonical_cache / "packages" / identity.sha256;
    require(destination.parent_path() == canonical_cache / "packages" && !fs::exists(destination),
            "Invalid or occupied package destination");
    fs::rename(stage,
               destination); // Both resolved roots above are inside this owned cache.
    manifest.path = destination / relative_manifest;
    AcquiredMod result{std::move(manifest), destination, identity.sha256, identity.bytes, files, false, {}, {}, {}};
    for (const auto &mismatch : mismatches) {
        const auto duplicate=std::find_if(result.stale_manifest_hashes.begin(),
            result.stale_manifest_hashes.end(),[&](const auto &entry){
                return entry.path==mismatch.path;});
        if(duplicate!=result.stale_manifest_hashes.end()){
            require(duplicate->declared_md5==mismatch.expected&&
                    duplicate->actual_md5==mismatch.actual,
                    "Conflicting duplicate manifest MD5 declarations");
            continue;
        }
        result.stale_manifest_md5_files.push_back(mismatch.path);
        result.stale_manifest_hashes.push_back({mismatch.path, mismatch.expected, mismatch.actual});
    }
    result.missing_manifest_assets=std::move(missing_assets);
    return result;
}
} // namespace
std::optional<AcquiredMod> find_cached_mod_by_identity(const fs::path &cache,
                                                        const ModTarget &target) {
    if (cache.empty() || !fs::exists(cache)) return {};
    require_plain_directory(cache);
    const auto marker = read_xml(package_file(cache, "civis-cache.xml"));
    require(marker.name == "CivisModCache" && marker.attribute("schema") == "1",
            "Unsupported cache marker");
    const auto packages = cache / "packages";
    require_plain_directory(packages);
    std::optional<AcquiredMod> found;
    for (const auto &entry : fs::directory_iterator(packages)) {
        require_plain_directory(entry.path());
        const auto sha = assets::path_utf8(entry.path().filename());
        require(digest(sha, 64), "Invalid cache package name");
        const auto receipt = read_xml(package_file(entry.path(), "receipt.xml"));
        require(receipt.name == "CivisAcquiredMod" && receipt.attribute("schema") == "1" &&
                    receipt.attribute("sha256") == sha,
                "Invalid cache receipt");
        if (runtime::ascii_lower(receipt.attribute("id")) != runtime::ascii_lower(target.id) ||
            decimal(receipt.attribute("version")) != target.version)
            continue;
        auto verified = cached_package(cache, target, sha, false, true,true);
        require(verified.has_value(), "Cached mod disappeared during verification");
        if (found)
            throw AmbiguousCachedMod("Multiple verified cached archives have the saved mod ID and version");
        found = std::move(*verified);
    }
    return found;
}
std::optional<AcquiredMod> verify_cached_mod_package(const fs::path &cache,
                                                     const PackageManifest &package) {
    if (package.kind != PackageKind::mod || cache.empty() || !fs::exists(cache))
        return {};
    const auto packages = fs::canonical(cache) / "packages";
    const auto selected = fs::canonical(platform::extended_windows_path(package.path));
    const auto relative = selected.lexically_relative(packages);
    if (relative.empty() || relative.is_absolute() || *relative.begin() == fs::path(".."))
        return {};
    require_plain_directory(cache);
    require_plain_directory(packages);
    const auto marker = read_xml(package_file(cache, "civis-cache.xml"));
    require(marker.name == "CivisModCache" && marker.attribute("schema") == "1",
            "Selected mod is outside an owned Civis cache");
    auto part = relative.begin();
    const auto sha = assets::path_utf8(*part++);
    require(digest(sha, 64) && part != relative.end() && *part == fs::path("content"),
            "Selected mod has an invalid cache package path");
    const ModTarget target{package.id, package.name, package.version};
    auto acquired = cached_package(cache, target, sha, false, true,true);
    require(acquired && fs::canonical(platform::extended_windows_path(acquired->manifest.path)) == selected,
            "Cached receipt does not identify the selected mod manifest");
    return acquired;
}
std::vector<ModSource> read_mod_sources(const fs::path &file) {
    const auto root = read_xml(file);
    require(root.name == "CivisModSources" && root.attribute("schema") == "1",
            "Unsupported mod source catalog");
    std::vector<ModSource> result;
    std::set<std::pair<std::string, std::uint64_t>> seen;
    for (const auto &node : root.children) {
        require(node.name == "Mod" && node.children.size() == 1, "Invalid source record");
        ModSource source;
        source.target = {runtime::ascii_lower(node.attribute("id")), node.attribute("name"),
                         decimal(node.attribute("version"))};
        require(!source.target.id.empty() && seen.emplace(source.target.id, source.target.version).second,
                "Empty or duplicate mod source identity");
        const auto &provider = node.children.front();
        require(provider.children.empty(), "Invalid nested mod provider");
        if (provider.name == "Workshop") {
            source.workshop_id = decimal(provider.attribute("id"));
            require(source.workshop_id != 0, "Invalid Workshop ID");
        } else if (provider.name == "Archive") {
            source.archive_url = provider.attribute("url");
            require(source.archive_url.starts_with("https://"), "Archive source must use HTTPS");
            source.archive_member = provider.attribute("member");
            if (!source.archive_member.empty())
                archive_relative_path(fs::path(std::u8string(source.archive_member.begin(),
                                                            source.archive_member.end())).native());
            source.package_root = provider.attribute("root");
            if (!source.package_root.empty())
                archive_relative_path(fs::path(std::u8string(source.package_root.begin(),
                                                            source.package_root.end())).native());
        } else if (provider.name == "Installer") {
            source.installer_bundle = provider.attribute("bundle");
            require(source.installer_bundle == "vp-5.4.2" &&
                        provider.attribute("sha256") == vp_542_installer_sha256 &&
                        ((source.target.id == "d1b6328c-ff44-4b0d-aad7-c657f83610cd" &&
                          source.target.version == 151) ||
                         (source.target.id == "8411a7a8-dad3-4622-a18e-fcc18324c799" &&
                          source.target.version == 17)),
                    "Unknown installer bundle or target");
        } else
            throw std::runtime_error("Unknown mod source provider");
        source.sha256 = runtime::ascii_lower(provider.attribute("sha256"));
        require(digest(source.sha256, 64), "Source catalog requires a pinned archive SHA-256");
        const auto md5_policy = provider.attribute("manifest_md5");
        require(md5_policy.empty() || md5_policy == "archive",
                "Unknown source manifest MD5 policy");
        source.allow_stale_manifest_md5 = md5_policy == "archive";
        require(!source.allow_stale_manifest_md5 || provider.name == "Archive",
                "Stale manifest MD5 policy requires a pinned archive source");
        const auto assets_policy=provider.attribute("manifest_assets");
        require(assets_policy.empty()||assets_policy=="archive",
                "Unknown source missing-asset policy");
        source.allow_missing_manifest_assets=assets_policy=="archive";
        require(!source.allow_missing_manifest_assets||provider.name=="Archive",
                "Missing-asset policy requires a pinned archive source");
        result.push_back(std::move(source));
    }
    return result;
}
std::vector<ModDependency> read_mod_dependencies(const PackageManifest &package) {
    require(package.kind == PackageKind::mod, "Expected mod manifest for dependency inspection");
    std::vector<ModDependency> result;
    const auto *dependencies = package.document.child("Dependencies");
    if (!dependencies)
        return result;
    for (const auto &node : dependencies->children) {
        if (node.name == "Game" || node.name == "Dlc")
            continue;
        if (node.name != "Mod")
            throw std::runtime_error("Unsupported package dependency: " + node.name);
        ModDependency dependency;
        dependency.id = runtime::ascii_lower(node.attribute("id"));
        dependency.name = node.attribute("title");
        require(!dependency.id.empty(), "Mod dependency has no package id");
        const auto minimum = node.attribute("minversion");
        const auto maximum = node.attribute("maxversion");
        dependency.minimum_version = minimum.empty() ? 0 : decimal(minimum);
        dependency.maximum_version = maximum.empty() ? std::numeric_limits<std::uint64_t>::max()
                                                      : decimal(maximum);
        require(dependency.minimum_version <= dependency.maximum_version,
                "Mod dependency minimum version exceeds maximum version");
        result.push_back(std::move(dependency));
    }
    return result;
}
const char *mod_acquisition_status_name(ModAcquisitionStatus status) {
    switch (status) {
    case ModAcquisitionStatus::source_unresolved:
        return "source_unresolved";
    case ModAcquisitionStatus::version_unavailable:
        return "version_unavailable";
    case ModAcquisitionStatus::download_required:
        return "download_required";
    case ModAcquisitionStatus::installer_profile_required:
        return "installer_profile_required";
    case ModAcquisitionStatus::cached_verified:
        return "cached_verified";
    case ModAcquisitionStatus::cache_ambiguous:
        return "cache_ambiguous";
    case ModAcquisitionStatus::cache_invalid:
        return "cache_invalid";
    }
    throw std::runtime_error("Unknown acquisition status");
}
ModAcquisitionPlan plan_mod_acquisition(std::span<const ModTarget> targets,
                                        std::span<const ModSource> sources, const fs::path &cache) {
    ModAcquisitionPlan plan;
    if (targets.empty())
        return plan;
    std::map<std::pair<std::string, std::uint64_t>, std::size_t> indexed;
    std::set<std::string> source_ids, target_ids;
    for (std::size_t i = 0; i < sources.size(); ++i) {
        const auto &source = sources[i];
        const auto id = runtime::ascii_lower(source.target.id);
        require(!id.empty() && indexed.emplace(std::make_pair(id, source.target.version), i).second,
                "Empty or duplicate mod source identity");
        require(digest(source.sha256, 64), "Source catalog requires a pinned archive SHA-256");
        source_ids.insert(id);
    }
    for (const auto &target : targets) {
        const auto id = runtime::ascii_lower(target.id);
        require(!id.empty() && target_ids.insert(id).second, "Empty or duplicate requested mod identity");
        ModAcquisitionItem item;
        item.target = target;
        const auto found = indexed.find({id, target.version});
        if (found == indexed.end()) {
            item.status = source_ids.contains(id) ? ModAcquisitionStatus::version_unavailable
                                                  : ModAcquisitionStatus::source_unresolved;
            try {
                item.cached = find_cached_mod_by_identity(cache, target);
                if (item.cached) item.status = ModAcquisitionStatus::cached_verified;
            } catch (const AmbiguousCachedMod &error) {
                item.status = ModAcquisitionStatus::cache_ambiguous;
                item.reason = error.what();
            } catch (const std::exception &error) {
                item.status = ModAcquisitionStatus::cache_invalid;
                item.reason = error.what();
            }
        } else {
            item.source_index = found->second;
            const auto &source = sources[found->second];
            if (!source.installer_bundle.empty()) {
                item.status = ModAcquisitionStatus::installer_profile_required;
            } else {
                item.status = ModAcquisitionStatus::download_required;
                try {
                    if (!cache.empty() && fs::exists(cache)) {
                        require_plain_directory(cache);
                        const auto marker = read_xml(package_file(cache, "civis-cache.xml"));
                        require(marker.name == "CivisModCache" && marker.attribute("schema") == "1",
                                "Unsupported cache marker");
                        require_plain_directory(cache / "packages");
                        item.cached = cached_package(cache, target, source.sha256,
                                                     !source.package_root.empty(),
                                                     source.allow_stale_manifest_md5,
                                                     source.allow_missing_manifest_assets);
                        if (item.cached)
                            item.status = ModAcquisitionStatus::cached_verified;
                    }
                } catch (const std::exception &error) {
                    item.status = ModAcquisitionStatus::cache_invalid;
                    item.reason = error.what();
                }
            }
        }
        plan.all_verified &= item.status == ModAcquisitionStatus::cached_verified;
        plan.items.push_back(std::move(item));
    }
    return plan;
}
SavedModAcquisitionReport acquire_saved_mods(std::span<const ModTarget> targets,
                                              std::span<const ModSource> sources,
                                              const fs::path &cache, const fs::path &decoder,
                                              const fs::path &local_installer,
                                              platform::DownloadProgress progress,
                                              ArchiveBudget budget) {
    struct Pending {
        ModTarget target;
    };
    SavedModAcquisitionReport report;
    if (targets.empty())
        return report;
    initialize_mod_cache(cache);
    std::deque<Pending> pending;
    std::set<std::pair<std::string, std::uint64_t>> queued;
    const auto enqueue = [&](ModTarget target) {
        target.id = runtime::ascii_lower(target.id);
        if (queued.emplace(target.id, target.version).second)
            pending.push_back({std::move(target)});
    };
    for (const auto &target : targets)
        enqueue(target);
    while (!pending.empty()) {
        const auto current = std::move(pending.front());
        pending.pop_front();
        std::vector<const ModSource *> exact;
        for (const auto &source : sources)
            if (runtime::ascii_lower(source.target.id) == current.target.id &&
                source.target.version == current.target.version)
                exact.push_back(&source);
        std::optional<AcquiredMod> independently_cached;
        if (exact.empty()) {
            try {
                independently_cached = find_cached_mod_by_identity(cache, current.target);
            } catch (const AmbiguousCachedMod &error) {
                report.diagnostics.push_back(current.target.id + ": cache_ambiguous: " + error.what());
                report.all_acquired = false;
                continue;
            } catch (const std::exception &error) {
                report.diagnostics.push_back(current.target.id + ": cache_invalid: " + error.what());
                report.all_acquired = false;
                continue;
            }
            if (!independently_cached) {
                report.diagnostics.push_back(current.target.id + " v" +
                                             std::to_string(current.target.version) +
                                             ": source_unresolved");
                report.all_acquired = false;
                continue;
            }
        }
        if (!exact.empty() && exact.size() != 1) {
            report.diagnostics.push_back(current.target.id + " v" +
                                         std::to_string(current.target.version) +
                                         ": source_ambiguous");
            report.all_acquired = false;
            continue;
        }
        try {
            const auto *source = exact.empty() ? nullptr : exact.front();
            if (source && !source->installer_bundle.empty()) {
                // Installer bundles contain native/runtime files and therefore
                // remain an explicit activation profile until their x64 port is
                // selected. Downloading one is useful, but it is not proof that
                // the saved rules can run.
                (void)acquire_vp_542_bundle(cache, decoder, local_installer, progress, budget);
                ++report.retained_installer_bundles;
                report.diagnostics.push_back(current.target.id + ": installer_profile_required");
                report.all_acquired = false;
                continue;
            }
            const auto acquired = independently_cached ? *independently_cached
                                                       : acquire_mod(*source, cache, progress, budget);
            ++report.acquired;
            if (acquired.reused)
                ++report.reused;
            try {
                for (const auto &dependency : read_mod_dependencies(acquired.manifest)) {
                    std::size_t already_queued = 0;
                    for (const auto &[id, version] : queued)
                        if (id == dependency.id && version >= dependency.minimum_version &&
                            version <= dependency.maximum_version)
                            ++already_queued;
                    if (already_queued == 1) continue;
                    if (already_queued > 1) {
                        report.diagnostics.push_back(dependency.id + ": dependency_version_ambiguous");
                        report.all_acquired = false;
                        continue;
                    }
                    std::vector<const ModSource *> candidates;
                    for (const auto &source : sources) {
                        if (runtime::ascii_lower(source.target.id) == dependency.id &&
                            source.target.version >= dependency.minimum_version &&
                            source.target.version <= dependency.maximum_version)
                            candidates.push_back(&source);
                    }
                    if (candidates.size() == 1) {
                        enqueue({candidates.front()->target.id, dependency.name,
                                 candidates.front()->target.version});
                    } else {
                        report.diagnostics.push_back(
                            dependency.id + " [" + std::to_string(dependency.minimum_version) + "," +
                            std::to_string(dependency.maximum_version) + "] " +
                            (candidates.empty() ? "source_unresolved" : "source_ambiguous"));
                        report.all_acquired = false;
                    }
                }
            } catch (const std::exception &error) {
                report.diagnostics.push_back(current.target.id + ": " + error.what());
                report.all_acquired = false;
            }
        } catch (const std::exception &error) {
            report.diagnostics.push_back(current.target.id + ": " + error.what());
            report.all_acquired = false;
        }
    }
    return report;
}
WorkshopFile parse_workshop_details(std::string text, std::uint64_t expected_id) {
    // Valve emits this exact empty doctype. Remove only it; XmlLite continues to
    // reject every external/internal DTD and entity declaration.
    const std::string doctype = "<!DOCTYPE response>";
    const auto position = text.find(doctype);
    if (position != std::string::npos && position < text.find("<response>"))
        text.erase(position, doctype.size());
    const auto root = read_xml_metadata(text);
    require(root.name == "response" && decimal(field(root, "result")) == 1 &&
                decimal(field(root, "resultcount")) == 1,
            "Workshop metadata query failed");
    const auto *list = root.child("publishedfiledetails");
    require(list && list->children.size() == 1 && list->children[0].name == "publishedfile",
            "Unexpected Workshop result count");
    const auto &item = list->children[0];
    require(decimal(field(item, "publishedfileid")) == expected_id && decimal(field(item, "result")) == 1,
            "Workshop item unavailable or mismatched");
    require(decimal(field(item, "consumer_app_id")) == 8930, "Workshop item belongs to a different game");
    WorkshopFile result{
        expected_id,          decimal(field(item, "file_size")), decimal(field(item, "hcontent_file")),
        field(item, "title"), field(item, "file_url"),           field(item, "filename")};
    require(result.bytes > 0 && result.url.starts_with("https://"),
            "Workshop item has no downloadable archive");
    return result;
}
void initialize_mod_cache(const fs::path &root) {
    const auto cache = fs::absolute(root).lexically_normal();
    require_plain_directory(cache.parent_path());
    if (!fs::exists(cache))
        require(fs::create_directory(cache), "Cannot create cache root");
    require_plain_directory(cache);
    const auto marker = cache / "civis-cache.xml";
    if (!fs::exists(marker)) {
        require(fs::is_empty(cache), "Existing directory is not a Civis mod cache");
        write_new_text(marker, "<CivisModCache schema=\"1\"/>");
    }
    const auto document = read_xml(package_file(cache, "civis-cache.xml"));
    require(document.name == "CivisModCache" && document.attribute("schema") == "1",
            "Unsupported cache marker");
    for (const auto *directory : {"staging", "packages"}) {
        fs::create_directory(cache / directory);
        require_plain_directory(cache / directory);
    }
}
static AcquiredMod acquire_mod_source(const ModSource &source, const fs::path &cache,
                                     platform::DownloadProgress progress, ArchiveBudget budget,
                                     bool allow_unpinned) {
    require(source.installer_bundle.empty(), "Installer bundle requires an explicit activation profile");
    require((allow_unpinned && source.sha256.empty()) || digest(source.sha256, 64),
            "Acquisition requires a valid source SHA-256");
    require(!(source.allow_stale_manifest_md5||source.allow_missing_manifest_assets) ||
                (!allow_unpinned && digest(source.sha256, 64) && !source.archive_url.empty()),
            "Manifest exceptions require a catalog-pinned archive source");
    if (allow_unpinned)
        require(source.workshop_id != 0
                    ? source.archive_url.empty() && source.archive_member.empty()
                    : source.archive_url.starts_with("https://"),
                "Explicit mod source must be one Workshop item or HTTPS URL");
    if (!source.archive_member.empty())
        archive_relative_path(fs::path(std::u8string(source.archive_member.begin(),
                                                    source.archive_member.end())).native());
    if (!source.package_root.empty())
        archive_relative_path(fs::path(std::u8string(source.package_root.begin(),
                                                    source.package_root.end())).native());
    initialize_mod_cache(cache);
    if (!source.sha256.empty())
        if (auto existing = cached_package(cache, source.target, source.sha256,
                                           !source.package_root.empty(),
                                           source.allow_stale_manifest_md5,
                                           source.allow_missing_manifest_assets))
            return *existing;
    const auto stage = fresh_stage(cache);
    std::string url = source.archive_url, origin = source.archive_url;
    std::optional<std::uint64_t> expected_bytes;
    if (source.workshop_id != 0) {
        require(source.archive_url.empty(), "Ambiguous source provider");
        const auto id = std::to_string(source.workshop_id);
        platform::download_https({"https://api.steampowered.com/ISteamRemoteStorage/"
                                  "GetPublishedFileDetails/v1/?format=xml",
                                  "itemcount=1&publishedfileids%5B0%5D=" + id},
                                 stage / "workshop.xml", 16 * 1024 * 1024);
        const auto details = parse_workshop_details(text_file(stage / "workshop.xml"), source.workshop_id);
        url = details.url;
        expected_bytes = details.bytes;
        origin = "https://steamcommunity.com/sharedfiles/filedetails/?id=" + id;
    }
    const auto nested = !source.archive_member.empty();
    require(!nested || (source.workshop_id == 0 && !source.archive_url.empty()),
            "Nested archive requires an HTTPS archive source");
    const auto downloaded = platform::download_https({url, {}},
                                                     stage / (nested ? "source.zip" : "archive.civ5mod"),
                                                     budget.maximum_expanded_bytes, std::move(progress));
    require(!expected_bytes || downloaded.bytes == *expected_bytes, "Workshop archive size mismatch");
    if (nested) {
        const auto extracted = extract_mod_zip(stage / "source.zip", stage / "source_content", budget);
        const auto member = archive_relative_path(fs::path(std::u8string(source.archive_member.begin(),
                                                                         source.archive_member.end())).native());
        require(extracted.entries.size() == 1 && !extracted.entries.front().directory &&
                    extracted.entries.front().relative == member,
                "Source ZIP must contain only the pinned archive member");
        const auto selected = package_file(stage / "source_content", source.archive_member);
        require(source.sha256.empty() || runtime::identify_file(selected).sha256 == source.sha256,
                "Nested mod archive SHA-256 mismatch");
        fs::rename(selected, stage / "archive.civ5mod");
        require(fs::canonical(stage / "source_content").parent_path() == fs::canonical(stage),
                "Nested extraction escaped staging directory");
        fs::remove_all(stage / "source_content");
        fs::remove(stage / "source.zip");
    }
    return publish_stage(stage, source.target, cache, source.sha256, budget, origin,
                         source.package_root, source.allow_stale_manifest_md5,
                         source.allow_missing_manifest_assets);
}
AcquiredMod acquire_mod(const ModSource &source, const fs::path &cache, platform::DownloadProgress progress,
                        ArchiveBudget budget) {
    return acquire_mod_source(source, cache, std::move(progress), budget, false);
}
AcquiredMod acquire_mod_from_url(const ModTarget &target, const std::string &url,
                                 const fs::path &cache, const std::string &expected_sha256,
                                 const std::string &archive_member, const std::string &package_root,
                                 platform::DownloadProgress progress, ArchiveBudget budget) {
    ModSource source;
    source.target = target;
    source.archive_url = url;
    source.sha256 = expected_sha256;
    source.archive_member = archive_member;
    source.package_root = package_root;
    return acquire_mod_source(source, cache, std::move(progress), budget, true);
}
AcquiredMod acquire_mod_from_workshop(const ModTarget &target, std::uint64_t workshop_id,
                                      const fs::path &cache, const std::string &expected_sha256,
                                      platform::DownloadProgress progress, ArchiveBudget budget) {
    require(workshop_id != 0, "Workshop item ID must be nonzero");
    ModSource source;
    source.target = target;
    source.workshop_id = workshop_id;
    source.sha256 = expected_sha256;
    return acquire_mod_source(source, cache, std::move(progress), budget, true);
}
AcquiredMod install_mod_archive(const fs::path &archive, const ModTarget &target, const fs::path &cache,
                                const std::string &expected_sha256, ArchiveBudget budget,
                                const std::string &package_root, bool allow_stale_manifest_md5,
                                bool allow_missing_manifest_assets) {
    require(!(allow_stale_manifest_md5||allow_missing_manifest_assets)||digest(expected_sha256,64),
            "Manifest exceptions require a pinned local archive");
    require(expected_sha256.empty() || digest(expected_sha256, 64), "Malformed expected archive hash");
    const auto identity = runtime::identify_file(archive);
    require(expected_sha256.empty() || identity.sha256 == expected_sha256,
            "Supplied archive SHA-256 mismatch");
    initialize_mod_cache(cache);
    if (auto existing = cached_package(cache, target, identity.sha256, !package_root.empty(),
                                       allow_stale_manifest_md5,allow_missing_manifest_assets))
        return *existing;
    const auto stage = fresh_stage(cache);
    require(fs::copy_file(archive, stage / "archive.civ5mod", fs::copy_options::none),
            "Cannot stage supplied archive");
    return publish_stage(stage, target, cache, identity.sha256, budget, "local_archive",
                         package_root, allow_stale_manifest_md5,allow_missing_manifest_assets);
}
} // namespace civ5::content
