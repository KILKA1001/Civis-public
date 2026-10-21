#pragma once

#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/manifest.hpp"
#include <filesystem>
#include <string>

namespace civ5::ui {

enum class SelectedBnwIconSupportStatus { source_blocked, include_executed };

// Executes the second authored CivilopediaScreen include, IconSupport, in a
// disposable VM. It binds read-only GameInfo from this selected live owner and
// registers only the two Events names that the stock source itself requests.
// The VM is never published as the live UI context and cannot authorize input.
struct SelectedBnwIconSupportStep {
    SelectedBnwIconSupportStatus status = SelectedBnwIconSupportStatus::source_blocked;
    std::string context_id;
    std::string context_source_sha256;
    std::string instance_manager_sha256;
    std::string icon_support_path, icon_support_sha256;
    std::string vector_path, vector_sha256;
    std::uint64_t registered_invalidation_listeners = 0;
    std::string blocker;
};

SelectedBnwIconSupportStep execute_selected_bnw_icon_support_include(
    const std::filesystem::path &game_root,
    const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected);

} // namespace civ5::ui
