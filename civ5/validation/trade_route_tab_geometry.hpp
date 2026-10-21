#pragma once

#include "civ5/ui/trade_route_overview_input.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>

namespace civ5::validation {

// The active Expansion2 TradeRouteOverview.xml fixes the tab strip inside a
// centered 990x653 panel. Return no targets if the selected source, hierarchy
// or physical viewport differs; a stale rectangle may never route a click.
inline std::optional<civ5::ui::TradeRouteTabRects> measure_trade_route_tabs(
    const civ5::assets::ResourceMountTable &resources,
    const civ5::ui::TradeRouteTabSourceProof &proof,
    const civ5::ui::TradeRouteTabOwner &owner) {
    using civ5::content::XmlNode;
    using civ5::ui::OverviewRect;
    using civ5::ui::TradeRouteTabRects;
    if (!proof.verified || proof.xml_sha256.empty() ||
        proof.xml_path.find("Expansion2") == std::string::npos ||
        owner.xml_sha256 != proof.xml_sha256 ||
        owner.lua_sha256 != proof.lua_sha256 ||
        owner.viewport_width < 990 || owner.viewport_height < 665)
        return std::nullopt;
    const auto selected = resources.resolve(civ5::assets::AssetKey(
        "Assets/UI/InGame/Popups/TradeRouteOverview.xml"));
    const auto *asset = std::get_if<civ5::assets::AssetHandle>(&selected.asset);
    if (!asset) return std::nullopt;
    try {
        const auto bytes = asset->read_bytes(8ULL * 1024 * 1024);
        if (civ5::runtime::identify_bytes(bytes).sha256 != proof.xml_sha256)
            return std::nullopt;
        const auto xml = civ5::content::read_xml_metadata(
            std::string(bytes.begin(), bytes.end()));
        if (xml.name != "Context") return std::nullopt;
        const XmlNode *panel = nullptr;
        for (const auto &child : xml.children) {
            if (child.name != "Grid" || child.attribute("Size") != "990,653" ||
                child.attribute("Anchor") != "C,C" ||
                child.attribute("Offset") != "0,6" ||
                child.attribute("Style") != "Grid9DetailFive140") continue;
            if (panel) return std::nullopt;
            panel = &child;
        }
        if (!panel) return std::nullopt;
        const XmlNode *box = nullptr;
        for (const auto &child : panel->children) {
            if (child.name != "Box" || child.attribute("Anchor") != "C,T" ||
                child.attribute("Size") != "955,32" ||
                child.attribute("Offset") != "0,44") continue;
            if (box) return std::nullopt;
            box = &child;
        }
        if (!box) return std::nullopt;
        const XmlNode *stack = nullptr;
        for (const auto &child : box->children) {
            if (child.name != "Stack" || child.attribute("Anchor") != "C,T" ||
                child.attribute("Size") != "971,32" ||
                child.attribute("Offset") != "0,0" ||
                child.attribute("Padding") != "0" ||
                child.attribute("StackGrowth") != "Right") continue;
            if (stack) return std::nullopt;
            stack = &child;
        }
        if (!stack) return std::nullopt;
        const std::array<std::string_view, 3> expected{
            "TabButtonYourTR", "TabButtonAvailableTR", "TabButtonTRWithYou"};
        std::size_t next = 0;
        for (const auto &child : stack->children) {
            if (child.name != "Button") continue;
            if (next >= expected.size() || child.attribute("ID") != expected[next] ||
                child.attribute("Anchor") != "L,T" ||
                child.attribute("Size") != "300,24" ||
                child.attribute("Offset") != "0,0") return std::nullopt;
            ++next;
        }
        if (next != expected.size()) return std::nullopt;
        const std::int32_t panel_x = (owner.viewport_width - 990) / 2;
        const std::int32_t panel_y = (owner.viewport_height - 653) / 2 + 6;
        const std::int32_t box_x = panel_x + (990 - 955) / 2;
        const std::int32_t stack_x = box_x + (955 - 971) / 2;
        const std::int32_t tab_y = panel_y + 44;
        const auto inside = [&](OverviewRect r) {
            return r.x >= 0 && r.y >= 0 &&
                std::int64_t(r.x) + r.width <= owner.viewport_width &&
                std::int64_t(r.y) + r.height <= owner.viewport_height;
        };
        TradeRouteTabRects result;
        result.owner = owner;
        result.your_routes = {stack_x, tab_y, 300, 24};
        result.available_routes = {stack_x + 300, tab_y, 300, 24};
        result.routes_to_you = {stack_x + 600, tab_y, 300, 24};
        return inside(result.your_routes) && inside(result.available_routes) &&
                       inside(result.routes_to_you)
            ? std::optional<TradeRouteTabRects>(std::move(result)) : std::nullopt;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

} // namespace civ5::validation
