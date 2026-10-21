#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace civ5::ui {
struct FrontendSaveDiscoveryOptions {
    // Root files have depth zero. Each child directory adds one level.
    std::size_t maximum_depth = 6;
    // Aggregate directory entries inspected across all supplied roots.
    std::size_t maximum_entries = 20000;
};
struct FrontendSaveDiscoveryResult {
    std::vector<std::filesystem::path> saves;
    std::vector<std::string> diagnostics;
};
// Enumerates only the supplied save directories; missing optional roots are
// ignored. No save payload/header is opened or validated and no files change.
// Directory symlinks and Windows reparse directories are never traversed.
// Canonical absolute paths are deduplicated, then ordered by modification
// time (newest first), with the canonical path breaking equal-time ties.
// Access failures and incomplete searches are reported in diagnostics.
FrontendSaveDiscoveryResult discover_frontend_saves(
    const std::vector<std::filesystem::path> &roots,
    FrontendSaveDiscoveryOptions options = {});
} // namespace civ5::ui
