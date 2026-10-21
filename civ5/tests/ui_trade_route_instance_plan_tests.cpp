#include "civ5/ui/trade_route_instance_plan.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
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
        const auto source = inspect_trade_route_instance_source(resources);
        require(source.verified && source.nominal_row_width == 1000 &&
            source.nominal_row_height == 34 &&
            source.nominal_scroll_width == 956 &&
            source.nominal_scroll_height == 400,
            "selected Expansion2 TRInstance/MainScroll source is verified");

        PopupEvent event;
        event.context_id = "TradeRouteOverview";
        event.event_id = 31;
        PopupFrame popup{7, {event}, 12, 5};
        OverviewFrameState overview;
        overview.player_id = 7;
        overview.pages["TradeRouteOverview"].rows.push_back(
            {"Trade route", "Capital → Harbor", ""});
        overview.pages["TradeRouteOverview"].rows.push_back(
            {"Unassigned trade unit", "#51", ""});
        overview.pages["TradeRouteOverview"].unavailable_reason =
            "Native outgoing yields are unavailable";
        auto snapshot = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(popup))
            .with(std::make_shared<const OverviewFrameState>(overview));
        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/TradeRouteOverview" ID="TradeRouteOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        require(screens.dispatch(UiOpenScreen{"TradeRouteOverview"}, snapshot)
                    .status == UiCommandStatus::navigation,
            "owner popup has modal focus");
        int host = 0, other_host = 0;
        LiveUiFrame frame{snapshot, 3, &host,
            {}, {}, {}, {}, {}, {}, {}, {}, false};
        TradeRouteTabOwner owner{7, 31, 12, 20, 5, &host, 3, 1200, 800,
            1, source.tabs.lua_sha256, source.tabs.xml_sha256};
        TradeRouteMeasuredInstances measured{owner, TradeRouteTab::your_routes,
            {100, 100, 956, 400}, {{100, 110, 1000, 34}}};
        const std::string rules(64, 'a');
        auto outgoing = plan_trade_route_instances(frame, screens, source,
            measured, rules);
        require(outgoing.available && outgoing.rows.size() == 1 &&
            outgoing.rows[0].outgoing_summary &&
            !outgoing.rows[0].complete_row &&
            !outgoing.partial_reason.empty() &&
            outgoing.rows[0].visible_rect.width == 956,
            "YourTR summary is clipped and explicitly partial");
        auto stale = frame;
        stale.sequence = 4;
        require(!plan_trade_route_instances(stale, screens, source,
                    measured, rules).available,
            "stale frame cannot reuse measured rows");
        auto foreign = frame;
        foreign.owner = &other_host;
        require(!plan_trade_route_instances(foreign, screens, source,
                    measured, rules).available,
            "foreign host cannot reuse measured rows");

        TradeRouteAvailableRow row;
        row.from_player = 7;
        row.to_player = 8;
        row.from_city = 101;
        row.to_city = 201;
        row.from_civ = "Civ A";
        row.to_civ = "Civ B";
        row.from_city_name = "Capital";
        row.to_city_name = "Harbor";
        row.tooltip = "Owner route tooltip";
        row.domain = TradeRouteAvailableRow::Domain::sea;
        row.from_gpt = 300;
        row.to_gpt = 100;
        row.to_food = 0;
        row.to_production = 0;
        row.from_religion = -1;
        row.to_religion = -1;
        row.from_pressure = 0;
        row.to_pressure = 0;
        row.from_science = 0;
        row.to_science = 0;
        TradeRouteAvailableQuote native{owner, rules, {row}};
        TradeRouteAvailableCallback available = [&](const auto &,
            std::string_view) -> std::optional<TradeRouteAvailableQuote> {
            return native;
        };
        measured.tab = TradeRouteTab::available_routes;
        require(!plan_trade_route_instances(frame, screens, source,
                    measured, rules).available,
            "AvailableTR without native callback has no display plan");
        const auto available_plan = plan_trade_route_instances(frame, screens,
            source, measured, rules, available);
        require(available_plan.available &&
            available_plan.rows.size() == 1 &&
            available_plan.rows.front().complete_row &&
            !available_plan.rows.front().outgoing_summary,
            "AvailableTR accepts complete native quote only");
        measured.row_rects = {{100, 110, 1000, 34}, {100, 130, 1000, 34}};
        require(!plan_trade_route_instances(frame, screens, source,
                    measured, rules, available).available,
            "row count mismatch cannot produce ambiguous layout");
        measured.row_rects = {{100, 110, 1000, 34}};
        measured.tab = TradeRouteTab::routes_to_you;
        auto incoming_row = row;
        incoming_row.from_player = 8;
        incoming_row.to_player = 7;
        TradeRouteIncomingQuote incoming_native{owner, rules, {incoming_row}};
        TradeRouteIncomingCallback incoming = [&](const auto &,
            std::string_view) -> std::optional<TradeRouteIncomingQuote> {
            return incoming_native;
        };
        require(!plan_trade_route_instances(frame, screens, source,
                    measured, rules).available,
            "TRWithYou without native callback has no display plan");
        require(plan_trade_route_instances(frame, screens, source,
                    measured, rules, {}, incoming).available,
            "TRWithYou accepts complete native incoming quote");
        std::cout << "selected BNW TRInstance measured plan: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
