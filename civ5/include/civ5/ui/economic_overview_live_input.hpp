#pragma once

#include "civ5/ui/economic_overview_active_tabs.hpp"
#include "civ5/ui/economic_overview_interaction.hpp"
#include "civ5/ui/live_ui_host.hpp"

namespace civ5::ui {

struct EconomicOverviewInputRects {
    EconomicTabOwner owner;
    OverviewRect general_button, happiness_button, close_button;
};

enum class EconomicOverviewInputAction { none, general, happiness, close };
enum class EconomicOverviewInputStatus { unavailable, performed, uncertain };

struct EconomicOverviewInputResult {
    // A current modal EconomicOverview consumes the whole pointer click,
    // including a release outside any control, so it cannot move the camera.
    bool consumed = false;
    EconomicOverviewInputAction action = EconomicOverviewInputAction::none;
    EconomicOverviewInputStatus status = EconomicOverviewInputStatus::unavailable;
    std::string blocker;
};

// Presenter-supplied physical-pixel rectangles are bound to one LiveUiFrame.
// This adapter owns no layout or native input: the caller must deliver both
// press and release from that same current frame and use consumed for routing.
class EconomicOverviewLiveInput {
  public:
    EconomicOverviewLiveInput(const runtime::LiveSession &session,
        EconomicOverviewLifecycle &lifecycle,
        EconomicOverviewActiveTabs &tabs,
        EconomicOverviewInteraction &interaction);

    bool bind(const runtime::LiveSession &session, const LiveUiFrame &frame,
        const EconomicOverviewInputRects &rects,
        std::optional<bool> happiness_enabled, std::string &blocker);

    EconomicOverviewInputResult click(const runtime::LiveSession &session,
        const LiveUiFrame &frame, std::int32_t press_x, std::int32_t press_y,
        std::int32_t release_x, std::int32_t release_y);

  private:
    const runtime::LiveSession *session_;
    EconomicOverviewLifecycle *lifecycle_;
    EconomicOverviewActiveTabs *tabs_;
    EconomicOverviewInteraction *interaction_;
    std::optional<EconomicOverviewInputRects> rects_;
    const void *frame_owner_ = nullptr;
    std::uint64_t frame_sequence_ = 0;
    std::uint64_t snapshot_frame_ = 0;
};

} // namespace civ5::ui
