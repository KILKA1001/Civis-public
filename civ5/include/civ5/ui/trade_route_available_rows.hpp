#pragma once

#include "civ5/ui/trade_route_overview_input.hpp"
#include <functional>

namespace civ5::ui {

// Scalar schema shared by the two native route queries and BNW DisplayData.
// The owner must resolve city objects, civilization names and the authored
// BuildTradeRouteToolTipString before publishing this read-only quote.
struct TradeRouteAvailableRow {
    std::int64_t from_player = -1, to_player = -1;
    std::int64_t from_city = -1, to_city = -1;
    std::string from_civ, to_civ, from_city_name, to_city_name;
    std::string tooltip;
    enum class Domain { land, sea };
    std::optional<Domain> domain;
    // Presence matters: zero is a valid authored value, but an omitted field
    // must not be silently interpreted as zero by DisplayData.
    std::optional<std::int64_t> from_gpt, to_gpt;
    std::optional<std::int64_t> to_food, to_production;
    std::optional<std::int64_t> from_religion, to_religion;
    std::optional<std::int64_t> from_pressure, to_pressure;
    std::optional<std::int64_t> from_science, to_science;
    std::optional<std::int32_t> turns_left;
};

struct TradeRouteAvailableQuote {
    TradeRouteTabOwner owner;
    std::string rules_sha256;
    std::vector<TradeRouteAvailableRow> rows;
};

using TradeRouteAvailableCallback = std::function<std::optional<
    TradeRouteAvailableQuote>(const TradeRouteTabOwner &, std::string_view)>;

struct TradeRouteAvailableResult {
    bool available = false;
    std::vector<TradeRouteAvailableRow> rows;
    std::string blocker;
};

// Calling this does not select the tab, create native TRInstance controls or
// launch a trade route. The UI can consume rows only after owner presentation.
TradeRouteAvailableResult quote_trade_routes_available(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteTabSourceProof &source, const TradeRouteTabOwner &owner,
    std::string_view active_rules_sha256,
    const TradeRouteAvailableCallback &callback);

struct TradeRouteIncomingQuote {
    TradeRouteTabOwner owner;
    std::string rules_sha256;
    std::vector<TradeRouteAvailableRow> rows;
};
using TradeRouteIncomingCallback = std::function<std::optional<
    TradeRouteIncomingQuote>(const TradeRouteTabOwner &, std::string_view)>;
using TradeRouteIncomingResult = TradeRouteAvailableResult;

// RefreshTRWithYou uses the same SetData/SortData/DisplayData path, but its
// native result must terminate at the active player. No save-derived fallback.
TradeRouteIncomingResult quote_trade_routes_to_you(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteTabSourceProof &source, const TradeRouteTabOwner &owner,
    std::string_view active_rules_sha256,
    const TradeRouteIncomingCallback &callback);

} // namespace civ5::ui
