#include "civ5/ui/trade_route_available_rows.hpp"
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
        const auto source = inspect_trade_route_tab_source(resources);
        require(source.verified && source.lua_path.find("Expansion2") !=
            std::string::npos, "selected BNW TRWithYou callback is proven");

        PopupEvent event;
        event.context_id = "TradeRouteOverview";
        event.event_id = 41;
        PopupFrame popup{7, {event}, 12, 5};
        auto snapshot = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(popup));
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
        TradeRouteTabOwner owner{7, 41, 12, 20, 5, &host, 3, 1200, 800,
            3, source.lua_sha256, source.xml_sha256};
        const std::string rules(64, 'a');
        TradeRouteAvailableRow row;
        row.from_player = 8;
        row.to_player = 7;
        row.from_city = 201;
        row.to_city = 101;
        row.from_civ = "Civ B";
        row.to_civ = "Civ A";
        row.from_city_name = "Harbor";
        row.to_city_name = "Capital";
        row.tooltip = "Owner-provided inbound route tooltip";
        row.domain = TradeRouteAvailableRow::Domain::land;
        row.from_gpt = 100;
        row.to_gpt = 300;
        row.to_food = 50;
        row.to_production = 10;
        row.from_science = 20;
        row.to_science = 0;
        row.from_religion = -1;
        row.to_religion = -1;
        row.from_pressure = 0;
        row.to_pressure = 0;
        row.turns_left = 9;
        TradeRouteIncomingQuote native{owner, rules, {row}};
        int calls = 0;
        TradeRouteIncomingCallback callback = [&](const auto &requested,
            std::string_view requested_rules) -> std::optional<TradeRouteIncomingQuote> {
            ++calls;
            require(requested.player_id == 7 && requested_rules == rules,
                "owner receives current player and active rules identity");
            return native;
        };
        require(!quote_trade_routes_to_you(frame, screens, source, owner,
                    rules, {}).available && calls == 0,
            "missing native callback keeps incoming tab unavailable");
        const auto ready = quote_trade_routes_to_you(frame, screens, source,
            owner, rules, callback);
        require(ready.available && ready.rows.size() == 1 &&
            ready.rows.front().to_player == 7 && calls == 1,
            "complete native inbound row is exposed read-only");
        auto stale = frame;
        stale.sequence = 4;
        require(!quote_trade_routes_to_you(stale, screens, source, owner,
                    rules, callback).available && calls == 1,
            "stale frame cannot invoke native query");
        auto foreign = frame;
        foreign.owner = &other_host;
        require(!quote_trade_routes_to_you(foreign, screens, source, owner,
                    rules, callback).available && calls == 1,
            "foreign host cannot invoke native query");
        native.rules_sha256 = std::string(64, 'b');
        require(!quote_trade_routes_to_you(frame, screens, source, owner,
                    rules, callback).available,
            "foreign rules result is rejected");
        native.rules_sha256 = rules;
        native.rows.front().to_player = 8;
        native.rows.front().from_player = 7;
        require(!quote_trade_routes_to_you(frame, screens, source, owner,
                    rules, callback).available,
            "outgoing route cannot masquerade as incoming");
        native.rows.front() = row;
        native.rows.front().from_city_name.clear();
        require(!quote_trade_routes_to_you(frame, screens, source, owner,
                    rules, callback).available,
            "partial DisplayData row is rejected");
        native.rows.front() = row;
        native.rows.front().to_science.reset();
        require(!quote_trade_routes_to_you(frame, screens, source, owner,
                    rules, callback).available,
            "missing numeric yield is distinct from a genuine zero");
        native.rows.clear();
        require(quote_trade_routes_to_you(frame, screens, source, owner,
                    rules, callback).available,
            "authoritative empty result differs from missing callback");
        auto closed = frame;
        closed.snapshot = UiFrameSnapshot(21, 7, 12);
        require(!quote_trade_routes_to_you(closed, screens, source, owner,
                    rules, callback).available,
            "closed popup cannot publish incoming rows");
        std::cout << "selected BNW incoming route quote: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
