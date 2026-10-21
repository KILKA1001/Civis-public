#include "civ5/ui/bnw_add_popup_queue.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace civ5;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

runtime::LiveSession session() {
    saves::SavedSession source;
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

ui::BnwNotificationPopupInfo popup(std::uint64_t sequence = 1) {
    return {"BUTTONPOPUP_CHOOSETECH", 7, 42, 1, 0, false, false,
            "Tech", 4, 2, 3, 12, 12, sequence};
}

} // namespace

int main() {
    auto live = session();
    auto foreign = session();
    ui::BnwAddPopupQueue queue(live, 7);
    check(!queue.current(live) && !queue.unacknowledged(live),
          "Saved notification must not create an AddPopup event");

    check(queue.enqueue(foreign, popup()).status ==
              ui::BnwAddPopupQueueStatus::blocked &&
          !queue.current(live),
          "Matching player and turn from another session must be rejected");
    bool foreign_sink_rejected = false;
    try { queue.add_popup_sink(foreign)(popup()); }
    catch (const std::runtime_error &) { foreign_sink_rejected = true; }
    check(foreign_sink_rejected && !queue.current(live),
          "Foreign session AddPopup callback must throw without a side effect");
    auto stale = popup();
    stale.turn = 11;
    check(queue.enqueue(live, stale).status ==
              ui::BnwAddPopupQueueStatus::blocked &&
          !queue.current(live), "Stale popup must not enter the queue");
    auto wrong_player = popup();
    wrong_player.data1 = 8;
    check(queue.enqueue(live, wrong_player).status ==
              ui::BnwAddPopupQueueStatus::blocked,
          "Foreign player popup must not enter the queue");
    auto invalid = popup();
    invalid.button_popup_type = "BUTTONPOPUP_CHOOSEPOLICY";
    check(queue.enqueue(live, invalid).status ==
              ui::BnwAddPopupQueueStatus::blocked,
          "Unsupported popup type must not enter the queue");

    const auto sink = queue.add_popup_sink(live);
    sink(popup());
    check(queue.unacknowledged(live) && queue.current(live) &&
          queue.current(live)->owner_sequence == 1,
          "Accepted AddPopup must retain owned popup data");
    check(!queue.unacknowledged(foreign) &&
          !queue.acknowledge(foreign, 1) && !queue.close(foreign, 1),
          "Foreign session must not observe or complete a popup");
    check(!queue.acknowledge(live, 2) && queue.acknowledge(live, 1) &&
          !queue.acknowledge(live, 1) && !queue.unacknowledged(live) &&
          queue.current(live),
          "Ack must be exact and idempotently reject a second delivery");
    bool rejected = false;
    try { sink(popup(2)); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected && queue.current(live)->owner_sequence == 1,
          "Occupied sink must throw and preserve the first popup");
    check(!queue.close(live, 2) && queue.close(live, 1) &&
          !queue.current(live), "Close must require the acknowledged owner");
    check(queue.enqueue(live, popup()).status ==
              ui::BnwAddPopupQueueStatus::blocked,
          "Closed owner sequence must not be replayed");

    sink(popup(2));
    live.mutable_state().setup.game_turn = 13;
    check(!queue.current(live) && !queue.unacknowledged(live) &&
          !queue.acknowledge(live, 2) && !queue.close(live, 2),
          "Turn change must make a pending popup inaccessible");
    check(queue.enqueue(live, popup(3)).status ==
              ui::BnwAddPopupQueueStatus::blocked,
          "Old popup must be rejected after source turn advances");
    auto next_turn = popup(3);
    next_turn.turn = 13;
    check(queue.enqueue(live, next_turn).status ==
              ui::BnwAddPopupQueueStatus::accepted &&
          queue.unacknowledged(live)->owner_sequence == 3,
          "New turn may replace an inaccessible stale popup");
}
