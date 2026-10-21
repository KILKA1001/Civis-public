#include "civ5/ui/trade_route_overview_input.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

civ5::ui::LiveUiFrame frame(civ5::ui::UiFrameSnapshot snapshot,
    std::uint64_t sequence, const void *owner) {
    return {std::move(snapshot), sequence, owner,
            {}, {}, {}, {}, {}, {}, {}, {}, false};
}
} // namespace

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "installed BNW game root required");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        bnw->add_file(root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/TradeRouteOverview.lua");
        bnw->add_file(root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/TradeRouteOverview.xml");
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
            assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
            assets::resource_keys(bnw)});
        const auto proof = inspect_trade_route_tab_source(resources);
        require(proof.verified && !proof.lua_sha256.empty() &&
                !proof.xml_sha256.empty() &&
                proof.lua_path.find("Expansion2") != std::string::npos &&
                proof.xml_path.find("Expansion2") != std::string::npos,
            "selected Expansion2 TradeRouteOverview callbacks and tabs verified");
        assets::ResourceMountTable missing(assets::resource_lookup(base));
        require(!inspect_trade_route_tab_source(missing).verified,
            "missing selected source cannot enable tab input");

        runtime::ResearchContext game;
        game.players[7].alive = true;
        runtime::CityYieldContext cities;
        runtime::TreasuryContext treasury;
        saves::TradeRouteObservation route;
        route.origin_owner = 7;
        route.origin_x = 2;
        route.origin_y = 4;
        route.destination_x = 9;
        route.destination_y = 5;
        route.turn_route_complete = 40;
        route.unit_id = 51;
        treasury.trade_routes.push_back(route);
        auto overview = capture_overview_frame(game, cities, treasury, 7);
        require(overview.pages.at("TradeRouteOverview").rows.size() == 1 &&
                !overview.pages.at("TradeRouteOverview").unavailable_reason.empty(),
            "owned outgoing route exists but native yields remain unresolved");
        PopupEvent event;
        event.context_id = "TradeRouteOverview";
        event.event_id = 31;
        PopupFrame popup{7, {event}, 12, 5};
        auto snapshot = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(popup))
            .with(std::make_shared<const OverviewFrameState>(overview));
        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/TradeRouteOverview" ID="TradeRouteOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        int host = 0, foreign_host = 0;
        auto current = frame(snapshot, 3, &host);
        require(screens.dispatch(UiOpenScreen{"TradeRouteOverview"}, snapshot)
                    .status == UiCommandStatus::navigation,
            "owner-issued popup focus opens selected screen in fixture");
        TradeRouteOverviewInput input(proof);
        TradeRouteTabOwner owner{7, 31, 12, 20, 5, &host, 3, 1200, 800,
                                 1, proof.lua_sha256, proof.xml_sha256};
        TradeRouteTabRects rects{owner,
            {100, 100, 200, 24}, {350, 100, 200, 24}, {600, 100, 200, 24}};
        std::string blocker;
        auto bad_rects = rects;
        bad_rects.routes_to_you = bad_rects.your_routes;
        require(!input.bind(current, screens, bad_rects, blocker),
            "overlapping native tab hit targets are rejected");
        auto foreign = frame(snapshot, 3, &foreign_host);
        require(!input.bind(foreign, screens, rects, blocker),
            "another host frame cannot bind measurements");
        require(input.bind(current, screens, rects, blocker),
            "current owner event, route frame and measured tabs bind");
        const auto initial = input.view(current, screens);
        require(initial.available && initial.selected == TradeRouteTab::your_routes &&
                initial.current_owned_rows.size() == 1 &&
                !initial.partial_reason.empty(),
            "Data2=1 selects read-only YourTR rows with explicit missing fields");
        const auto outside = input.click(current, screens, 900, 500, 900, 500);
        require(outside.consumed && outside.status ==
                    TradeRouteTabStatus::unavailable,
            "modal outside click never reaches camera");
        const auto release_outside = input.click(current, screens,
                                                 110, 110, 900, 500);
        require(release_outside.consumed && release_outside.status ==
                    TradeRouteTabStatus::unavailable,
            "release outside selected tab cannot refresh content");
        const auto available = input.click(current, screens,
                                            360, 110, 360, 110);
        require(available.consumed && available.status ==
                    TradeRouteTabStatus::unavailable &&
                available.blocker.find("GetTradeRoutesAvailable") != std::string::npos &&
                input.view(current, screens).selected == TradeRouteTab::your_routes,
            "unowned available-route query remains explicitly unavailable");
        const auto to_you = input.click(current, screens, 610, 110, 610, 110);
        require(to_you.consumed && to_you.status ==
                    TradeRouteTabStatus::unavailable &&
                to_you.blocker.find("GetTradeRoutesToYou") != std::string::npos,
            "unowned incoming-route query remains explicitly unavailable");
        const auto your = input.click(current, screens, 110, 110, 110, 110);
        require(your.consumed && your.status ==
                    TradeRouteTabStatus::performed_read_only &&
                input.view(current, screens).current_owned_rows.size() == 1,
            "selected YourTR callback refreshes only owned outgoing route rows");
        auto stale = frame(snapshot, 4, &host);
        require(input.click(stale, screens, 110, 110, 110, 110).consumed &&
                input.click(stale, screens, 110, 110, 110, 110).status ==
                    TradeRouteTabStatus::unavailable,
            "new frame sequence invalidates old geometry while isolating camera");
        auto no_popup = frame(UiFrameSnapshot(21, 7, 12).with(
            std::make_shared<const OverviewFrameState>(overview)), 5, &host);
        require(!input.bind(no_popup, screens, rects, blocker),
            "route data without owner popup event grants no hits");
        std::cout << "selected BNW TradeRouteOverview tab input: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
