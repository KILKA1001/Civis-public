#pragma once

#include "civ5/ui/client_window_input.hpp"
#include "civ5/runtime/bnw_unit_cycle.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/runtime/bnw_city_dirty_event.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/terrain/camera_controller.hpp"
#include <array>
#include <string>

namespace civ5::ui {
class BnwPlotYieldRefreshOwnedUiStorage;
class SelectedBnwPreviewHost;
class BnwClientSelectionControl;

// Explicit Civis manual input. This is not an engine/automatic-cycle receipt.
struct BnwManualCycleSelectionCommand {
    runtime::BnwUnitCycleDirection direction=runtime::BnwUnitCycleDirection::forward;
    runtime::BnwUnitCycleFilter filter=runtime::BnwUnitCycleFilter::all;
};

struct BnwClientSelection {
    std::optional<std::int64_t> unit, city;
    // Publication of explicit canonical picking input; never inferred from unit.
    std::optional<std::int64_t> plot={};
    bool operator==(const BnwClientSelection &) const = default;
};
struct BnwClientFrameIdentity {
    std::uint64_t owner = 0, revision = 0, frame = 0;
    std::string session;
    std::int64_t player = -1, team = -1;
    std::int32_t turn = -1;
    std::array<std::uint8_t, 16> map_guid{};
    std::uint64_t selection_revision=0,ui_source_revision=0;
    bool operator==(const BnwClientFrameIdentity &) const = default;
};
// Published in UiFrameSnapshot as a typed payload. The UI host and presenter
// share this exact stamp; a UI sequence alone cannot identify a world revision.
struct BnwClientFrameStamp {
    BnwClientFrameIdentity identity;
    BnwClientSelection selection;
    ClientViewport viewport;
};
struct BnwClientFrame {
    BnwClientFrameStamp stamp;
    saves::SavedSessionSnapshot world;
    WorldViewMapVisibility visibility;
    terrain::CameraControllerState camera;
    UiFrameSnapshot ui;
};

// Single UI-thread owner of publication. Renderers consume frame.world and
// frame.visibility, never independently restore the original save. Arbitrary
// external writes/restores must call notify_world_changed before rendering or
// dispatch; typed source stores carry a separate owned revision and invalidate
// frames immediately, before dirty queue consumption. The borrowed constructor
// still requires session/rules/providers to outlive it; from_selected_host pins
// the actual Host and shares canonical selection with its actual UI receiver.
class BnwClientFrameOwner {
  public:
    BnwClientFrameOwner(const runtime::LiveSession &session, std::string session_identity,
                        std::int64_t active_player, LiveSessionUiSources sources = {});
    ~BnwClientFrameOwner();
    BnwClientFrameOwner(const BnwClientFrameOwner &) = delete;
    BnwClientFrameOwner &operator=(const BnwClientFrameOwner &) = delete;

    static std::unique_ptr<BnwClientFrameOwner> from_selected_host(
        std::shared_ptr<const SelectedBnwPreviewHost>, LiveSessionUiSources sources={});
    std::shared_ptr<LiveUiHost> create_selected_ui_host(
        UiContextRegistry,UiGameplayBackend*,LiveUiSources sources={});
    std::vector<runtime::BnwCityDirtyEvent> take_city_dirty_events();
    // Actual owned new-client flags and ordered delivery; no render ack/reset.
    BnwClientUnitDirtySnapshot unit_dirty_state()const;
    std::vector<BnwClientUnitDirtyEvent> take_unit_dirty_events();
    void select(BnwClientSelection selection);
    // Direct input uses the currently published frame. Renderer picking must
    // pass the exact frame it picked; stale frames refuse BEFORE input stores.
    void select_plot(std::optional<std::int64_t> plot);
    void select_plot(std::optional<std::int64_t> plot,
                     std::shared_ptr<const BnwClientFrame> picked_frame);
    void notify_world_changed();
    std::shared_ptr<const BnwClientFrame> capture(ClientViewport viewport,
                                               terrain::CameraControllerState camera);
    std::shared_ptr<const BnwClientFrame> current_frame() const;
    bool is_current(const std::shared_ptr<const BnwClientFrame> &frame) const;
    LiveUiHost::FrameProvider ui_provider() const;
    UiCommandResult dispatch(LiveUiHost &host, const LiveUiFrame &ui_frame,
                             const std::shared_ptr<const BnwClientFrame> &frame,
                             const UiCommand &command);
    // The caller swaps only when true. A callback that changes the world or
    // selection invalidates the whole composed image, including its UI.
    bool present(const std::shared_ptr<const BnwClientFrame> &frame,
                 const std::function<void(const BnwClientFrame &)> &compose) const;

  private:
    friend class BnwClientSceneFrame;
    friend class BnwOwnedCameraGuiScreen;
    friend class BnwAlphaProgressionBackend;
    friend class UiScreenCompositor;
    void require_same_gui_ui_host(const std::shared_ptr<LiveUiHost>&)const;
    friend class BnwPlotYieldRefreshUiAdapter;
    // Private retained old-publication identity during known plot stores.
    // This is never a public currentness/render/input grant.
    void require_plot_tail_publication_identity(
        const std::shared_ptr<const BnwClientFrame>&,
        const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>&,
        const std::shared_ptr<const SelectedBnwPreviewHost>&,
        const std::shared_ptr<LiveUiHost>&,
        const std::shared_ptr<const runtime::LiveSessionState>&,
        std::uint64_t generation,std::uint64_t expected_world_revision)const;
    BnwClientFrameOwner(std::shared_ptr<const SelectedBnwPreviewHost>,LiveSessionUiSources);
    void register_created_selected_ui_host(const std::shared_ptr<LiveUiHost>&);
    friend class BnwPlotYieldRefreshOwnedUiContext;
    void retain_owned_ui_sources(std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>);
    struct Publication;
    friend class BnwWorldViewTitleFontIssuer;
    UiCommandResult cycle_selection(const std::shared_ptr<const BnwClientFrame>&,
                                    const BnwManualCycleSelectionCommand&);
    void validate_selection(BnwClientSelection selection) const;
    const runtime::LiveSession &session_;
    std::int64_t player_;
    LiveSessionUiSources sources_;
    std::shared_ptr<Publication> publication_;
};

} // namespace civ5::ui
