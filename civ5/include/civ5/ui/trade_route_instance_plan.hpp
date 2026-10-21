#pragma once

#include "civ5/ui/trade_route_available_rows.hpp"

namespace civ5::ui {

struct TradeRouteInstanceSource {
    TradeRouteTabSourceProof tabs;
    bool verified = false;
    std::int32_t nominal_row_width = 0, nominal_row_height = 0;
    std::int32_t nominal_scroll_width = 0, nominal_scroll_height = 0;
    std::string blocker;
};

// Checks the selected Expansion2 TRInstance/MainScroll XML and Lua build,
// refresh and layout call order. Nominal sizes are source metadata, not a
// substitute for measured physical-pixel geometry in a live frame.
TradeRouteInstanceSource inspect_trade_route_instance_source(
    const assets::ResourceMountTable &resources);

struct TradeRouteMeasuredInstances {
    TradeRouteTabOwner owner;
    // Current tab reported by the native popup owner after TabSelect; Data2
    // alone only identifies the initial tab and cannot prove later changes.
    TradeRouteTab tab = TradeRouteTab::your_routes;
    OverviewRect main_scroll;
    std::vector<OverviewRect> row_rects;
};

struct TradeRouteInstanceView {
    OverviewRect row_rect, visible_rect;
    std::optional<TradeRouteAvailableRow> complete_row;
    std::optional<OverviewRow> outgoing_summary;
};

struct TradeRouteInstancePlan {
    bool available = false;
    TradeRouteTab tab = TradeRouteTab::your_routes;
    OverviewRect main_scroll;
    std::vector<TradeRouteInstanceView> rows;
    std::string partial_reason, blocker;
    // No row commands: TradeRouteOverview DisplayData registers no per-row
    // gameplay callback, and native MainScroll/instance layout is not emulated.
};

TradeRouteInstancePlan plan_trade_route_instances(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteInstanceSource &source,
    const TradeRouteMeasuredInstances &measured,
    std::string_view active_rules_sha256,
    const TradeRouteAvailableCallback &available_callback = {},
    const TradeRouteIncomingCallback &incoming_callback = {});

} // namespace civ5::ui
