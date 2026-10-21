#include "civ5/ui/economic_overview_scene.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "../validation/economic_popup_feed_mux.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

civ5::runtime::LiveSession session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{source.snapshot, runtime::ResearchContext{},
        std::vector<runtime::TeamTechnologies>{}, runtime::PlayerResearchContext{},
        runtime::CityYieldContext{}, runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{}, runtime::TreasuryContext{},
        runtime::ScienceContext{}, runtime::ResearchTurnState{},
        runtime::SessionRandomStreams{
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {5, 0, 0}),
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {7, 0, 0})},
        runtime::ResearchSelectionState{}, runtime::BnwGameClockState{},
        {}, {}, {}, {}, {}, {}, {}, {}, {}};
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    return runtime::LiveSession::from_restored_bnw(std::move(source),
                                                    std::move(state));
}

civ5::ui::LiveUiFrame live_frame(civ5::ui::UiFrameSnapshot snapshot,
    std::uint64_t sequence, const void *host) {
    return {std::move(snapshot), sequence, host,
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
        auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->scan({root / "Assets/UI", {}, false});
        expansion->scan({root / "Assets/DLC/Expansion/UI", {}, false});
        bnw->scan({root / "Assets/DLC/Expansion2/UI", {}, false});
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
            assets::resource_keys(base));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {},
            assets::resource_keys(expansion)});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
            assets::resource_keys(bnw)});
        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        auto live = session();
        auto foreign = session();
        int processed = 0;
        EconomicOverviewScene *scene_ptr = nullptr;
        EconomicOverviewScene scene(live, 7, screens, resources,
            [&](const EconomicPopupProcessedEvent &event) {
                require(scene_ptr && !screens.visible("EconomicOverview") &&
                        scene_ptr->lifecycle().turn_timer_semaphore() == 0 &&
                        event.player_id == 7 && event.event_id == 64,
                    "Processed follows dequeue, hidden context and timer decrement");
                ++processed;
            });
        scene_ptr = &scene;
        require(scene.gold_source().verified && scene.tab_source().verified &&
                scene.close_source().verified && scene.field_source().verified &&
                scene.tab_source().xml_path.find("Expansion2") != std::string::npos,
            "selected BNW source chain resolves from mounted XML and Lua");
        LiveSessionUiSources economic_sources;
        std::string blocker;
        require(scene.install_popup_feed(economic_sources, blocker) &&
                !scene.install_popup_feed(economic_sources, blocker) && !blocker.empty(),
            "scene installs one live popup owner without replacing another");
        LiveSessionUiSources sources;
        bool notification_active = false;
        sources.active_popups = [&](const runtime::LiveSession &requested)
            -> std::optional<LivePopupEventFeed> {
            if (&requested != &live || !notification_active) return std::nullopt;
            PopupEvent event;
            event.context_id = "TechPopup";
            event.event_id = 77;
            return LivePopupEventFeed{7, 12, 77, {event}};
        };
        const auto notification_feed = sources.active_popups;
        const auto mux_status =
            std::make_shared<civ5::validation::PopupFeedMuxStatus>();
        sources.active_popups = civ5::validation::compose_popup_feeds(
            notification_feed, economic_sources.active_popups, mux_status);
        notification_active = true;
        require(sources.active_popups(live) &&
                sources.active_popups(live)->events.front().context_id == "TechPopup",
            "existing notification popup provider survives composition");
        notification_active = false;
        int host = 0;
        auto before = live_frame(capture_live_session_ui_frame(live, 20, 7, sources),
                                 1, &host);
        require(!before.snapshot.find<PopupFrame>(),
            "saved session has no synthetic active EconomicOverview event");
        before.top_panel.emplace();
        before.top_panel->show_info_stack = true;
        before.top_panel->gold_text = "[ICON_GOLD]10";
        EconomicGoldInputMeasurement gold{
            {7, 64, 12, 20, 5, 1200, 800, {40, 4, 110, 30},
             scene.gold_source().top_panel_lua_sha256,
             scene.gold_source().top_panel_xml_sha256},
            &host, 1};
        const auto opened = scene.gold_click(live, before, gold, 1200, 800,
                                             50, 10, 50, 10);
        require(opened.consumed && opened.status ==
                    EconomicPopupQueueStatus::queued && opened.popup &&
                screens.focused_screen() ==
                    std::optional<std::string>{"EconomicOverview"},
            "source-owned measured GoldPerTurn opens focused popup");
        require(sources.active_popups(live) &&
                sources.active_popups(live)->events.front().context_id ==
                    "EconomicOverview",
            "Gold popup publishes through the composed provider");
        notification_active = true;
        require(!sources.active_popups(live) && !mux_status->blocker.empty(),
            "simultaneous notification and economic owners refuse collision");
        notification_active = false;
        auto current = live_frame(capture_live_session_ui_frame(live, 21, 7, sources),
                                  2, &host);
        const auto *published = current.snapshot.find<PopupFrame>();
        require(published && published->player_id == 7 &&
                published->owner_sequence == 5 &&
                published->events.size() == 1 &&
                published->events.front().event_id == 64 &&
                !capture_live_session_ui_frame(foreign, 22, 7, sources)
                    .find<PopupFrame>(),
            "LiveSessionUiSources publishes only this live session's event");
        const auto owner = scene.current_owner(live, current, 1200, 800);
        require(owner && owner->frame == 21 && owner->event_id == 64,
            "fresh UI frame carries the current popup owner");
        EconomicOverviewInputRects rects{*owner,
            {100, 100, 200, 30}, {350, 100, 200, 30}, {100, 700, 150, 30}};
        require(scene.bind_popup(live, current, rects, std::nullopt, blocker),
            "measured current popup controls bind to live frame");
        const auto fields = scene.fields(live, current, 1200, 800);
        require(fields.available && fields.tab == EconomicTab::general &&
                fields.player_id == 7 && fields.frame == 21 &&
                !fields.fields.empty(),
            "General fields project only from the current live frame");
        UiScreenCompositor compositor(resources, screens.registry());
        require(compositor.compose(current, screens, 1200, 800).hits.empty() &&
                compositor.compose(current, screens, 1200, 800, nullptr, &fields)
                    .hits.empty(),
            "EconomicOverview generic hits stay disabled with and without scene fields");
        const auto outside = scene.popup_click(live, current,
                                               900, 500, 900, 500);
        require(outside.consumed && outside.status ==
                    EconomicOverviewInputStatus::unavailable,
            "modal outside click is not delivered to the camera");
        const auto general = scene.popup_click(live, current,
                                               110, 110, 110, 110);
        require(general.consumed && general.status ==
                    EconomicOverviewInputStatus::performed &&
                scene.tab_view(live, current).general_visible,
            "selected General callback changes the live local screen state");
        const auto closed = scene.popup_click(live, current,
                                              110, 710, 110, 710);
        require(closed.consumed && closed.status ==
                    EconomicOverviewInputStatus::performed && processed == 1 &&
                scene.lifecycle().turn_timer_semaphore() == 0 &&
                !screens.visible("EconomicOverview") &&
                !capture_live_session_ui_frame(live, 22, 7, sources)
                    .find<PopupFrame>(),
            "Close performs UI dequeue, hidden context, timer and Processed");
        notification_active = true;
        require(sources.active_popups(live) &&
                sources.active_popups(live)->events.front().context_id == "TechPopup",
            "notification popup provider resumes after economic Processed");
        std::cout << "selected BNW EconomicOverview scene: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
