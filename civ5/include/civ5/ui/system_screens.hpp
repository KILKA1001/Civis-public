#pragma once

#include "civ5/ui/world_view.hpp"
#include <cstdint>
#include <functional>
#include <map>

namespace civ5::ui {

struct SystemControl {
    std::string id, element, text_key, texture, style;
};
struct SystemScreenSource {
    std::string context_id, source_key, selected_source_path;
    bool source_available = true;
    std::vector<SystemControl> controls;
    std::int32_t authored_width = 960, authored_height = 658;
    std::string panel_style = "Grid9DetailFive140";
    bool has(std::string_view id) const;
};

// These are the twelve active single-player contexts owned by U9 in the
// current BNW registry. Selection must pass through the mounted resource VFS.
std::vector<SystemScreenSource> read_system_sources(const assets::ResourceMountTable &resources,
                                                     const UiContextRegistry &registry);

enum class SystemOperationKind {
    restart, quick_save, save, load, open_load_menu, delete_save, save_map, retire,
    exit_to_main, exit_game, continue_after_end, apply_options,
    reset_options, change_password, switch_player, start_loaded_game,
    open_beyond_earth_store, view_game_details, view_active_mods
};
struct SystemOperation {
    SystemOperationKind kind;
    std::string target;
};

struct SystemSaveSlot {
    std::string id, name, detail;
};
struct SystemPediaEntry {
    std::string id, title, description, category, selected_source_path;
    // Only authored, active links to other entries in this catalog.
    std::vector<std::string> prerequisite_tech_ids;
    // Selected BuildingClasses XML row that classified Buildings or Wonders.
    std::string category_source_path;
};
struct SystemPediaArticleLayout {
    std::size_t wrap_columns = 0, visible_lines = 0;
};
struct SystemScreenFrame;
SystemPediaArticleLayout civilopedia_article_layout(std::int32_t width,
                                                     std::int32_t height);
std::size_t civilopedia_article_max_scroll(const SystemScreenFrame &state);
using SystemLocalizer = std::function<std::optional<std::string>(std::string_view)>;
// The caller supplies the active BNW localization catalog. Authored rows are
// overlaid in base -> Expansion -> Expansion2 order by Type; only resolved
// authored article text is shown, with source provenance per row.
std::vector<SystemPediaEntry> read_civilopedia_entries(
    const assets::ResourceMountTable &resources, const SystemLocalizer &localize);
struct SystemScreenFrame {
    // The owner of the live session publishes actual slots and encyclopedia
    // entries. Empty vectors mean that source is absent, not an empty game.
    std::vector<SystemSaveSlot> save_slots;
    std::vector<SystemPediaEntry> pedia_entries;
    std::optional<std::string> selected_save_id;
    std::optional<std::string> selected_pedia_id;
    std::size_t pedia_page = 0;
    // Authored CategoryButton number; zero keeps the complete catalog visible.
    std::uint8_t pedia_category_tab = 0;
    // A local session or a live host that persists category changes owns this.
    // A renderer without such an owner keeps category/submit actions gated.
    bool pedia_category_session_owned = false;
    std::string pedia_search;
    std::vector<std::string> pedia_history;
    std::size_t pedia_history_index = 0;
    std::size_t pedia_article_first_line = 0;
    SystemPediaArticleLayout pedia_article_layout;
    std::optional<std::string> end_game_text;
    // GameMenu.lua hides ModsButton when Modding.GetActivatedMods() is empty.
    // Missing owner data leaves the authored control hidden.
    std::optional<std::size_t> game_menu_active_mod_count;
    // GameMenu.lua OnShowHide owns the multiplayer/mod DisableQuickSave check.
    // Missing owner data must not expose the selected QuickSaveButton.
    std::optional<bool> game_menu_quick_save_visible;
    // The active GameMenu.lua OnShowHide callback hides LoadGameButton for
    // network multiplayer or DisableLoadGameOption=1. Unknown stays hidden.
    std::optional<bool> game_menu_load_game_visible;
    // EndGameMenu.lua reveals BeyondButton only for the active team's
    // EndGameTypes.Technology result. Absent owner data keeps it hidden.
    std::optional<bool> end_game_technology_victory;
    std::optional<SystemOperationKind> pending_exit;
};

struct SystemScreenPlan {
    std::string context_id, selected_source_path;
    std::vector<WorldViewPaint> paints;
    std::vector<WorldViewHit> hits;
    std::vector<std::string> unresolved;
    std::optional<WorldViewRect> search_edit_box;
    std::optional<WorldViewRect> article_scroll_panel;
    const WorldViewHit *hit(std::int32_t x, std::int32_t y) const;
};

// Only authored controls with a known route become hits. Unknown/gameplay
// controls remain visible and explicitly unavailable, never successful.
SystemScreenPlan plan_system_screen(const SystemScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const SystemLocalizer &localize = {}, const UiGameplayBackend *backend = nullptr);

// Routes the selected BNW controls to U0. File writes, option mutations and
// session termination remain unavailable until an actual backend is supplied.
std::optional<UiCommand> system_control_command(std::string_view context_id,
                                                 std::string_view control_id,
                                                 const SystemScreenFrame *state = nullptr);

enum class SystemPediaActionKind {
    select_article, follow_link, previous_page, next_page, search, submit_search,
    history_back, history_forward, article_scroll_up, article_scroll_down, select_category
};
struct SystemPediaAction {
    SystemPediaActionKind kind;
    // Article ID for selection; localized title query for search.
    std::string article_id;
};

// Builds an action from the authored SearchEditBox text. Its caller must use
// the currently focused frame; empty text clears the filter.
UiCommand civilopedia_search_command(std::string_view query);
UiCommandResult apply_civilopedia_action(SystemScreenFrame &state,
    const SystemPediaAction &action);

class SystemGameplayBackend final : public UiGameplayBackend {
  public:
    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;
};

// Keeps the intent behind the shared ExitConfirm window without mutating the
// immutable game frame. A denied confirmation remains open and reports why.
class SystemMenuSession {
  public:
    UiCommandResult activate(std::string_view context_id, std::string_view control_id,
        const UiFrameSnapshot &frame, UiScreenController &controller,
        UiGameplayBackend *backend = nullptr);
    std::optional<SystemOperationKind> pending_exit() const { return pending_exit_; }
    UiFrameSnapshot with_pedia_state(const UiFrameSnapshot &frame) const;
    std::optional<std::string> selected_pedia_id() const { return selected_pedia_id_; }
    std::size_t pedia_page() const { return pedia_page_; }
    std::uint8_t pedia_category_tab() const { return pedia_category_tab_; }
    std::size_t pedia_article_first_line() const { return pedia_article_first_line_; }
    const std::string &pedia_search() const { return pedia_search_; }

  private:
    std::optional<SystemOperationKind> pending_exit_;
    std::optional<std::string> selected_pedia_id_;
    std::size_t pedia_page_ = 0;
    std::uint8_t pedia_category_tab_ = 0;
    std::string pedia_search_;
    std::vector<std::string> pedia_history_;
    std::size_t pedia_history_index_ = 0;
    std::size_t pedia_article_first_line_ = 0;
};

} // namespace civ5::ui
