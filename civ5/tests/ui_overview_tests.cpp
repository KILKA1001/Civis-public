#include "civ5/ui/overview_screens.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/runtime/science.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "assets root required");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->scan({"Assets/UI"});
        expansion->scan({"Assets/DLC/Expansion/UI"});
        bnw->scan({"Assets/DLC/Expansion2/UI"});
        assets::ResourceMountTable mounts(assets::resource_lookup(base), {},
                                           assets::resource_keys(base));
        mounts.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {},
                      assets::resource_keys(expansion)});
        mounts.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                      assets::resource_keys(bnw)});
        const auto registry = UiContextRegistry::from_resources(mounts);
        const auto sources = read_overview_sources(mounts, registry);
        require(sources.size() == 19, "all 19 active overview contexts selected");
        for (const auto &entry : sources)
            require(!entry.controls.empty() && !entry.selected_source_path.empty(),
                "overview source has authored controls and provenance");
        const auto trade_source = std::find_if(sources.begin(), sources.end(),
            [](const auto &s) { return s.context_id == "TradeRouteOverview"; });
        require(trade_source != sources.end() &&
                trade_source->selected_source_path.find("Expansion2") != std::string::npos,
            "BNW trade overview comes from Expansion2");
        runtime::ResearchContext game;
        game.players[7].alive = true;
        game.players[7].policies_resolved = true;
        game.players[7].policies.insert(3);
        runtime::CityYieldContext cities;
        cities.players[7].total_population = 4;
        cities.players[7].cities[12].id = 12;
        cities.players[7].excess_happiness = 3;
        cities.leagues.push_back({{7, 8}, {}, {}});
        cities.players[7].cities[12].owner = 7;
        cities.players[7].cities[12].population = 4;
        cities.players[7].cities[12].religions.push_back({3, 3, 12});
        runtime::TreasuryContext treasury;
        treasury.players[7].balance_times100 = 1750;
        treasury.players[7].connections_times100 = 235;
        treasury.religions.push_back({.religion = 3, .founder = 7});
        saves::TradeRouteObservation route;
        route.origin_owner = 7;
        route.origin_x = 2;
        route.origin_y = 4;
        route.destination_x = 9;
        route.destination_y = 5;
        route.turn_route_complete = 40;
        route.unit_id = 51;
        treasury.trade_routes.push_back(route);
        auto empty_slot = route;
        empty_slot.origin_x = empty_slot.origin_y = -1;
        empty_slot.destination_x = empty_slot.destination_y = -1;
        treasury.trade_routes.push_back(empty_slot);
        auto incoming = route;
        incoming.origin_owner = 8;
        incoming.destination_owner = 7;
        incoming.unit_id = 99;
        treasury.trade_routes.push_back(incoming);
        auto state = capture_overview_frame(game, cities, treasury, 7);
        require(state.pages.at("Demographics").rows.size() == 1 &&
                state.pages.at("Demographics").rows.front().value == "69%" &&
                state.pages.at("EndGameDemographics").rows.front().value == "69%" &&
                !state.pages.at("Demographics").unavailable_reason.empty(),
            "owned excess happiness supplies native approval without inventing rivals");
        auto missing_happiness = cities;
        missing_happiness.players[7].excess_happiness.reset();
        require(capture_overview_frame(game, missing_happiness, treasury, 7)
                    .pages.at("Demographics").rows.empty(),
            "unresolved happiness cannot become zero approval");
        auto extreme_happiness = cities;
        extreme_happiness.players[7].excess_happiness = -200;
        require(capture_overview_frame(game, extreme_happiness, treasury, 7)
                    .pages.at("Demographics").rows.front().value == "0%",
            "native approval clamps negative happiness");
        extreme_happiness.players[7].excess_happiness = 200;
        require(capture_overview_frame(game, extreme_happiness, treasury, 7)
                    .pages.at("Demographics").rows.front().value == "100%",
            "native approval clamps positive happiness");
        require(state.pages.at("EconomicOverview").rows.size() == 3,
            "economy uses current owned city and treasury state");
        require(state.pages.at("EconomicOverview").rows[2].value == "17.50",
            "treasury presents source hundredths without dropping precision");
        require(state.pages.at("EconomicGeneralInfo").rows.size() == 2 &&
                state.pages.at("EconomicGeneralInfo").rows.front().label == "TotalGoldValue" &&
                state.pages.at("EconomicGeneralInfo").rows.front().value == "17.50" &&
                state.pages.at("EconomicGeneralInfo").rows[1].label == "TradeIncomeValue" &&
                state.pages.at("EconomicGeneralInfo").rows[1].value == "2.35" &&
                state.pages.at("EconomicGeneralInfo").cities.size() == 1 &&
                state.pages.at("EconomicGeneralInfo").cities.front().population == 4,
            "economic general info maps owned city and live treasury to authored fields");
        runtime::TreasuryNetQuote complete;
        complete.net_times100 = -125;
        complete.income.total_times100 = 375;
        complete.income.cities = 250;
        complete.income.diplomacy = -125;
        complete.income.religion = -20;
        complete.income.connections = 235;
        complete.expenses = runtime::MaintenanceQuote{};
        complete.expenses->total = 5;
        auto partial = complete;
        partial.expenses.reset();
        require(capture_overview_frame(game, cities, treasury, 7,
                    nullptr, nullptr, {}, nullptr, {}, &partial)
                    .pages.at("EconomicGeneralInfo").rows.size() == 2,
            "partial treasury quote cannot publish economic flow values");
        const auto quoted = capture_overview_frame(game, cities, treasury, 7,
            nullptr, nullptr, {}, nullptr, {}, &complete);
        const auto &economic = quoted.pages.at("EconomicGeneralInfo");
        const auto economic_value = [&](std::string_view id) {
            const auto field = std::find_if(economic.rows.begin(), economic.rows.end(),
                [&](const OverviewRow &row) { return row.label == id; });
            return field == economic.rows.end() ? std::string{} : field->value;
        };
        require(economic.rows.size() == 12 && economic.science_lost_visible == true &&
                economic_value("NetGoldValue") == "-1.25" &&
                economic_value("TotalExpenseValue") == "5" &&
                economic_value("DiploIncomeValue") == "0.00" &&
                economic_value("ReligionIncomeValue") == "0.00" &&
                economic_value("DiploExpenseValue") == "1.25",
            "completed quote supplies BNW income and expense fields with signed diplomacy split");
        runtime::ScienceQuote funded_science;
        funded_science.gold_rate_times100 = complete.net_times100;
        funded_science.budget_deficit = 0;
        const auto funded_economy = capture_overview_frame(game, cities, treasury, 7,
            nullptr, nullptr, {}, nullptr, {}, &complete, &funded_science)
            .pages.at("EconomicGeneralInfo");
        require(funded_economy.rows.back().label == "ScienceLostValue" &&
                    funded_economy.rows.back().value == "0.00",
            "negative GPT with sufficient balance has no science loss");
        auto deep_deficit = complete;
        deep_deficit.net_times100 = -2000;
        runtime::ScienceQuote lost_science;
        lost_science.gold_rate_times100 = deep_deficit.net_times100;
        lost_science.budget_deficit = -250;
        const auto lost_state = capture_overview_frame(game, cities, treasury, 7,
            nullptr, nullptr, {}, nullptr, {}, &deep_deficit, &lost_science);
        const auto &lost_economy = lost_state.pages.at("EconomicGeneralInfo");
        require(lost_economy.rows.back().label == "ScienceLostValue" &&
                    lost_economy.rows.back().value == "-2.50",
            "matching owned science and gold quotes supply exact budget deficit");
        auto stale_science = lost_science;
        stale_science.gold_rate_times100 = -1999;
        require(capture_overview_frame(game, cities, treasury, 7,
                    nullptr, nullptr, {}, nullptr, {}, &deep_deficit, &stale_science)
                    .pages.at("EconomicGeneralInfo").rows.size() == 12,
            "science quote with a different gold rate cannot populate this frame");
        stale_science = lost_science;
        stale_science.budget_deficit = -249;
        require(capture_overview_frame(game, cities, treasury, 7,
                    nullptr, nullptr, {}, nullptr, {}, &deep_deficit, &stale_science)
                    .pages.at("EconomicGeneralInfo").rows.size() == 12,
            "inconsistent science deficit cannot masquerade as owned value");
        auto no_treasury_owner = treasury;
        no_treasury_owner.players.erase(7);
        require(capture_overview_frame(game, cities, no_treasury_owner, 7,
                    nullptr, nullptr, {}, nullptr, {}, &deep_deficit, &lost_science)
                    .pages.at("EconomicGeneralInfo").rows.empty(),
            "missing active-player treasury owner cannot borrow deficit quotes");
        auto positive = complete;
        positive.net_times100 = 125;
        positive.income.diplomacy = 225;
        positive.income.religion = 125;
        const auto positive_economy = capture_overview_frame(game, cities, treasury, 7,
            nullptr, nullptr, {}, nullptr, {}, &positive)
            .pages.at("EconomicGeneralInfo");
        const auto positive_value = [&](std::string_view id) {
            const auto field = std::find_if(positive_economy.rows.begin(),
                positive_economy.rows.end(),
                [&](const OverviewRow &row) { return row.label == id; });
            return field == positive_economy.rows.end() ? std::string{} : field->value;
        };
        require(positive_value("DiploIncomeValue") == "2.25" &&
                positive_value("DiploExpenseValue") == "0.00" &&
                positive_value("ReligionIncomeValue") == "1.25",
            "positive owned diplomacy and religion remain income, not expense");
        require(capture_overview_frame(game, cities, treasury, 7,
                    nullptr, nullptr, {}, nullptr, {}, &positive)
                    .pages.at("EconomicGeneralInfo").science_lost_visible == false,
            "native deficit row is hidden at nonnegative net gold");
        require(!state.pages.at("MilitaryOverview").unavailable_reason.empty(),
            "missing army backend remains explicit");
        runtime::MaintenanceContext army;
        army.players[7].units[41] = runtime::MaintenanceUnit{.type = 12, .military = true, .civilian = false};
        army.players[7].units[42] = runtime::MaintenanceUnit{.type = 13, .military = false, .civilian = true};
        army.players[8].units[99] = runtime::MaintenanceUnit{.type = 14, .military = true, .civilian = false};
        // These saved BNW expense inputs are tempting numeric substitutes for
        // the native GetNumUnitsSupplied() family, but own different units.
        army.players[7].base_free = 44;
        army.players[7].outside_units = 66;
        army.players[7].supply_modifier = 77;
        army.players[7].cached_supply_expense = 99;
        const auto owned = capture_overview_frame(game, cities, treasury, 7, &army);
        content::RulesDatabase unit_database;
        unit_database.apply_sql("CREATE TABLE Units(ID INTEGER, Description TEXT);"
                                "INSERT INTO Units VALUES(12,'TXT_KEY_UNIT_WARRIOR');");
        const auto named_owned = capture_overview_frame(game, cities, treasury, 7, &army,
            &unit_database, [](std::string_view key) {
                return key == "TXT_KEY_UNIT_WARRIOR" ? std::string("Warrior")
                                                     : std::string(key);
            });
        require(named_owned.pages.at("MilitaryOverview").rows.front().value ==
                    "#41 · Warrior" &&
                    named_owned.pages.at("MilitaryOverview").rows[1].value ==
                    "#42 · type #13",
                "unit description resolves only through an exact active rules/text row");
        const auto &roster = owned.pages.at("MilitaryOverview");
        require(roster.rows.size() == 3 && roster.rows[0].value == "#41 · type #12" &&
                    roster.rows[0].label_key == "TXT_KEY_MILITARY_UNIT" &&
                    roster.rows[1].label_key == "TXT_KEY_CIVILIAN_UNIT" &&
                    roster.rows[2].label_key == "TXT_KEY_SUPPLY_USE" &&
                    roster.rows[2].value == "2" &&
                    roster.unavailable_reason.find("Supply capacity, components,") !=
                        std::string::npos &&
                    std::none_of(roster.rows.begin(), roster.rows.end(),
                        [](const OverviewRow &row) {
                            return row.label_key == "TXT_KEY_SUPPLY_CAP" ||
                                   row.label_key == "TXT_KEY_SUPPLY_REMAINING" ||
                                   row.label_key == "TXT_KEY_SUPPLY_DEFICIT";
                        }),
            "owned unit count excludes other players; expense inputs cannot supply capacity");
        auto no_owned_units = army;
        no_owned_units.players[7].units.clear();
        const auto empty_army = capture_overview_frame(game, cities, treasury, 7,
            &no_owned_units).pages.at("MilitaryOverview");
        require(empty_army.rows.size() == 1 &&
                    empty_army.rows.front().label_key == "TXT_KEY_SUPPLY_USE" &&
                    empty_army.rows.front().value == "0",
            "an authoritative empty owned roster shows zero supply use");
        require(state.pages.at("TradeRouteOverview").rows.size() == 1 &&
                state.pages.at("TradeRouteOverview").rows.front().value == "2,4 → 9,5" &&
                state.pages.at("ReligionOverview").rows.size() == 2 &&
                state.pages.at("ReligionOverview").rows[1].value == "3" &&
                !state.pages.at("ReligionOverview").unavailable_reason.empty() &&
                state.pages.at("LeagueOverviewPopup").rows.size() == 1,
            "outgoing trade, owned religion followers and league membership are scoped");
        auto later_cities = cities;
        later_cities.players[7].cities[12].religions.front().followers = 2;
        later_cities.players[8].cities[3] = runtime::CityYieldState{
            .owner = 8, .id = 3, .population = 9,
            .religions = {{3, 9, 30}}};
        const auto later_religion = capture_overview_frame(game, later_cities, treasury, 7);
        require(state.pages.at("ReligionOverview").rows[1].value == "3" &&
                later_religion.pages.at("ReligionOverview").rows[1].value == "2",
            "successive frames copy only current owned city followers");
        later_cities.players[7].cities[12].religions.push_back({3, 1, 1});
        const auto duplicate_religion = capture_overview_frame(game, later_cities, treasury, 7);
        require(duplicate_religion.pages.at("ReligionOverview").rows.size() == 1,
            "duplicate city religion rows cannot produce a fabricated subtotal");
        auto invalid_followers = cities;
        invalid_followers.players[7].cities[12].religions.front().followers = 5;
        require(capture_overview_frame(game, invalid_followers, treasury, 7)
                    .pages.at("ReligionOverview").rows.size() == 1,
            "a follower count above city population cannot become a displayed subtotal");
        auto missing_owned_cities = cities;
        missing_owned_cities.players.erase(7);
        require(capture_overview_frame(game, missing_owned_cities, treasury, 7)
                    .pages.at("ReligionOverview").rows.size() == 1,
            "missing active-player city state cannot be displayed as zero followers");
        runtime::MaintenanceContext trade_units;
        trade_units.players[7].units[51].type = 12;
        trade_units.players[7].units[52].type = 13;
        trade_units.players[7].units[53].type = 14;
        trade_units.players[8].units[99].type = 13;
        content::RulesDatabase trade_database;
        trade_database.apply_sql("CREATE TABLE Units(ID INTEGER, Trade INTEGER, Description TEXT);"
            "INSERT INTO Units VALUES(12,1,'TXT_KEY_UNIT_CARAVAN');"
            "INSERT INTO Units VALUES(13,1,'TXT_KEY_UNIT_CARGO_SHIP');"
            "INSERT INTO Units VALUES(14,0,'TXT_KEY_UNIT_WARRIOR');");
        const auto trade_state = capture_overview_frame(game, cities, treasury, 7,
            &trade_units, &trade_database);
        require(trade_state.pages.at("TradeRouteOverview").rows.size() == 2 &&
                trade_state.pages.at("TradeRouteOverview").rows.back().value == "#52" &&
                trade_state.pages.at("TradeRouteOverview").rows.back().label_key ==
                    "TXT_KEY_UNIT_CARGO_SHIP",
            "owned free trade unit is distinguished from route unit, other player and warrior");
        auto named_cities = cities;
        auto &origin_city = named_cities.players[7].cities.at(12);
        origin_city.owner = 7;
        origin_city.x = 2;
        origin_city.y = 4;
        origin_city.source_core = std::pair<std::uint64_t, std::uint64_t>{100, 200};
        auto &destination_city = named_cities.players[8].cities[3];
        destination_city.owner = 8;
        destination_city.id = 3;
        destination_city.x = 9;
        destination_city.y = 5;
        destination_city.source_core = std::pair<std::uint64_t, std::uint64_t>{300, 400};
        auto named_treasury = treasury;
        named_treasury.trade_routes.front().destination_owner = 8;
        saves::SavedSessionSnapshot named_save;
        named_save.all_players.emplace();
        const auto add_saved_city = [&](std::int64_t owner, std::int64_t id,
                                        std::int64_t x, std::int64_t y,
                                        std::uint64_t begin, std::uint64_t end,
                                        std::string_view name) {
            auto &player = named_save.all_players->players.emplace_back();
            player.saved_player_id = owner;
            auto &city = player.cities.cities.emplace_back();
            city.core.start_offset = begin;
            city.core.end_offset = end;
            const auto add_number = [&](const char *field, std::int64_t value) {
                city.core.fields.push_back({field, saves::CityCoreEncoding::i32});
                city.core.fields.back().values = {value};
            };
            add_number("m_iID", id);
            add_number("m_eOwner", owner);
            add_number("m_iX", x);
            add_number("m_iY", y);
            city.core.fields.push_back({"m_strName", saves::CityCoreEncoding::string32});
            city.core.fields.back().bytes.assign(name.begin(), name.end());
        };
        add_saved_city(7, 12, 2, 4, 100, 200, "TXT_KEY_CITY_NAME_DELHI");
        add_saved_city(8, 3, 9, 5, 300, 400, "Kyoto");
        const auto city_text = [](std::string_view key) -> std::optional<std::string> {
            return key == "TXT_KEY_CITY_NAME_DELHI" ? std::optional("Delhi") : std::nullopt;
        };
        const auto named_trade = capture_overview_frame(game, named_cities,
            named_treasury, 7, &trade_units, &trade_database, {}, &named_save, city_text);
        require(named_trade.pages.at("EconomicGeneralInfo").cities.front().name ==
                    "Delhi",
            "economic city table uses the matching owned live city name");
        require(named_trade.pages.at("TradeRouteOverview").rows.front().value ==
                    "Delhi → Kyoto" &&
                    named_trade.named_outgoing_trade_routes &&
                    *named_trade.named_outgoing_trade_routes ==
                        std::vector<std::string>{"Delhi → Kyoto"} &&
                    named_trade.pages.at("TradeRouteOverview").unavailable_reason ==
                    "Trade route yields and destination choices are unavailable",
                "exact live and saved owner, city ID, coordinates and text name both endpoints");
        auto with_incoming = named_treasury;
        auto &foreign_route = with_incoming.trade_routes.back();
        foreign_route.origin_x = 9;
        foreign_route.origin_y = 5;
        foreign_route.destination_x = 2;
        foreign_route.destination_y = 4;
        const auto two_directions = capture_overview_frame(game, named_cities,
            with_incoming, 7, &trade_units, &trade_database, {}, &named_save, city_text);
        require(two_directions.named_outgoing_trade_routes &&
                    *two_directions.named_outgoing_trade_routes ==
                        std::vector<std::string>{"Delhi → Kyoto"} &&
                    two_directions.named_incoming_trade_routes &&
                    *two_directions.named_incoming_trade_routes ==
                        std::vector<std::string>{"Kyoto → Delhi"} &&
                    two_directions.pages.at("TradeRouteOverview").rows.front().value ==
                        "Delhi → Kyoto",
                "incoming city pairs are owned by the foreign origin and do not enter YourTR");
        auto stale_incoming = named_save;
        stale_incoming.all_players->players[0].cities.cities[0].core.fields[3].values = {6};
        const auto incomplete_incoming = capture_overview_frame(game, named_cities,
            with_incoming, 7, &trade_units, &trade_database, {}, &stale_incoming, city_text);
        require(!incomplete_incoming.named_incoming_trade_routes &&
                    incomplete_incoming.named_outgoing_trade_routes == std::nullopt,
                "a stale active-player endpoint does not publish a guessed incoming name");
        auto partial_incoming = with_incoming;
        partial_incoming.trade_routes.back().origin_x = -1;
        const auto partial_incoming_state = capture_overview_frame(game, named_cities,
            partial_incoming, 7, &trade_units, &trade_database, {}, &named_save, city_text);
        require(!partial_incoming_state.named_incoming_trade_routes &&
                    partial_incoming_state.named_outgoing_trade_routes &&
                    *partial_incoming_state.named_outgoing_trade_routes ==
                        std::vector<std::string>{"Delhi → Kyoto"},
                "a partially decoded foreign route cannot contaminate either route list");
        auto stale_save = named_save;
        stale_save.all_players->players[1].cities.cities[0].core.fields[3].values = {6};
        const auto stale_trade = capture_overview_frame(game, named_cities,
            named_treasury, 7, &trade_units, &trade_database, {}, &stale_save, city_text);
        require(stale_trade.pages.at("TradeRouteOverview").rows.front().value ==
                    "Delhi → 9,5" && !stale_trade.named_outgoing_trade_routes,
                "stale saved endpoint coordinates cannot label a live trade route");
        auto wrong_city_id = named_save;
        wrong_city_id.all_players->players[1].cities.cities[0].core.fields[0].values = {4};
        require(capture_overview_frame(game, named_cities, named_treasury, 7,
                    &trade_units, &trade_database, {}, &wrong_city_id, city_text)
                    .pages.at("TradeRouteOverview").rows.front().value ==
                    "Delhi → 9,5",
                "wrong saved city ID cannot label a live trade route");
        auto wrong_city_owner = named_save;
        wrong_city_owner.all_players->players[1].cities.cities[0].core.fields[1].values = {7};
        require(capture_overview_frame(game, named_cities, named_treasury, 7,
                    &trade_units, &trade_database, {}, &wrong_city_owner, city_text)
                    .pages.at("TradeRouteOverview").rows.front().value ==
                    "Delhi → 9,5",
                "wrong saved city owner cannot label a live trade route");
        auto moved_cities = named_cities;
        moved_cities.players[8].cities.at(3).x = 10;
        const auto moved_trade = capture_overview_frame(game, moved_cities,
            named_treasury, 7, &trade_units, &trade_database, {}, &named_save, city_text);
        require(moved_trade.pages.at("TradeRouteOverview").rows.front().value ==
                    "Delhi → 9,5",
                "live endpoint movement invalidates the saved route name");
        auto moved_origin = named_cities;
        moved_origin.players[7].cities.at(12).x = 3;
        require(!capture_overview_frame(game, moved_origin, named_treasury, 7,
                    &trade_units, &trade_database, {}, &named_save, city_text)
                    .pages.at("EconomicGeneralInfo").cities.front().name,
            "economic table rejects a saved name after live city coordinates change");
        const auto untranslated_trade = capture_overview_frame(game, named_cities,
            named_treasury, 7, &trade_units, &trade_database, {}, &named_save);
        require(untranslated_trade.pages.at("TradeRouteOverview").rows.front().value ==
                    "2,4 → Kyoto",
                "untranslated localization keys never appear as city names");
        const auto frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
            std::make_shared<const OverviewFrameState>(state));
        const auto economics = std::find_if(sources.begin(), sources.end(),
            [](const auto &s) { return s.context_id == "EconomicOverview"; });
        const auto general = std::find_if(sources.begin(), sources.end(),
            [](const auto &s) { return s.context_id == "EconomicGeneralInfo"; });
        require(economics != sources.end() && general != sources.end() &&
                economics->selected_source_path.find("Expansion2") != std::string::npos &&
                general->selected_source_path.find("Expansion2") != std::string::npos,
            "economic shell and child layout resolve from active BNW sources");
        const auto plan = plan_overview(*economics, frame, 1280, 960);
        require(plan.paints.size() >= 7 && plan.hits.size() >= 3,
            "render plan includes visible live rows and authored navigation");
        const auto named_frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
            std::make_shared<const OverviewFrameState>(named_trade));
        const auto named_plan = plan_overview(*economics, named_frame, 1280, 960);
        require(std::any_of(named_plan.paints.begin(), named_plan.paints.end(),
                [](const OverviewPaint &paint) { return paint.value == "Delhi"; }),
            "economic city table paints only the resolved live city name");
        const auto positive_state = capture_overview_frame(game, cities, treasury, 7,
            nullptr, nullptr, {}, nullptr, {}, &positive);
        const auto positive_frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
            std::make_shared<const OverviewFrameState>(positive_state));
        const auto positive_plan = plan_overview(*economics, positive_frame, 1280, 960);
        require(std::none_of(positive_plan.paints.begin(), positive_plan.paints.end(),
                [](const OverviewPaint &paint) {
                    return paint.value == "TXT_KEY_EO_PENALTY";
                }),
            "native science penalty row stays hidden when net gold is nonnegative");
        const auto deficit_frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
            std::make_shared<const OverviewFrameState>(quoted));
        const auto deficit_plan = plan_overview(*economics, deficit_frame, 1280, 960);
        require(std::any_of(deficit_plan.paints.begin(), deficit_plan.paints.end(),
                [](const OverviewPaint &paint) {
                    return paint.value == "TXT_KEY_EO_PENALTY";
                }) && std::any_of(deficit_plan.unresolved.begin(), deficit_plan.unresolved.end(),
                [](const std::string &reason) {
                    return reason.find("ScienceLostValue") != std::string::npos;
                }),
            "negative net gold shows the native row without inventing science loss");
        require(std::any_of(economics->controls.begin(), economics->controls.end(),
                    [](const OverviewControl &control) {
                        return control.id == "ScienceLostValue";
                    }) &&
                std::any_of(economics->controls.begin(), economics->controls.end(),
                    [](const OverviewControl &control) {
                        return control.text_key == "TXT_KEY_EO_PENALTY";
                    }),
            "active Expansion2 economic XML owns the science penalty field");
        const auto lost_frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
            std::make_shared<const OverviewFrameState>(lost_state));
        const auto lost_plan = plan_overview(*economics, lost_frame, 1280, 960);
        require(std::any_of(lost_plan.paints.begin(), lost_plan.paints.end(),
                    [](const OverviewPaint &paint) { return paint.value == "-2.50"; }) &&
                std::none_of(lost_plan.unresolved.begin(), lost_plan.unresolved.end(),
                    [](const std::string &reason) {
                        return reason.find("ScienceLostValue") != std::string::npos;
                    }),
            "resolved native science loss paints without an invented placeholder");
        // Scan all language folders through the native basename VFS. The
        // catalog must recover the exact requested path from resolve_all.
        auto base_text = std::make_shared<assets::LooseResourceIndex>(root);
        auto expansion_text = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw_text = std::make_shared<assets::LooseResourceIndex>(root);
        base_text->scan({"Assets/Gameplay/XML/NewText"});
        expansion_text->scan({"Assets/DLC/Expansion/Gameplay/XML/Text"});
        bnw_text->scan({"Assets/DLC/Expansion2/Gameplay/XML/Text"});
        assets::ResourceMountTable text_mounts(assets::resource_lookup(base_text),
            assets::resource_lookup_all(base_text), assets::resource_keys(base_text));
        text_mounts.mount({1, 0, 0, assets::resource_lookup(expansion_text), true, true,
            assets::resource_lookup_all(expansion_text), assets::resource_keys(expansion_text)});
        text_mounts.mount({2, 0, 0, assets::resource_lookup(bnw_text), true, true,
            assets::resource_lookup_all(bnw_text), assets::resource_keys(bnw_text)});
        const auto spanish = TopPanelLocalizationCatalog::read_resources(text_mounts,
            {"Assets/DLC/Expansion/Gameplay/XML/Text/RU_RU/Civ5CivilopediaDLC_Spain_Expansion.xml"},
            "Language_RU_RU");
        require(spanish.rows.at("TXT_KEY_CIV5_DAWN_SPAIN_TITLE") == "Испания",
            "same-name language collision resolves exact Russian XML");
        bool wrong_language_rejected = false;
        try {
            (void)TopPanelLocalizationCatalog::read_resources(text_mounts,
                {"Assets/DLC/Expansion/Gameplay/XML/Text/RU_RU/Civ5CivilopediaDLC_Spain_Expansion.xml"},
                "Language_en_US");
        } catch (const std::runtime_error &error) {
            wrong_language_rejected = std::string_view(error.what()).find(
                "language root is missing") != std::string_view::npos;
        }
        require(wrong_language_rejected,
            "an actually missing requested language root remains a hard error");
        for (const auto &[language, expected_cities, expected_treasury, locale] : {
                 std::tuple{"Language_en_US", "Cities", "Treasury", OverviewLanguage::english},
                 std::tuple{"Language_RU_RU", "Города", "Казна", OverviewLanguage::russian}}) {
            const auto texts = TopPanelLocalizationCatalog::read_active_bnw_resources(
                text_mounts, language);
            const OverviewLocalizer localize = [&texts](std::string_view key)
                -> std::optional<std::string> {
                const auto found = texts.rows.find(std::string(key));
                return found == texts.rows.end() ? std::nullopt
                                                 : std::optional<std::string>(found->second);
            };
            const auto localized = plan_overview(*economics, frame, 1280, 960,
                                                  localize, locale);
            const auto has_paint = [](const OverviewRenderPlan &render,
                                      std::string_view text) {
                return std::any_of(render.paints.begin(), render.paints.end(),
                    [&](const OverviewPaint &paint) { return paint.value == text; });
            };
            require(has_paint(localized, expected_treasury) &&
                    has_paint(localized, "17.50"),
                "live economy labels use active BNW text and exact gold fraction");
            const auto localized_deficit = plan_overview(*economics, lost_frame,
                1280, 960, localize, locale);
            require(localize("TXT_KEY_EO_PENALTY") &&
                    has_paint(localized_deficit, *localize("TXT_KEY_EO_PENALTY")) &&
                    has_paint(localized_deficit, "-2.50"),
                "active EN/RU BNW text labels the exact science penalty");
            require(texts.rows.contains("TXT_KEY_TRADE_ROUTE_OVERVIEW") &&
                    texts.rows.contains("TXT_KEY_TRO_TAB_EXISTING_TR") &&
                    texts.rows.contains("TXT_KEY_UNIT_CARGO_SHIP") &&
                    texts.rows.contains("TXT_KEY_RO_WR_RELIGION") &&
                    texts.rows.contains("TXT_KEY_LEAGUE_OVERVIEW") &&
                    texts.rows.contains("TXT_KEY_POLICIES_ADOPTED"),
                "other dynamic overview labels exist in active BNW text layers");
            const auto trade_frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
                std::make_shared<const OverviewFrameState>(trade_state));
            const auto trade_render = plan_overview(*trade_source, trade_frame, 1280, 960,
                localize, locale);
            const auto demographics = std::find_if(sources.begin(), sources.end(),
                [](const OverviewSource &entry) {
                    return entry.context_id == "Demographics";
                });
            require(demographics != sources.end() &&
                    demographics->selected_source_path.find("Expansion2") != std::string::npos &&
                    localize("TXT_KEY_DEMOGRAPHICS_APPROVAL") &&
                    localize("TXT_KEY_DEMOGRAPHICS_RIVAL_BEST"),
                "demographics shell and EN/RU labels resolve from active BNW resources");
            const auto demographic_render = plan_overview(*demographics, frame,
                1280, 960, localize, locale);
            require(has_paint(demographic_render,
                        *localize("TXT_KEY_DEMOGRAPHICS_APPROVAL")) &&
                    has_paint(demographic_render, "69%") &&
                    has_paint(demographic_render, "N/A") &&
                    !demographic_render.unresolved.empty() &&
                    std::any_of(demographic_render.hits.begin(),
                                demographic_render.hits.end(), [](const OverviewHit &hit) {
                        const auto *close = std::get_if<UiCloseScreen>(&hit.command);
                        return close && close->id == "Demographics";
                    }),
                "native demographic table shows owned approval, unknown fields and typed back");
            require(has_paint(trade_render, *localize("TXT_KEY_TRO_TAB_EXISTING_TR")) &&
                    has_paint(trade_render,
                        (locale == OverviewLanguage::russian
                            ? "Свободный торговый юнит: " : "Unassigned trade unit: ") +
                        *localize("TXT_KEY_UNIT_CARGO_SHIP")) &&
                    has_paint(trade_render, "#52") && !trade_render.unresolved.empty(),
                "active EN/RU BNW resources label live outgoing route and idle trade unit");
            for (const auto &[id, key] : {
                     std::pair{"CultureOverview", "TXT_KEY_POLICIES_ADOPTED"},
                     std::pair{"TradeRouteOverview", "TXT_KEY_TRO_TAB_EXISTING_TR"},
                     std::pair{"ReligionOverview", "TXT_KEY_RO_WR_RELIGION"},
                     std::pair{"LeagueOverviewPopup", "TXT_KEY_LEAGUE_OVERVIEW"},
                     std::pair{"HappinessInfo", "TXT_KEY_CO_VICTORY_EXCESS_HAPPINESS_HEADER"}}) {
                const auto source = std::find_if(sources.begin(), sources.end(),
                    [&](const OverviewSource &entry) { return entry.context_id == id; });
                require(source != sources.end() && localize(key).has_value(),
                    "dynamic overview source has an active EN/RU text key");
                require(has_paint(plan_overview(*source, frame, 1280, 960, localize, locale),
                                  *localize(key)),
                    "dynamic overview row takes its label from active BNW text");
            }
            const auto religion = std::find_if(sources.begin(), sources.end(),
                [](const OverviewSource &entry) {
                    return entry.context_id == "ReligionOverview";
                });
            require(religion != sources.end() &&
                    religion->selected_source_path.find("Expansion2") != std::string::npos,
                "religion overview uses the active BNW XML");
            const auto religion_plan = plan_overview(*religion, frame, 1280, 960,
                localize, locale);
            require(has_paint(religion_plan, locale == OverviewLanguage::russian
                        ? "Последователи в своих городах" : "Followers in owned cities") &&
                    has_paint(religion_plan, "3") &&
                    has_paint(religion_plan, locale == OverviewLanguage::russian
                        ? "Недоступно: вера, верования и мировые итоги религий"
                        : "Unavailable: Faith, beliefs and world religion totals are unavailable"),
                "active EN/RU religion page distinguishes owned live count from missing world data");
            require(std::any_of(religion_plan.hits.begin(), religion_plan.hits.end(),
                    [](const OverviewHit &hit) {
                        const auto *close = std::get_if<UiCloseScreen>(&hit.command);
                        return close && close->id == "ReligionOverview";
                    }),
                "selected BNW religion close control carries a typed navigation hit");
            const auto military = std::find_if(sources.begin(), sources.end(),
                [](const OverviewSource &entry) { return entry.context_id == "MilitaryOverview"; });
            require(military != sources.end(), "military overview source");
            require(military->selected_source_path.find("Expansion2") != std::string::npos &&
                    std::any_of(military->controls.begin(), military->controls.end(),
                        [](const OverviewControl &control) {
                            return control.id == "MilitaryStack";
                        }),
                "active BNW military XML owns the displayed roster stack");
            const auto missing = plan_overview(*military, frame, 1280, 960, localize, locale);
            require(!missing.unresolved.empty() &&
                    has_paint(missing, locale == OverviewLanguage::russian
                        ? "Недоступно: данные обзора BNW не подключены"
                        : "Unavailable: Live BNW overview data is not connected"),
                "missing backend remains explicit in the selected UI language");
            constexpr std::array missing_supply_controls{
                "HandicapSupplyValue", "CitiesSupplyValue",
                "PopulationSupplyValue", "SupplyCapValue",
                "SupplyRemainingValue", "SupplyDeficitValue"};
            require(localize("TXT_KEY_MILITARY_UNIT") &&
                    localize("TXT_KEY_CIVILIAN_UNIT") &&
                    localize("TXT_KEY_SUPPLY_USE") &&
                    std::any_of(military->controls.begin(), military->controls.end(),
                        [](const OverviewControl &control) {
                            return control.id == "SupplyUseValue";
                        }) &&
                    std::all_of(missing_supply_controls.begin(),
                        missing_supply_controls.end(),
                        [&](const char *id) {
                            return std::any_of(military->controls.begin(), military->controls.end(),
                                [&](const OverviewControl &control) {
                                    return control.id == id;
                                });
                        }),
                "active BNW XML separates Supply Use from six unowned supply fields");
            const auto army_frame = UiFrameSnapshot(4, 7, 19).with<OverviewFrameState>(
                std::make_shared<const OverviewFrameState>(owned));
            const auto army_plan = plan_overview(*military, army_frame, 1280, 960,
                localize, locale);
            require(has_paint(army_plan, *localize("TXT_KEY_MILITARY_UNIT")) &&
                    has_paint(army_plan, *localize("TXT_KEY_CIVILIAN_UNIT")) &&
                    has_paint(army_plan, *localize("TXT_KEY_SUPPLY_USE")) &&
                    has_paint(army_plan, "2") &&
                    has_paint(army_plan, "#41 · type #12") &&
                    has_paint(army_plan, locale == OverviewLanguage::russian
                        ? "Недоступно: предел и составляющие снабжения, остаток или нехватка, состояние, движение и сила юнитов"
                        : "Unavailable: Supply capacity, components, remaining/deficit, unit status, movement and strength are unavailable") &&
                    !has_paint(army_plan, "44") && !has_paint(army_plan, "66") &&
                    !has_paint(army_plan, "77") && !has_paint(army_plan, "99") &&
                    !army_plan.unresolved.empty(),
                "active BNW military XML paints owned use and keeps expense inputs out of unowned supply fields");
        }
        require(plan.paints.front().bounds.x == 140 &&
                plan.paints.front().bounds.y == 187 &&
                plan.paints.front().bounds.width == 1000 &&
                plan.paints.front().bounds.height == 658 &&
                std::any_of(plan.paints.begin(), plan.paints.end(),
                    [](const auto &paint) {
                        return paint.value == "17.50" &&
                               paint.bounds.x == 309 && paint.bounds.y == 290;
                    }),
            "shell and treasury scroll geometry come from selected Expansion2 XML");
        require(std::any_of(plan.paints.begin(), plan.paints.end(),
                [](const auto &paint) { return paint.value == "17.50"; }) &&
                std::any_of(plan.paints.begin(), plan.paints.end(),
                [](const auto &paint) { return paint.value == "N/A"; }) &&
                !plan.unresolved.empty(),
            "selected economic XML renders the known balance and explicit missing fields");
        const auto localized = plan_overview(*general, frame, 1280, 960,
            [](std::string_view key) -> std::optional<std::string> {
                return "localized:" + std::string(key);
            });
        require(std::any_of(localized.paints.begin(), localized.paints.end(),
                [](const auto &paint) {
                    return paint.value == "localized:TXT_KEY_EO_TOTAL_GOLD";
                }) && localized.hits.size() >= 3,
            "child layout accepts active localization and parent modal controls");
        require(plan.hit(plan.hits.front().bounds.x + 1, plan.hits.front().bounds.y + 1),
            "navigation is clickable");
        require(std::get<UiOpenScreen>(*overview_control_command(
            "EconomicOverview", "HappinessButton")).id == "HappinessInfo",
            "economic tab goes through U0 navigation");
        require(std::get<UiCloseScreen>(*overview_control_command(
            "ReligionOverview", "CloseButton")).id == "ReligionOverview",
            "religion close button returns a typed U0 navigation command");
        UiScreenController controller(registry);
        require(controller.dispatch(UiOpenScreen{"EconomicOverview"}, frame).status ==
                    UiCommandStatus::navigation &&
                controller.dispatch(*overview_control_command(
                    "EconomicOverview", "GeneralInfoButton"), frame).status ==
                    UiCommandStatus::navigation &&
                controller.focused_screen() == "EconomicGeneralInfo" &&
                controller.dispatch(*overview_control_command(
                    "EconomicGeneralInfo", "CloseButton"), frame).status ==
                    UiCommandStatus::navigation &&
                controller.focused_screen() == "EconomicOverview",
            "overview tab opens a registered nested BNW context");
        require(controller.dispatch(*overview_control_command(
                    "EconomicOverview", "HappinessButton"), frame).status ==
                    UiCommandStatus::navigation &&
                controller.focused_screen() == "HappinessInfo" &&
                controller.dispatch(*overview_control_command(
                    "HappinessInfo", "CloseButton"), frame).status ==
                    UiCommandStatus::navigation &&
                controller.focused_screen() == "EconomicOverview",
            "happiness tab closes back through the modal economic parent");
        const auto stale = plan_overview(*economics, UiFrameSnapshot(5, 8, 19)
            .with<OverviewFrameState>(std::make_shared<const OverviewFrameState>(state)),
            1280, 960);
        require(!stale.unresolved.empty() && stale.paints.size() < plan.paints.size(),
            "stale active player cannot inherit overview values");
        std::cout << "ui overview tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
