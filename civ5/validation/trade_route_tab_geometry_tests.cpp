#include "trade_route_tab_geometry.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
}

int main(int argc, char **argv) {
    try {
        require(argc == 2, "provide selected BNW asset root");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        auto expansion = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        base->scan({"Assets/UI"});
        expansion->scan({"Assets/DLC/Expansion/UI"});
        bnw->scan({"Assets/DLC/Expansion2/UI"});
        civ5::assets::ResourceMountTable resources(civ5::assets::resource_lookup(base));
        resources.mount({1, 0, 0, civ5::assets::resource_lookup(expansion)});
        resources.mount({2, 0, 0, civ5::assets::resource_lookup(bnw)});
        const auto proof = civ5::ui::inspect_trade_route_tab_source(resources);
        require(proof.verified && proof.xml_path.find("Expansion2") !=
                    std::string::npos, "selected BNW trade-route tab source");
        civ5::ui::TradeRouteTabOwner owner;
        owner.viewport_width = 1280;
        owner.viewport_height = 960;
        owner.xml_sha256 = proof.xml_sha256;
        owner.lua_sha256 = proof.lua_sha256;
        const auto small = civ5::validation::measure_trade_route_tabs(
            resources, proof, owner);
        require(small && small->your_routes.x == 154 &&
                small->your_routes.y == 203 &&
                small->available_routes.x == 454 &&
                small->routes_to_you.x == 754 &&
                small->routes_to_you.width == 300 &&
                small->routes_to_you.height == 24,
                "1280x960 tab bounds follow selected XML hierarchy");
        owner.viewport_width = 1920;
        owner.viewport_height = 1080;
        const auto full_hd = civ5::validation::measure_trade_route_tabs(
            resources, proof, owner);
        require(full_hd && full_hd->your_routes.x == 474 &&
                full_hd->your_routes.y == 263 &&
                full_hd->available_routes.x == 774 &&
                full_hd->routes_to_you.x == 1074,
                "1920x1080 tab bounds follow selected XML hierarchy");
        auto stale = owner;
        stale.xml_sha256 = "stale";
        require(!civ5::validation::measure_trade_route_tabs(
                    resources, proof, stale),
                "stale source identity cannot measure controls");
        stale = owner;
        stale.viewport_width = 640;
        require(!civ5::validation::measure_trade_route_tabs(
                    resources, proof, stale),
                "undersized viewport cannot expose truncated controls");

        using namespace civ5::ui;
        const auto in_game = civ5::content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/TradeRouteOverview" ID="TradeRouteOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, civ5::content::read_xml_metadata("<Context/>")));
        PopupEvent event;
        event.context_id = "TradeRouteOverview";
        event.event_id = 31;
        PopupFrame popup{7, {event}, 12, 5};
        OverviewFrameState overview;
        overview.player_id = 7;
        overview.pages["TradeRouteOverview"].rows.push_back({"Route", "Owned"});
        int host = 0;
        auto snapshot = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(popup))
            .with(std::make_shared<const OverviewFrameState>(overview));
        LiveUiFrame frame{snapshot, 3, &host,
            {}, {}, {}, {}, {}, {}, {}, {}, false};
        require(screens.dispatch(UiOpenScreen{"TradeRouteOverview"}, snapshot)
                    .status == UiCommandStatus::navigation,
                "focused trade-route popup fixture");
        TradeRouteOverviewInput input(proof);
        require(input.click(frame, screens, 160, 210, 160, 210).consumed &&
                input.click(frame, screens, 160, 210, 160, 210).status ==
                    TradeRouteTabStatus::unavailable,
                "unbound focused popup consumes click without action");
        owner.player_id = 7;
        owner.turn = 12;
        owner.owner_sequence = 5;
        owner.event_id = 31;
        owner.frame = 20;
        owner.frame_owner = &host;
        owner.frame_sequence = 3;
        owner.viewport_width = 1280;
        owner.viewport_height = 960;
        owner.popup_data2 = std::nullopt;
        const auto rects = civ5::validation::measure_trade_route_tabs(
            resources, proof, owner);
        require(rects.has_value(), "current owner measured tab rectangles");
        std::string blocker;
        require(input.bind(frame, screens, *rects, blocker),
                "current popup and route page bind tab input");
        require(input.view(frame, screens).available &&
                !input.view(frame, screens).selected,
                "missing native Data2 leaves initial tab unresolved");
        const auto your = input.click(frame, screens, 160, 210, 160, 210);
        require(your.consumed && your.status ==
                    TradeRouteTabStatus::performed_read_only,
                "current owned-route tab can refresh read-only rows");
        const auto native_query = input.click(frame, screens, 460, 210, 460, 210);
        require(native_query.consumed && native_query.status ==
                    TradeRouteTabStatus::unavailable,
                "unavailable native route query consumes its tab click");
        auto stale_frame = frame;
        stale_frame.sequence = 4;
        const auto stale_click = input.click(stale_frame, screens, 160, 210, 160, 210);
        require(stale_click.consumed && stale_click.status ==
                    TradeRouteTabStatus::unavailable,
                "stale frame consumes click before camera or action");
        LiveUiFrame incomplete{UiFrameSnapshot(20, 7, 12).with(
            std::make_shared<const OverviewFrameState>(overview)), 3, &host,
            {}, {}, {}, {}, {}, {}, {}, {}, false};
        const auto incomplete_click = input.click(
            incomplete, screens, 160, 210, 160, 210);
        require(incomplete_click.consumed && incomplete_click.status ==
                    TradeRouteTabStatus::unavailable,
                "missing popup event consumes click before camera or action");
        std::cout << "trade route tab geometry tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
