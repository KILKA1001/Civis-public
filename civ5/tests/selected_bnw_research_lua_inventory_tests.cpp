#include "civ5/ui/selected_bnw_research_lua_inventory.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
namespace fs = std::filesystem;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void write(const fs::path &path, const char *data) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << data;
    if (!out) throw std::runtime_error("Could not write Lua inventory fixture");
}

void synthetic() {
    const auto root = fs::temp_directory_path() / "civis_bnw_lua_inventory_fixture";
    fs::remove_all(root);
    write(root / "Assets/UI/InGame/InGame.xml",
          "<Context><LuaContext ID='Base' FileName='Assets/UI/Base'/></Context>");
    write(root / "Assets/UI/Base.lua", "GameEvents.PlayerCanResearch.Add(function() return false end)");
    write(root / "Assets/DLC/Expansion2/UI/InGame/InGame.xml",
          "<Context><LuaContext ID='First' FileName='Assets/DLC/Expansion2/UI/First'/>"
          "<LuaContext ID='Second' FileName='Assets/DLC/Expansion2/UI/Second'/></Context>");
    write(root / "Assets/DLC/Expansion2/UI/First.lua", "-- first");
    write(root / "Assets/DLC/Expansion2/UI/Second.lua", "-- second");
    write(root / "Assets/DLC/Unselected/UI/Other.lua", "-- should not be selected");
    write(root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg", "<Package/>");
    write(root / "Assets/DLC/Unselected/Unselected.Civ5Pkg", "<Package/>");
    civ5::content::Catalog catalog;
    civ5::content::PackageManifest selected;
    selected.kind = civ5::content::PackageKind::dlc;
    selected.path = root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
    catalog.packages.push_back(selected);
    selected.path = root / "Assets/DLC/Unselected/Unselected.Civ5Pkg";
    catalog.packages.push_back(selected);
    civ5::runtime::SavedContentProfile profile;
    profile.all_requirements_located = true;
    profile.enabled_dlc = {0};
    const auto report = civ5::ui::inventory_selected_bnw_research_lua(root, catalog, profile);
    check(report.blocker == civ5::ui::SelectedBnwResearchBlocker::stock_ui_contexts_unverified &&
          report.fpk_archives_inspected == 0 && report.context_declarations.size() == 3 &&
          report.context_declarations[1].id == "First" &&
          report.context_declarations[2].id == "Second" &&
          report.context_declarations[1].ordinal < report.context_declarations[2].ordinal,
          "Authored context declarations must retain document order and stay blocked");
    check(report.lua_sources.size() == 3 &&
          std::none_of(report.lua_sources.begin(), report.lua_sources.end(),
                       [](const auto &source) { return source.key.find("Unselected") != std::string::npos; }),
          "Lua inventory must exclude unselected DLC");
    check(std::any_of(report.lua_sources.begin(), report.lua_sources.end(),
                      [](const auto &source) {
                          return source.key == "Assets/UI/Base.lua" &&
                                 source.mentions_research_test && source.sha256.size() == 64;
                      }),
          "Authored Lua bytes and research listener evidence must retain provenance");
    profile.enabled_dlc = {0, 0};
    const auto bad = civ5::ui::inventory_selected_bnw_research_lua(root, catalog, profile);
    check(!bad.unresolved.empty() && bad.blocker ==
          civ5::ui::SelectedBnwResearchBlocker::stock_ui_contexts_unverified,
          "Duplicate DLC selection cannot validate TestAll");
    fs::remove_all(root);
}

void selected(const fs::path &save, const fs::path &root) {
    const auto bootstrap = civ5::ui::bootstrap_selected_bnw_preview(save, root, 0);
    check(static_cast<bool>(bootstrap.preview), "Selected Gandhi bootstrap failed");
    const auto catalog = civ5::content::discover_packages({root / "Assets/DLC"});
    const auto report = civ5::ui::inventory_selected_bnw_research_lua(
        root, catalog, bootstrap.profile);
    check(report.blocker == civ5::ui::SelectedBnwResearchBlocker::stock_ui_contexts_unverified &&
          report.context_declarations.size() > 10 && report.lua_sources.size() > 100,
          "Selected stock UI Lua inventory must remain observational only");
    check(std::any_of(report.context_declarations.begin(), report.context_declarations.end(),
                      [](const auto &entry) {
                          return entry.document_path.find("Expansion2") != std::string::npos &&
                                 entry.id == "TechTree" && entry.document_sha256.size() == 64;
                      }),
          "Selected Expansion2 UI context declaration was not inventoried");
    std::cout << "selected_dlc=" << bootstrap.profile.enabled_dlc.size()
              << " contexts=" << report.context_declarations.size()
              << " lua_sources=" << report.lua_sources.size()
              << " fpk=" << report.fpk_archives_inspected
              << " literal_research_mentions=" << std::count_if(
                     report.lua_sources.begin(), report.lua_sources.end(),
                     [](const auto &source) { return source.mentions_research_test; })
              << " blocker=stock_ui_contexts_unverified\n";
}

} // namespace

int main(int argc, char **argv) {
    synthetic();
    if (argc == 3) selected(argv[1], argv[2]);
    else if (argc != 1) throw std::runtime_error("Expected optional save and game root");
}
