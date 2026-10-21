#pragma once

#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/manifest.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::ui {

enum class SelectedBnwCivilopediaPreambleStatus {
    source_blocked, source_verified, preamble_executed, article_list_bound,
    prereq_tech_bound, manager_block_bound
};

struct SelectedBnwManagerBinding {
    std::string lua_name, instance_name, instance_root;
    std::string parent_control, parent_kind, xml_owner_path;
    std::size_t source_line = 0;
};

struct SelectedBnwManagerBlockContract {
    bool verified = false;
    std::vector<SelectedBnwManagerBinding> bindings;
    std::size_t next_native_line = 0;
    std::string blocker;
};

struct SelectedBnwCategoryCallbackBoundary {
    bool source_verified = false;
    std::size_t first_callback_line = 0;
    std::size_t authored_category_buttons = 0;
    std::string required_mouse_event;
    std::string blocker;
};

struct SelectedBnwArticleListControlContract {
    bool verified = false;
    std::string control_id, control_kind, parent_id;
    std::string item_instance, item_root, heading_instance, heading_root;
    std::size_t next_native_line = 0;
    std::string blocker;
};

struct SelectedBnwPrereqTechControlContract {
    bool verified = false;
    std::string control_id, control_kind, parent_id;
    std::string instance_name, instance_root, next_native_control;
    std::size_t next_native_line = 0;
    std::string blocker;
};

// Only the authored source prefix before the first native Controls dependency
// is executed. The returned blocker remains mandatory: no live UI context or
// Civilopedia event subscription is published by this diagnostic VM.
struct SelectedBnwCivilopediaPreamble {
    SelectedBnwCivilopediaPreambleStatus status =
        SelectedBnwCivilopediaPreambleStatus::source_blocked;
    std::string context_source_sha256;
    std::string xml_sha256;
    std::string first_native_control;
    std::size_t first_native_line = 0;
    std::size_t authored_later_event_registrations = 0;
    std::int64_t owner_player_id = -1;
    std::int64_t owner_game_turn = -1;
    std::string owner_payload_sha256, owner_rules_sha256;
    std::vector<std::string> observed_include_order;
    bool live_context_published = false;
    std::size_t bound_article_list_managers = 0;
    std::size_t bound_prereq_tech_managers = 0;
    std::size_t bound_manager_count = 0;
    std::string next_native_control;
    std::string blocker;
};

// Source shape is checked separately so fixtures can catch changes in the
// original registration order without needing a synthetic live session.
SelectedBnwCivilopediaPreamble inspect_civilopedia_preamble_source(
    std::string_view source, const content::XmlNode &layout);

SelectedBnwArticleListControlContract inspect_civilopedia_article_list_control(
    std::string_view civilopedia_source, const content::XmlNode &layout,
    std::string_view instance_manager_source);

SelectedBnwPrereqTechControlContract inspect_civilopedia_prereq_tech_control(
    std::string_view civilopedia_source, const content::XmlNode &layout,
    std::string_view instance_manager_source);

SelectedBnwManagerBlockContract inspect_civilopedia_manager_block(
    std::string_view civilopedia_source, const content::XmlNode &layout,
    std::string_view instance_manager_source);

// Inspects the first authored RegisterCallback without executing it. Native
// screen/style dimensions and CalculateInternalSize occur earlier and keep
// callback registration unavailable until an owner supplies those services.
SelectedBnwCategoryCallbackBoundary inspect_civilopedia_first_callback(
    std::string_view civilopedia_source, const content::XmlNode &layout,
    std::string_view instance_manager_source);

SelectedBnwCivilopediaPreamble execute_selected_bnw_civilopedia_preamble(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected);

// The diagnostic entry for a future live UI owner. It runs the two selected
// includes and the following Civilopedia prefix in one disposable Lua VM,
// records the include order actually observed, and stops at the first native
// Controls dependency. Its result never grants a live context or input.
SelectedBnwCivilopediaPreamble run_selected_bnw_stock_ui_context_diagnostic(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected);

// Binds the source-verified ListOfArticles XML Stack as an identity-only Lua
// token and executes the two authored InstanceManager constructors that merely
// retain it. No native methods, BuildInstance, callbacks or UI context run.
SelectedBnwCivilopediaPreamble execute_selected_bnw_article_list_binding(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected);

// Extends the same ordered VM through the PrereqTechInstance constructor only.
// Its Grid token has no native methods, children or callback support.
SelectedBnwCivilopediaPreamble execute_selected_bnw_prereq_tech_binding(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected);

// Executes the full authored manager-constructor block in source order using
// XML-owned identity tokens. Stops before UIManager/GetSize and leaves all
// native effects and callbacks unavailable.
SelectedBnwCivilopediaPreamble execute_selected_bnw_manager_block_binding(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected);

} // namespace civ5::ui
