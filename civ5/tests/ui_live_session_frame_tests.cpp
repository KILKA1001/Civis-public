#include "civ5/ui/live_session_frame.hpp"
#include "civ5/ui/overview_screens.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace civ5;

void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

saves::SavedSession source() {
    saves::SavedSession result;
    result.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    result.source.bytes = {1, 2, 3};
    result.snapshot.payload_end_offset = result.source.bytes.size();
    return result;
}

runtime::LiveSessionState state(const saves::SavedSession &saved) {
    using namespace runtime;
    LiveSessionState result{saved.snapshot, ResearchContext{},
        std::vector<TeamTechnologies>{}, PlayerResearchContext{},
        CityYieldContext{}, BnwUnhappinessContext{}, MaintenanceContext{},
        TreasuryContext{}, ScienceContext{}, ResearchTurnState{},
        SessionRandomStreams{SimulationRandom(RandomAlgorithm::bnw, {5, 0, 0}),
                             SimulationRandom(RandomAlgorithm::bnw, {7, 0, 0})}};
    result.setup.game_turn = 12;
    auto &player = result.setup.players[7];
    player.alive = true;
    player.team = 3;
    auto &city = result.cities.players[7].cities[9];
    city.owner = 7;
    city.id = 9;
    city.population = 5;
    result.treasury.players[7].balance_times100 = 10000;
    saves::TradeRouteObservation route;
    route.origin_owner = 7;
    route.destination_owner = 8;
    route.origin_x = 2;
    route.origin_y = 4;
    route.destination_x = 9;
    route.destination_y = 5;
    result.treasury.trade_routes.push_back(route);
    result.maintenance.players[7].units[21] = MaintenanceUnit{.type = 4, .military = true, .civilian = false};
    return result;
}

ui::UiContextRegistry registry() {
    return ui::UiContextRegistry::from_documents(
        content::read_xml_metadata(R"(<Context>
            <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
        </Context>)"), content::read_xml_metadata("<Context/>"));
}

struct ReadyWorldBackend final : ui::UiGameplayBackend {
    mutable int availability_calls = 0;
    int execution_calls = 0;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &,
                                      const ui::UiFrameSnapshot &) const override {
        ++availability_calls;
        return {ui::UiCommandStatus::ready, {}};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &,
                                 const ui::UiFrameSnapshot &) override {
        ++execution_calls;
        return {ui::UiCommandStatus::performed, {}};
    }
};

} // namespace

