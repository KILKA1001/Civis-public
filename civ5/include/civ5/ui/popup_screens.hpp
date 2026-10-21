#pragma once

#include "civ5/ui/world_view.hpp"
#include <functional>
#include <map>

namespace civ5::ui {

// Data is supplied by the live Lua/GameCore event owner. In particular, a
// popup choice never comes from a save fixture or from its XML template.
struct PopupChoice {
    std::int64_t id = -1;
    std::string label;
    std::string detail;
    bool available = false;
    // Live GameInfo.Religions IconAtlas/PortraitIndex, resolved by the popup
    // event owner. An absent portrait must never be inferred from the ID.
    std::string icon_atlas;
    std::int32_t portrait_index = -1;
};

// BUTTONPOPUP_FOUND_RELIGION is issued by the live GameCore/Lua popup owner.
// Data1/Data2 identify the city and Option1 distinguishes founding from
// enhancement. This is evidence for presentation, not an adoption command.
enum class ChooseReligionMode { founding, enhancing };
struct ChooseReligionOwnerEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1;
    std::int64_t city_x = -1, city_y = -1;
    ChooseReligionMode mode = ChooseReligionMode::founding;
    std::string selected_source_path;
    // LabelReligionName has no XML Size. The active presenter must measure it
    // and the name dialog controls in physical pixels for this viewport.
    struct NameGeometry {
        std::int32_t viewport_width = 0, viewport_height = 0;
        WorldViewRect label, popup, edit, cancel, default_button, ok;
    };
    std::optional<NameGeometry> name_geometry;
};

// The active Lua owner obtains Data2 and GetNextDigCompletePlot() at popup
// time, then builds the visible choices from live artifact and slot queries.
// Measured row bounds are optional; XML declares dynamic row heights.
struct ChooseArchaeologyOwnerEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1;
    std::int64_t unit_index = -1;
    std::int64_t dig_plot_index = -1;
    bool written_artifact = false;
    std::string selected_source_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
    std::vector<WorldViewRect> measured_choice_bounds;
};

enum class ChoosePantheonMode { pantheon, reformation };
struct ChoosePantheonOwnerEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1;
    // BUTTONPOPUP_FOUND_PANTHEON Data2: positive selects pantheon beliefs;
    // zero selects reformation beliefs in the selected BNW Lua.
    std::int64_t data2 = -1;
    ChoosePantheonMode mode = ChoosePantheonMode::pantheon;
    std::string selected_source_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
    // Native owner reports only currently visible item buttons, with their
    // current physical-pixel rectangles after Lua's dynamic row sizing.
    std::vector<std::pair<std::int64_t, WorldViewRect>> visible_choice_bounds;
    struct ConfirmGeometry {
        WorldViewRect panel, yes, no;
    };
    std::optional<ConfirmGeometry> confirm_geometry;
    // Localized TXT_KEY_CONFIRM_PANTHEON text after Lua's ShortDescription arg.
    std::map<std::int64_t, std::string> confirm_text_by_choice;
};

// BUTTONPOPUP_GOODY_HUT_REWARD is informational: GameCore awarded the reward
// before Lua queued the popup. Data1 selects GameInfo.GoodyHuts; Data2 is the
// localized amount for gold/culture. The owner supplies the resolved body.
struct GoodyHutRewardOwnerEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0, token = 0;
    std::int64_t event_id = -1, goody_type = -1, reward_amount = -1;
    std::string selected_source_path;
};
struct GoodyHutClose {
    std::uint64_t owner_sequence = 0, token = 0;
    std::int64_t event_id = -1;
};
// Published by LiveUiHost only when its current native reader and processed
// callback are both connected to this exact owner event.
struct GoodyHutCloseView {
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0, token = 0;
    std::string selected_source_path;
};

// UI-only draft. Selecting a belief does not send the native network command;
// it only opens the authored confirmation overlay for this exact owner event.
struct PantheonConfirmView {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1, belief_id = -1;
    std::string selected_source_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
};
struct ReligionSelectionView {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1, religion_id = -1;
    std::string selected_source_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
    std::string default_name, name, draft;
    bool editing_name = false, name_error = false;
};
enum class ReligionUiActionKind {
    select_symbol, open_name, default_name, accept_name, cancel_name,
    found_unavailable
};
struct ReligionUiAction {
    ReligionUiActionKind kind = ReligionUiActionKind::select_symbol;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1, religion_id = -1;
};
enum class PantheonUiActionKind { select_belief, cancel_confirmation, confirm_unavailable };
struct PantheonUiAction {
    PantheonUiActionKind kind = PantheonUiActionKind::select_belief;
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1, belief_id = -1;
};

