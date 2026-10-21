#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/manifest.hpp"
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
fs::path utf8_path(const std::string &value) { return fs::path(std::u8string(value.begin(), value.end())); }
void shape(const XmlNode &node, std::initializer_list<std::string> allowed) {
    if (node.text.find_first_not_of(" \t\r\n") != std::string::npos)
        throw std::runtime_error("Unexpected text in loose resource plan");
    for (const auto &[name, value] : node.attributes)
        if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
            throw std::runtime_error("Unknown loose resource plan attribute: " + name);
}
std::string required(const XmlNode &node, const std::string &name) {
    const auto value = node.attribute(name);
    if (value.empty())
        throw std::runtime_error("Missing loose resource plan attribute: " + name);
    return value;
}
bool flag(const XmlNode &node, const std::string &name, bool fallback) {
    for (const auto &[attribute, value] : node.attributes) {
        if (attribute != name)
            continue;
        if (value == "true")
            return true;
        if (value == "false")
            return false;
        throw std::runtime_error("Expected true/false for loose resource plan flag");
    }
    return fallback;
}
std::shared_ptr<assets::LooseResourceIndex> source(const fs::path &root, const XmlNode &node) {
    auto result = std::make_shared<assets::LooseResourceIndex>(root);
    for (const auto &entry : node.children) {
        if (entry.name == "Scan") {
            shape(entry, {"directory", "excludeScenariosAndSpeech"});
            assets::LooseResourceScan scan{
                utf8_path(required(entry, "directory")), {}, flag(entry, "excludeScenariosAndSpeech", false)};
            for (const auto &excluded : entry.children) {
                shape(excluded, {"directory"});
                if (excluded.name != "Exclude" || !excluded.children.empty())
                    throw std::runtime_error("Expected Exclude in resource Scan");
                scan.excluded_directories.push_back(utf8_path(required(excluded, "directory")));
            }
            result->scan(scan);
        } else if (entry.name == "File") {
            shape(entry, {"path"});
            if (!entry.children.empty())
                throw std::runtime_error("Unexpected children in resource File");
            result->add_file(utf8_path(required(entry, "path")));
        } else
            throw std::runtime_error("Expected Scan or File in resource source");
    }
    return result;
}
fs::path selected_relative(const std::string &value) {
    if (value.empty() || value.find('\0') != std::string::npos)
        throw std::runtime_error("Empty selected DLC resource path");
    for (std::size_t first = 0; first < value.size();) {
        const auto last = value.find_first_of("/\\", first);
        const auto end = last == std::string::npos ? value.size() : last;
        const auto part = value.substr(first, end - first);
        if (part.empty() || part == "." || part == ".." || part.find(':') != std::string::npos ||
            part.back() == '.' || part.back() == ' ')
            throw std::runtime_error("Unsafe selected DLC resource path");
        first = end == value.size() ? end : end + 1;
        if (first == value.size() && end != value.size())
            throw std::runtime_error("Trailing separator in selected DLC resource path");
    }
    const auto path = utf8_path(value);
    if (path.is_absolute() || path.has_root_path())
        throw std::runtime_error("Absolute selected DLC resource path");
    return path;
}
std::vector<fs::path> declarations(const PackageManifest &package, PackageCategory category,
                                   std::uint64_t occurrence, const std::string &selector) {
    PackageResourceQuery query;
    query.selector = selector;
    query.enabled_categories = {true, true, true};
    query.category = category;
    query.category_occurrence = occurrence;
    std::vector<fs::path> result;
    for (const auto &entry : package_resources(package, query))
        result.push_back(selected_relative(entry.value));
    return result;
}
void append(std::vector<fs::path> &destination, const std::vector<fs::path> &source) {
    destination.insert(destination.end(), source.begin(), source.end());
}
std::vector<fs::path> directories(const PackageManifest &package, PackageCategory category,
                                  std::uint64_t occurrence = 0) {
    auto result = declarations(package, category, occurrence, "Directory");
    append(result, declarations(package, category, occurrence, "MapDirectory"));
    return result;
}
} // namespace
LooseResourcePlan read_loose_resource_plan(const fs::path &path) {
    const auto document = read_xml(path);
    shape(document, {"root", "defaultEnabled"});
    if (document.name != "LooseResourceMountPlan")
        throw std::runtime_error("Expected LooseResourceMountPlan");
    auto root = utf8_path(required(document, "root"));
    if (!root.is_absolute())
        root = path.parent_path() / root;
    root = fs::canonical(root);
    const auto *base = document.child("Default");
    if (!base)
        throw std::runtime_error("Loose resource plan needs Default source");
    shape(*base, {});
    const auto base_source = source(root, *base);
    LooseResourcePlan plan{assets::ResourceMountTable(assets::resource_lookup(base_source),
                                                     assets::resource_lookup_all(base_source),
                                                     assets::resource_keys(base_source),{base_source}),
                           root,
                           base_source->indexed_files()};
    plan.resources.set_default_enabled(flag(document, "defaultEnabled", true));
    std::unordered_set<std::uint64_t> ids;
    for (const auto &entry : document.children) {
        if (&entry == base)
            continue;
        shape(entry, {"id", "group", "priority", "enabled", "preferred"});
        if (entry.name != "Mount")
            throw std::runtime_error("Expected Mount after Default resource source");
        const auto id = decimal(required(entry, "id"));
        if (!ids.insert(id).second)
            throw std::runtime_error("Duplicate resource mount ID");
        const auto mounted = source(root, entry);
        if (mounted->indexed_files() > std::numeric_limits<std::uint64_t>::max() - plan.indexed_files)
            throw std::runtime_error("Resource plan count overflow");
        plan.indexed_files += mounted->indexed_files();
        plan.resources.mount({id, decimal(required(entry, "group")), decimal(required(entry, "priority")),
                              assets::resource_lookup(mounted), flag(entry, "enabled", true),
                              flag(entry, "preferred", true), assets::resource_lookup_all(mounted),
                              assets::resource_keys(mounted),mounted});
    }
    return plan;
}
LooseResourcePlan build_selected_loose_resource_plan(const fs::path &game_root,
                                                     std::span<const std::size_t> enabled_dlc,
                                                     const Catalog &catalog) {
    const auto root = fs::canonical(game_root);
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    base->scan({"Assets/Gameplay/XML", {}, true});
    base->scan({"Assets/Sounds/XML", {}, true});
    std::vector<std::shared_ptr<assets::LooseResourceIndex>> defaults{base};
    std::vector<std::shared_ptr<assets::LooseResourceIndex>> mounted;
    std::uint64_t indexed = base->indexed_files();
    const auto count = [&](const std::shared_ptr<assets::LooseResourceIndex> &index) {
        if (index->indexed_files() > UINT64_MAX - indexed)
            throw std::overflow_error("Selected resource index count overflow");
        indexed += index->indexed_files();
    };
    for (const auto package_index : enabled_dlc) {
        const auto &package = catalog.packages.at(package_index);
        if (package.kind != PackageKind::dlc)
            throw std::runtime_error("Selected resource plan requires DLC packages");
        const auto package_root = fs::canonical(package.path.parent_path());
        std::vector<fs::path> skin_exclusions;
        std::uint64_t skins = 0;
        for (const auto &child : package.document.children)
            if (child.name == "UISkin") ++skins;
        for (std::uint64_t occurrence = 0; occurrence < skins; ++occurrence) {
            append(skin_exclusions, declarations(package, PackageCategory::ui_skin, occurrence,
                                                 "Skin/Directory"));
            append(skin_exclusions, declarations(package, PackageCategory::ui_skin, occurrence,
                                                 "GameplaySkin/Directory"));
        }
        for (const auto category : {PackageCategory::common, PackageCategory::gameplay}) {
            const auto own_directories = directories(package, category);
            auto exclusions = directories(package, category == PackageCategory::common
                                                        ? PackageCategory::gameplay
                                                        : PackageCategory::common);
            append(exclusions, skin_exclusions);
            if (category == PackageCategory::common && own_directories.empty()) {
                auto fallback = std::make_shared<assets::LooseResourceIndex>(package_root);
                fallback->scan({".", exclusions, true});
                count(fallback);
                defaults.push_back(std::move(fallback));
            }
            const auto files = declarations(package, category, 0, "File");
            if (own_directories.empty() && files.empty()) continue;
            auto index = std::make_shared<assets::LooseResourceIndex>(package_root);
            for (const auto &directory : own_directories)
                index->scan({directory, exclusions, true});
            for (const auto &file : files)
                index->add_file(file);
            count(index);
            mounted.push_back(std::move(index));
        }
    }
    auto default_lookup = [defaults](const assets::AssetKey &key) {
        assets::ResourceMountHit result;
        for (const auto &index : defaults) {
            auto hit = assets::resource_lookup(index)(key);
            if (!std::holds_alternative<std::monostate>(hit.asset)) result = std::move(hit);
        }
        return result;
    };
    auto default_all = [defaults](const assets::AssetKey &key) {
        std::vector<assets::ResourceMountHit> result;
        for (const auto &index : defaults) {
            auto hits = assets::resource_lookup_all(index)(key);
            result.insert(result.end(), std::make_move_iterator(hits.begin()),
                          std::make_move_iterator(hits.end()));
        }
        return result;
    };
    auto default_keys = [defaults](std::string_view prefix) {
        std::vector<std::string> result;
        for (const auto &index : defaults) {
            auto keys = index->keys(prefix);
            result.insert(result.end(), std::make_move_iterator(keys.begin()),
                          std::make_move_iterator(keys.end()));
        }
        return result;
    };
    std::vector<std::shared_ptr<const assets::AssetSource>> port_defaults(defaults.begin(),defaults.end());
    LooseResourcePlan plan{assets::ResourceMountTable(default_lookup, default_all, default_keys,std::move(port_defaults)),
                           root, indexed};
    std::uint64_t id = 0;
    for (const auto &index : mounted)
        plan.resources.mount({++id, 0, 0, assets::resource_lookup(index), true, true,
                              assets::resource_lookup_all(index), assets::resource_keys(index),index});
    return plan;
}
} // namespace civ5::content
