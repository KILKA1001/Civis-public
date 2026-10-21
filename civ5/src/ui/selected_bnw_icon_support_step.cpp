#include "civ5/ui/selected_bnw_icon_support_step.hpp"
#include "civ5/ui/selected_bnw_stock_ui_lua_step.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/assets/asset_source.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace civ5::ui {
namespace {
namespace fs = std::filesystem;

std::string read_source(const fs::path &path, std::string &sha256) {
    const auto identity = runtime::identify_file(path);
    if (identity.bytes > 64ULL * 1024 * 1024)
        throw std::runtime_error("Selected UI Lua source exceeds 64 MiB");
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Selected UI Lua source cannot be opened");
    const std::string source((std::istreambuf_iterator<char>(stream)),
                             std::istreambuf_iterator<char>());
    if (!stream.eof() && stream.fail())
        throw std::runtime_error("Selected UI Lua source cannot be read");
    const auto bytes = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t *>(source.data()), source.size());
    if (runtime::identify_bytes(bytes).sha256 != identity.sha256)
        throw std::runtime_error("Selected UI Lua source changed during read");
    sha256 = identity.sha256;
    return source;
}

bool selected_override(const content::Catalog &catalog,
                       const runtime::SavedContentProfile &profile,
                       std::string_view filename) {
    for (const auto index : profile.enabled_dlc) {
        const auto &root = catalog.packages.at(index).path.parent_path();
        const auto ui = root / "UI";
        if (fs::exists(ui))
            for (const auto &entry : fs::recursive_directory_iterator(ui))
                if (entry.is_regular_file() &&
                    assets::path_utf8(entry.path().filename()) == filename)
                    return true;
        const auto gameplay = root / "Gameplay";
        if (fs::exists(gameplay))
            for (const auto &entry : fs::recursive_directory_iterator(gameplay))
                if (entry.is_regular_file() &&
                    assets::path_utf8(entry.path().filename()) == filename)
                    return true;
    }
    return false;
}

} // namespace

SelectedBnwIconSupportStep execute_selected_bnw_icon_support_include(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected) {
    SelectedBnwIconSupportStep result;
    try {
        const auto first = execute_selected_bnw_first_stock_ui_include(
            game_root, catalog, profile);
        if (first.status != SelectedBnwStockUiLuaStatus::first_include_executed)
            throw std::runtime_error("First CivilopediaScreen include is blocked: " + first.blocker);
        result.context_id = first.context_id;
        result.context_source_sha256 = first.context_source_sha256;
        result.instance_manager_sha256 = first.include_sha256;
        const auto &session = selected.session();
        const auto &state = session.state();
        const auto &identity = selected.identity();
        if (identity.payload_sha256 !=
                runtime::identify_bytes(session.source().source.bytes).sha256 ||
            identity.rules_sha256 != runtime::identify_bytes(
                selected.database().gameplay_fingerprint_data()).sha256 ||
            state.setup.profile != content::TechnologyProfile::bnw ||
            selected.active_player_id() < 0 ||
            !state.setup.players.contains(selected.active_player_id()) ||
            !state.setup.players.at(selected.active_player_id()).alive)
            throw std::runtime_error("Selected live BNW owner identity or setup changed");
        if (selected_override(catalog, profile, "IconSupport.lua") ||
            selected_override(catalog, profile, "FLuaVector.lua"))
            throw std::runtime_error("Selected DLC overrides IconSupport dependency; UI VFS order unverified");
        const auto root = fs::canonical(game_root);
        result.icon_support_path = assets::path_utf8(root / "Assets/UI/IconSupport.lua");
        result.vector_path = assets::path_utf8(root / "Assets/Gameplay/Lua/FLuaVector.lua");
        const auto icon = read_source(root / "Assets/UI/IconSupport.lua",
                                      result.icon_support_sha256);
        const auto vector = read_source(root / "Assets/Gameplay/Lua/FLuaVector.lua",
                                        result.vector_sha256);
        std::string instance_sha;
        const auto instance = read_source(root / "Assets/UI/InstanceManager.lua", instance_sha);
        if (instance_sha != first.include_sha256 ||
            icon.find("include( \"FLuaVector\" );") == std::string::npos ||
            icon.find("Events.AfterModsDeactivate.Add(") == std::string::npos ||
            icon.find("Events.AfterModsActivate.Add(") == std::string::npos)
            throw std::runtime_error("Selected IconSupport authored include/event contract changed");

        auto detached_setup = state.setup;
        // ScriptQuery follows retail DB.Query semantics and can write its DB.
        // The diagnostic Lua VM therefore receives a private rules clone.
        auto detached_database = selected.database().clone_gameplay();
        auto types = detached_database->cache_info_types();
        if (!types.report().all_succeeded())
            throw std::runtime_error("Selected GameInfo type catalog is incomplete");
        runtime::ScriptSourceProvider sources;
        sources.files = [](std::string_view) {
            return std::vector<std::string>{"Assets/Gameplay/Lua/FLuaVector.lua"};
        };
        sources.read = [vector](std::string_view, std::string_view path) {
            if (path != "Assets/Gameplay/Lua/FLuaVector.lua")
                throw runtime::ScriptError("Unexpected selected UI include");
            return vector;
        };
        runtime::ScriptUiEventProvider ui_events;
        // Names come from the verified IconSupport bytes. No native event is
        // emitted and these registrations live only in this discarded VM.
        ui_events.notifications = {"AfterModsDeactivate", "AfterModsActivate"};
        runtime::LuaScriptHost host({64ULL * 1024 * 1024, 2'000'000},
            std::move(sources), {},
            runtime::lua_gameplay_services(*detached_database, detached_setup,
                                           state.technologies, types),
            {}, {}, {}, std::move(ui_events));
        host.execute(result.context_id, instance, "@Assets/UI/InstanceManager.lua");
        host.execute(result.context_id, icon, "@Assets/UI/IconSupport.lua");
        host.execute(result.context_id,
            "assert(type(IconLookup)=='function' and "
            "type(GameInfo.IconTextureAtlases)=='userdata' and "
            "#GameInfo.IconTextureAtlases>0 and "
            "Events.AfterModsDeactivate.Count()==1 and "
            "Events.AfterModsActivate.Count()==1)",
            "=IconSupport-owner-check");
        result.registered_invalidation_listeners = 2;
        result.status = SelectedBnwIconSupportStatus::include_executed;
        result.blocker = "Full CivilopediaScreen/native UI event lifecycle and research TestAll remain unverified";
    } catch (const std::exception &error) {
        result.blocker = error.what();
    }
    return result;
}

} // namespace civ5::ui
