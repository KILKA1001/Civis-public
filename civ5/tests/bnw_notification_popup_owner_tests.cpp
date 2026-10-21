#include "civ5/ui/bnw_notification_popup_owner.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace civ5;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

saves::PlayerAdministrationRecord row(std::uint64_t index,
                                       std::int64_t lookup = 4,
                                       std::int64_t owner = 7,
                                       bool dismissed = false) {
    saves::PlayerAdministrationRecord result{"notification", 0, 0, 0,
                                             {}, {}, {}, {}};
    result.index = index;
    result.fields = {
        {"m_eNotificationType_bits", 0x5acfa6f7}, {"m_iX", 2},
        {"m_iY", 3}, {"m_iGameDataIndex", 42},
        {"m_iExtraGameData", 1}, {"m_iTurn", 12},
        {"m_iLookupIndex", lookup}, {"m_bDismissed", dismissed},
        {"m_ePlayerID", owner}};
    result.blobs = {{"m_strMessage", {'T', 'e', 'c', 'h'}},
                    {"m_strSummary", {'R', 'e', 's', 'e', 'a', 'r', 'c', 'h'}}};
    return result;
}

saves::SavedSession saved() {
    saves::SavedSession source;
    source.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    source.snapshot.all_players.emplace();
    source.snapshot.all_players->layout = saves::GameStateLayout::expansion2_version1;
    saves::SavedPlayerObservation player;
    player.saved_player_id = 7;
    player.administration.layout = saves::GameStateLayout::expansion2_version1;
    saves::PlayerAdministrationRecord ring{"notifications", 0, 0, 0,
                                           {}, {}, {}, {}};
    ring.fields = {{"has_notifications", 1}, {"version", 2}, {"m_ePlayer", 7},
                   {"m_iCurrentLookupIndex", 5},
                   {"m_iNotificationsBeginIndex", 0},
                   {"m_iNotificationsEndIndex", 1}};
    for (std::uint64_t index = 0; index < 100; ++index)
        ring.children.push_back(row(index));
    player.administration.blocks.push_back(std::move(ring));
    source.snapshot.all_players->players.push_back(std::move(player));
    return source;
}

runtime::LiveSession session() {
    auto source = saved();
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
    return runtime::LiveSession::from_restored_bnw(std::move(source), std::move(state));
}

saves::PlayerAdministrationRecord &ring(runtime::LiveSession &session) {
    return session.mutable_state().world.all_players->players.front().administration.blocks.front();
}

ui::BnwNotificationActivationServices services(std::vector<std::string> &calls) {
    return {
        [&](std::int64_t lookup, std::uint32_t type, const std::string &message,
            std::int32_t x, std::int32_t y, std::int64_t data,
            std::int64_t extra, std::int64_t player) {
            check(lookup == 4 && type == 0x5acfa6f7u && message == "Tech" &&
                  x == 2 && y == 3 && data == 42 && extra == 1 && player == 7,
                  "ActivateNotification arguments differ from the decoded slot");
            calls.push_back("activate");
        },
        [&](std::int32_t x, std::int32_t y, std::int64_t marker) {
            check(x == 2 && y == 3 && marker == 5, "Minimap marker differs from SDK");
            calls.push_back("minimap");
        },
        [&](const ui::BnwNotificationPopupInfo &popup) {
            check(popup.button_popup_type == "BUTTONPOPUP_CHOOSETECH" &&
                  popup.data1 == 7 && popup.data2 == 42 && popup.data3 == 1 &&
                  popup.flags == 0 && !popup.option1 && !popup.option2 &&
                  popup.text == "Tech" && popup.notification_lookup == 4,
                  "AddPopup fields differ from SDK CvNotifications::Activate");
            calls.push_back("popup");
        }};
}
} // namespace