int main() {
    try {
        saves::SavedMapProjection plot_map;
        plot_map.grid = {2, 1};
        plot_map.cells.resize(2);
        plot_map.cells[0].index = 0;
        plot_map.cells[0].resource_type = 5;
        plot_map.cells[0].cached_yields = std::map<std::int64_t, std::int64_t>{{0, 3}};
        plot_map.cells[1].index = 1;
        plot_map.cells[1].resource_type = 6;
        plot_map.cells[1].cached_yields = std::map<std::int64_t, std::int64_t>{{0, 4}};
        ui::WorldViewState plot_world;
        plot_world.player_id = 7;
        plot_world.map_visibility = ui::WorldViewMapVisibility{
            7, 3, 2, 1, {ui::WorldViewPlotFog::visible, ui::WorldViewPlotFog::hidden}};
        ui::LiveWorldViewPlotFeed plot_feed;
        plot_feed.player_id = 7;
        plot_feed.team_id = 3;
        plot_feed.turn = 12;
        plot_feed.frame_number = 1;
        plot_feed.hovered_plot = std::pair<std::uint64_t, std::string>{0, "Grassland"};
        plot_feed.resources.push_back({0, 5, 1,
            {0, {300, 400}, {}, "resource.dds", {0, 0, 64, 64}, true}});
        plot_feed.resources.push_back({1, 6, 1,
            {1, {330, 400}, {}, "hidden-resource.dds", {0, 0, 64, 64}, true}});
        plot_feed.yields.push_back({0, 0, 3,
            {2, {300, 430}, {}, "yield.dds", {0, 256, 128, 128}, true}});
        plot_feed.yields.push_back({1, 0, 4,
            {3, {330, 430}, {}, "hidden-yield.dds", {0, 384, 128, 128}, true}});
        const auto projected_plots = ui::apply_live_world_view_plot_feed(plot_world,
            plot_map, plot_feed, 1, 12,
            [](std::uint64_t, std::int64_t) { return std::optional<std::int64_t>{5}; },
            [](std::uint64_t, std::int64_t) { return std::optional<std::int64_t>{3}; });
        require(projected_plots.plot_help == "Grassland" &&
                    projected_plots.resource_icons.size() == 1 &&
                    projected_plots.resource_icons.front().texture == "resource.dds" &&
                    projected_plots.yield_icons.size() == 1 &&
                    projected_plots.yield_icons.front().texture == "yield.dds",
                "current hover and queried revealed plot icons enter live WorldView");
        const auto no_queries = ui::apply_live_world_view_plot_feed(plot_world,
            plot_map, plot_feed, 1, 12, {}, {});
        require(no_queries.plot_help == "Grassland" &&
                    no_queries.resource_icons.empty() && no_queries.yield_icons.empty(),
                "saved resource and yield caches alone never publish visible icons");
        require(ui::apply_live_world_view_plot_feed(plot_world, plot_map,
                    plot_feed, 2, 12, {}, {}).plot_help == std::nullopt,
                "stale camera frame cannot publish tile help");
        plot_feed.hovered_plot = std::pair<std::uint64_t, std::string>{1, "Hidden"};
        require(!ui::apply_live_world_view_plot_feed(plot_world, plot_map,
                    plot_feed, 1, 12, {}, {}).plot_help,
                "unrevealed plot cannot publish tile help");
        auto imported = source();
        auto session = runtime::LiveSession::from_restored_bnw(imported, state(imported));
        content::TechnologyRules technologies;
        content::RulesDatabase database;
        std::optional<std::int64_t> selected_city = 9;
        std::optional<ui::ProgressionTechPopupEvent> tech_popup_event;
        std::optional<ui::ProgressionIdeologyPopupEvent> ideology_popup_event;
        std::optional<ui::LiveWorldViewNotificationFeed> notification_feed;
        std::optional<ui::LivePopupEventFeed> popup_feed;
        std::optional<ui::LiveWorldViewInfoCornerFeed> info_feed;
        std::optional<std::int64_t> selected_unit = 21;
        int quote_calls = 0;
        bool expose_bnw_quotes = false;
        ui::LiveSessionUiSources sources;
        sources.technologies = &technologies;
        sources.database = &database;
        sources.selected_city = [&] { return selected_city; };
        sources.selected_unit = [&] { return selected_unit; };
        sources.active_notifications = [&](const runtime::LiveSession &) {
            return notification_feed;
        };
        sources.active_popups = [&](const runtime::LiveSession &) {
            return popup_feed;
        };
        sources.info_corner_items = [&](const runtime::LiveSession &) {
            return info_feed;
        };
        sources.active_tech_popup = [&](const runtime::LiveSession &) {
            return tech_popup_event;
        };
        sources.active_ideology_popup = [&](const runtime::LiveSession &) {
            return ideology_popup_event;
        };
        sources.quotes = [&](const runtime::LiveSession &live,
                             std::uint64_t frame_number) {
            ++quote_calls;
            ui::LiveSessionUiQuotes quotes;
            runtime::ScienceQuote science;
            science.total_times100 = 300;
            science.gold_rate_times100 = 100;
            science.happiness = 20;
            quotes.science.emplace(7, science);
            runtime::TreasuryNetQuote gold;
            gold.net_times100 = live.state().treasury.players.at(7).balance_times100 / 10;
            gold.expenses = runtime::MaintenanceQuote{};
            quotes.gold.emplace(7, gold);
            quotes.trade.emplace(8, ui::TopPanelTradeQuote{.details_resolved = true,
                .used = 9, .available = 9});
            if (expose_bnw_quotes) {
                quotes.trade.emplace(7, ui::TopPanelTradeQuote{.details_resolved = true,
                    .used = 1, .available = 3, .owner_player_id = 7,
                    .owner_turn = live.state().setup.game_turn,
                    .owner_frame_number = frame_number});
                quotes.tourism.emplace(7, ui::TopPanelTourismQuote{
                    .influence_resolved = true, .per_turn = 7, .great_works = 2,
                    .great_work_slots = 4, .influential_civilizations = 1,
                    .required_civilizations = 5});
                quotes.faith.emplace(7, ui::TopPanelFaithQuote{.progression_resolved = true,
                    .balance = 19, .per_turn = 3, .progression_text = "Known progression"});
            }
            return quotes;
        };

        auto provider = ui::make_live_session_frame_provider(session, 7, sources);
        ui::LiveUiHost host(registry(), provider, nullptr);
        auto first = host.begin_frame(1280, 960);
        const auto *first_city = first.snapshot.find<ui::CityScreenFrame>();
        const auto *first_top = first.snapshot.find<ui::TopPanelLiveFrame>();
        const auto *first_progression = first.snapshot.find<ui::ProgressionFrameState>();
        const auto *first_world = first.snapshot.find<ui::WorldViewState>();
        const auto *first_overview = first.snapshot.find<ui::OverviewFrameState>();
        require(first_overview && first_overview->pages.at("MilitaryOverview").rows.size() == 2 &&
                    first_overview->pages.at("MilitaryOverview").rows.front().value == "#21 · type #4" &&
                    first_overview->pages.at("MilitaryOverview").rows.back().value == "1",
                "live frame publishes owned unit roster and native Supply Use count");
        require(first_overview->pages.at("TradeRouteOverview").rows.size() == 1 &&
                    first_overview->pages.at("TradeRouteOverview").rows.front().value ==
                        "2,4 → 9,5",
                "live frame publishes owned established trade route");
        require(first.snapshot.frame() == 1 && first.snapshot.active_player_id() == 7 &&
                    first.snapshot.turn() == 12 && quote_calls == 1,
                "provider captures one current turn and active player per frame");
        require(first_city && first_city->selected_city_id == 9 &&
                    first_city->cities.size() == 1 && first_city->cities.front().population == 5,
                "city payload comes from owned live city");
        require(first_world && first_world->player_id == 7 &&
                    !first_world->end_turn_available &&
                    !first_world->end_turn_label &&
                    first_world->end_turn_unavailable_reason ==
                        "Full BNW turn continuation is unavailable" &&
                    first_world->notifications.empty() &&
                    !first_world->selected_unit &&
                    first_world->unit_flags.empty() &&
                    first_world->city_banners.empty(),
                "alive owned player publishes only defensible world state and disabled End Turn");

        ReadyWorldBackend ready_backend;
        ui::LiveUiHost guarded(registry(),
            ui::make_live_session_frame_provider(session, 7, sources), &ready_backend);
        const auto guarded_frame = guarded.begin_frame(1280, 960);
        const auto end_turn = ui::UiGameplayCommand::make("WorldView",
            ui::WorldViewAction{ui::WorldViewActionKind::end_turn});
        require(guarded.dispatch(end_turn, guarded_frame).status ==
                    ui::UiCommandStatus::unavailable &&
                    ready_backend.availability_calls == 0 &&
                    ready_backend.execution_calls == 0,
                "disabled live End Turn cannot reach an unrelated ready backend");
        require(first_top && first_top->players.size() == 1 &&
                    first_top->players.at(7).values.gold == 100 &&
                    first_top->players.at(7).outgoing_trade_routes == 1 &&
                    !first_top->players.at(7).trade &&
                    !first_top->players.at(7).tourism &&
                    !first_top->players.at(7).faith &&
                    first_top->players.at(7).available.contains(ui::TopPanelMetric::gold) &&
                    !first_top->players.at(7).available.contains(ui::TopPanelMetric::science) &&
                    !first_top->players.at(7).available.contains(ui::TopPanelMetric::happiness),
                "dirty happiness suppresses science but retains complete gold quote");
        session.mutable_state().science.no_science = true;
        session.mutable_state().cities.no_happiness = true;
        session.mutable_state().setup.game_options.push_back({"GAMEOPTION_NO_POLICIES", 1});
        const auto disabled_options = ui::capture_live_session_ui_frame(
            session, 90, 7, sources);
        const auto *disabled_top = disabled_options.find<ui::TopPanelLiveFrame>();
        require(disabled_top && disabled_top->no_science && disabled_top->no_happiness &&
                    disabled_top->no_policies && !first_top->no_policies &&
                    quote_calls == 3 &&
                    !first_top->no_science && !first_top->no_happiness,
                "Live BNW option transition did not capture one current quote or preserve the earlier frame");
        const auto disabled_panel = ui::present_top_panel_frame(disabled_options,
            ui::TopPanelTextLocalizer{.text = [](std::string_view key,
                const std::vector<std::string> &) { return std::string(key); }});
        require(disabled_panel &&
                    disabled_panel->culture_text == "TXT_KEY_TOP_PANEL_POLICIES_OFF" &&
                    disabled_panel->culture_tooltip == "TXT_KEY_TOP_PANEL_POLICIES_OFF_TOOLTIP",
                "Explicit live Policies Off did not reach the TopPanel label and tooltip");
        session.mutable_state().science.no_science = false;
        session.mutable_state().cities.no_happiness = false;
        session.mutable_state().setup.game_options.back().value = 0;
        require(first_progression && first_progression->player_id == 7 &&
                    !first_progression->unavailable_reason.empty(),
                "incomplete progression remains explicit");
        require(first_overview && first_overview->player_id == 7 &&
                    first_overview->pages.at("EconomicGeneralInfo").rows.front().value == "100.00" &&
                    first_overview->pages.at("EconomicGeneralInfo").cities.size() == 1,
                "overview snapshot reads the same live player and turn");
        auto dirty_deficit_sources = sources;
        dirty_deficit_sources.quotes = [](const runtime::LiveSession &,
                                          std::uint64_t) {
            ui::LiveSessionUiQuotes quotes;
            runtime::TreasuryNetQuote gold;
            gold.net_times100 = -10100;
            gold.expenses = runtime::MaintenanceQuote{};
            quotes.gold.emplace(7, gold);
            runtime::ScienceQuote science;
            science.gold_rate_times100 = -10100;
            science.budget_deficit = -100;
            quotes.science.emplace(7, science);
            return quotes;
        };
        const auto dirty_deficit = ui::capture_live_session_ui_frame(
            session, 91, 7, dirty_deficit_sources);
        const auto *dirty_economy = dirty_deficit.find<ui::OverviewFrameState>();
        require(dirty_economy &&
                    dirty_economy->pages.at("EconomicGeneralInfo").science_lost_visible == true &&
                    std::none_of(dirty_economy->pages.at("EconomicGeneralInfo").rows.begin(),
                                 dirty_economy->pages.at("EconomicGeneralInfo").rows.end(),
                        [](const ui::OverviewRow &row) {
                            return row.label == "ScienceLostValue";
                        }),
                "dirty happiness prevents a science quote from leaking into EconomicOverview");
        require(!first.snapshot.find<ui::ProgressionTechPopupState>(),
                "no chooser payload without a live popup event");
        require(!first.snapshot.find<ui::ProgressionIdeologyPopupEvent>(),
                "ideology rule rows do not create a popup event");
        require(!first.snapshot.find<ui::PopupFrame>(),
                "no GameCore/Lua event owner means no live modal queue");

        session.mutable_state().setup.game_turn = 13;
        session.mutable_state().cities.players.at(7).cities.at(9).population = 8;
        session.mutable_state().treasury.players.at(7).balance_times100 = 20000;
        session.mutable_state().treasury.trade_routes.front().destination_x = 10;
        session.mutable_state().maintenance.players.at(7).units[22] =
            runtime::MaintenanceUnit{.type = 5, .military = false, .civilian = true};
        expose_bnw_quotes = true;
        selected_city = std::nullopt;
        tech_popup_event = ui::ProgressionTechPopupEvent{
            7, ui::ProgressionTechPopupMode::research, std::nullopt, 101};
        ideology_popup_event = ui::ProgressionIdeologyPopupEvent{7, 202};
        notification_feed = ui::LiveWorldViewNotificationFeed{
            7, 13, 91, {{41, "Current event", {}, true}}};
        info_feed = ui::LiveWorldViewInfoCornerFeed{
            7, 13, 92, {{5, "Current corner", {}, {}, false}}};
        ui::PopupEvent first_event;
        first_event.context_id = "ChoosePantheon";
        first_event.event_id = 301;
        first_event.title = "Choose a pantheon";
        first_event.choices.push_back({11, "Fertility Rites",
            "Owner supplied belief description", true});
        first_event.choose_pantheon = ui::ChoosePantheonOwnerEvidence{
            7, 13, 93, 301, 1, ui::ChoosePantheonMode::pantheon,
            "Assets/DLC/Expansion2/UI/InGame/Popups/ChoosePantheonPopup.xml"};
        ui::PopupEvent queued_event;
        queued_event.context_id = "NewEraPopup";
        queued_event.event_id = 302;
        queued_event.title = "New Era";
        popup_feed = ui::LivePopupEventFeed{7, 13, 93,
            {first_event, queued_event}};
        auto second = host.begin_frame(1280, 960);
        const auto *second_city = second.snapshot.find<ui::CityScreenFrame>();
        const auto *second_top = second.snapshot.find<ui::TopPanelLiveFrame>();
        const auto *second_world = second.snapshot.find<ui::WorldViewState>();
        const auto *second_overview = second.snapshot.find<ui::OverviewFrameState>();
        require(second_overview && second_overview->pages.at("MilitaryOverview").rows.size() == 3 &&
                    second_overview->pages.at("MilitaryOverview").rows.back().value == "2" &&
                    first_overview->pages.at("MilitaryOverview").rows.size() == 2 &&
                    first_overview->pages.at("MilitaryOverview").rows.back().value == "1",
                "later frame rereads owned Supply Use without mutating earlier frame");
        require(second_overview->pages.at("TradeRouteOverview").rows.front().value ==
                    "2,4 → 10,5" &&
                    first_overview->pages.at("TradeRouteOverview").rows.front().value ==
                    "2,4 → 9,5",
                "trade route snapshot changes with live session but old frame is immutable");
        require(second.snapshot.frame() == 2 && second.snapshot.turn() == 13 &&
                    quote_calls == 4 && second_city && !second_city->selected_city_id &&
                    second_city->cities.front().population == 8 &&
                    second_top && second_top->players.at(7).values.gold == 200 &&
                    second_top->players.at(7).outgoing_trade_routes == 1 &&
                    second_top->players.at(7).trade &&
                    second_top->players.at(7).trade->used == 1 &&
                    second_top->players.at(7).tourism &&
                    second_top->players.at(7).faith &&
                    first_top->players.at(7).outgoing_trade_routes == 1,
                "second frame rereads turn, selection, city and treasury");
        require(second_world && second_world->player_id == 7 &&
                    !second_world->end_turn_available &&
                    second_world->notifications.size() == 1 &&
                    second_world->notifications.front().id == 41 &&
                    second_world->info_corner.size() == 1 &&
                    first_world->notifications.empty() &&
                    first_world->player_id == 7 && !first_world->end_turn_available,
                "current owner notification enters only the new immutable frame");
        require(second_overview &&
                    second_overview->pages.at("EconomicGeneralInfo").rows.front().value == "200.00" &&
                    first_overview->pages.at("EconomicGeneralInfo").rows.front().value == "100.00",
                "economic values are immutable across live frame boundaries");
        require(first_city->cities.front().population == 5 &&
                    first_top->players.at(7).values.gold == 100,
                "earlier immutable frame is not overwritten");
        session.mutable_state().treasury.trade_routes.front().origin_owner = 8;
        const auto changed_routes = ui::capture_live_session_ui_frame(session, 4, 7, sources);
        const auto *changed_top = changed_routes.find<ui::TopPanelLiveFrame>();
        require(changed_top && changed_top->players.at(7).outgoing_trade_routes == 0 &&
                    second_top->players.at(7).outgoing_trade_routes == 1,
                "Owned outgoing route count is refreshed without rewriting an earlier frame");
        session.mutable_state().treasury.trade_routes.front().destination_x = -1;
        const auto partial_route = ui::capture_live_session_ui_frame(session, 5, 7, sources);
        const auto *partial_top = partial_route.find<ui::TopPanelLiveFrame>();
        require(partial_top && !partial_top->players.at(7).outgoing_trade_routes,
                "Partially decoded trade slot was presented as an established count");
        const auto *second_popup = second.snapshot.find<ui::ProgressionTechPopupState>();
        const auto *second_ideology = second.snapshot.find<ui::ProgressionIdeologyPopupEvent>();
        require(second_popup && second_popup->player_id == 7 &&
                    second_popup->eligible_technologies.empty() &&
                    second_popup->unavailable_reason ==
                        "Live BNW research selection backend is unavailable" &&
                    !first.snapshot.find<ui::ProgressionTechPopupState>(),
                "real chooser signal publishes explicit missing-backend boundary");
        require(second_ideology && second_ideology->player_id == 7 &&
                    second_ideology->token == 202 &&
                    !first.snapshot.find<ui::ProgressionIdeologyPopupEvent>(),
                "real ideology popup signal is published in the current frame only");
        const auto *second_modals = second.snapshot.find<ui::PopupFrame>();
        require(second_modals && second_modals->player_id == 7 &&
                    second_modals->turn == 13 && second_modals->owner_sequence == 93 &&
                    second_modals->events.size() == 2 &&
                    second_modals->events[0].context_id == "ChoosePantheon" &&
                    second_modals->events[0].choices[0].id == 11 &&
                    second_modals->events[1].context_id == "NewEraPopup" &&
                    !first.snapshot.find<ui::PopupFrame>(),
                "current owner queue enters one immutable player-scoped frame in order");
        ui::PopupEvent religion;
        religion.context_id = "ChooseReligion";
        religion.event_id = 304;
        religion.choices.push_back({1, "Live symbol", {}, true});
        religion.choose_religion = ui::ChooseReligionOwnerEvidence{
            7, 13, 94, 304, 0, 0, ui::ChooseReligionMode::founding,
            "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseReligionPopup.xml"};
        popup_feed = ui::LivePopupEventFeed{7, 13, 94, {religion}};
        const auto religion_frame = ui::capture_live_session_ui_frame(
            session, 3, 7, sources);
        require(religion_frame.find<ui::PopupFrame>() &&
                    religion_frame.find<ui::PopupFrame>()->events.front()
                        .choose_religion->city_x == 0,
                "current owner religion event resolves to the active player's city");
        popup_feed->events.front().choose_religion->city_x = 9;
        require(!ui::capture_live_session_ui_frame(session, 4, 7, sources)
                    .find<ui::PopupFrame>(),
                "religion event for a different city cannot enter live frame");
        popup_feed->events.front().choose_religion->city_x = 0;
        popup_feed->events.front().choose_religion->owner_sequence = 95;
        require(!ui::capture_live_session_ui_frame(session, 5, 7, sources)
                    .find<ui::PopupFrame>(),
                "stale religion owner sequence cannot enter live frame");
        ui::PopupEvent archaeology;
        archaeology.context_id = "ChooseArchaeology";
        archaeology.event_id = 305;
        archaeology.choices = {{1, "Landmark", {}, true},
                               {2, "Artifact", {}, true}};
        archaeology.choose_archaeology = ui::ChooseArchaeologyOwnerEvidence{
            7, 13, 96, 305, 21, 40, false,
            "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseArchaeologyPopup.xml"};
        popup_feed = ui::LivePopupEventFeed{7, 13, 96, {archaeology}};
        require(ui::capture_live_session_ui_frame(session, 6, 7, sources)
                    .find<ui::PopupFrame>(),
                "owner-issued archaeology event enters the existing modal queue");
        popup_feed->events.front().choices.front().id = 5;
        require(!ui::capture_live_session_ui_frame(session, 7, 7, sources)
                    .find<ui::PopupFrame>(),
                "archaeology choice incompatible with artifact type is rejected");
        popup_feed->events.front().choose_archaeology->written_artifact = true;
        popup_feed->events.front().choices = {{4, "Cultural renaissance", {}, true},
                                              {5, "Written artifact", {}, true}};
        require(ui::capture_live_session_ui_frame(session, 8, 7, sources)
                    .find<ui::PopupFrame>(),
                "written artifact publishes only its owner-issued choice IDs");
        popup_feed = ui::LivePopupEventFeed{7, 13, 93,
            {first_event, queued_event}};

        session.mutable_state().treasury.players.erase(7);
        tech_popup_event->player_id = 8;
        ideology_popup_event->player_id = 8;
        notification_feed->player_id = 8;
        popup_feed->player_id = 8;
        info_feed->player_id = 8;
        const auto quote_calls_before_third = quote_calls;
        auto third = host.begin_frame(1280, 960);
        require(third.snapshot.frame() == 3 &&
                    third.snapshot.find<ui::CityScreenFrame>() &&
                    !third.snapshot.find<ui::TopPanelLiveFrame>() &&
                    !third.snapshot.find<ui::ProgressionTechPopupState>() &&
                    !third.snapshot.find<ui::ProgressionIdeologyPopupEvent>() &&
                    !third.snapshot.find<ui::PopupFrame>() &&
                    quote_calls == quote_calls_before_third,
                "missing treasury removes TopPanel payload without reusing old data");
        require(third.snapshot.find<ui::WorldViewState>() &&
                    third.snapshot.find<ui::WorldViewState>()->notifications.empty() &&
                    third.snapshot.find<ui::WorldViewState>()->info_corner.empty() &&
                    second_world->notifications.size() == 1,
                "wrong-player notification feed cannot leak into active frame");
        notification_feed->player_id = 7;
        notification_feed->turn = 12;
        popup_feed->player_id = 7;
        popup_feed->turn = 12;
        require(ui::capture_live_session_ui_frame(session, 6, 7, sources)
                    .find<ui::WorldViewState>()->notifications.empty(),
                "old-turn notification feed is not republished");
        require(!ui::capture_live_session_ui_frame(session, 7, 7, sources)
                    .find<ui::PopupFrame>(),
                "old-turn modal queue is not republished");
        popup_feed->turn = 13;
        popup_feed->events[1].context_id = popup_feed->events[0].context_id;
        require(!ui::capture_live_session_ui_frame(session, 8, 7, sources)
                    .find<ui::PopupFrame>(),
                "duplicate modal context cannot make an ambiguous queue");
        popup_feed->events[1].context_id = "NewEraPopup";
        popup_feed->events[0].choices.push_back(
            popup_feed->events[0].choices.front());
        require(!ui::capture_live_session_ui_frame(session, 9, 7, sources)
                    .find<ui::PopupFrame>(),
                "duplicate choice ID cannot publish an ambiguous decision");
        require(second_modals->events.size() == 2 &&
                    second_modals->events[1].context_id == "NewEraPopup" &&
                    second_modals->events[0].choices.size() == 1,
                "later malformed owner data cannot mutate a published snapshot");
        const auto *third_overview = third.snapshot.find<ui::OverviewFrameState>();
        require(third_overview &&
                    third_overview->pages.at("EconomicGeneralInfo").rows.empty(),
                "missing live treasury removes economic balance");
        const auto absent = ui::capture_live_session_ui_frame(session, 99, 8, sources);
        require(absent.active_player_id() == 8 && !absent.find<ui::CityScreenFrame>() &&
                    !absent.find<ui::ProgressionFrameState>() &&
                    !absent.find<ui::TopPanelLiveFrame>() &&
                    !absent.find<ui::WorldViewState>() &&
                    !absent.find<ui::OverviewFrameState>(),
                "unknown active player yields no borrowed player payload");

        session.mutable_state().setup.game_turn =
            static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) + 1;
        bool rejected = false;
        try { (void)ui::capture_live_session_ui_frame(session, 100, 7, sources); }
        catch (const std::out_of_range &) { rejected = true; }
        require(rejected, "turn overflow rejected instead of truncating");
        std::cout << "live session UI frame tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
