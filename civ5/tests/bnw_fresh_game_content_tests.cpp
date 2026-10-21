#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace civ5;

namespace {
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

content::PackageManifest dlc(const char *id, const char *name) {
    content::PackageManifest package;
    package.kind = content::PackageKind::dlc;
    package.id = id;
    package.name = name;
    package.version = 1;
    return package;
}

void test_explicit_bnw_requirement() {
    content::Catalog catalog;
    catalog.packages.push_back(dlc("6da07636-4123-4018-b643-6575b4ec336b", "Expansion2"));
    catalog.packages.push_back(dlc("ff612ad8-8a18-43d3-93fa-1dff6ad24352", "Unrelated DLC"));

    const auto no_selection = runtime::bootstrap_bnw_fresh_game_content({}, catalog, {});
    check(no_selection.has_blocker(runtime::FreshGameContentBlockerCode::expansion2_not_selected),
          "BNW must require selected Expansion2");
    check(!no_selection.content_ready() && !no_selection.activation,
          "An unselected BNW package must not activate rules");

    constexpr std::array<std::size_t, 1> selected{0};
    const auto result = runtime::bootstrap_bnw_fresh_game_content(
        std::filesystem::path("nonexistent-bnw-game-root"), catalog, selected);
    check(result.profile.enabled_dlc.size() == 1 && result.profile.enabled_dlc.front() == 0,
          "Only explicitly selected DLC may be activated");
    check(!result.has_blocker(runtime::FreshGameContentBlockerCode::expansion2_not_selected),
          "Selected Expansion2 must satisfy the BNW identity requirement");
    check(result.has_blocker(runtime::FreshGameContentBlockerCode::rules_activation_failed),
          "Missing local gameplay assets must be an explicit blocker");
    check(result.has_blocker(runtime::FreshGameContentBlockerCode::lua_loading_pending) &&
              result.has_blocker(runtime::FreshGameContentBlockerCode::map_script_pending) &&
              result.has_blocker(runtime::FreshGameContentBlockerCode::world_setup_pending),
          "Content bootstrap must expose all later session prerequisites");
    check(!result.content_ready(), "An activation failure must not expose ready content");
}

void test_native_mod_gate() {
    content::Catalog catalog;
    catalog.packages.push_back(dlc("6da07636-4123-4018-b643-6575b4ec336b", "Expansion2"));
    content::PackageManifest mod;
    mod.kind = content::PackageKind::mod;
    mod.id = "76044a5b-1ea9-4bd9-94d4-d42ed8309b55";
    mod.name = "Native mod";
    mod.version = 1;
    mod.path = "nonexistent-native-mod.modinfo";
    mod.files.push_back({"CvGameCore_Expansion2.dll", "", false});
    catalog.packages.push_back(std::move(mod));
    constexpr std::array<std::size_t, 1> dlcs{0}, mods{1};
    const auto result = runtime::bootstrap_bnw_fresh_game_content({}, catalog, dlcs, mods);
    check(result.has_blocker(runtime::FreshGameContentBlockerCode::native_gamecore_unavailable),
          "A selected native mod must require a compatible x64 GameCore");
    check(!result.activation && !result.content_ready(),
          "A native mod must stop before rule activation");
}
} // namespace

int main() {
    try {
        test_explicit_bnw_requirement();
        test_native_mod_gate();
        std::cout << "BNW fresh content boundary passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
