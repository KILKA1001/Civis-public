#include "civ5/ui/economic_overview_open_bridge.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <climits>

namespace civ5::ui {
namespace {

struct SelectedText {
    std::string text, path, sha256;
};

std::optional<SelectedText> selected_text(
    const assets::ResourceMountTable &resources, const char *key) {
    const auto resolved = resources.resolve(assets::AssetKey(key));
    const auto *handle = std::get_if<assets::AssetHandle>(&resolved.asset);
    if (!handle) return std::nullopt;
    const auto bytes = handle->read_bytes(1024 * 1024);
    const auto &provenance = handle->candidate().provenance;
    const auto path = provenance.archive_relative_path
        ? *provenance.archive_relative_path + ":" + provenance.source_relative_path
        : provenance.source_relative_path;
    return SelectedText{std::string(bytes.begin(), bytes.end()), path,
                        runtime::identify_bytes(bytes).sha256};
}

bool ordered(std::string_view source, std::initializer_list<std::string_view> tokens) {
    std::size_t cursor = 0;
    for (const auto token : tokens) {
        const auto found = source.find(token, cursor);
        if (found == std::string_view::npos) return false;
        cursor = found + token.size();
    }
    return true;
}

bool contains(const OverviewRect &r, std::int32_t x, std::int32_t y) {
    return x >= r.x && y >= r.y &&
        std::int64_t(x) < std::int64_t(r.x) + r.width &&
        std::int64_t(y) < std::int64_t(r.y) + r.height;
}

} // namespace

EconomicGoldSourceProof inspect_economic_gold_source(
    const assets::ResourceMountTable &resources) {
    EconomicGoldSourceProof proof;
    try {
        const auto top_lua = selected_text(resources, "Assets/UI/InGame/TopPanel.lua");
        const auto top_xml = selected_text(resources, "Assets/UI/InGame/TopPanel.xml");
        const auto economic_lua = selected_text(resources,
            "Assets/UI/InGame/Popups/EconomicOverview.lua");
        if (!top_lua || !top_xml || !economic_lua) {
            proof.blocker = "Selected BNW TopPanel or EconomicOverview source is missing or ambiguous";
            return proof;
        }
        const auto layout = TopPanelLayout::from_xml(
            content::read_xml_metadata(top_xml->text));
        const auto gold_count = std::count_if(layout.info_stack.children.begin(),
            layout.info_stack.children.end(), [](const TopPanelElement &child) {
                return child.name == "TextButton" &&
                       child.attribute("ID") == "GoldPerTurn";
            });
        if (gold_count != 1 || !ordered(top_lua->text, {
            "function OnGoldClicked()",
            "Events.SerialEventGameMessagePopup( { Type = ButtonPopupTypes.BUTTONPOPUP_ECONOMIC_OVERVIEW } );",
            "Controls.GoldPerTurn:RegisterCallback( Mouse.eLClick, OnGoldClicked );"}) ||
            !ordered(economic_lua->text, {
                "function OnPopup( popupInfo )",
                "ButtonPopupTypes.BUTTONPOPUP_ECONOMIC_OVERVIEW",
                "UIManager:QueuePopup( ContextPtr, PopupPriority.EconOverview );",
                "Events.SerialEventGameMessagePopup.Add( OnPopup );"})) {
            proof.blocker = "Selected BNW gold callback, XML control or popup listener differs from verified source";
            return proof;
        }
        proof.verified = true;
        proof.top_panel_lua_path = top_lua->path;
        proof.top_panel_lua_sha256 = top_lua->sha256;
        proof.top_panel_xml_path = top_xml->path;
        proof.top_panel_xml_sha256 = top_xml->sha256;
        proof.economic_lua_path = economic_lua->path;
        proof.economic_lua_sha256 = economic_lua->sha256;
    } catch (const std::exception &) {
        proof.blocker = "Selected BNW gold source could not be verified";
    }
    return proof;
}

EconomicOverviewOpenBridge::EconomicOverviewOpenBridge(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    EconomicOverviewLifecycle &lifecycle, EconomicGoldSourceProof source)
    : session_(&session), active_player_id_(active_player_id),
      lifecycle_(&lifecycle), source_(std::move(source)) {}

EconomicGoldClickResult EconomicOverviewOpenBridge::click(
    const runtime::LiveSession &source_session, const UiFrameSnapshot &frame,
    const EconomicGoldClick &measurement, std::int32_t x, std::int32_t y) {
    const auto &r = measurement.gold_button;
    if (!source_.verified || source_.top_panel_lua_sha256.empty() ||
        source_.top_panel_xml_sha256.empty() ||
        source_.economic_lua_sha256.empty() ||
        session_ != &source_session || active_player_id_ < 0 ||
        measurement.player_id != active_player_id_ ||
        measurement.turn < 0 ||
        measurement.turn != source_session.state().setup.game_turn ||
        !frame.frame() || frame.frame() != measurement.frame ||
        frame.active_player_id() != measurement.player_id ||
        frame.turn() != measurement.turn ||
        measurement.top_panel_lua_sha256 != source_.top_panel_lua_sha256 ||
        measurement.top_panel_xml_sha256 != source_.top_panel_xml_sha256 ||
        r.width <= 0 || r.height <= 0 || r.x < 0 || r.y < 0 ||
        measurement.viewport_width <= 0 || measurement.viewport_height <= 0 ||
        std::int64_t(r.x) + r.width > measurement.viewport_width ||
        std::int64_t(r.y) + r.height > measurement.viewport_height)
        return {false, EconomicPopupQueueStatus::blocked,
                "GoldPerTurn source, owner frame or measured control is unavailable"};
    if (!contains(r, x, y)) return {};
    if (measurement.event_id < 0 || !measurement.owner_sequence ||
        measurement.owner_sequence <= last_attempted_sequence_)
        return {true, EconomicPopupQueueStatus::blocked,
                "GoldPerTurn event identity is stale or unavailable"};
    // Reserve before any popup effect. A partial/uncertain effect cannot be
    // retried by delivering the same click sequence again.
    last_attempted_sequence_ = measurement.owner_sequence;
    const auto queued = lifecycle_->queue(source_session, frame,
        EconomicPopupRequest{"BUTTONPOPUP_ECONOMIC_OVERVIEW",
            active_player_id_, measurement.event_id, measurement.turn, 0,
            measurement.owner_sequence});
    return {true, queued.status, queued.blocker};
}

} // namespace civ5::ui
