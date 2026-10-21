#pragma once

#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/overview_screens.hpp"

namespace civ5::ui {

struct TradeRouteTabSourceProof {
    bool verified = false;
    std::string lua_path, lua_sha256, xml_path, xml_sha256;
    std::string blocker;
};

// The selected Expansion2 Lua/XML own the three tab callbacks. This verifies
// their authored order and each named control, not the native UIManager.
TradeRouteTabSourceProof inspect_trade_route_tab_source(
    const assets::ResourceMountTable &resources);

enum class TradeRouteTab { your_routes, available_routes, routes_to_you };
struct TradeRouteTabOwner {
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0, owner_sequence = 0;
    const void *frame_owner = nullptr;
    std::uint64_t frame_sequence = 0;
    std::int32_t viewport_width = 0, viewport_height = 0;
    // From the current BUTTONPOPUP_TRADE_ROUTE_OVERVIEW Data2. The generic
    // PopupFrame does not encode it, so an absent native value is not guessed.
    std::optional<std::int32_t> popup_data2;
    std::string lua_sha256, xml_sha256;
};

struct TradeRouteTabRects {
    TradeRouteTabOwner owner;
    OverviewRect your_routes, available_routes, routes_to_you;
};

struct TradeRouteTabView {
    bool available = false;
    std::optional<TradeRouteTab> selected;
    std::vector<OverviewRow> current_owned_rows;
    std::string partial_reason, blocker;
};

enum class TradeRouteTabStatus { unavailable, performed_read_only };
struct TradeRouteTabInputResult {
    bool consumed = false;
    TradeRouteTab tab = TradeRouteTab::your_routes;
    TradeRouteTabStatus status = TradeRouteTabStatus::unavailable;
    std::string blocker;
};

// Only the owned outgoing-route snapshot supports a local YourTR refresh.
// Native available/incoming route queries and row commands remain unavailable.
class TradeRouteOverviewInput {
  public:
    explicit TradeRouteOverviewInput(TradeRouteTabSourceProof source);
    bool bind(const LiveUiFrame &frame, const UiScreenController &screens,
        const TradeRouteTabRects &rects, std::string &blocker);
    TradeRouteTabView view(const LiveUiFrame &frame,
        const UiScreenController &screens) const;
    TradeRouteTabInputResult click(const LiveUiFrame &frame,
        const UiScreenController &screens, std::int32_t press_x,
        std::int32_t press_y, std::int32_t release_x,
        std::int32_t release_y);

  private:
    bool current(const LiveUiFrame &frame,
                 const UiScreenController &screens) const;
    TradeRouteTabSourceProof source_;
    std::optional<TradeRouteTabRects> rects_;
    std::optional<TradeRouteTab> selected_;
};

} // namespace civ5::ui
