#include "civ5/ui/economic_overview_scene.hpp"
#include <algorithm>

namespace civ5::ui {

OverviewSource EconomicOverviewScene::selected_overview(
    const assets::ResourceMountTable &resources,
    const UiContextRegistry &registry) {
    try {
        const auto sources = read_overview_sources(resources, registry);
        const auto found = std::find_if(sources.begin(), sources.end(),
            [](const OverviewSource &source) {
                return source.context_id == "EconomicOverview";
            });
        if (found != sources.end()) return *found;
    } catch (const std::exception &) {
        // The source inspectors below retain their own typed blockers.
    }
    return {};
}

EconomicOverviewScene::EconomicOverviewScene(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    UiScreenController &screens, const assets::ResourceMountTable &resources,
    EconomicPopupProcessedSink processed_sink)
    : session_(&session), overview_(selected_overview(resources, screens.registry())),
      gold_source_(inspect_economic_gold_source(resources)),
      tab_source_(inspect_economic_tab_source(resources, overview_)),
      close_source_(inspect_economic_close_source(resources, overview_)),
      field_source_(inspect_economic_field_source(resources, tab_source_)),
      lifecycle_(session, active_player_id, screens, close_source_,
                 std::move(processed_sink)),
      open_bridge_(session, active_player_id, lifecycle_, gold_source_),
      interaction_(session, active_player_id, lifecycle_,
                   gold_source_, close_source_),
      tabs_(lifecycle_, tab_source_, close_source_),
      fields_(lifecycle_, tabs_, field_source_),
      gold_input_(session, screens, open_bridge_, lifecycle_),
      popup_input_(session, lifecycle_, tabs_, interaction_) {}

bool EconomicOverviewScene::install_popup_feed(LiveSessionUiSources &sources,
    std::string &blocker) const {
    if (sources.active_popups) {
        blocker = "An active popup event owner already supplies the live frame";
        return false;
    }
    if (!gold_source_.verified || !tab_source_.verified ||
        !close_source_.verified ||
        gold_source_.economic_lua_sha256 != close_source_.lua_sha256 ||
        tab_source_.lua_sha256 != close_source_.lua_sha256) {
        blocker = "Selected EconomicOverview source chain is incomplete";
        return false;
    }
    sources.active_popups = make_economic_overview_popup_feed_provider(
        *session_, lifecycle_);
    blocker.clear();
    return true;
}

EconomicGoldInputResult EconomicOverviewScene::gold_click(
    const runtime::LiveSession &session, const LiveUiFrame &frame,
    const EconomicGoldInputMeasurement &measurement,
    std::int32_t viewport_width, std::int32_t viewport_height,
    std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    return gold_input_.click(session, frame, measurement,
        viewport_width, viewport_height, press_x, press_y,
        release_x, release_y);
}

std::optional<EconomicTabOwner> EconomicOverviewScene::current_owner(
    const runtime::LiveSession &session, const LiveUiFrame &frame,
    std::int32_t viewport_width, std::int32_t viewport_height) const {
    if (&session != session_ || !frame.owner || !frame.sequence)
        return std::nullopt;
    return lifecycle_.current_owner(session, frame.snapshot,
        viewport_width, viewport_height);
}

bool EconomicOverviewScene::bind_popup(const runtime::LiveSession &session,
    const LiveUiFrame &frame, const EconomicOverviewInputRects &rects,
    std::optional<bool> happiness_enabled, std::string &blocker) {
    return popup_input_.bind(session, frame, rects,
                             happiness_enabled, blocker);
}

EconomicOverviewInputResult EconomicOverviewScene::popup_click(
    const runtime::LiveSession &session, const LiveUiFrame &frame,
    std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    return popup_input_.click(session, frame,
        press_x, press_y, release_x, release_y);
}

EconomicBoundFields EconomicOverviewScene::fields(
    const runtime::LiveSession &session, const LiveUiFrame &frame,
    std::int32_t viewport_width, std::int32_t viewport_height) const {
    return fields_.capture(session, frame.snapshot,
        viewport_width, viewport_height);
}

EconomicTabView EconomicOverviewScene::tab_view(
    const runtime::LiveSession &session, const LiveUiFrame &frame) const {
    return tabs_.view(session, frame.snapshot);
}

} // namespace civ5::ui
