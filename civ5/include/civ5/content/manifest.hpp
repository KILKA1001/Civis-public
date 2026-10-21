#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace civ5::content {
struct XmlNode {
    std::string name, text;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<XmlNode> children;
    const XmlNode *child(const std::string &key) const;
    std::string attribute(const std::string &key) const;
};
XmlNode read_xml(const std::filesystem::path &path, std::uint64_t maximum_bytes = 64 * 1024 * 1024);
// Small network/manifest metadata, with an explicit 64 MiB resource budget.
XmlNode read_xml_metadata(const std::string &text);
enum class PackageKind { dlc, mod };
struct ModFile {
    std::string path, md5;
    bool import = false;
};
struct PackageManifest {
    PackageKind kind;
    std::filesystem::path path;
    std::string id, name;
    std::uint64_t version = 0;
    std::vector<ModFile> files;
    // Preserve authored actions, dependencies, references, blocks, entry points,
    // audio declarations and unknown extensions without changing their order.
    XmlNode document;
};
PackageManifest read_manifest(const std::filesystem::path &path);
enum class PackageCategory { common, gameplay, ui_skin };
std::string package_category_name(PackageCategory category);
struct PackageResourceDeclaration {
    std::string value;
    // nullopt identifies the package root, which is distinct from Common.
    std::optional<PackageCategory> category;
    std::uint64_t category_occurrence = 0;
    bool file_list = false;
};
struct PackageResourceQuery {
    std::string selector;
    // Common, Gameplay, UISkin. The caller supplies actual category activation;
    // merely discovering a descriptor does not activate it.
    std::array<bool, 3> enabled_categories{};
    // nullopt: root -> first Gameplay -> first UISkin -> first Common.
    std::optional<PackageCategory> category;
    std::uint64_t category_occurrence = 0;
};
// The descriptor traversal of EXE 0x54B8D0/0x5955F0. Selectors are XML paths,
// not filesystem paths. Returned values still need the shared VFS resolver.
// Package eligibility/ownership is checked by the caller, not inferred here.
std::vector<PackageResourceDeclaration> package_resources(const PackageManifest &package,
                                                          const PackageResourceQuery &query);
std::string canonical_guid(std::string value);
std::uint64_t decimal(const std::string &value);
std::filesystem::path package_file(const std::filesystem::path &root, const std::string &relative);
struct Catalog {
    std::vector<PackageManifest> packages;
    std::vector<std::string> diagnostics;
};
// An unavailable or malformed root is reported, never silently skipped.
Catalog discover_packages(const std::vector<std::filesystem::path> &roots);
// Prefer a fully verified cached copy when it matches authored MD5s or has an
// explicit pinned-archive receipt for stale MD5s, and a local mirror differs
// solely in declared presentation assets. Changed SQL/XML/Lua or native code
// may be the save's actual rules, so same-ID/version variants stay ambiguous.
std::uint64_t prefer_verified_cached_mods(Catalog &catalog,
                                          const std::filesystem::path &cache_root);
} // namespace civ5::content