int main() {
    auto live = session();
    ui::BnwNotificationPopupOwner owner(live, 7);
    std::vector<std::string> calls;
    const auto hooks = services(calls);
    check(!owner.active_popups() && !owner.active_tech_popup() &&
          !owner.queued_popup(), "Saved notification must not open a popup");
    check(owner.activate_tech(8, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Unknown lookup must not call any UI hook");
    check(owner.activate_tech(4, 11, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Stale turn must not call any UI hook");
    check(owner.activate_tech(4, 12, {}).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Missing mandatory UI hooks must block activation");
    check(owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::published &&
          calls == std::vector<std::string>({"activate", "minimap", "popup"}),
          "Only explicit activation may publish the ordered SDK calls");
    const auto feed = owner.active_popups();
    const auto tech = owner.active_tech_popup();
    check(feed && feed->player_id == 7 && feed->turn == 12 &&
          feed->owner_sequence == 1 && feed->events.size() == 1 &&
          feed->events[0].context_id == "TechPopup" &&
          feed->events[0].event_id == 1 && tech && tech->token == 1 &&
          tech->mode == ui::ProgressionTechPopupMode::research,
          "Confirmed AddPopup must publish the same typed owner identity");
    check(owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.size() == 3,
          "A pending popup cannot be emitted twice");
    check(!owner.close(99) && owner.close(1) && !owner.active_popups() &&
          owner.activate_tech(4, 12, hooks).status ==
              ui::BnwNotificationActivationStatus::blocked,
          "Closing the popup must not replay the same lookup");

    auto dismissed = session();
    ring(dismissed).children[0].fields[7].value = 1;
    ui::BnwNotificationPopupOwner dismissed_owner(dismissed, 7);
    calls.clear();
    check(dismissed_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Dismissed notification cannot open a popup");

    auto duplicate = session();
    auto &duplicate_ring = ring(duplicate);
    duplicate_ring.fields[5].value = 2;
    duplicate_ring.children[1] = row(1, 4);
    ui::BnwNotificationPopupOwner duplicate_owner(duplicate, 7);
    check(duplicate_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Ambiguous lookup cannot open a popup");

    auto wrong = session();
    ring(wrong).children[0].fields[8].value = 8;
    ui::BnwNotificationPopupOwner wrong_owner(wrong, 7);
    check(wrong_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Notification owned by another player cannot open a popup");

    auto wrong_type = session();
    ring(wrong_type).children[0].fields[0].value = 0x7ee00dfd;
    ui::BnwNotificationPopupOwner wrong_type_owner(wrong_type, 7);
    check(wrong_type_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "A different authored notification type cannot become CHOOSETECH");

    auto legacy = session();
    ring(legacy).fields[1].value = 1;
    ui::BnwNotificationPopupOwner legacy_owner(legacy, 7);
    check(legacy_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::blocked && calls.empty(),
          "Legacy notification indices need explicit source translation");

    auto uncertain = session();
    ui::BnwNotificationPopupOwner uncertain_owner(uncertain, 7);
    auto throwing = services(calls);
    throwing.add_popup = [](const ui::BnwNotificationPopupInfo &) {
        throw std::runtime_error("device failed after AddPopup");
    };
    check(uncertain_owner.activate_tech(4, 12, throwing).status ==
          ui::BnwNotificationActivationStatus::uncertain &&
          !uncertain_owner.active_popups() &&
          uncertain_owner.activate_tech(4, 12, hooks).status ==
              ui::BnwNotificationActivationStatus::blocked,
          "Partial callback outcome must remain unpublished and one-shot");

    auto stale = session();
    ui::BnwNotificationPopupOwner stale_owner(stale, 7);
    check(stale_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::published,
          "Fresh notification activation failed");
    stale.mutable_state().setup.game_turn = 13;
    check(!stale_owner.active_popups() && !stale_owner.active_tech_popup(),
          "Popup from a prior turn must not enter the current frame");

    auto changed = session();
    ui::BnwNotificationPopupOwner changed_owner(changed, 7);
    check(changed_owner.activate_tech(4, 12, hooks).status ==
          ui::BnwNotificationActivationStatus::published,
          "Fresh notification activation failed");
    ring(changed).children[0].fields[3].value = 99;
    check(!changed_owner.active_popups() && !changed_owner.active_tech_popup(),
          "Changed source notification must invalidate the owned popup view");
}
