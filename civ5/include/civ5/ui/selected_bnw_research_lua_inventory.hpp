#pragma once

#include "civ5/content/manifest.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/ui/selected_bnw_research_preview.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace civ5::ui {

struct SelectedBnwLuaSourceEvidence {
    std::string key;
    std::string source_path;
    std::string sha256;
    std::uint64_t bytes = 0;
    bool archived = false;
    bool mentions_research_test = false;
};

struct SelectedBnwLuaContextEvidence {
    std::string document_path;
    std::string document_sha256;
    std::size_t ordinal = 0;
    std::string id;
    std::string source_key;
};

// A source inventory, not a Lua execution plan. InGame.xml declaration order
// cannot establish native context activation, hidden-context timing, include()
// order or listeners registered dynamically. TestAll remains blocked.
struct SelectedBnwResearchLuaInventory {
    SelectedBnwResearchBlocker blocker =
        SelectedBnwResearchBlocker::stock_ui_contexts_unverified;
    std::vector<SelectedBnwLuaSourceEvidence> lua_sources;
    std::vector<SelectedBnwLuaContextEvidence> context_declarations;
    std::uint64_t fpk_archives_inspected = 0;
    std::vector<std::string> unresolved;
};

SelectedBnwResearchLuaInventory inventory_selected_bnw_research_lua(
    const std::filesystem::path &game_root,
    const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile);

} // namespace civ5::ui
