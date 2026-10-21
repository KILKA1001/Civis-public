#pragma once

#include "civ5/content/manifest.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <filesystem>
#include <string>

namespace civ5::ui {

enum class SelectedBnwStockUiLuaStatus {
    source_blocked,
    first_include_executed,
};

// The first selected Expansion2 InGame.xml LuaContext is CivilopediaScreen;
// its first include is InstanceManager. This step executes that exact stock
// include in a disposable CivilopediaScreen Lua context. It does not run the
// parent context, publish listeners, or complete the UI/GameEvents load.
struct SelectedBnwStockUiLuaStep {
    SelectedBnwStockUiLuaStatus status = SelectedBnwStockUiLuaStatus::source_blocked;
    std::string context_id;
    std::string declaration_path, declaration_sha256;
    std::string context_source_path, context_source_sha256;
    std::string include_path, include_sha256;
    std::string blocker;
};

SelectedBnwStockUiLuaStep execute_selected_bnw_first_stock_ui_include(
    const std::filesystem::path &game_root,
    const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile);

} // namespace civ5::ui
