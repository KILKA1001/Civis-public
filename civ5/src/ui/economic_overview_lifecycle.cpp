#include "civ5/ui/economic_overview_lifecycle.hpp"

namespace civ5::ui {

EconomicOverviewLifecycle::EconomicOverviewLifecycle(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    UiScreenController &screens, EconomicCloseSourceProof source,
    EconomicPopupProcessedSink processed_sink)
    : session_(&session), active_player_id_(active_player_id),
      screens_(&screens), source_(std::move(source)),
      processed_sink_(std::move(processed_sink)) {}

bool EconomicOverviewLifecycle::owns(
    const runtime::LiveSession &source_session) const {
    if (session_ != &source_session || active_player_id_ < 0) return false;
    const auto &setup = source_session.state().setup;
    const auto player = setup.players.find(active_player_id_);
    return player != setup.players.end() && player->second.alive &&
           setup.game_turn >= 0 && setup.game_turn <= INT32_MAX;
}

EconomicPopupQueueResult EconomicOverviewLifecycle::queue(
    const runtime::LiveSession &source_session, const UiFrameSnapshot &frame,
    const EconomicPopupRequest &request) {
    if (!owns(source_session) || !source_.verified ||
        source_.lua_sha256.empty() || source_.xml_path.empty() ||
        request.button_popup_type != "BUTTONPOPUP_ECONOMIC_OVERVIEW" ||
        request.player_id != active_player_id_ || request.event_id < 0 ||
        request.turn != source_session.state().setup.game_turn ||
        !frame.active_player_id() || !frame.turn() || !frame.frame() ||
        *frame.active_player_id() != request.player_id ||
        *frame.turn() != request.turn || !request.owner_sequence ||
        request.owner_sequence <= last_sequence_)
        return {EconomicPopupQueueStatus::blocked,
                "EconomicOverview source session, popup type or owner identity is invalid"};
    if (current_ || turn_timer_semaphore_ || screens_->visible("EconomicOverview"))
        return {EconomicPopupQueueStatus::blocked,
                request.data1 == 1
                    ? "Visible Data1=1 toggle requires the existing popup close owner"
                    : "EconomicOverview popup queue is already occupied"};
    try {
        // Reserve the synchronous Processed journal before any visible effect.
        processed_events_.reserve(processed_events_.size() + 1);
        Queued next{request, request.data1 == 1
            ? EconomicPopupPriority::in_game_utmost
            : EconomicPopupPriority::economic_overview, false};
        last_sequence_ = request.owner_sequence;
        const auto opened = screens_->dispatch(
            UiOpenScreen{"EconomicOverview"}, frame);
        if (opened.status != UiCommandStatus::navigation ||
            screens_->focused_screen() !=
                std::optional<std::string>{"EconomicOverview"})
            return {EconomicPopupQueueStatus::uncertain,
                    "EconomicOverview local UIManager queue did not confirm focus"};
        current_ = std::move(next);
        ++turn_timer_semaphore_;
        return {EconomicPopupQueueStatus::queued, {}};
    } catch (const std::exception &) {
        return {EconomicPopupQueueStatus::uncertain,
                "EconomicOverview UI queue effect is uncertain; sequence cannot replay"};
    }
}

std::optional<PopupFrame> EconomicOverviewLifecycle::active_popups(
    const runtime::LiveSession &source_session) const {
    if (!owns(source_session) || !current_ ||
        current_->request.turn != source_session.state().setup.game_turn ||
        turn_timer_semaphore_ != 1 || !screens_->visible("EconomicOverview"))
        return std::nullopt;
    PopupEvent event;
    event.context_id = "EconomicOverview";
    event.event_id = current_->request.event_id;
    return PopupFrame{active_player_id_, {std::move(event)},
                      current_->request.turn,
                      current_->request.owner_sequence};
}

std::optional<EconomicTabOwner> EconomicOverviewLifecycle::current_owner(
    const runtime::LiveSession &source_session, const UiFrameSnapshot &frame,
    std::int32_t viewport_width, std::int32_t viewport_height) const {
    if (!active_popups(source_session) || !frame.active_player_id() ||
        !frame.turn() || !frame.frame() ||
        *frame.active_player_id() != active_player_id_ ||
        *frame.turn() != current_->request.turn ||
        viewport_width <= 0 || viewport_height <= 0) return std::nullopt;
    return EconomicTabOwner{active_player_id_, current_->request.event_id,
        current_->request.turn, frame.frame(),
        current_->request.owner_sequence, source_.lua_sha256,
        source_.xml_path, viewport_width, viewport_height};
}

bool EconomicOverviewLifecycle::same_current(const EconomicTabOwner &owner) const {
    return current_ && owner.player_id == active_player_id_ &&
           owner.event_id == current_->request.event_id &&
           owner.turn == current_->request.turn &&
           owner.owner_sequence == current_->request.owner_sequence &&
           owner.lua_sha256 == source_.lua_sha256 &&
           owner.xml_path == source_.xml_path;
}

std::optional<EconomicTabOwner> EconomicOverviewLifecycle::claim_close(
    const runtime::LiveSession &source_session, const UiFrameSnapshot &frame,
    std::int32_t viewport_width, std::int32_t viewport_height) {
    const auto owner = current_owner(source_session, frame,
                                     viewport_width, viewport_height);
    if (!owner || !same_current(*owner) || current_->close_claimed)
        return std::nullopt;
    current_->close_claimed = true;
    return owner;
}

bool EconomicOverviewLifecycle::decrement_turn_timer(
    const runtime::LiveSession &source_session,
    const EconomicTabOwner &owner) {
    if (!owns(source_session) || !same_current(owner) ||
        !current_->close_claimed || turn_timer_semaphore_ != 1 ||
        screens_->visible("EconomicOverview")) return false;
    --turn_timer_semaphore_;
    return true;
}

bool EconomicOverviewLifecycle::publish_processed(
    const runtime::LiveSession &source_session,
    const EconomicTabOwner &owner) {
    if (!owns(source_session) || !same_current(owner) ||
        !current_->close_claimed || turn_timer_semaphore_ != 0 ||
        screens_->visible("EconomicOverview") ||
        processed_events_.size() == processed_events_.capacity()) return false;
    processed_events_.push_back({active_player_id_, owner.event_id,
        owner.turn, owner.owner_sequence, 0});
    if (processed_sink_) processed_sink_(processed_events_.back());
    current_.reset();
    return true;
}

EconomicCloseService EconomicOverviewLifecycle::close_service(
    const runtime::LiveSession &source_session, UiFrameSnapshot frame,
    std::int32_t viewport_width, std::int32_t viewport_height) {
    if (!current_owner(source_session, frame, viewport_width, viewport_height))
        return {};
    auto *bound_session = &source_session;
    EconomicCloseOwnerServices services;
    services.current_owner = [this, bound_session, frame, viewport_width,
                              viewport_height] {
        return claim_close(*bound_session, frame, viewport_width,
                           viewport_height);
    };
    services.decrement_turn_timer = [this, bound_session](const auto &owner) {
        return decrement_turn_timer(*bound_session, owner);
    };
    services.publish_processed = [this, bound_session](const auto &owner) {
        return publish_processed(*bound_session, owner);
    };
    return make_economic_close_service(*screens_, std::move(frame),
                                       std::move(services));
}

std::optional<EconomicPopupPriority>
EconomicOverviewLifecycle::current_priority() const {
    return current_ ? std::optional{current_->priority} : std::nullopt;
}

} // namespace civ5::ui
