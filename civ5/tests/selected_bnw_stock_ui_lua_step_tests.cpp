#include "civ5/ui/selected_bnw_stock_ui_lua_step.hpp"
#include "civ5/saves/save_header.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
namespace fs = std::filesystem;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void write(const fs::path &path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
    if (!out) throw std::runtime_error("Could not create stock UI Lua fixture");
}

struct Fixture {
    fs::path root;
    civ5::content::Catalog catalog;
    civ5::runtime::SavedContentProfile profile;
    Fixture() : root(fs::temp_directory_path() /
        ("civis-ui-lua-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))) {
        write(root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg", "<Package/>");
        write(root / "Assets/DLC/Expansion2/UI/InGame/InGame.xml",
              "<Context><LuaContext ID='CivilopediaScreen' "
              "FileName='Assets/UI/Civilopedia/CivilopediaScreen'/>"
              "<LuaContext ID='WorldView' FileName='Assets/UI/InGame/WorldView/WorldView'/>"
              "</Context>");
        write(root / "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",
              "-- selected context\r\ninclude( \"InstanceManager\" );\r\n");
        write(root / "Assets/UI/InstanceManager.lua",
              "InstanceManager={new=function() return {} end}\n");
        civ5::content::PackageManifest package;
        package.kind = civ5::content::PackageKind::dlc;
        package.id = "6da07636-4123-4018-b643-6575b4ec336b";
        package.path = root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
        catalog.packages.push_back(std::move(package));
        profile.kind = civ5::runtime::ContentProfileKind::dlc;
        profile.enabled_dlc = {0};
        profile.all_requirements_located = true;
        profile.can_activate = true;
    }
    ~Fixture() { std::error_code ignored; fs::remove_all(root, ignored); }
};

void synthetic() {
    Fixture f;
    using Status = civ5::ui::SelectedBnwStockUiLuaStatus;
    auto step = civ5::ui::execute_selected_bnw_first_stock_ui_include(
        f.root, f.catalog, f.profile);
    check(step.status == Status::first_include_executed &&
          step.context_id == "CivilopediaScreen" &&
          step.context_source_sha256.size() == 64 &&
          step.include_sha256.size() == 64 && !step.blocker.empty(),
          "Exact first authored stock UI Lua include must execute without completing the context");
    write(f.root / "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",
          "Controls.Fake()\ninclude( \"InstanceManager\" );\n");
    step = civ5::ui::execute_selected_bnw_first_stock_ui_include(
        f.root, f.catalog, f.profile);
    check(step.status == Status::source_blocked,
          "A changed first statement must not be skipped to run a later include");
    write(f.root / "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",
          "include( \"InstanceManager\" );\n");
    write(f.root / "Assets/DLC/Expansion2/UI/InstanceManager.lua", "-- override");
    step = civ5::ui::execute_selected_bnw_first_stock_ui_include(
        f.root, f.catalog, f.profile);
    check(step.status == Status::source_blocked &&
          step.blocker.find("precedence") != std::string::npos,
          "Unverified selected DLC include override must block source execution");
}

void selected(const fs::path &save, const fs::path &root) {
    const auto catalog = civ5::content::discover_packages({root / "Assets/DLC"});
    const auto profile = civ5::runtime::resolve_saved_content_profile(
        civ5::saves::read_header(save), catalog);
    const auto step = civ5::ui::execute_selected_bnw_first_stock_ui_include(
        root, catalog, profile);
    check(step.status == civ5::ui::SelectedBnwStockUiLuaStatus::first_include_executed &&
          step.declaration_sha256.size() == 64 &&
          step.context_source_path.find("Expansion2") != std::string::npos &&
          step.blocker.find("TestAll") != std::string::npos,
          "Selected Gandhi stock UI first include failed or claimed full Lua activation");
    std::cout << "context=" << step.context_id
              << " first_include=InstanceManager"
              << " source_sha256=" << step.context_source_sha256
              << " include_sha256=" << step.include_sha256
              << " full_context=blocked\n";
}

} // namespace

int main(int argc, char **argv) {
    synthetic();
    if (argc == 3) selected(argv[1], argv[2]);
    else if (argc != 1) throw std::runtime_error("Expected optional selected save and game root");
}
