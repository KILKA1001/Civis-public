#pragma once

#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace civ5::ui {

struct CivilopediaOwnedControl {
    std::string id, kind, parent_id, style;
};

struct CivilopediaNativeControlTree {
    bool verified = false;
    std::vector<CivilopediaOwnedControl> controls;
    std::string blocker;
};

// Verify the authored XML tree for every control touched by the first native
// layout block. This establishes identities and parents, not live instances.
CivilopediaNativeControlTree inspect_bnw_civilopedia_native_control_tree(
    const content::XmlNode &layout);

struct SelectedBnwCivilopediaLayoutSources {
    bool verified = false;
    std::string civilopedia_sha256, xml_sha256, instance_manager_sha256;
    std::string styles_path, styles_sha256;
    std::int32_t authored_vert_slider_width = 0;
    CivilopediaNativeControlTree control_tree;
    std::string blocker;
};

struct SelectedBnwCivilopediaVfsStyle {
    bool selected_source_verified = false;
    std::string requested_key, source_id, source_relative_path, sha256;
    std::optional<std::uint64_t> mount_id;
    std::int32_t authored_vert_slider_width = 0;
    std::string blocker;
};

// Resolves Styles.xml through the active resource table and pins the chosen
// bytes and provenance to the selected base source. Size is the authored style
// value, not a native ScrollBar::GetSize result.
SelectedBnwCivilopediaVfsStyle resolve_bnw_civilopedia_vert_slider_style(
    const assets::ResourceMountTable &resources,
    const SelectedBnwCivilopediaLayoutSources &sources);

// Values observed by the renderer/native UI owner for one frame. The authored
// Styles.xml width is provenance, not a replacement for measured GetSize().x.
struct CivilopediaMeasuredLayout {
    std::uint64_t frame_number = 0;
    std::string owner_token;
    std::int64_t player_id = -1, game_turn = -1;
    std::string payload_sha256, rules_sha256, resolved_styles_sha256;
    std::int32_t viewport_width = 0, viewport_height = 0;
    std::int32_t scrollbar_width = 0;
    std::int32_t scroll_panel_width = 0, scroll_panel_height = 0;
    std::int32_t left_panel_width = 0, left_panel_height = 0;
};

enum class CivilopediaLayoutOperation { set_size, set_offset, calculate_internal_size };
struct CivilopediaLayoutInstruction {
    CivilopediaLayoutOperation operation;
    std::string control_id;
    std::int32_t x = 0, y = 0;
    std::size_t authored_line = 0;
};

struct CivilopediaLayoutOwnerKey {
    std::uint64_t frame_number = 0;
    std::string owner_token;
    std::int64_t player_id = -1, game_turn = -1;
    std::string payload_sha256, rules_sha256;
    std::string styles_sha256, civilopedia_sha256, xml_sha256;
};

struct SelectedBnwCivilopediaLayoutPreflight {
    bool source_and_owner_verified = false;
    bool native_layout_executed = false;
    bool callbacks_registered = false;
    CivilopediaLayoutOwnerKey owner;
    std::vector<CivilopediaLayoutInstruction> instructions;
    std::string blocker;
};

// A native owner may report completion of the exact ordered plan. This is a
// structural receipt only: the validator cannot execute or authenticate a
// Controls method, and it never grants a live Civilopedia context.
struct CivilopediaLayoutOperationReceipt {
    CivilopediaLayoutInstruction instruction;
    bool completed = false;
};
struct CivilopediaInternalSizeResult {
    std::string control_id;
    bool completed = false;
};
struct CivilopediaNativeLayoutReceipt {
    CivilopediaLayoutOwnerKey owner;
    std::vector<CivilopediaLayoutOperationReceipt> operations;
    std::array<CivilopediaInternalSizeResult, 2> internal_size_results;
};
struct CivilopediaNativeLayoutReceiptValidation {
    bool receipt_matches_plan = false;
    bool native_controls_available = false;
    std::string blocker;
};

CivilopediaNativeLayoutReceiptValidation validate_bnw_civilopedia_layout_receipt(
    const SelectedBnwCivilopediaLayoutPreflight &plan,
    const CivilopediaLayoutOwnerKey &current_owner,
    const CivilopediaNativeLayoutReceipt &receipt);

SelectedBnwCivilopediaLayoutSources inspect_bnw_civilopedia_layout_sources(
    const std::filesystem::path &game_root);

// Computes the exact authored setter sequence from measured owner values.
// CalculateInternalSize remains an unexecuted required operation, so this
// function never makes the live Civilopedia context or callbacks available.
SelectedBnwCivilopediaLayoutPreflight plan_bnw_civilopedia_native_layout(
    const SelectedBnwCivilopediaLayoutSources &sources,
    const runtime::SessionCheckpointIdentity &identity,
    std::int64_t active_player_id, std::int64_t current_turn,
    std::uint64_t expected_frame_number,
    const CivilopediaMeasuredLayout &measured);

SelectedBnwCivilopediaLayoutPreflight preflight_selected_bnw_civilopedia_layout(
    const std::filesystem::path &game_root,
    const assets::ResourceMountTable &selected_ui_resources,
    const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected,
    std::uint64_t expected_frame_number,
    const CivilopediaMeasuredLayout &measured);

} // namespace civ5::ui
