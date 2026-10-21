#include "civ5/ui/top_panel_live.hpp"
#include "civ5/ui/overview_screens.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace civ5;
namespace {
void require(bool yes, const char *message) {
    if (!yes) throw std::runtime_error(message);
}
}
int main() {
    auto root = std::filesystem::current_path();
    for (int n = 0; n < 8 && !std::filesystem::exists(root / "civ5-assets/Assets/DLC/Expansion2/UI/InGame/TopPanel.xml"); ++n)
        root = root.parent_path();
    auto source = std::make_shared<assets::LooseSource>(root / "civ5-assets");
    assets::ResourceMountTable resources(assets::resource_lookup(source), {}, assets::resource_keys(source));
    const auto en = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources);
    const auto ru = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources, "Language_RU_RU");
    for (const auto &catalog : {&en, &ru}) {
        require(catalog->rows.contains("TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_1"), "BNW tourism text missing");
        require(catalog->rows.contains("TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT_UNASSIGNED"),
                "BNW trade text missing");
        require(catalog->rows.contains("TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_PLAYER_TT"),
                "Selected EN/RU BNW outgoing-route heading is missing");
        require(catalog->rows.contains("TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_OTHER_TT"),
                "Selected EN/RU BNW incoming-route heading is missing");
        require(catalog->rows.contains("TXT_KEY_TP_FAITH_ACCUMULATED"), "BNW faith text missing");
        require(catalog->rows.contains("TXT_KEY_TP_AVAILABLE_GOLD"),
                "Selected language has no available-gold tooltip text");
        require(catalog->format("TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_1", {"2"}).find("2") != std::string::npos,
                "BNW tourism placeholder was not filled");
        const auto partial_trade = catalog->format(
            "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES", {"2", "N/A"});
        require(partial_trade.find("2") != std::string::npos &&
                    partial_trade.find("N/A") != std::string::npos,
                "EN/RU BNW trade label lost known count or unknown capacity");
    }
    require(en.format("TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_OTHER_TT") ==
                "Trade routes others established to your civilization" &&
            ru.format("TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_OTHER_TT") ==
                "Торговые пути, проложенные другими к вашей цивилизации",
            "Active Expansion2 EN/RU incoming-route heading was not selected");
    ui::TopPanelLiveFrame live;
    auto &player = live.players[7];
    player.city_count = 1;
    player.values.city_count = 1;
    const auto localizer = ui::TopPanelTextLocalizer{.text = [](std::string_view key,
            const std::vector<std::string> &args) {
        std::string result(key);
        for (const auto &arg : args) result += "(" + arg + ")";
        return result;
    }};
    const auto frame = ui::UiFrameSnapshot(1, 7, 0);
    std::optional<ui::OverviewFrameState> overview;
    const auto present = [&] {
        auto published = frame.with(std::make_shared<const ui::TopPanelLiveFrame>(live));
        if (overview)
            published = published.with(std::make_shared<const ui::OverviewFrameState>(*overview));
        return ui::present_top_panel_frame(published, localizer);
    };
    auto output = present();
    require(output && output->international_trade_routes_text == "N/A" && output->tourism_text == "N/A" &&
                output->faith_text == "N/A", "Absent BNW quotes were shown as zero");
    player.gold_balance = 12;
    player.values.gold = 12;
    output = present();
    require(output && output->gold_text && output->gold_text->find("12") != std::string::npos &&
                output->gold_text->find("N/A") != std::string::npos &&
                output->gold_tooltip && output->gold_tooltip->find("TXT_KEY_TP_AVAILABLE_GOLD") != std::string::npos,
            "Known balance with an unknown rate was not represented separately");
    player.available.insert(ui::TopPanelMetric::trade);
    player.available.insert(ui::TopPanelMetric::tourism);
    player.available.insert(ui::TopPanelMetric::faith);
    player.trade = ui::TopPanelTradeQuote{};
    player.tourism = ui::TopPanelTourismQuote{};
    player.faith = ui::TopPanelFaithQuote{};
    output = present();
    require(output && output->international_trade_routes_text == "N/A" && output->tourism_text == "N/A" &&
                output->faith_text == "N/A", "Partial BNW quotes inherited stale availability");
    player.tourism = ui::TopPanelTourismQuote{.great_works_resolved = true,
        .great_works = 2, .great_work_slots = 4};
    player.faith = ui::TopPanelFaithQuote{.per_turn_resolved = true, .per_turn = 3};
    output = present();
    require(output && output->tourism_text == "N/A" && output->tourism_tooltip &&
                output->tourism_tooltip->find("TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_1(2)") != std::string::npos &&
                output->faith_text == "[ICON_PEACE]N/A (+3)" && output->faith_tooltip &&
                output->faith_tooltip->find("N/A") != std::string::npos,
            "Known work counts or faith rate were mistaken for complete BNW values");
    player.tourism = ui::TopPanelTourismQuote{.per_turn_resolved = true, .per_turn = 7};
    player.faith = ui::TopPanelFaithQuote{.balance_resolved = true, .balance = 19};
    output = present();
    require(output && output->tourism_text == "[ICON_TOURISM] +7" &&
                output->tourism_tooltip == "N/A" && output->faith_text &&
                output->faith_text->find("19 (N/A)") != std::string::npos &&
                output->faith_tooltip && output->faith_tooltip->find("TXT_KEY_TP_FAITH_ACCUMULATED(19)") !=
                    std::string::npos && output->faith_tooltip->find("N/A") != std::string::npos,
            "Independently known tourism rate or faith balance was hidden or missing data was invented");
    player.tourism->great_works_resolved = true;
    player.tourism->great_works = 2;
    player.tourism->great_work_slots = 4;
    player.faith->per_turn_resolved = true;
    player.faith->per_turn = 3;
    player.faith->sources_resolved = true;
    player.faith->from_cities = 2;
    output = present();
    require(output && output->tourism_tooltip &&
                output->tourism_tooltip->find("TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_1(2)") != std::string::npos &&
                output->tourism_tooltip->find("TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_3") == std::string::npos &&
                output->faith_text == "[ICON_PEACE]19 (+3)" && output->faith_tooltip &&
                output->faith_tooltip->find("TXT_KEY_TP_FAITH_FROM_CITIES(2)") != std::string::npos &&
                output->faith_tooltip->find("N/A") != std::string::npos,
            "Partial BNW sources concealed known values or claimed unknown influence/progression");
    player.tourism = ui::TopPanelTourismQuote{};
    player.faith = ui::TopPanelFaithQuote{};
    player.outgoing_trade_routes = 2;
    output = present();
    require(output && output->international_trade_routes_text &&
                output->international_trade_routes_text->find("(2)(N/A)") != std::string::npos &&
                output->international_trade_routes_tooltip &&
                output->international_trade_routes_tooltip->find("(2)(N/A)") != std::string::npos &&
                output->tourism_text == "N/A" && output->faith_text == "N/A",
            "Owned outgoing route count did not preserve unknown capacity and other metrics");
    overview = ui::OverviewFrameState{};
    overview->player_id = 7;
    overview->named_outgoing_trade_routes = {std::vector<std::string>{"Delhi → Rome", "Agra → Lisbon"}};
    overview->named_incoming_trade_routes = {std::vector<std::string>{"Kyoto → Delhi"}};
    output = present();
    require(output && output->international_trade_routes_tooltip &&
                output->international_trade_routes_tooltip->find("Delhi → Rome") != std::string::npos &&
                output->international_trade_routes_tooltip->find("Agra → Lisbon") != std::string::npos &&
                output->international_trade_routes_tooltip->find("TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_OTHER_TT") !=
                    std::string::npos &&
                output->international_trade_routes_tooltip->find("Kyoto → Delhi") != std::string::npos &&
                output->international_trade_routes_tooltip->find("Delhi → Rome") <
                    output->international_trade_routes_tooltip->find("Kyoto → Delhi") &&
                output->international_trade_routes_tooltip->find("(2)(N/A)") != std::string::npos,
            "Complete same-player route names in both directions did not extend partial trade tooltip");
    overview->named_incoming_trade_routes.reset();
    output = present();
    require(output && output->international_trade_routes_tooltip &&
                output->international_trade_routes_tooltip->find("Kyoto → Delhi") == std::string::npos,
            "Unresolved incoming city pair remained in the partial trade tooltip");
    overview->named_outgoing_trade_routes = {std::vector<std::string>{"Delhi → Rome"}};
    output = present();
    require(output && output->international_trade_routes_tooltip &&
                output->international_trade_routes_tooltip->find("Delhi → Rome") == std::string::npos,
            "Route list inconsistent with owned count appeared in TopPanel");
    overview->player_id = 8;
    overview->named_outgoing_trade_routes = {std::vector<std::string>{"Delhi → Rome", "Agra → Lisbon"}};
    overview->named_incoming_trade_routes = {std::vector<std::string>{"Kyoto → Delhi"}};
    output = present();
    require(output && output->international_trade_routes_tooltip &&
                output->international_trade_routes_tooltip->find("Delhi → Rome") == std::string::npos &&
                output->international_trade_routes_tooltip->find("Kyoto → Delhi") == std::string::npos,
            "Another player's route lists leaked into TopPanel");
    overview.reset();
    player.trade = ui::TopPanelTradeQuote{.details_resolved = true, .used = 1, .available = 3, .idle_land_units = 2,
        .idle_sea_units = 1, .land_unit_description_key = "TXT_KEY_CARAVAN",
        .sea_unit_description_key = "TXT_KEY_CARGO_SHIP", .established_by_player = "Delhi / Rome",
        .owner_player_id = 7, .owner_turn = 0, .owner_frame_number = 1};
    player.tourism = ui::TopPanelTourismQuote{.influence_resolved = true, .per_turn = 7, .great_works = 2, .great_work_slots = 4,
        .cultural_victory_enabled = true, .influential_civilizations = 1, .required_civilizations = 5};
    player.faith = ui::TopPanelFaithQuote{.progression_resolved = true, .balance = 19, .per_turn = 3, .from_cities = 2,
        .from_minors = 1, .progression_text = "Religion slots: 2"};
    output = present();
    require(output && output->international_trade_routes_text &&
                output->international_trade_routes_text->find("(2)(N/A)") != std::string::npos,
            "Contradictory complete trade quote bypassed owned route count");
    player.trade->used = 2;
    output = present();
    require(output && output->international_trade_routes_tooltip &&
                output->international_trade_routes_tooltip->find("Delhi / Rome") != std::string::npos &&
                output->international_trade_routes_tooltip->find("TXT_KEY_CARGO_SHIP") != std::string::npos &&
                output->tourism_tooltip && output->tourism_tooltip->find("TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_3") != std::string::npos &&
                output->faith_tooltip && output->faith_tooltip->find("Religion slots: 2") != std::string::npos,
            "Complete BNW quotes were not shown");
    auto other = ui::UiFrameSnapshot(2, 8, 1).with(
        std::make_shared<const ui::TopPanelLiveFrame>(live));
    require(!ui::present_top_panel_frame(other, localizer),
            "Active player switch reused another player's BNW quotes");
    player.city_count = 0;
    output = present();
    require(output && !output->show_info_stack &&
                !output->international_trade_routes_text && !output->tourism_text &&
                !output->faith_text, "No-city TopPanel displayed BNW metrics");
    player.city_count = 1;
    live.no_religion = true;
    output = present();
    require(output && output->faith_text == "TXT_KEY_TOP_PANEL_RELIGION_OFF" &&
                output->faith_tooltip == "TXT_KEY_TOP_PANEL_RELIGION_OFF_TOOLTIP",
            "Disabled religion did not use the authored BNW message");
    std::cout << "PASS: BNW EN/RU text and live trade, tourism, faith presentation\n";
}
