#include "civ5/ui/selected_bnw_stock_ui_lua_step.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include <algorithm>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace civ5::ui {
namespace {
namespace fs = std::filesystem;

const content::XmlNode *first_lua_context(const content::XmlNode &node) {
    if (node.name == "LuaContext") return &node;
    for (const auto &child : node.children)
        if (const auto *found = first_lua_context(child)) return found;
    return nullptr;
}

std::string exact_text(const fs::path &path, std::string &sha256) {
    const auto identity = runtime::identify_file(path);
    if (identity.bytes > 64ULL * 1024 * 1024)
        throw std::runtime_error("Selected stock UI Lua source exceeds 64 MiB");
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Selected stock UI Lua source cannot be opened");
    std::string data((std::istreambuf_iterator<char>(stream)),
                     std::istreambuf_iterator<char>());
    if (!stream.eof() && stream.fail())
        throw std::runtime_error("Selected stock UI Lua source cannot be read");
    if (runtime::identify_bytes(std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t *>(data.data()), data.size())).sha256 !=
        identity.sha256)
        throw std::runtime_error("Selected stock UI Lua source changed during read");
    sha256 = identity.sha256;
    return data;
}

bool first_include_is_instance_manager(const std::string &script) {
    std::istringstream input(script);
    std::string line;
    while (std::getline(input, line)) {
        const auto begin = line.find_first_not_of(" \t\r");
        if (begin == std::string::npos || line.compare(begin, 2, "--") == 0)
            continue;
        const auto end = line.find_last_not_of(" \t\r");
        const auto active = line.substr(begin, end - begin + 1);
        return active == "include( \"InstanceManager\" );" ||
               active == "include( 'InstanceManager' );";
    }
    return false;
}

} // namespace

SelectedBnwStockUiLuaStep execute_selected_bnw_first_stock_ui_include(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile) {
    SelectedBnwStockUiLuaStep result;
    try {
        if (!profile.can_activate || !profile.all_requirements_located ||
            !profile.enabled_mods.empty())
            throw std::runtime_error("Selected stock BNW profile is incomplete or includes mods");
        const auto root = fs::canonical(game_root);
        constexpr std::string_view expansion2_id =
            "6da07636-4123-4018-b643-6575b4ec336b";
        std::optional<fs::path> expansion2;
        for (const auto index : profile.enabled_dlc) {
            if (index >= catalog.packages.size())
                throw std::runtime_error("Selected DLC index is outside catalog");
            const auto &package = catalog.packages[index];
            if (package.kind != content::PackageKind::dlc)
                throw std::runtime_error("Selected package is not DLC");
            if (content::canonical_guid(package.id) == expansion2_id) {
                if (expansion2) throw std::runtime_error("Selected Expansion2 package is ambiguous");
                expansion2 = fs::canonical(package.path.parent_path());
            }
        }
        if (!expansion2 || !fs::equivalent(*expansion2, root / "Assets/DLC/Expansion2"))
            throw std::runtime_error("Selected Expansion2 UI source is not exact installed DLC");
        const auto xml = *expansion2 / "UI/InGame/InGame.xml";
        result.declaration_path = assets::path_utf8(xml);
        result.declaration_sha256 = runtime::identify_file(xml).sha256;
        const auto document = content::read_xml(xml);
        const auto *first = first_lua_context(document);
        if (document.name != "Context" || !first ||
            first->attribute("ID") != "CivilopediaScreen" ||
            first->attribute("FileName") != "Assets/UI/Civilopedia/CivilopediaScreen")
            throw std::runtime_error("Selected InGame.xml first LuaContext is not CivilopediaScreen");
        result.context_id = "CivilopediaScreen";
        const auto script = *expansion2 / "UI/Civilopedia/CivilopediaScreen.lua";
        result.context_source_path = assets::path_utf8(script);
        const auto source = exact_text(script, result.context_source_sha256);
        if (!first_include_is_instance_manager(source))
            throw std::runtime_error("Selected CivilopediaScreen first statement is not InstanceManager include");
        for (const auto index : profile.enabled_dlc) {
            const auto &package = catalog.packages[index];
            if (fs::exists(package.path.parent_path() / "UI/InstanceManager.lua"))
                throw std::runtime_error("Selected DLC overrides InstanceManager; include precedence unverified");
        }
        const auto include = root / "Assets/UI/InstanceManager.lua";
        result.include_path = assets::path_utf8(include);
        const auto include_source = exact_text(include, result.include_sha256);
        runtime::LuaScriptHost host({8ULL * 1024 * 1024, 1'000'000});
        host.execute(result.context_id, include_source, "@Assets/UI/InstanceManager.lua");
        host.execute(result.context_id,
            "assert(type(InstanceManager)=='table' and type(InstanceManager.new)=='function')",
            "=InstanceManager-shape-check");
        result.status = SelectedBnwStockUiLuaStatus::first_include_executed;
        result.blocker = "Full stock UI context activation, GameInfo/Controls/events and TestAll order remain unverified";
    } catch (const std::exception &error) {
        result.blocker = error.what();
    }
    return result;
}

} // namespace civ5::ui
