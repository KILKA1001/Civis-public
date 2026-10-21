#include "civ5/ui/bnw_selected_notification_backend.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace civ5;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

runtime::LiveSession live_session() {
    saves::SavedSession saved;
    saved.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    saved.snapshot.all_players.emplace();
    saved.snapshot.all_players->layout = saves::GameStateLayout::expansion2_version1;
    saves::SavedPlayerObservation player;
    player.saved_player_id = 7;
    player.administration.layout = saves::GameStateLayout::expansion2_version1;
    saves::PlayerAdministrationRecord ring{"notifications", 0, 0, 0,
                                           {}, {}, {}, {}};
    ring.fields = {{"has_notifications", 1}, {"version", 2}, {"m_ePlayer", 7},
                   {"m_iCurrentLookupIndex", 5},
                   {"m_iNotificationsBeginIndex", 0},
                   {"m_iNotificationsEndIndex", 1}};
    for (std::uint64_t index = 0; index < 100; ++index) {
        saves::PlayerAdministrationRecord row{"notification", index, 0, 0,
                                               {}, {}, {}, {}};
        row.fields = {{"m_eNotificationType_bits", 0x5acfa6f7},
                      {"m_iX", 2}, {"m_iY", 3}, {"m_iGameDataIndex", 42},
                      {"m_iExtraGameData", 1}, {"m_iTurn", 12},
                      {"m_iLookupIndex", 4}, {"m_bDismissed", 0},
                      {"m_ePlayerID", 7}};
        row.blobs = {{"m_strMessage", {'T', 'e', 'c', 'h'}},
                     {"m_strSummary", {'R', 'e', 's', 'e', 'a', 'r', 'c', 'h'}}};
        ring.children.push_back(std::move(row));
    }
    player.administration.blocks.push_back(std::move(ring));
    saved.snapshot.all_players->players.push_back(std::move(player));
    runtime::LiveSessionState state{saved.snapshot, runtime::ResearchContext{},
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
    return runtime::LiveSession::from_restored_bnw(std::move(saved), std::move(state));
}

ui::UiFrameSnapshot frame(const ui::BnwNotificationPopupOwner &owner,
                          std::int32_t turn = 12) {
    ui::UiFrameSnapshot result(1, 7, turn);
    ui::WorldViewState world;
    world.player_id = 7;
    if (const auto feed = owner.active_notifications())
        world.notifications = feed->events;
    return result.with(std::make_shared<const ui::WorldViewState>(std::move(world)));
}

ui::BnwNotificationActivationServices hooks(std::vector<std::string> &calls) {
    return {
        [&](std::int64_t lookup, std::uint32_t type, const std::string &text,
            std::int32_t, std::int32_t, std::int64_t, std::int64_t,
            std::int64_t player) {
            check(lookup == 4 && type == 0x5acfa6f7u && text == "Tech" &&
                  player == 7, "Notification activation arguments changed");
            calls.push_back("activate");
        },
        [&](std::int32_t, std::int32_t, std::int64_t marker) {
            check(marker == 5, "Minimap marker changed");
            calls.push_back("minimap");
        },
        [&](const ui::BnwNotificationPopupInfo &popup) {
            check(popup.data1 == 7 && popup.data2 == 42 && popup.data3 == 1,
                  "AddPopup source data changed");
            calls.push_back("popup");
        }};
}
} // namespace

int main() {
    auto live = live_session();
    ui::BnwNotificationPopupOwner owner(live, 7);
    runtime::ProductionProgressRules production_rules;
    production_rules.profile = content::TechnologyProfile::bnw;
    const auto visible = owner.active_notifications();
    check(visible && visible->events.size() == 1 &&
          visible->events[0].id == 4 && visible->events[0].text == "Tech" &&
          !owner.active_popups(),
          "Decoded notification must be visible without becoming a popup");
    ui::LiveSessionUiSources sources;
    ui::bind_bnw_notification_frame_sources(sources, owner);
    auto foreign = live_session();
    check(sources.active_notifications(live) &&
          !sources.active_notifications(foreign) &&
          !sources.active_popups(foreign) &&
          !sources.active_tech_popup(foreign),
          "Frame callbacks must reject a foreign session with matching player and turn");
    const auto command = ui::UiGameplayCommand::make("ActionInfoPanel",
        ui::WorldViewAction{ui::WorldViewActionKind::notification, 4});
    auto first_frame = frame(owner);
    ui::BnwSelectedNotificationBackend blocked(live, production_rules, {}, {},
                                                owner, {});
    check(dynamic_cast<ui::BnwCityProductionUiBackend *>(&blocked) != nullptr &&
          blocked.availability(command, first_frame).status ==
              ui::UiCommandStatus::unavailable &&
          blocked.execute(command, first_frame).status ==
              ui::UiCommandStatus::unavailable && !owner.active_popups(),
          "Default selected preview must preserve production type and block clicks");

    std::vector<std::string> calls;
    ui::BnwSelectedNotificationBackend backend(live, production_rules, {}, {},
                                               owner, hooks(calls));
    check(backend.availability(command, frame(owner, 11)).status ==
              ui::UiCommandStatus::unavailable &&
          backend.availability(ui::UiGameplayCommand::make("ActionInfoPanel",
              ui::WorldViewAction{ui::WorldViewActionKind::notification, 8}),
              first_frame).status == ui::UiCommandStatus::unavailable &&
          calls.empty(), "Stale turn and unknown lookup must be rejected");
    ui::WorldViewState stale_world;
    stale_world.player_id = 7;
    stale_world.notifications.push_back({4, "Old text", {}, true, std::nullopt});
    auto stale_text = first_frame.with(
        std::make_shared<const ui::WorldViewState>(std::move(stale_world)));
    check(backend.availability(command, stale_text).status ==
              ui::UiCommandStatus::unavailable && calls.empty(),
          "Changed notification text must invalidate a stale frame");
    check(backend.availability(command, first_frame).status ==
              ui::UiCommandStatus::ready &&
          backend.execute(command, first_frame).status ==
              ui::UiCommandStatus::performed &&
          calls == std::vector<std::string>({"activate", "minimap", "popup"}) &&
          owner.active_popups() && owner.active_tech_popup() &&
          sources.active_popups(live) && sources.active_tech_popup(live),
          "Current click must call Activate and publish only confirmed AddPopup");
    check(backend.execute(command, first_frame).status ==
              ui::UiCommandStatus::unavailable && calls.size() == 3,
          "Repeat click cannot replay a pending owner event");

    auto partial_live = live_session();
    ui::BnwNotificationPopupOwner partial_owner(partial_live, 7);
    auto partial_hooks = hooks(calls);
    partial_hooks.add_popup = [](const ui::BnwNotificationPopupInfo &) {
        throw std::runtime_error("AddPopup outcome uncertain");
    };
    ui::BnwSelectedNotificationBackend partial_backend(partial_live, production_rules,
        {}, {}, partial_owner, partial_hooks);
    const auto partial_frame = frame(partial_owner);
    check(partial_backend.execute(command, partial_frame).status ==
              ui::UiCommandStatus::unavailable && !partial_owner.active_popups() &&
          partial_backend.execute(command, partial_frame).status ==
              ui::UiCommandStatus::unavailable,
          "Partial AddPopup must be one-shot without a published success");
}
