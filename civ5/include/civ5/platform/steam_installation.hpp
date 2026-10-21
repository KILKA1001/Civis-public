#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace civ5::platform {
struct SteamInstallation {
    std::filesystem::path library_root, game_root, workshop_root;
    std::string build_id;
};
struct SteamDiscovery {
    std::vector<SteamInstallation> installations;
    std::vector<std::string> diagnostics;
};
std::filesystem::path default_steam_root();
// Read-only appmanifest/libraryfolders discovery; no Steam client mutation.
SteamDiscovery discover_civ5(const std::filesystem::path &steam_root);
void validate_game_root(const std::filesystem::path &root);
} // namespace civ5::platform
