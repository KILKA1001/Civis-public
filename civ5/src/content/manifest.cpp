#include "civ5/content/manifest.hpp"
#include "civ5/content/mod_acquisition.hpp"
#include "civ5/platform/windows_path.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include "civ5/runtime/file_identity.hpp"
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <limits>
#include <memory>
#include <set>
#include <shlwapi.h>
#include <stdexcept>
#include <string_view>
#include <xmllite.h>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
template <class T> struct Release {
    void operator()(T *value) const {
        if (value)
            value->Release();
    }
};
void require_hr(HRESULT hr) {
    if (FAILED(hr))
        throw std::runtime_error("XML read failed: " + std::to_string(static_cast<unsigned long>(hr)));
}
std::string utf8(const WCHAR *value, UINT count) {
    if (!count)
        return {};
    if (count > static_cast<UINT>(std::numeric_limits<int>::max()))
        throw std::runtime_error("XML text too large");
    const auto length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, static_cast<int>(count),
                                            nullptr, 0, nullptr, nullptr);
    if (!length)
        throw std::runtime_error("Invalid Unicode XML text");
    std::string result(static_cast<std::size_t>(length), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, static_cast<int>(count), result.data(),
                             length, nullptr, nullptr))
        throw std::runtime_error("XML Unicode conversion failed");
    return result;
}
std::string trim(const std::string &text) {
    const auto first = text.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return {};
    return text.substr(first, text.find_last_not_of(" \t\n\r") - first + 1);
}
std::string required_text(const XmlNode &node, const std::string &key) {
    const auto *child = node.child(key);
    if (!child || !child->children.empty() || trim(child->text).empty())
        throw std::runtime_error("Missing/scalar XML field: " + key);
    return trim(child->text);
}
bool reparse(const fs::path &path) {
    const auto attrs = GetFileAttributesW(platform::extended_windows_path(path).c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
        throw std::runtime_error("Cannot inspect path: " + assets::path_utf8(path));
    return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}
void safe_relative(const std::string &text) {
    if (text.empty() || text.find('\0') != std::string::npos || text.find(':') != std::string::npos ||
        !runtime::valid_utf8(text))
        throw std::runtime_error("Unsafe package path");
    const fs::path path = fs::path(std::u8string(text.begin(), text.end()));
    if (path.is_absolute() || path.has_root_path())
        throw std::runtime_error("Absolute package path");
    for (const auto &part : path) {
        const auto component = assets::path_utf8(part);
        if (component.empty() || component == ".." || component == "." || component.back() == '.' ||
            component.back() == ' ')
            throw std::runtime_error("Package path traversal or Windows path alias");
    }
}
struct PackageTreeEntry {
    std::string path;
    bool directory = false;
    std::uint64_t bytes = 0;
    std::string sha256;
    bool operator==(const PackageTreeEntry &) const = default;
};
std::vector<PackageTreeEntry> package_tree(const PackageManifest &package) {
    const auto root = package.path.parent_path();
    if (reparse(root)) throw std::runtime_error("Reparse point in mod package root");
    std::vector<PackageTreeEntry> entries;
    const auto extended_root=platform::extended_windows_path(root);
    for (fs::recursive_directory_iterator it(extended_root), end; it != end; ++it) {
        const auto relative=it->path().lexically_relative(extended_root);
        if (relative.empty()||relative.is_absolute()||*relative.begin()==fs::path(".."))
            throw std::runtime_error("Package entry escaped manifest root");
        const auto path=root/relative;
        if (reparse(path)) throw std::runtime_error("Reparse point in mod package tree");
        PackageTreeEntry entry;
        entry.path = assets::path_utf8(relative);
        if (it->is_directory())
            entry.directory = true;
        else if (it->is_regular_file()) {
            const auto identity = runtime::identify_file(path);
            entry.bytes = identity.bytes;
            entry.sha256 = identity.sha256;
        } else
            throw std::runtime_error("Special file in mod package tree");
        entries.push_back(std::move(entry));
    }
    std::sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) {
        return left.path < right.path;
    });
    return entries;
}
bool same_mod_package(const PackageManifest &left, const PackageManifest &right) {
    if (left.kind != PackageKind::mod || right.kind != PackageKind::mod ||
        left.id != right.id || left.version != right.version)
        return false;
    const auto a = runtime::identify_file(left.path);
    const auto b = runtime::identify_file(right.path);
    return a.bytes == b.bytes && a.sha256 == b.sha256 && package_tree(left) == package_tree(right);
}
bool manifest_hashes_match(const PackageManifest &package) {
    if (package.files.empty()) return false;
    for (const auto &file : package.files) {
        if (file.md5.size() != 32 ||
            !std::all_of(file.md5.begin(), file.md5.end(), [](unsigned char digit) {
                return std::isxdigit(digit) != 0;
            })) return false;
        if(!fs::exists(platform::extended_windows_path(package.path.parent_path()/
               fs::path(std::u8string(file.path.begin(),file.path.end())))))
            return false;
        if (runtime::ascii_lower(runtime::file_md5(package_file(package.path.parent_path(), file.path))) !=
            runtime::ascii_lower(file.md5)) return false;
    }
    return true;
}
bool only_declared_presentation_assets_differ(const PackageManifest &cached,
                                             const PackageManifest &local) {
    const auto a = package_tree(cached), b = package_tree(local);
    if (a.size() != b.size()) return false;
    std::set<std::string> declared;
    for (const auto &file : cached.files) declared.insert(assets::normalize_asset_name(file.path));
    const auto presentation_asset=[](const std::string &path){
        const auto normalized=assets::normalize_asset_name(path);
        static constexpr std::array<std::string_view,16> extensions={
            ".dds",".gr2",".fxsxml",".ggxml",".wav",".ogg",".mp3",".bnk",
            ".wem",".ttf",".otf",".png",".jpg",".jpeg",".bmp",".tga"};
        return std::any_of(extensions.begin(),extensions.end(),
            [&](std::string_view extension){return normalized.ends_with(extension);});
    };
    bool changed = false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].path != b[i].path || a[i].directory != b[i].directory) return false;
        if (a[i].bytes != b[i].bytes || a[i].sha256 != b[i].sha256) {
            if (a[i].directory || !declared.contains(assets::normalize_asset_name(a[i].path)) ||
                !presentation_asset(a[i].path))
                return false;
            changed = true;
        }
    }
    return changed;
}
bool below(const fs::path &root, const fs::path &path) {
    const auto relative = path.lexically_relative(root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != fs::path("..");
}
} // namespace
const XmlNode *XmlNode::child(const std::string &key) const {
    const XmlNode *result = nullptr;
    for (const auto &node : children)
        if (node.name == key) {
            if (result)
                throw std::runtime_error("Duplicate XML field: " + key);
            result = &node;
        }
    return result;
}
std::string XmlNode::attribute(const std::string &key) const {
    for (const auto &[name, value] : attributes)
        if (name == key)
            return value;
    return {};
}
static XmlNode read_xml_stream(IStream *stream, std::uint64_t maximum_bytes) {
    STATSTG stat{};
    require_hr(stream->Stat(&stat, STATFLAG_NONAME));
    if (stat.cbSize.QuadPart > maximum_bytes)
        throw std::runtime_error("XML exceeds resource budget");
    IXmlReader *raw_reader = nullptr;
    require_hr(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void **>(&raw_reader), nullptr));
    std::unique_ptr<IXmlReader, Release<IXmlReader>> reader(raw_reader);
    require_hr(reader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit));
    require_hr(reader->SetProperty(XmlReaderProperty_MaxElementDepth, 128));
    require_hr(reader->SetInput(stream));
    XmlNode document;
    std::vector<XmlNode *> stack{&document};
    XmlNodeType type;
    HRESULT hr;
    while ((hr = reader->Read(&type)) == S_OK) {
        const WCHAR *text = nullptr;
        UINT count = 0;
        if (type == XmlNodeType_Element) {
            require_hr(reader->GetQualifiedName(&text, &count));
            stack.back()->children.push_back(XmlNode{});
            auto *node = &stack.back()->children.back();
            node->name = utf8(text, count);
            const bool empty = reader->IsEmptyElement() != FALSE;
            auto attr_hr = reader->MoveToFirstAttribute();
            while (attr_hr == S_OK) {
                require_hr(reader->GetQualifiedName(&text, &count));
                const auto name = utf8(text, count);
                require_hr(reader->GetValue(&text, &count));
                node->attributes.emplace_back(name, utf8(text, count));
                attr_hr = reader->MoveToNextAttribute();
            }
            require_hr(attr_hr);
            require_hr(reader->MoveToElement());
            if (!empty)
                stack.push_back(node);
        } else if (type == XmlNodeType_EndElement) {
            if (stack.size() <= 1)
                throw std::runtime_error("Unexpected XML close element");
            stack.pop_back();
        } else if (type == XmlNodeType_Text || type == XmlNodeType_CDATA || type == XmlNodeType_Whitespace) {
            require_hr(reader->GetValue(&text, &count));
            stack.back()->text += utf8(text, count);
        }
    }
    require_hr(hr);
    if (stack.size() != 1 || document.children.size() != 1)
        throw std::runtime_error("Incomplete XML document");
    return std::move(document.children.front());
}
XmlNode read_xml(const fs::path &path, std::uint64_t maximum_bytes) {
    IStream *raw_stream = nullptr;
    require_hr(SHCreateStreamOnFileEx(path.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL,
                                      FALSE, nullptr, &raw_stream));
    std::unique_ptr<IStream, Release<IStream>> stream(raw_stream);
    return read_xml_stream(stream.get(), maximum_bytes);
}
XmlNode read_xml_metadata(const std::string &text) {
    constexpr std::uint64_t budget = 64ULL * 1024 * 1024;
    if (text.size() > budget)
        throw std::runtime_error("XML metadata exceeds resource budget");
    std::unique_ptr<IStream, Release<IStream>> stream(
        SHCreateMemStream(reinterpret_cast<const BYTE *>(text.data()), static_cast<UINT>(text.size())));
    if (!stream)
        throw std::runtime_error("Cannot allocate XML metadata stream");
    return read_xml_stream(stream.get(), budget);
}
std::uint64_t decimal(const std::string &value) {
    std::uint64_t result = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || end != value.data() + value.size() || value.empty())
        throw std::runtime_error("Invalid unsigned number: " + value);
    return result;
}
std::string canonical_guid(std::string value) {
    if (value.size() == 38 && value.front() == '{' && value.back() == '}')
        value = value.substr(1, 36);
    value = runtime::ascii_lower(value);
    if (value.size() != 36)
        throw std::runtime_error("Invalid DLC GUID");
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (value[i] == '-')
                continue;
        } else if ((value[i] >= '0' && value[i] <= '9') || (value[i] >= 'a' && value[i] <= 'f'))
            continue;
        throw std::runtime_error("Invalid DLC GUID");
    }
    return value;
}
std::string package_category_name(PackageCategory category) {
    switch (category) {
    case PackageCategory::common:
        return "Common";
    case PackageCategory::gameplay:
        return "Gameplay";
    case PackageCategory::ui_skin:
        return "UISkin";
    }
    throw std::runtime_error("Invalid package category");
}
std::vector<PackageResourceDeclaration> package_resources(const PackageManifest &package,
                                                          const PackageResourceQuery &query) {
    if (package.kind != PackageKind::dlc || runtime::ascii_lower(package.document.name) != "civ5package")
        throw std::runtime_error("Expected DLC package descriptor");
    if (query.selector.empty() || query.selector.find('\0') != std::string::npos)
        throw std::runtime_error("Expected nonempty XML resource selector");
    if (!query.category && query.category_occurrence != 0)
        throw std::runtime_error("Category occurrence requires a specific category");
    std::vector<std::string> segments;
    for (std::size_t start = 0; start < query.selector.size();) {
        const auto slash = query.selector.find('/', start);
        const auto end = slash == std::string::npos ? query.selector.size() : slash;
        if (end == start)
            throw std::runtime_error("Empty XML resource selector component");
        segments.push_back(query.selector.substr(start, end - start));
        start = end + 1;
    }
    if (query.selector.back() == '/')
        throw std::runtime_error("Empty XML resource selector component");
    std::vector<PackageResourceDeclaration> result;
    const auto visit = [&](const XmlNode &scope, std::optional<PackageCategory> category,
                           std::uint64_t occurrence) {
        const auto *parent = &scope;
        // FindFirstChild for intermediate selector components. The final
        // component uses FindFirst/NextSibling; do not recursively collect all.
        for (std::size_t i = 0; i + 1 < segments.size(); ++i) {
            const XmlNode *next = nullptr;
            for (const auto &child : parent->children)
                if (child.name == segments[i]) {
                    next = &child;
                    break;
                }
            parent = next;
            if (!parent)
                return;
        }
        for (const auto &node : parent->children) {
            if (node.name != segments.back())
                continue;
            if (node.children.empty())
                result.push_back({node.text, category, occurrence, false});
            else
                for (const auto &file : node.children)
                    if (file.name == "File")
                        result.push_back({file.text, category, occurrence, true});
        }
    };
    const auto visit_category = [&](PackageCategory category, std::uint64_t occurrence) {
        const auto name = package_category_name(category);
        const auto index = static_cast<std::size_t>(category);
        if (!query.enabled_categories[index])
            return;
        std::uint64_t ordinal = 0;
        for (const auto &node : package.document.children)
            if (node.name == name && ordinal++ == occurrence) {
                visit(node, category, occurrence);
                break;
            }
    };
    if (!query.category) {
        visit(package.document, {}, 0);
        for (const auto category :
             {PackageCategory::gameplay, PackageCategory::ui_skin, PackageCategory::common})
            visit_category(category, 0);
    } else {
        const auto category = *query.category;
        (void)package_category_name(category); // Validate before indexing.
        visit_category(category, query.category_occurrence);
        // Common additionally reads the root, even if the requested Common
        // occurrence does not exist. A disabled Common category reads neither.
        if (category == PackageCategory::common && query.enabled_categories[0])
            visit(package.document, {}, 0);
    }
    return result;
}
PackageManifest read_manifest(const fs::path &path) {
    PackageManifest result{};
    result.path = platform::unextended_windows_path(
        fs::canonical(platform::extended_windows_path(path)));
    result.document = read_xml(platform::extended_windows_path(path));
    const auto &root = result.document;
    if (root.name == "Civ5Package") {
        result.kind = PackageKind::dlc;
        result.id = canonical_guid(required_text(root, "GUID"));
        result.version = decimal(required_text(root, "Version"));
        if (const auto *names = root.child("Name"))
            for (const auto &name : names->children) {
                if (result.name.empty() || name.attribute("language") == "en_US")
                    result.name = trim(name.text);
            }
    } else if (root.name == "Mod") {
        result.kind = PackageKind::mod;
        // Civ V accepts authored IDs that are not RFC UUIDs (Semper Fidelis).
        result.id = runtime::ascii_lower(root.attribute("id"));
        if (result.id.empty())
            throw std::runtime_error("Empty mod identity");
        result.version = decimal(root.attribute("version"));
        if (const auto *props = root.child("Properties"))
            result.name = required_text(*props, "Name");
        if (const auto *list = root.child("Files"))
            for (const auto &file : list->children) {
                if (file.name != "File" || !file.children.empty())
                    throw std::runtime_error("Unsupported mod Files entry");
                const auto relative = trim(file.text);
                safe_relative(relative);
                // Real Semper Fidelis manifests repeat identical declarations.
                // Preserve their order, including differing metadata; resolving
                // their execution semantics belongs to activation, not XML parsing.
                const auto imported = file.attribute("import");
                if (!imported.empty() && imported != "0" && imported != "1")
                    throw std::runtime_error("Invalid mod import flag");
                result.files.push_back({relative, file.attribute("md5"), imported == "1"});
            }
    } else
        throw std::runtime_error("Unknown package XML root: " + root.name);
    return result;
}
fs::path package_file(const fs::path &root, const std::string &relative) {
    safe_relative(relative);
    auto current = platform::unextended_windows_path(
        fs::canonical(platform::extended_windows_path(root)));
    DWORD attrs = INVALID_FILE_ATTRIBUTES;
    for (const auto &part : fs::path(std::u8string(relative.begin(), relative.end()))) {
        current /= part;
        attrs = GetFileAttributesW(platform::extended_windows_path(current).c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            const auto error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
                throw std::runtime_error("Missing package file: " + relative);
            throw std::runtime_error("Cannot inspect path: " + assets::path_utf8(current));
        }
        if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)
            throw std::runtime_error("Reparse point in package file path");
    }
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
        throw std::runtime_error("Package file path is a directory: " + relative);
    return current;
}
Catalog discover_packages(const std::vector<fs::path> &roots) {
    Catalog result;
    std::set<fs::path> seen;
    std::vector<fs::path> paths;
    for (const auto &root : roots)
        try {
            if (!fs::is_directory(platform::extended_windows_path(root)) || reparse(root))
                throw std::runtime_error("Unavailable package root");
            const auto absolute_root=fs::absolute(root).lexically_normal();
            const auto extended_root=platform::extended_windows_path(absolute_root);
            for (fs::recursive_directory_iterator it(extended_root), end; it != end; ++it) {
                const auto relative=it->path().lexically_relative(extended_root);
                if (relative.empty()||relative.is_absolute()||*relative.begin()==fs::path(".."))
                    throw std::runtime_error("Package entry escaped discovery root");
                const auto path=absolute_root/relative;
                if (reparse(path)) {
                    it.disable_recursion_pending();
                    result.diagnostics.push_back("Skipped reparse point: " + assets::path_utf8(path));
                    continue;
                }
                if (!it->is_regular_file())
                    continue;
                const auto extension = runtime::ascii_lower(assets::path_utf8(path.extension()));
                if (extension == ".modinfo" || extension == ".civ5pkg") {
                    if (seen.insert(path).second)
                        paths.push_back(path);
                }
            }
        } catch (const std::exception &error) {
            result.diagnostics.push_back(assets::path_utf8(root) + ": " + error.what());
        }
    std::sort(paths.begin(), paths.end()); // Stable diagnostics only; never activation order.
    for (const auto &path : paths)
        try {
            auto package = read_manifest(path);
            bool mirror = false;
            if (package.kind == PackageKind::mod)
                for (const auto &existing : result.packages)
                    if (existing.kind == package.kind && existing.id == package.id &&
                        existing.version == package.version) {
                        try {
                            if (same_mod_package(existing, package)) {
                                mirror = true;
                                break;
                            }
                        } catch (const std::exception &error) {
                            result.diagnostics.push_back("Cannot compare duplicate mod packages: " +
                                assets::path_utf8(existing.path) + " / " + assets::path_utf8(package.path) +
                                ": " + error.what());
                        }
                    }
            if (!mirror) result.packages.push_back(std::move(package));
        } catch (const std::exception &error) {
            result.diagnostics.push_back(assets::path_utf8(path) + ": " + error.what());
        }
    return result;
}
std::uint64_t prefer_verified_cached_mods(Catalog &catalog, const fs::path &cache_root) {
    if (!fs::is_directory(cache_root)) return 0;
    const auto root = fs::canonical(cache_root);
    std::set<fs::path> discard;
    for (const auto &cached : catalog.packages) {
        if (cached.kind != PackageKind::mod || !below(root, cached.path)) continue;
        const auto has_local = std::any_of(catalog.packages.begin(), catalog.packages.end(),
            [&](const auto &candidate) {
                return candidate.kind == PackageKind::mod && !below(root, candidate.path) &&
                    candidate.id == cached.id && candidate.version == cached.version;
            });
        if (!has_local) continue;
        bool verified = false;
        try {
            verified = manifest_hashes_match(cached);
            if (!verified) {
                const auto acquired = verify_cached_mod_package(root.parent_path(), cached);
                verified = acquired.has_value() &&
                    (!acquired->stale_manifest_hashes.empty()||
                     !acquired->missing_manifest_assets.empty());
            }
        }
        catch (const std::exception &) { continue; }
        if (!verified) continue;
        const auto manifest = runtime::identify_file(cached.path);
        for (const auto &local : catalog.packages) {
            if (&local == &cached || local.kind != PackageKind::mod || below(root, local.path) ||
                local.id != cached.id || local.version != cached.version) continue;
            try {
                const auto candidate = runtime::identify_file(local.path);
                if (candidate.bytes != manifest.bytes || candidate.sha256 != manifest.sha256 ||
                    !only_declared_presentation_assets_differ(cached, local) ||
                    manifest_hashes_match(local)) continue;
                discard.insert(local.path);
            } catch (const std::exception &) {
                // An unreadable or unsafe candidate remains ambiguous.
            }
        }
    }
    std::erase_if(catalog.packages, [&](const auto &package) { return discard.contains(package.path); });
    return discard.size();
}
} // namespace civ5::content
