#pragma once

#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/economic_overview_close_service.hpp"

namespace civ5::ui {

enum class EconomicPopupPriority { economic_overview, in_game_utmost };
enum class EconomicPopupQueueStatus { queued, blocked, uncertain };

struct EconomicPopupRequest {
    std::string button_popup_type;
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1, data1 = 0;
    std::uint64_t owner_sequence = 0;
};

struct EconomicPopupQueueResult {
    EconomicPopupQueueStatus status = EconomicPopupQueueStatus::blocked;
    std::string blocker;
};

struct EconomicPopupProcessedEvent {
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    // BNW Lua calls Processed(BUTTONPOPUP_ECONOMIC_OVERVIEW, 0).
    std::int32_t processed_result = 0;
};

using EconomicPopupProcessedSink = std::function<void(
    const EconomicPopupProcessedEvent &)>;

// A Civis-owned, capacity-one local popup queue. It accepts only a current
// typed SerialEventGameMessagePopup from the bound live session. A save's
// notification/popup slots are never an opening event. UIManager navigation,
// visibility, semaphore and Processed journal all mutate this owner directly.
// This is Civis UI state, not proof of the original native UIManager ABI.
class EconomicOverviewLifecycle {
  public:
    EconomicOverviewLifecycle(const runtime::LiveSession &session,
        std::int64_t active_player_id, UiScreenController &screens,
        EconomicCloseSourceProof source,
        EconomicPopupProcessedSink processed_sink = {});

    EconomicPopupQueueResult queue(const runtime::LiveSession &source_session,
        const UiFrameSnapshot &frame, const EconomicPopupRequest &request);
    std::optional<PopupFrame> active_popups(
        const runtime::LiveSession &source_session) const;
    std::optional<EconomicTabOwner> current_owner(
        const runtime::LiveSession &source_session,
        const UiFrameSnapshot &frame,
        std::int32_t viewport_width, std::int32_t viewport_height) const;
    // Captures the current source frame. All borrows must outlive the returned
    // callback. A partial close is one-shot even if a later frame rebinds.
    EconomicCloseService close_service(const runtime::LiveSession &source_session,
        UiFrameSnapshot frame,
        std::int32_t viewport_width, std::int32_t viewport_height);

    std::uint32_t turn_timer_semaphore() const { return turn_timer_semaphore_; }
    const std::vector<EconomicPopupProcessedEvent> &processed_events() const {
        return processed_events_;
    }
    std::optional<EconomicPopupPriority> current_priority() const;

  private:
    struct Queued {
        EconomicPopupRequest request;
        EconomicPopupPriority priority = EconomicPopupPriority::economic_overview;
        bool close_claimed = false;
    };
    bool owns(const runtime::LiveSession &source_session) const;
    bool same_current(const EconomicTabOwner &owner) const;
    std::optional<EconomicTabOwner> claim_close(
        const runtime::LiveSession &source_session,
        const UiFrameSnapshot &frame,
        std::int32_t viewport_width, std::int32_t viewport_height);
    bool decrement_turn_timer(const runtime::LiveSession &source_session,
                              const EconomicTabOwner &owner);
    bool publish_processed(const runtime::LiveSession &source_session,
                           const EconomicTabOwner &owner);

    const runtime::LiveSession *session_;
    std::int64_t active_player_id_;
    UiScreenController *screens_;
    EconomicCloseSourceProof source_;
    EconomicPopupProcessedSink processed_sink_;
    std::optional<Queued> current_;
    std::uint64_t last_sequence_ = 0;
    std::uint32_t turn_timer_semaphore_ = 0;
    std::vector<EconomicPopupProcessedEvent> processed_events_;
};

} // namespace civ5::ui
