#pragma once

#include "civ5/content/manifest.hpp"
#include "civ5/assets/dds.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/ui/glyphgen.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace civ5::ui {

struct TopPanelElement {
    std::string name;
    std::vector<std::pair<std::string, std::string>> attributes;

    std::string attribute(std::string_view key) const;
};

struct TopPanelStack {
    TopPanelElement element;
    std::vector<TopPanelElement> children;
};

enum class TopPanelStackGrowth { left, right };

// Parsed placement attributes of an authored Stack. Child widget sizes and
// font metrics remain renderer-owned; this contract only exposes the source
// stack's origin, spacing, and direction.
struct TopPanelStackPlacement {
    std::int32_t offset_x = 0;
    std::int32_t offset_y = 0;
    std::int32_t padding = 0;
    TopPanelStackGrowth growth = TopPanelStackGrowth::right;
};

struct TopPanelPoint {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct TopPanelElementSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Source/layout contract for Assets/UI/InGame/TopPanel.xml. It retains raw
// authored attributes and order; it does not resolve assets or create widgets.
struct TopPanelLayout {
    TopPanelElement root;
    TopPanelElement center_bar;
    TopPanelStack info_stack;
    TopPanelElement right_bar;
    TopPanelStack right_stack;
    TopPanelElement tooltip_type;
    TopPanelElement tooltip_grid;
    TopPanelElement tooltip_label;

    static TopPanelLayout from_xml(const content::XmlNode &root);
};

TopPanelLayout read_top_panel(const std::string &path);
// Read authored UI XML through the caller's active VFS mount order. The VFS
// owns source precedence; this loader only validates and parses the selected
// TopPanel document.
TopPanelLayout read_top_panel(const assets::ResourceMountTable &resources);

TopPanelStackPlacement plan_top_panel_stack(const TopPanelStack &stack);
TopPanelElementSize top_panel_element_size(const TopPanelElement &element);
TopPanelPoint top_panel_element_offset(const TopPanelElement &element);
TopPanelPoint top_panel_element_texture_offset(const TopPanelElement &element);
std::int32_t top_panel_element_integer(const TopPanelElement &element, std::string_view key);

struct TopPanelBarBinding {
    assets::Provenance provenance;
    std::string entry_name;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    assets::DdsFormat format = assets::DdsFormat::dxt1;
    std::uint32_t mip_count = 0;
};

// Bind both authored bars to one caller-selected source. The source decides
// archive/DLC precedence; this function only accepts one unambiguous DDS and
// validates its actual payload metadata.
TopPanelBarBinding bind_top_panel_bar(const TopPanelLayout &layout,
                                      const std::shared_ptr<const assets::AssetSource> &source);
TopPanelBarBinding bind_top_panel_bar(const TopPanelLayout &layout,
                                      const assets::ResourceMountTable &resources);

struct TopPanelRect {
    std::uint32_t x = 0, y = 0, width = 0, height = 0;
};
struct TopPanelRenderPlan {
    TopPanelRect center_bar;
    TopPanelRect right_bar;
};

struct TopPanelGrid9Rect {
    std::int32_t x = 0, y = 0, width = 0, height = 0;
};

// One authored nine-slice style from Assets/UI/Styles.xml. Source rectangles
// remain in texture pixels; no filtering, state choice, or widget behavior is
// inferred here.
struct TopPanelGrid9Style {
    std::string name;
    std::string texture;
    std::array<TopPanelGrid9Rect, 9> source{};
    TopPanelGrid9Rect state_offset{};

    static TopPanelGrid9Style from_xml(const content::XmlNode &node);
};

TopPanelGrid9Style read_top_panel_grid9_style(const std::string &path, std::string_view style_name);
TopPanelGrid9Style read_top_panel_grid9_style(const assets::ResourceMountTable &resources,
                                              std::string_view style_name);

struct TopPanelGrid9Plan {
    TopPanelRect frame;
    TopPanelRect content;
    std::array<TopPanelRect, 9> destination{};
};

TopPanelGrid9Plan plan_top_panel_grid9(const TopPanelLayout &layout, const TopPanelGrid9Style &style,
                                       TopPanelRect frame);

struct TopPanelTooltipPlan {
    TopPanelGrid9Plan grid;
    std::vector<GlyphGenInlineLine> lines;
    // First line origin in the same baseline convention as the glyph projector.
    TopPanelPoint text_origin;
    std::int32_t line_step = 0;
};

// Measured preview counterpart of the authored tooltip DoAutoSize calls.
// WrapWidth, LeadingOffset, label offsets and grid padding come from XML.
// Bounds include glyph ink (including icons/overhangs) and explicit blank lines.
// This uses the existing compatibility glyph metrics; native autosize pixel
// equivalence still requires an independent original UI capture.
TopPanelTooltipPlan plan_top_panel_tooltip(
    const TopPanelLayout &layout, const TopPanelGrid9Style &style,
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry,
    std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point,
    TopPanelPoint anchor, std::uint32_t viewport_width, std::uint32_t viewport_height);

// Caller-selected language directory after source precedence has been
// resolved. The catalog retains exact Text payloads and only formats the
// positional placeholders needed by the typed TopPanel presentation.
struct TopPanelLocalizationCatalog {
    std::unordered_map<std::string, std::string> rows;

