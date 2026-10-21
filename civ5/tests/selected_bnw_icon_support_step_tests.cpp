#include "civ5/ui/selected_bnw_icon_support_step.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

std::string read(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Selected stock Lua source is missing");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void source_only(const std::filesystem::path &root) {
    civ5::content::RulesDatabase database;
    database.apply_sql(
        "CREATE TABLE IconTextureAtlases(ID INTEGER, Type TEXT, Atlas TEXT, "
        "IconSize INTEGER, Filename TEXT, IconsPerRow INTEGER, IconsPerColumn INTEGER);"
        "INSERT INTO IconTextureAtlases VALUES(0,'TEST_ATLAS_ROW','TEST_ATLAS',64,'Test.dds',8,8);");
    auto types = database.cache_info_types();
    civ5::runtime::ResearchContext setup;
    std::vector<civ5::runtime::TeamTechnologies> teams;
    const auto vector = read(root / "Assets/Gameplay/Lua/FLuaVector.lua");
    const auto icon = read(root / "Assets/UI/IconSupport.lua");
    civ5::runtime::ScriptSourceProvider sources;
    sources.files = [](std::string_view) {
        return std::vector<std::string>{"Assets/Gameplay/Lua/FLuaVector.lua"};
    };
    sources.read = [vector](std::string_view, std::string_view key) {
        if (key != "Assets/Gameplay/Lua/FLuaVector.lua")
            throw civ5::runtime::ScriptError("Unexpected UI include");
        return vector;
    };
    civ5::runtime::ScriptUiEventProvider events;
    events.notifications = {"AfterModsDeactivate", "AfterModsActivate"};
    civ5::runtime::LuaScriptHost host({64ULL * 1024 * 1024, 2'000'000},
        std::move(sources), {},
        civ5::runtime::lua_gameplay_services(database, setup, teams, types),
        {}, {}, {}, std::move(events));
    host.execute("CivilopediaScreen", icon, "@Assets/UI/IconSupport.lua");
    host.execute("CivilopediaScreen",
        "local p,f=IconLookup(0,64,'TEST_ATLAS'); "
        "assert(p.x==0 and p.y==0 and f=='Test.dds'); "
        "assert(Events.AfterModsDeactivate.Count()==1 and "
        "Events.AfterModsActivate.Count()==1)",
        "=IconSupport-selected-source-GameInfo-check");
}
}

int main(int argc, char **argv) {
    if (argc == 2) { source_only(argv[1]); return 0; }
    if (argc != 3) throw std::runtime_error("Expected game root, or Gandhi save and game root");
    const std::filesystem::path save(argv[1]), root(argv[2]);
    source_only(root);
    auto selected = civ5::ui::bootstrap_selected_bnw_preview(save, root, 0);
    check(static_cast<bool>(selected.preview), "Selected BNW host must restore for IconSupport fixture");
    const auto catalog = civ5::content::discover_packages({root / "Assets/DLC"});
    const auto &state = selected.preview.host->session().state();
    const auto queue_before = state.player_research.players.at(0).queue;
    const auto gameplay_rng_before = state.random.gameplay.state().calls;
    const auto map_rng_before = state.random.map.state().calls;

    const auto step = civ5::ui::execute_selected_bnw_icon_support_include(
        root, catalog, selected.profile, *selected.preview.host);
    check(step.status == civ5::ui::SelectedBnwIconSupportStatus::include_executed &&
          step.context_id == "CivilopediaScreen" &&
          step.registered_invalidation_listeners == 2 &&
          step.context_source_sha256.size() == 64 &&
          step.icon_support_sha256.size() == 64 &&
          step.vector_sha256.size() == 64 &&
          step.blocker.find("TestAll") != std::string::npos,
          "Selected IconSupport must execute with GameInfo and authored Events but remain blocked");
    check(state.player_research.players.at(0).queue == queue_before &&
          state.random.gameplay.state().calls == gameplay_rng_before &&
          state.random.map.state().calls == map_rng_before &&
          !selected.preview.host->session().readiness().can_continue(),
          "Diagnostic UI Lua must not mutate the live session or enable turns");

    auto missing_profile = selected.profile;
    missing_profile.enabled_dlc.clear();
    const auto blocked = civ5::ui::execute_selected_bnw_icon_support_include(
        root, catalog, missing_profile, *selected.preview.host);
    check(blocked.status == civ5::ui::SelectedBnwIconSupportStatus::source_blocked &&
          blocked.registered_invalidation_listeners == 0,
          "Missing selected Expansion2 source must block second include");
    std::cout << "IconSupport sha256=" << step.icon_support_sha256
              << " vector_sha256=" << step.vector_sha256
              << " GameInfo=selected listeners=2 live_context=blocked\n";
}