struct PopupEvent {
    std::string context_id;
    std::int64_t event_id = -1;
    std::optional<std::string> title;
    std::optional<std::string> body;
    std::vector<PopupChoice> choices;
    std::optional<std::string> entered_text;
    bool dismissible = false;
    // The popup event owner supplies the BUTTONPOPUP_* table ID and stable
    // sequence. Merely navigating to a popup context cannot select a cue.
    std::optional<UiAuthoredAudioCue> audio_cue;
    std::optional<ChooseReligionOwnerEvidence> choose_religion;
    std::optional<ChooseArchaeologyOwnerEvidence> choose_archaeology;
    std::optional<ChoosePantheonOwnerEvidence> choose_pantheon;
    std::optional<GoodyHutRewardOwnerEvidence> goody_hut_reward;
};

struct PopupFrame {
    std::int64_t player_id = -1;
    std::vector<PopupEvent> events;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
};

bool valid_choose_religion_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path = {});
bool valid_religion_name_geometry(
    const ChooseReligionOwnerEvidence::NameGeometry &geometry);
bool valid_choose_archaeology_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path = {});
bool valid_choose_pantheon_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path = {});
bool valid_pantheon_confirm_geometry(const ChoosePantheonOwnerEvidence &owner);
bool valid_pantheon_visible_rows(const PopupEvent &event);
bool valid_goody_hut_reward_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path = {});

struct PopupDecision {
    std::string context_id;
    std::int64_t event_id = -1;
    std::int64_t choice_id = -1;
    std::optional<std::string> entered_text;
};

// Presenter-owned draft for the focused authored rename EditBox. The host
// publishes it only while the same player's live popup event owns focus.
struct PopupTextView {
    std::string context_id;
    std::int64_t player_id = -1, event_id = -1;
    std::string text;
};
struct PopupTextEdit {
    std::string context_id;
    std::int64_t event_id = -1;
    std::string previous_text, text;
};
bool valid_popup_name(std::string_view text, std::size_t max_characters = 15);

// Local UI navigation only. It never acknowledges or executes the event.
enum class PopupScrollDirection { up, down };
struct PopupScroll {
    std::string context_id;
    std::int64_t event_id = -1;
    std::size_t first_visible = 0;
    PopupScrollDirection direction = PopupScrollDirection::down;
};
struct PopupScrollView {
    std::string context_id;
    std::int64_t player_id = -1, event_id = -1;
    std::size_t first_visible = 0;
};

struct PopupScreenSource {
    std::string context_id;
    std::string selected_source_path;
    content::XmlNode document;
    std::vector<std::string> control_ids;
    bool contains(std::string_view id) const;
};

// Resolves exactly the source selected by the base -> Expansion -> Expansion2
// resource mounts. All active single-player U8 contexts are catalogued.
class PopupCatalog {
  public:
    static PopupCatalog from_resources(const assets::ResourceMountTable &resources,
                                       const UiContextRegistry &registry);
    const PopupScreenSource *find(std::string_view id) const;
    const std::vector<PopupScreenSource> &sources() const { return sources_; }
    const std::vector<std::string> &unresolved() const { return unresolved_; }

  private:
    std::vector<PopupScreenSource> sources_;
    std::vector<std::string> unresolved_;
};

struct PopupScreenPlan {
    std::string context_id;
    std::string selected_source_path;
    std::vector<WorldViewPaint> paints;
    std::vector<WorldViewHit> hits;
    std::vector<std::string> unresolved;
    std::optional<WorldViewRect> scroll_bounds;
    std::optional<WorldViewRect> edit_box;
    const WorldViewHit *hit(std::int32_t x, std::int32_t y) const;
};

// Call only for the focused controller screen. The plan itself rejects a
// mismatched or stale player event, and paints no invented event contents.
PopupScreenPlan plan_popup_screen(const PopupScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t viewport_width,
    std::int32_t viewport_height, const UiGameplayBackend *backend = nullptr,
    const std::function<std::optional<std::string>(std::string_view)> &localize = {});

// The controller's modal stack selects the one popup that may receive input.
// A queued, covered popup is not painted or made clickable.
std::optional<PopupScreenPlan> plan_focused_popup(const PopupCatalog &catalog,
    const UiScreenController &controller, const UiFrameSnapshot &frame,
    std::int32_t viewport_width, std::int32_t viewport_height,
    const UiGameplayBackend *backend = nullptr,
    const std::function<std::optional<std::string>(std::string_view)> &localize = {});

UiCommand popup_choice_command(const PopupDecision &decision);

// Default boundary for absent gameplay plumbing: all choices unavailable.
class PopupGameplayBackend final : public UiGameplayBackend {
  public:
    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;
};

} // namespace civ5::ui
