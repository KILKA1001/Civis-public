#include "civ5/ui/trade_route_available_rows.hpp"

namespace civ5::ui {
namespace {
bool same_owner(const TradeRouteTabOwner &a, const TradeRouteTabOwner &b) {
    return a.player_id == b.player_id && a.event_id == b.event_id &&
        a.turn == b.turn && a.frame == b.frame &&
        a.owner_sequence == b.owner_sequence &&
        a.frame_owner == b.frame_owner &&
        a.frame_sequence == b.frame_sequence &&
        a.viewport_width == b.viewport_width &&
        a.viewport_height == b.viewport_height &&
        a.popup_data2 == b.popup_data2 &&
        a.lua_sha256 == b.lua_sha256 && a.xml_sha256 == b.xml_sha256;
}

bool sha256(std::string_view hash) {
    if (hash.size() != 64) return false;
    for (const char c : hash)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

bool complete(const TradeRouteAvailableRow &r, std::int64_t active_player,
              bool incoming) {
    return (incoming ? r.to_player == active_player && r.from_player >= 0
                     : r.from_player == active_player && r.to_player >= 0) &&
        r.from_city >= 0 && r.to_city >= 0 &&
        !r.from_civ.empty() && !r.to_civ.empty() &&
        !r.from_city_name.empty() && !r.to_city_name.empty() &&
        !r.tooltip.empty() && r.domain &&
        (*r.domain == TradeRouteAvailableRow::Domain::land ||
         *r.domain == TradeRouteAvailableRow::Domain::sea) &&
        r.from_gpt && r.to_gpt && r.to_food && r.to_production &&
        r.from_religion && r.to_religion &&
        r.from_pressure && r.to_pressure &&
        r.from_science && r.to_science &&
        *r.from_religion >= -1 && *r.to_religion >= -1 &&
        *r.from_pressure >= 0 && *r.to_pressure >= 0 &&
        (!r.turns_left || *r.turns_left >= -1);
}

template <typename Callback>
TradeRouteAvailableResult quote_routes(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteTabSourceProof &source, const TradeRouteTabOwner &owner,
    std::string_view active_rules_sha256, const Callback &callback,
    bool incoming, std::string_view native_name) {
    TradeRouteAvailableResult result;
    const auto *context = screens.registry().find("TradeRouteOverview");
    const auto *popup = frame.snapshot.find<PopupFrame>();
    if (!source.verified || !sha256(source.lua_sha256) ||
        !sha256(source.xml_sha256) || !sha256(active_rules_sha256) ||
        !callback || !context || !context->modal ||
        screens.focused_screen() !=
            std::optional<std::string>{"TradeRouteOverview"} ||
        !screens.visible("TradeRouteOverview") ||
        !frame.owner || !frame.sequence ||
        owner.frame_owner != frame.owner ||
        owner.frame_sequence != frame.sequence ||
        owner.frame != frame.snapshot.frame() ||
        !frame.snapshot.active_player_id() ||
        owner.player_id != *frame.snapshot.active_player_id() ||
        !frame.snapshot.turn() || owner.turn != *frame.snapshot.turn() ||
        owner.event_id < 0 || !owner.owner_sequence ||
        owner.lua_sha256 != source.lua_sha256 ||
        owner.xml_sha256 != source.xml_sha256 ||
        !popup || popup->player_id != owner.player_id ||
        popup->turn != owner.turn ||
        popup->owner_sequence != owner.owner_sequence ||
        popup->events.size() != 1 ||
        popup->events.front().context_id != "TradeRouteOverview" ||
        popup->events.front().event_id != owner.event_id) {
        result.blocker = "Current popup, active player, selected rules or native query owner is unavailable";
        return result;
    }
    try {
        const auto quote = callback(owner, active_rules_sha256);
        if (!quote || !same_owner(quote->owner, owner) ||
            quote->rules_sha256 != active_rules_sha256) {
            result.blocker = "Native " + std::string(native_name) +
                " quote is absent or belongs to another owner/ruleset";
            return result;
        }
        for (const auto &row : quote->rows) {
            if (!complete(row, owner.player_id, incoming)) {
                result.blocker = "Native " + std::string(native_name) +
                    " row lacks required BNW DisplayData fields";
                return result;
            }
        }
        result.available = true;
        result.rows = quote->rows;
    } catch (const std::exception &) {
        result.blocker = "Native " + std::string(native_name) + " callback failed";
    }
    return result;
}
} // namespace

TradeRouteAvailableResult quote_trade_routes_available(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteTabSourceProof &source, const TradeRouteTabOwner &owner,
    std::string_view active_rules_sha256,
    const TradeRouteAvailableCallback &callback) {
    return quote_routes(frame, screens, source, owner, active_rules_sha256,
        callback, false, "GetTradeRoutesAvailable");
}

TradeRouteIncomingResult quote_trade_routes_to_you(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteTabSourceProof &source, const TradeRouteTabOwner &owner,
    std::string_view active_rules_sha256,
    const TradeRouteIncomingCallback &callback) {
    return quote_routes(frame, screens, source, owner, active_rules_sha256,
        callback, true, "GetTradeRoutesToYou");
}

} // namespace civ5::ui