    static TopPanelLocalizationCatalog read_directory(const std::string &directory,
                                                       std::string_view language_root = "Language_en_US");
    static TopPanelLocalizationCatalog read_resources(
        const assets::ResourceMountTable &resources, const std::vector<std::string> &keys,
        std::string_view language_root = "Language_en_US");
    // Load the selected single-player BNW text layers in authored order.
    // Later Expansion/Expansion2 rows replace an earlier Tag, as in the game
    // database; scenario and leader-dialog files are outside TopPanel scope.
    static TopPanelLocalizationCatalog read_active_bnw_resources(
        const assets::ResourceMountTable &resources,
        std::string_view language_root = "Language_en_US");
    std::string format(std::string_view key, const std::vector<std::string> &arguments = {}) const;
};

// Convert only the authored Full.32 / 295.32 bar sizes to a top-left render
// plan. The graphics backend owns coordinate-system conversion and draw state.
TopPanelRenderPlan plan_top_panel(const TopPanelLayout &layout, std::uint32_t viewport_width,
                                  std::uint32_t viewport_height);

struct StrategicResource {
    std::string icon_text_key;
    int available = 0;
    int used = 0;
    bool revealed_and_city_tradeable = false;
    std::string description_text_key;
    int total = 0;
    int happiness = 0;
};

struct TopPanelPlayerState {
    int city_count = 0;
    int science_per_turn = 0;
    int science_from_budget_deficit_times100 = 0;
    int science_from_cities_times100 = 0;
    int science_from_other_players_times100 = 0;
    int science_from_happiness_times100 = 0;
    int science_from_research_agreements_times100 = 0;
    int gold = 0;
    int gold_per_turn = 0;
    int international_trade_routes_used = 0;
    int international_trade_routes_available = 0;
    int tourism = 0;
    int faith = 0;
    int faith_per_turn = 0;
    int golden_age_tourism_modifier = 0;
    int gold_per_turn_from_diplomacy = 0;
    int gold_from_cities_times100 = 0;
    int city_connection_gold_times100 = 0;
    int unit_cost = 0;
    int unit_supply = 0;
    int building_maintenance = 0;
    int improvement_maintenance = 0;
    int excess_happiness = 0;
    bool empire_unhappy = false;
    bool empire_very_unhappy = false;
    bool empire_super_unhappy = false;
    int happiness_from_policies = 0;
    int happiness_from_resources = 0;
    int extra_happiness_per_luxury = 0;
    int happiness_from_buildings = 0;
    int happiness_from_garrisoned_units = 0;
    int happiness_from_trade_routes = 0;
    int happiness_from_religion = 0;
    int happiness_from_natural_wonders = 0;
    int happiness_from_minors = 0;
    int extra_happiness_per_city = 0;
    int happiness_from_handicap = 0;
    int happiness_from_resource_variety = 0;
    int total_happiness = 0;
    int total_unhappiness = 0;
    int unhappiness_from_units_times100 = 0;
    int unhappiness_from_city_count_times100 = 0;
    int unhappiness_from_captured_city_count_times100 = 0;
    int unhappiness_from_puppet_city_population_times100 = 0;
    int unhappiness_from_city_specialists_times100 = 0;
    int unhappiness_from_city_population_times100 = 0;
    int unhappiness_from_occupied_cities_times100 = 0;
    int golden_age_turns = 0;
    int golden_age_progress = 0;
    int golden_age_threshold = 0;
    int culture = 0;
    int next_policy_cost = 0;
    int culture_per_turn = 0;
    int culture_for_free = 0;
    int culture_from_cities = 0;
    int culture_from_happiness = 0;
    int culture_from_minors = 0;
    int num_cities_policy_cost_mod = 0;
    std::vector<StrategicResource> strategic_resources;
    int unit_production_maintenance_mod = 0;
    int units_supplied = 0;
    int units_out_of_supply = 0;
};

enum class TopPanelMetric {
    science, gold, trade, happiness, golden_age, culture, tourism, faith, resources, unit_supply
};

struct TopPanelState {
    std::optional<TopPanelPlayerState> player;
    bool expansion2_ui = false;
    bool no_science = false;
    bool no_happiness = false;
    bool no_policies = false;
    bool no_religion = false;
    bool no_basic_help = false;
    bool head_selected_city = false;
    bool city_screen_up = false;
    int game_turn = 0;
    std::string turn_date;
    // Absent means the caller has supplied a complete retail-style state.
    // A partial live/saved source lists only metrics with all required inputs.
    std::optional<std::set<TopPanelMetric>> available_metrics;
};

struct TopPanelTextLocalizer {
    // The game-data runtime owns localization and parameter formatting. The
    // callback receives already typed text arguments in source order.
    std::function<std::string(std::string_view, const std::vector<std::string> &)> text;
};

enum class TopPanelAction { open_civilopedia, open_game_menu, exit_city_screen, open_policy_choice,
                            open_technology_tree };

// The retail TopPanel.lua routes these actions through distinct game/UI
// callbacks. Keep the route explicit so a future game-screen owner can bind
// the event without reverse-engineering the presentation action again.
enum class TopPanelActionRoute {
    search_for_pedia_entry,
    queue_game_menu,
    serial_event_exit_city_screen,
    choose_policy_popup,
    tech_tree_popup
};

struct TopPanelActionEvent {
    TopPanelAction action = TopPanelAction::open_game_menu;
    TopPanelActionRoute route = TopPanelActionRoute::queue_game_menu;
};

TopPanelActionEvent top_panel_action_event(TopPanelAction action);

using TopPanelActionSink = std::function<void(const TopPanelActionEvent &)>;

struct TopPanelButton {
    std::string text;
    std::string tooltip;
    TopPanelAction action = TopPanelAction::open_game_menu;
};

struct TopPanelPresentation {
    bool show_info_stack = false;
    bool show_unit_supply = false;
    std::optional<TopPanelButton> menu_button;
    TopPanelAction civilopedia_action = TopPanelAction::open_civilopedia;
    TopPanelAction science_action = TopPanelAction::open_technology_tree;
    TopPanelAction culture_action = TopPanelAction::open_policy_choice;
    std::optional<std::string> science_text;
    std::optional<std::string> gold_text;
    std::optional<std::string> international_trade_routes_text;
    std::optional<std::string> happiness_text;
    std::optional<std::string> golden_age_text;
    std::optional<std::string> culture_text;
    std::optional<std::string> tourism_text;
    std::optional<std::string> faith_text;
    std::optional<std::string> resource_text;
    std::optional<std::string> science_tooltip;
    std::optional<std::string> gold_tooltip;
    std::optional<std::string> international_trade_routes_tooltip;
    std::optional<std::string> happiness_tooltip;
    std::optional<std::string> golden_age_tooltip;
    std::optional<std::string> culture_tooltip;
    std::optional<std::string> tourism_tooltip;
    std::optional<std::string> faith_tooltip;
    std::optional<std::string> resource_tooltip;
    std::optional<std::string> unit_supply_tooltip;
    std::string current_turn_text;
    std::string current_date_text;
};

// Ports the visible base TopPanel.lua UpdateData branch. Lua is never
// executed; the caller must bind the exact retail source before calling this
// function and provide the authoritative game/localization state.
std::optional<TopPanelPresentation> present_top_panel(const TopPanelState &state,
                                                       const TopPanelTextLocalizer &localizer);

// Widget values begin at the authored XML defaults. UpdateData leaves every
// widget unchanged without an active player; without cities it hides the info
// stack and retains the menu text, while still updating turn/date and supply.
class TopPanelControls {
  public:
    TopPanelControls(const TopPanelLayout &layout, const TopPanelTextLocalizer &localizer);
    const TopPanelPresentation &apply(const std::optional<TopPanelPresentation> &update);
    const TopPanelPresentation &presentation() const { return presentation_; }

  private:
    TopPanelPresentation presentation_;
};

// A game-screen owned TopPanel frame. The provider is sampled once for each
// presentation, so a caller can publish an authoritative state snapshot at
// its frame boundary. Actions are converted to the same typed routes used by
// the authored Lua callbacks and forwarded to the screen owner after the
// conversion. This keeps preview code from owning game state or popups.
class TopPanelRuntime {
  public:
    using StateProvider = std::function<TopPanelState()>;

    TopPanelRuntime(StateProvider state_provider, TopPanelTextLocalizer localizer,
                    TopPanelActionSink action_sink = {});

    TopPanelState state() const;
    std::optional<TopPanelPresentation> present() const;
    TopPanelActionEvent dispatch(TopPanelAction action) const;

  private:
    StateProvider state_provider_;
    TopPanelTextLocalizer localizer_;
    TopPanelActionSink action_sink_;
};

} // namespace civ5::ui
