#include "civ5/ui/selected_bnw_research_lua_inventory.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <fstream>
#include <set>
#include <stdexcept>
#include <tuple>

namespace civ5::ui {
namespace {
namespace fs = std::filesystem;

std::string lower(std::string value) {
    for (auto &c : value)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return value;
}

std::string relative_key(const fs::path &file, const fs::path &root) {
    return assets::path_utf8(file.lexically_relative(root));
}

bool within(const fs::path &candidate, const fs::path &root) {
    const auto c = lower(assets::path_utf8(candidate.lexically_normal()));
    auto r = lower(assets::path_utf8(root.lexically_normal()));
    if (!r.empty() && r.back() != '/') r.push_back('/');
    return c == r.substr(0, r.size() - 1) || c.starts_with(r);
}

void add_contexts(const content::XmlNode &node, const std::string &document,
                  const std::string &sha, SelectedBnwResearchLuaInventory &result) {
    if (node.name == "LuaContext") {
        const auto source = node.attribute("FileName");
        if (!source.empty())
            result.context_declarations.push_back({document, sha,
                result.context_declarations.size(), node.attribute("ID"), source});
    }
    for (const auto &child : node.children) add_contexts(child, document, sha, result);
}

void scan_archive(const fs::path &file, const fs::path &game_root,
                  SelectedBnwResearchLuaInventory &result) {
    ++result.fpk_archives_inspected;
    const auto archive = relative_key(file, game_root);
    const auto opened = assets::FpkSource::open(file, archive);
    if (const auto *source = std::get_if<std::shared_ptr<assets::FpkSource>>(&opened)) {
        for (const auto &item : (*source)->entries()) {
            if (!lower(item.original_name).ends_with(".lua")) continue;
            const auto lookup = (*source)->find(assets::AssetKey(item.original_name));
            if (const auto *handle = std::get_if<assets::AssetHandle>(&lookup)) {
                const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
                const auto id = runtime::identify_bytes(bytes);
                const std::string contents(bytes.begin(), bytes.end());
                result.lua_sources.push_back({item.original_name, archive,
                    id.sha256, id.bytes, true,
                    contents.find("PlayerCanResearch") != std::string::npos ||
                    contents.find("PlayerCanEverResearch") != std::string::npos});
            } else {
                result.unresolved.push_back(archive + ": ambiguous Lua entry " + item.original_name);
            }
        }
    } else {
        result.unresolved.push_back(archive + ": " +
            std::get<assets::FpkDiagnostic>(opened).reason);
    }
}

void scan_tree(const fs::path &tree, const fs::path &game_root,
               SelectedBnwResearchLuaInventory &result) {
    if (!fs::exists(tree)) return;
    if (!fs::is_directory(tree)) throw std::runtime_error("Lua inventory root is not a directory");
    for (const auto &entry : fs::recursive_directory_iterator(tree)) {
        if (!entry.is_regular_file()) continue;
        const auto file = entry.path();
        const auto ext = lower(assets::path_utf8(file.extension()));
        if (ext == ".lua") {
            const auto id = runtime::identify_file(file);
            std::ifstream stream(file, std::ios::binary);
            std::string contents((std::istreambuf_iterator<char>(stream)),
                                 std::istreambuf_iterator<char>());
            if (!stream.eof() && stream.fail()) throw std::runtime_error("Lua source read failed");
            result.lua_sources.push_back({relative_key(file, game_root),
                assets::path_utf8(file), id.sha256, id.bytes, false,
                contents.find("PlayerCanResearch") != std::string::npos ||
                contents.find("PlayerCanEverResearch") != std::string::npos});
        } else if (ext == ".fpk") scan_archive(file, game_root, result);
    }
}

void scan_other_archives(const fs::path &source_root, const fs::path &game_root,
                         bool base, SelectedBnwResearchLuaInventory &result) {
    for (auto it = fs::recursive_directory_iterator(source_root);
         it != fs::recursive_directory_iterator(); ++it) {
        const auto &entry = *it;
        if (entry.is_directory()) {
            const auto name = lower(assets::path_utf8(entry.path().filename()));
            if ((base && name == "dlc") || name == "ui" || name == "gameplay")
                it.disable_recursion_pending();
        } else if (entry.is_regular_file() &&
                   lower(assets::path_utf8(entry.path().extension())) == ".fpk") {
            scan_archive(entry.path(), game_root, result);
        }
    }
}

} // namespace

SelectedBnwResearchLuaInventory inventory_selected_bnw_research_lua(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile) {
    SelectedBnwResearchLuaInventory result;
    if (!profile.all_requirements_located || !profile.enabled_mods.empty()) {
        result.unresolved.push_back("Selected stock BNW profile is incomplete or includes mods");
        return result;
    }
    try {
        const auto root = fs::canonical(game_root);
        const auto assets_root = root / "Assets";
        if (!fs::is_directory(assets_root)) throw std::runtime_error("Assets root is missing");
        std::vector<fs::path> roots{assets_root};
        std::set<std::size_t> unique;
        for (const auto index : profile.enabled_dlc) {
            if (index >= catalog.packages.size() || !unique.insert(index).second ||
                catalog.packages[index].kind != content::PackageKind::dlc)
                throw std::runtime_error("Selected DLC index is invalid or repeated");
            const auto package_root = fs::canonical(catalog.packages[index].path.parent_path());
            if (!within(package_root, assets_root / "DLC"))
                throw std::runtime_error("Selected DLC escapes installed Assets/DLC");
            roots.push_back(package_root);
        }
        for (std::size_t i = 0; i < roots.size(); ++i) {
            const auto &source_root = roots[i];
            const auto document = source_root / "UI/InGame/InGame.xml";
            if (fs::exists(document)) {
                const auto id = runtime::identify_file(document);
                const auto xml = content::read_xml(document);
                if (xml.name != "Context")
                    result.unresolved.push_back(relative_key(document, root) + ": no Context root");
                else add_contexts(xml, relative_key(document, root), id.sha256, result);
            }
            scan_tree(source_root / "UI", root, result);
            scan_tree(source_root / "Gameplay", root, result);
            // Base archives outside UI/Gameplay may contain stock Lua. DLC
            // archives are scoped to the selected package, never other DLC.
            scan_other_archives(source_root, root, i == 0, result);
        }
        std::sort(result.lua_sources.begin(), result.lua_sources.end(),
                  [](const auto &a, const auto &b) {
                      return std::tie(a.source_path, a.key) < std::tie(b.source_path, b.key);
                  });
    } catch (const std::exception &error) {
        result.unresolved.push_back(error.what());
    }
    result.unresolved.push_back(
        "Stock UI LuaContext activation, hidden contexts, includes and native listener order are unverified");
    return result;
}

} // namespace civ5::ui
