#include "civ5/ui/trade_route_overview_input.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>

namespace civ5::ui {
namespace {
bool ordered(std::string_view text, std::initializer_list<std::string_view> tokens) {
    std::size_t cursor = 0;
    for (const auto token : tokens) {
        const auto at = text.find(token, cursor);
        if (at == std::string_view::npos) return false;
        cursor = at + token.size();
    }
    return true;
}

bool valid(const OverviewRect &r, const TradeRouteTabOwner &owner) {
    return r.x >= 0 && r.y >= 0 && r.width > 0 && r.height > 0 &&
        owner.viewport_width > 0 && owner.viewport_height > 0 &&
        std::int64_t(r.x) + r.width <= owner.viewport_width &&
        std::int64_t(r.y) + r.height <= owner.viewport_height;
}

bool overlaps(const OverviewRect &a, const OverviewRect &b) {
    return std::int64_t(a.x) < std::int64_t(b.x) + b.width &&
        std::int64_t(b.x) < std::int64_t(a.x) + a.width &&
        std::int64_t(a.y) < std::int64_t(b.y) + b.height &&
        std::int64_t(b.y) < std::int64_t(a.y) + a.height;
}

bool contains(const OverviewRect &r, std::int32_t x, std::int32_t y) {
    return x >= r.x && y >= r.y &&
        std::int64_t(x) < std::int64_t(r.x) + r.width &&
        std::int64_t(y) < std::int64_t(r.y) + r.height;
}

std::optional<TradeRouteTab> hit(const TradeRouteTabRects &r,
    std::int32_t x, std::int32_t y) {
    if (contains(r.your_routes, x, y)) return TradeRouteTab::your_routes;
    if (contains(r.available_routes, x, y)) return TradeRouteTab::available_routes;
    if (contains(r.routes_to_you, x, y)) return TradeRouteTab::routes_to_you;
    return std::nullopt;
}

bool unique_button(const content::XmlNode &xml, std::string_view id) {
    std::size_t count = 0;
    const auto visit = [&](const auto &self, const content::XmlNode &node) -> void {
        if (node.name == "Button" && node.attribute("ID") == id) ++count;
        for (const auto &child : node.children) self(self, child);
    };
    visit(visit, xml);
    return count == 1;
}

std::string selected_path(const assets::AssetHandle &handle) {
    const auto &p = handle.candidate().provenance;
    return p.archive_relative_path
        ? *p.archive_relative_path + ":" + p.source_relative_path
        : p.source_relative_path;
}
} // namespace

TradeRouteTabSourceProof inspect_trade_route_tab_source(
    const assets::ResourceMountTable &resources) {
    TradeRouteTabSourceProof proof;
    const auto lua_result = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/Popups/TradeRouteOverview.lua"));
    const auto xml_result = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/Popups/TradeRouteOverview.xml"));
    const auto *lua_handle = std::get_if<assets::AssetHandle>(&lua_result.asset);
    const auto *xml_handle = std::get_if<assets::AssetHandle>(&xml_result.asset);
    if (!lua_handle || !xml_handle) {
        proof.blocker = "Selected BNW TradeRouteOverview Lua or XML is missing or ambiguous";
        return proof;
    }
    try {
        const auto lua_bytes = lua_handle->read_bytes(1024 * 1024);
        const auto xml_bytes = xml_handle->read_bytes(8 * 1024 * 1024);
        const std::string lua(lua_bytes.begin(), lua_bytes.end());
        const auto xml = content::read_xml_metadata(
            std::string(xml_bytes.begin(), xml_bytes.end()));
        if (xml.name != "Context" ||
            !unique_button(xml, "TabButtonYourTR") ||
            !unique_button(xml, "TabButtonAvailableTR") ||
            !unique_button(xml, "TabButtonTRWithYou") ||
            !ordered(lua, {"function OnPopupMessage(popupInfo)",
                "BUTTONPOPUP_TRADE_ROUTE_OVERVIEW", "g_PopupInfo.Data2 == 1",
                "g_PopupInfo.Data2 == 2", "g_PopupInfo.Data2 == 3",
                "Events.SerialEventGameMessagePopup.Add( OnPopupMessage )"}) ||
            !ordered(lua, {"function TabSelect(tab)",
                "v.SelectHighlight:SetHide(bHide)", "g_CurrentTab = tab",
                "g_Tabs[tab].RefreshContent()",
                "TabButtonYourTR:RegisterCallback",
                "TabButtonAvailableTR:RegisterCallback",
                "TabButtonTRWithYou:RegisterCallback"}) ||
            !ordered(lua, {"function RefreshYourTR()", "GetTradeRoutes()",
                "SortData()", "DisplayData()", "function RefreshAvailableTR()",
                "GetTradeRoutesAvailable()", "function RefreshTRWithYou()",
                "GetTradeRoutesToYou()"})) {
            proof.blocker = "Selected TradeRouteOverview tab callback or controls differ from BNW source";
            return proof;
        }
        proof.lua_path = selected_path(*lua_handle);
        proof.xml_path = selected_path(*xml_handle);
        proof.lua_sha256 = runtime::identify_bytes(lua_bytes).sha256;
        proof.xml_sha256 = runtime::identify_bytes(xml_bytes).sha256;
        proof.verified = true;
    } catch (const std::exception &) {
        proof.blocker = "Selected TradeRouteOverview source cannot be read";
    }
    return proof;
}

TradeRouteOverviewInput::TradeRouteOverviewInput(TradeRouteTabSourceProof source)
    : source_(std::move(source)) {}

bool TradeRouteOverviewInput::bind(const LiveUiFrame &frame,
    const UiScreenController &screens, const TradeRouteTabRects &rects,
    std::string &blocker) {
    const auto previous = rects_;
    rects_.reset();
    const auto &o = rects.owner;
    const auto *popup = frame.snapshot.find<PopupFrame>();
    const auto *overview = frame.snapshot.find<OverviewFrameState>();
    const auto *context = screens.registry().find("TradeRouteOverview");
    const auto page = overview ? overview->pages.find("TradeRouteOverview")
                               : std::map<std::string, OverviewPage>::const_iterator{};
    if (!source_.verified || source_.lua_sha256.empty() ||
        source_.xml_sha256.empty() ||
        !context || !context->modal ||
        screens.focused_screen() != std::optional<std::string>{"TradeRouteOverview"} ||
        !screens.visible("TradeRouteOverview") || !frame.owner || !frame.sequence ||
        o.frame_owner != frame.owner || o.frame_sequence != frame.sequence ||
        !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
        o.player_id != *frame.snapshot.active_player_id() ||
        o.turn != *frame.snapshot.turn() || o.frame != frame.snapshot.frame() ||
        o.event_id < 0 || !o.owner_sequence ||
        (o.popup_data2 && (*o.popup_data2 < 1 || *o.popup_data2 > 3)) ||
        o.lua_sha256 != source_.lua_sha256 ||
        o.xml_sha256 != source_.xml_sha256 ||
        !popup || popup->player_id != o.player_id || popup->turn != o.turn ||
        popup->owner_sequence != o.owner_sequence ||
        popup->events.size() != 1 ||
        popup->events.front().context_id != "TradeRouteOverview" ||
        popup->events.front().event_id != o.event_id ||
        !overview || overview->player_id != o.player_id ||
        page == overview->pages.end() ||
        !valid(rects.your_routes, o) ||
        !valid(rects.available_routes, o) ||
        !valid(rects.routes_to_you, o) ||
        overlaps(rects.your_routes, rects.available_routes) ||
        overlaps(rects.your_routes, rects.routes_to_you) ||
        overlaps(rects.available_routes, rects.routes_to_you)) {
        blocker = "TradeRouteOverview owner, live route page or measured tabs are unavailable";
        return false;
    }
    if (!previous || previous->owner.player_id != o.player_id ||
        previous->owner.event_id != o.event_id ||
        previous->owner.turn != o.turn ||
        previous->owner.owner_sequence != o.owner_sequence ||
        previous->owner.lua_sha256 != o.lua_sha256 ||
        previous->owner.xml_sha256 != o.xml_sha256 ||
        previous->owner.popup_data2 != o.popup_data2)
        selected_ = o.popup_data2 == 1 ? std::optional{TradeRouteTab::your_routes}
                                       : std::nullopt;
    rects_ = rects;
    blocker.clear();
    return true;
}

bool TradeRouteOverviewInput::current(const LiveUiFrame &frame,
    const UiScreenController &screens) const {
    const auto *context = screens.registry().find("TradeRouteOverview");
    if (!rects_ || !frame.owner || !frame.sequence ||
        !context || !context->modal ||
        frame.owner != rects_->owner.frame_owner ||
        frame.sequence != rects_->owner.frame_sequence ||
        frame.snapshot.frame() != rects_->owner.frame ||
        frame.snapshot.active_player_id() != rects_->owner.player_id ||
        frame.snapshot.turn() != rects_->owner.turn ||
        screens.focused_screen() != std::optional<std::string>{"TradeRouteOverview"} ||
        !screens.visible("TradeRouteOverview")) return false;
    const auto *popup = frame.snapshot.find<PopupFrame>();
    return popup && popup->player_id == rects_->owner.player_id &&
        popup->turn == rects_->owner.turn &&
        popup->owner_sequence == rects_->owner.owner_sequence &&
        popup->events.size() == 1 &&
        popup->events.front().context_id == "TradeRouteOverview" &&
        popup->events.front().event_id == rects_->owner.event_id;
}

TradeRouteTabView TradeRouteOverviewInput::view(const LiveUiFrame &frame,
    const UiScreenController &screens) const {
    TradeRouteTabView result;
    if (!current(frame, screens)) {
        result.blocker = "TradeRouteOverview owner frame is stale or absent";
        return result;
    }
    const auto *overview = frame.snapshot.find<OverviewFrameState>();
    if (!overview || overview->player_id != rects_->owner.player_id ||
        !overview->pages.contains("TradeRouteOverview")) {
        result.blocker = "Owned outgoing routes are unavailable in this frame";
        return result;
    }
    result.available = true;
    result.selected = selected_;
    if (selected_ == TradeRouteTab::your_routes) {
        const auto &page = overview->pages.at("TradeRouteOverview");
        result.current_owned_rows = page.rows;
        result.partial_reason = page.unavailable_reason;
    } else {
        result.partial_reason = "Native available/incoming route query is unavailable";
    }
    return result;
}

TradeRouteTabInputResult TradeRouteOverviewInput::click(
    const LiveUiFrame &frame, const UiScreenController &screens,
    std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    TradeRouteTabInputResult result;
    const auto *context = screens.registry().find("TradeRouteOverview");
    if (screens.focused_screen() !=
            std::optional<std::string>{"TradeRouteOverview"} ||
        !screens.visible("TradeRouteOverview") ||
        !context || !context->modal) return result;
    result.consumed = true;
    if (!current(frame, screens)) {
        result.blocker = "TradeRouteOverview measured input owner is stale";
        return result;
    }
    const auto pressed = hit(*rects_, press_x, press_y);
    const auto released = hit(*rects_, release_x, release_y);
    if (!pressed || pressed != released) {
        result.blocker = "TradeRouteOverview press and release do not match one tab";
        return result;
    }
    result.tab = *pressed;
    if (*pressed != TradeRouteTab::your_routes) {
        result.blocker = *pressed == TradeRouteTab::available_routes
            ? "Native GetTradeRoutesAvailable query is unavailable"
            : "Native GetTradeRoutesToYou query is unavailable";
        return result;
    }
    if (!view(frame, screens).available) {
        result.blocker = "Current owned route rows are unavailable";
        return result;
    }
    selected_ = TradeRouteTab::your_routes;
    result.status = TradeRouteTabStatus::performed_read_only;
    return result;
}

} // namespace civ5::ui
