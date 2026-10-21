#pragma once

#include "civ5/ui/world_view.hpp"
#include "civ5/content/info_metadata.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/city_yields.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <functional>

namespace civ5::ui {

struct CityScreenOrder {
    std::int64_t type = -1, item = -1;
    std::optional<std::string> name;
};
struct CityScreenCity {
    std::int64_t owner = -1, id = -1, x = 0, y = 0, population = 0;
    std::optional<std::string> name;
    std::vector<CityScreenOrder> queue;
    bool queue_resolved = false;
    // Saved production cache restored into the owned live city. Absent when
    // the current item or its source Info cannot be resolved.
    std::optional<std::int64_t> current_production_progress_times100;
    bool puppet = false, resistance = false, razing = false;
};
struct CityScreenFrame {
    std::int64_t player_id = -1;
    std::optional<std::int64_t> selected_city_id;
    std::vector<CityScreenCity> cities;
};

struct ProductionPopupChoice {
    std::int64_t order_type = -1, item_id = -1;
    std::optional<std::string> authored_type;
    UiCommandResult availability;
    std::optional<std::string> localized_name;
};
struct ProductionPopupChoices {
    std::int64_t player_id = -1, city_id = -1;
    std::int32_t turn = -1;
    const UiGameplayBackend *backend_owner = nullptr;
    std::string selected_source_path;
    std::size_t first_visible = 0, visible_rows = 0;
    std::vector<ProductionPopupChoice> entries;
    std::vector<std::string> unresolved;
};

// Bind live city ownership, population and queue to the matching saved city
// name. A saved record alone never supplies the current production state.
CityScreenFrame capture_city_screen_frame(const runtime::CityYieldContext &live,
    const saves::SavedSessionSnapshot &source, std::int64_t player_id,
    std::optional<std::int64_t> selected_city_id = std::nullopt,
    const std::function<std::optional<std::string>(std::string_view)> &localize = {});
// Resolve queue labels from the activated rules database. Missing/ambiguous
// rows or untranslated keys stay absent; source IDs are never display text.
void resolve_city_queue_names(CityScreenFrame &frame,
    const content::RulesDatabase &database, const content::InfoLocalizer &localize);
const CityScreenCity *selected_city(const UiFrameSnapshot &frame);

struct CityScreenSource {
    std::string context_id, selected_source_path;
    content::XmlNode document;
    std::vector<std::string> control_ids;
    bool contains(std::string_view id) const;
};
CityScreenSource read_city_screen_source(const assets::ResourceMountTable &resources,
    std::string_view context_id);
// Zero means that the selected XML/Lua viewport cannot hold a whole button.
std::size_t production_popup_visible_rows(const CityScreenSource &source,
                                          std::int32_t viewport_height);

// The selected BNW XML and Lua place the viewport and scroll controls. Civis'
// choice source still lacks native Lua eligibility, sorting and category state.
struct ProductionPopupLayout {
    WorldViewRect scroll_panel, choice_button, up_button, down_button;
};
std::optional<ProductionPopupLayout> layout_production_popup(
    const CityScreenSource &source, std::int32_t width, std::int32_t height);

// The default (collapsed queue) Expansion2 CityView geometry. The selected
// CityView.lua sets ProdQueueBackground to 280 px before laying out its stack.
// A different source or unsupported control hierarchy has no inferred layout.
struct CityViewLayout {
    WorldViewRect title_panel, city_info, production_panel;
    WorldViewRect purchase_button, production_button, return_button;
    std::string title_texture;
};
std::optional<CityViewLayout> layout_city_view(
    const CityScreenSource &source, std::int32_t width, std::int32_t height);

struct CityScreenPlan {
    std::string context_id, selected_source_path;
    std::vector<WorldViewPaint> paints;
    std::vector<WorldViewHit> hits;
    std::vector<std::string> unresolved;
    const WorldViewHit *hit(std::int32_t x, std::int32_t y) const;
};
CityScreenPlan plan_city_screen(const CityScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const UiGameplayBackend *backend = nullptr,
    const std::function<std::optional<std::string>(std::string_view)> &localize = {});

struct CityChooseProduction {
    std::int64_t city_id = -1, order_type = -1, item_id = -1;
};
enum class ProductionPopupScrollDirection { up, down };
struct ProductionPopupScroll {
    std::int64_t city_id = -1;
    ProductionPopupScrollDirection direction = ProductionPopupScrollDirection::down;
};
struct CityQueueAction {
    enum class Kind { remove, move_up, move_down } kind = Kind::remove;
    std::int64_t city_id = -1;
    std::size_t position = 0;
};
UiCommand city_open_command();
UiCommand city_production_popup_command();
UiCommand city_choose_production(std::int64_t city_id, std::int64_t order_type,
                                 std::int64_t item_id);
UiCommand city_scroll_production_popup(std::int64_t city_id,
                                      ProductionPopupScrollDirection direction);
UiCommand city_queue_command(CityQueueAction action);

// A safe fallback when no selected BNW production services are injected.
class CityGameplayBackend final : public UiGameplayBackend {
  public:
    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;
};

} // namespace civ5::ui
