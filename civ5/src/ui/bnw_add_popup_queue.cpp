#include "civ5/ui/bnw_add_popup_queue.hpp"
#include <climits>
#include <stdexcept>

namespace civ5::ui {

BnwAddPopupQueue::BnwAddPopupQueue(const runtime::LiveSession &session,
                                   std::int64_t active_player_id)
    : session_(&session), active_player_id_(active_player_id) {}

bool BnwAddPopupQueue::source_is_current(
    const runtime::LiveSession &source) const {
    if (session_ != &source || active_player_id_ < 0 ||
        source.state().setup.game_turn < 0 ||
        source.state().setup.game_turn > INT32_MAX)
        return false;
    const auto player = source.state().setup.players.find(active_player_id_);
    return player != source.state().setup.players.end() && player->second.alive;
}

BnwAddPopupQueueResult BnwAddPopupQueue::enqueue(
    const runtime::LiveSession &source, const BnwNotificationPopupInfo &popup) {
    if (!source_is_current(source) || popup.turn != source.state().setup.game_turn ||
        popup.data1 != active_player_id_)
        return {BnwAddPopupQueueStatus::blocked,
                "AddPopup source session, player or turn is stale"};
    if (popup.button_popup_type != "BUTTONPOPUP_CHOOSETECH" ||
        popup.owner_sequence == 0 || popup.notification_lookup < 0 ||
        popup.notification_turn < 0 || popup.notification_turn > popup.turn ||
        popup.flags != 0 || popup.option1 || popup.option2 ||
        popup.text.empty())
        return {BnwAddPopupQueueStatus::blocked,
                "AddPopup lacks a supported BNW popup identity"};
    if (current_ && current_->turn != source.state().setup.game_turn) {
        current_.reset();
        acknowledged_ = false;
    }
    if (current_ || popup.owner_sequence <= last_sequence_)
        return {BnwAddPopupQueueStatus::blocked,
                "AddPopup queue is occupied or owner sequence was already used"};
    current_ = popup;
    last_sequence_ = popup.owner_sequence;
    acknowledged_ = false;
    return {BnwAddPopupQueueStatus::accepted, {}};
}

std::optional<BnwNotificationPopupInfo> BnwAddPopupQueue::current(
    const runtime::LiveSession &source) const {
    if (!source_is_current(source) || !current_ ||
        current_->turn != source.state().setup.game_turn)
        return std::nullopt;
    return current_;
}

std::optional<BnwNotificationPopupInfo> BnwAddPopupQueue::unacknowledged(
    const runtime::LiveSession &source) const {
    return acknowledged_ ? std::nullopt : current(source);
}

bool BnwAddPopupQueue::acknowledge(const runtime::LiveSession &source,
                                   std::uint64_t owner_sequence) {
    if (!current(source) || current_->owner_sequence != owner_sequence ||
        acknowledged_)
        return false;
    acknowledged_ = true;
    return true;
}

bool BnwAddPopupQueue::close(const runtime::LiveSession &source,
                             std::uint64_t owner_sequence) {
    if (!current(source) || current_->owner_sequence != owner_sequence ||
        !acknowledged_)
        return false;
    current_.reset();
    acknowledged_ = false;
    return true;
}

std::function<void(const BnwNotificationPopupInfo &)>
BnwAddPopupQueue::add_popup_sink(const runtime::LiveSession &source) {
    auto *queue = this;
    auto *bound = &source;
    return [queue, bound](const BnwNotificationPopupInfo &popup) {
        const auto result = queue->enqueue(*bound, popup);
        if (result.status != BnwAddPopupQueueStatus::accepted)
            throw std::runtime_error(result.reason);
    };
}

} // namespace civ5::ui
