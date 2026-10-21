#include "civ5/ui/trade_route_instance_plan.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <charconv>

namespace civ5::ui {
namespace {
const content::XmlNode *find_unique(const content::XmlNode &root,
    std::string_view element, std::string_view attribute,
    std::string_view value) {
    const content::XmlNode *found = nullptr;
    bool duplicate = false;
    const auto visit = [&](const auto &self, const content::XmlNode &node) -> void {
        if (node.name == element && node.attribute(std::string(attribute)) == value) {
            if (found) duplicate = true;
            else found = &node;
        }
        for (const auto &child : node.children) self(self, child);
    };
    visit(visit, root);
    return duplicate ? nullptr : found;
}

std::optional<std::pair<std::int32_t, std::int32_t>> size_of(
    std::string_view value) {
    const auto comma = value.find(',');
    if (comma == std::string_view::npos) return std::nullopt;
    std::int32_t x = 0, y = 0;
    const auto first = std::from_chars(value.data(), value.data() + comma, x);
    const auto second = std::from_chars(value.data() + comma + 1,
        value.data() + value.size(), y);
    if (first.ec != std::errc{} || second.ec != std::errc{} ||
        first.ptr != value.data() + comma ||
        second.ptr != value.data() + value.size() || x <= 0 || y <= 0)
        return std::nullopt;
    return {{x, y}};
}

bool ordered(std::string_view text,
    std::initializer_list<std::string_view> tokens) {
    std::size_t position = 0;
    for (const auto token : tokens) {
        const auto at = text.find(token, position);
        if (at == std::string_view::npos) return false;
        position = at + token.size();
    }
    return true;
}

bool in_viewport(const OverviewRect &rect, const TradeRouteTabOwner &owner) {
    return rect.x >= 0 && rect.y >= 0 && rect.width > 0 && rect.height > 0 &&
        std::int64_t(rect.x) + rect.width <= owner.viewport_width &&
        std::int64_t(rect.y) + rect.height <= owner.viewport_height;
}

bool sha256(std::string_view hash) {
    if (hash.size() != 64) return false;
    for (const auto c : hash)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

OverviewRect intersection(const OverviewRect &a, const OverviewRect &b) {
    const auto left = std::max(a.x, b.x), top = std::max(a.y, b.y);
    const auto right = std::min(std::int64_t(a.x) + a.width,
        std::int64_t(b.x) + b.width);
    const auto bottom = std::min(std::int64_t(a.y) + a.height,
        std::int64_t(b.y) + b.height);
    return {left, top, static_cast<std::int32_t>(std::max<std::int64_t>(0, right - left)),
        static_cast<std::int32_t>(std::max<std::int64_t>(0, bottom - top))};
}

bool current_owner(const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteInstanceSource &source,
    const TradeRouteTabOwner &owner) {
    const auto *context = screens.registry().find("TradeRouteOverview");
    const auto *popup = frame.snapshot.find<PopupFrame>();
    return source.verified && context && context->modal &&
        screens.visible("TradeRouteOverview") &&
        screens.focused_screen() ==
            std::optional<std::string>{"TradeRouteOverview"} &&
        frame.owner && frame.sequence && owner.frame_owner == frame.owner &&
        owner.frame_sequence == frame.sequence &&
        owner.frame == frame.snapshot.frame() &&
        frame.snapshot.active_player_id() == owner.player_id &&
        frame.snapshot.turn() == owner.turn &&
        (!owner.popup_data2 || (*owner.popup_data2 >= 1 &&
                                *owner.popup_data2 <= 3)) &&
        owner.event_id >= 0 && owner.owner_sequence &&
        owner.lua_sha256 == source.tabs.lua_sha256 &&
        owner.xml_sha256 == source.tabs.xml_sha256 &&
        popup && popup->player_id == owner.player_id &&
        popup->turn == owner.turn &&
        popup->owner_sequence == owner.owner_sequence &&
        popup->events.size() == 1 &&
        popup->events.front().context_id == "TradeRouteOverview" &&
        popup->events.front().event_id == owner.event_id;
}
} // namespace

TradeRouteInstanceSource inspect_trade_route_instance_source(
    const assets::ResourceMountTable &resources) {
    TradeRouteInstanceSource result;
    result.tabs = inspect_trade_route_tab_source(resources);
    if (!result.tabs.verified) {
        result.blocker = result.tabs.blocker;
        return result;
    }
    const auto lua_result = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/Popups/TradeRouteOverview.lua"));
    const auto xml_result = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/Popups/TradeRouteOverview.xml"));
    const auto *lua = std::get_if<assets::AssetHandle>(&lua_result.asset);
    const auto *xml = std::get_if<assets::AssetHandle>(&xml_result.asset);
    if (!lua || !xml) {
        result.blocker = "Selected TRInstance Lua/XML is missing or ambiguous";
        return result;
    }
    try {
        const auto lua_bytes = lua->read_bytes(1024 * 1024);
        const auto xml_bytes = xml->read_bytes(8 * 1024 * 1024);
        if (runtime::identify_bytes(lua_bytes).sha256 != result.tabs.lua_sha256 ||
            runtime::identify_bytes(xml_bytes).sha256 != result.tabs.xml_sha256) {
            result.blocker = "TRInstance source changed after tab preflight";
            return result;
        }
        const std::string script(lua_bytes.begin(), lua_bytes.end());
        const auto document = content::read_xml_metadata(
            std::string(xml_bytes.begin(), xml_bytes.end()));
        const auto *instance = find_unique(document, "Instance", "Name", "TRInstance");
        const auto *scroll = find_unique(document, "ScrollPanel", "ID", "MainScroll");
        const auto *stack = scroll ? find_unique(*scroll, "Stack", "ID", "MainStack")
                                   : nullptr;
        const auto *root = instance ? find_unique(*instance, "Box", "ID", "Root")
                                    : nullptr;
        const auto row_size = root ? size_of(root->attribute("Size")) : std::nullopt;
        const auto scroll_size = scroll ? size_of(scroll->attribute("Size"))
                                         : std::nullopt;
        if (document.name != "Context" || !instance || !root || !scroll ||
            !stack || !row_size || !scroll_size ||
            stack->attribute("StackGrowth") != "Bottom" ||
            scroll->attribute("Vertical") != "1" ||
            !ordered(script, {"function DisplayData()",
                "Controls.MainStack:DestroyAllChildren()",
                "ContextPtr:BuildInstanceForControl( \"TRInstance\"",
                "Controls.MainStack:CalculateSize()",
                "Controls.MainStack:ReprocessAnchoring()",
                "Controls.MainScroll:CalculateInternalSize()"})) {
            result.blocker = "Selected TRInstance creation or MainScroll layout differs from BNW source";
            return result;
        }
        for (const auto id : {"Domain_Land", "Domain_Sea", "FromCity",
                "ToCity", "FromGPT", "ToGPT", "ToFood", "ToProduction",
                "FromReligion", "ToReligion", "FromScience", "ToScience",
                "TurnsLeft"}) {
            // Elements differ, while the ID within one authored Instance is unique.
            std::size_t count = 0;
            const auto visit = [&](const auto &self, const content::XmlNode &node) -> void {
                if (node.attribute("ID") == id) ++count;
                for (const auto &child : node.children) self(self, child);
            };
            visit(visit, *instance);
            if (count != 1) {
                result.blocker = std::string("TRInstance control is missing or duplicated: ") + id;
                return result;
            }
        }
        result.nominal_row_width = row_size->first;
        result.nominal_row_height = row_size->second;
        result.nominal_scroll_width = scroll_size->first;
        result.nominal_scroll_height = scroll_size->second;
        result.verified = true;
    } catch (const std::exception &) {
        result.blocker = "Selected TRInstance source cannot be parsed";
    }
    return result;
}

TradeRouteInstancePlan plan_trade_route_instances(
    const LiveUiFrame &frame, const UiScreenController &screens,
    const TradeRouteInstanceSource &source,
    const TradeRouteMeasuredInstances &measured,
    std::string_view active_rules_sha256,
    const TradeRouteAvailableCallback &available_callback,
    const TradeRouteIncomingCallback &incoming_callback) {
    TradeRouteInstancePlan plan;
    plan.tab = measured.tab;
    const auto &owner = measured.owner;
    if (!current_owner(frame, screens, source, owner) ||
        !sha256(active_rules_sha256) ||
        !in_viewport(measured.main_scroll, owner)) {
        plan.blocker = "Current owner, selected rules or measured MainScroll is unavailable";
        return plan;
    }
    std::vector<TradeRouteAvailableRow> complete_rows;
    std::vector<OverviewRow> summary_rows;
    if (measured.tab == TradeRouteTab::your_routes) {
        const auto *overview = frame.snapshot.find<OverviewFrameState>();
        if (!overview || overview->player_id != owner.player_id ||
            !overview->pages.contains("TradeRouteOverview")) {
            plan.blocker = "Owned outgoing route summary is absent";
            return plan;
        }
        const auto &page = overview->pages.at("TradeRouteOverview");
        for (const auto &row : page.rows)
            if (row.label == "Trade route") summary_rows.push_back(row);
        plan.partial_reason = page.unavailable_reason.empty()
            ? "Native GetTradeRoutes/DisplayData row fields are unavailable"
            : page.unavailable_reason;
    } else if (measured.tab == TradeRouteTab::available_routes) {
        const auto quoted = quote_trade_routes_available(frame, screens,
            source.tabs, owner, active_rules_sha256, available_callback);
        if (!quoted.available) { plan.blocker = quoted.blocker; return plan; }
        complete_rows = quoted.rows;
    } else {
        const auto quoted = quote_trade_routes_to_you(frame, screens,
            source.tabs, owner, active_rules_sha256, incoming_callback);
        if (!quoted.available) { plan.blocker = quoted.blocker; return plan; }
        complete_rows = quoted.rows;
    }
    const auto count = measured.tab == TradeRouteTab::your_routes
        ? summary_rows.size() : complete_rows.size();
    if (measured.row_rects.size() != count) {
        plan.blocker = "Measured TRInstance row count differs from current owner data";
        return plan;
    }
    for (std::size_t i = 0; i < count; ++i) {
        const auto rect = measured.row_rects[i];
        if (rect.width <= 0 || rect.height <= 0 ||
            (i && std::int64_t(rect.y) <
                std::int64_t(measured.row_rects[i - 1].y) +
                    measured.row_rects[i - 1].height)) {
            plan.blocker = "Measured TRInstance rows overlap or lack geometry";
            plan.rows.clear();
            return plan;
        }
        TradeRouteInstanceView view;
        view.row_rect = rect;
        view.visible_rect = intersection(rect, measured.main_scroll);
        if (measured.tab == TradeRouteTab::your_routes)
            view.outgoing_summary = summary_rows[i];
        else view.complete_row = complete_rows[i];
        plan.rows.push_back(std::move(view));
    }
    plan.main_scroll = measured.main_scroll;
    plan.available = true;
    return plan;
}

} // namespace civ5::ui
