#include "civ5/ui/economic_overview_close.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>

namespace civ5::ui {
namespace {

bool control(const OverviewSource &source, std::string_view id,
             std::string_view element) {
    return std::count_if(source.controls.begin(), source.controls.end(),
        [&](const OverviewControl &entry) {
            return entry.id == id && entry.element == element;
        }) == 1;
}

bool exact_owner(const EconomicTabOwner &a, const EconomicTabOwner &b) {
    return a.player_id == b.player_id && a.event_id == b.event_id &&
           a.turn == b.turn && a.frame == b.frame &&
           a.owner_sequence == b.owner_sequence &&
           a.lua_sha256 == b.lua_sha256 && a.xml_path == b.xml_path &&
           a.viewport_width == b.viewport_width &&
           a.viewport_height == b.viewport_height;
}

bool same_popup(const EconomicTabOwner &a, const EconomicTabOwner &b) {
    return a.player_id == b.player_id && a.event_id == b.event_id &&
           a.turn == b.turn && a.owner_sequence == b.owner_sequence &&
           a.lua_sha256 == b.lua_sha256 && a.xml_path == b.xml_path;
}

bool in_rect(const OverviewRect &rect, std::int32_t x, std::int32_t y) {
    return x >= rect.x && y >= rect.y &&
           std::int64_t(x) < std::int64_t(rect.x) + rect.width &&
           std::int64_t(y) < std::int64_t(rect.y) + rect.height;
}

bool valid_rect(const OverviewRect &rect, const EconomicTabOwner &owner) {
    return rect.x >= 0 && rect.y >= 0 && rect.width > 0 && rect.height > 0 &&
           std::int64_t(rect.x) + rect.width <= owner.viewport_width &&
           std::int64_t(rect.y) + rect.height <= owner.viewport_height;
}

} // namespace

EconomicCloseSourceProof inspect_economic_close_source(
    const assets::ResourceMountTable &resources, const OverviewSource &source) {
    EconomicCloseSourceProof proof;
    const auto tabs = inspect_economic_tab_source(resources, source);
    if (!tabs.verified || !control(source, "CloseButton", "GridButton")) {
        proof.blocker = "Selected EconomicOverview close control or Lua source is unverified";
        return proof;
    }
    const auto resolution = resources.resolve(
        assets::AssetKey("Assets/UI/InGame/Popups/EconomicOverview.lua"));
    const auto *handle = std::get_if<assets::AssetHandle>(&resolution.asset);
    if (!handle) {
        proof.blocker = "Selected EconomicOverview Lua is missing or ambiguous";
        return proof;
    }
    try {
        const auto bytes = handle->read_bytes(1024 * 1024);
        if (runtime::identify_bytes(bytes).sha256 != tabs.lua_sha256) {
            proof.blocker = "Selected EconomicOverview Lua changed during verification";
            return proof;
        }
        const std::string lua(bytes.begin(), bytes.end());
        const auto close = lua.find("function OnClose()");
        const auto dequeue = lua.find("UIManager:DequeuePopup( ContextPtr )", close);
        const auto registered = lua.find(
            "CloseButton:RegisterCallback( Mouse.eLClick, OnClose)", dequeue);
        const auto escape = lua.find("Keys.VK_ESCAPE", registered);
        const auto enter = lua.find("Keys.VK_RETURN", escape);
        const auto close_key = lua.find("OnClose();", enter);
        const auto hide = lua.find("function ShowHideHandler(", close_key);
        const auto timer = lua.find("UI.decTurnTimerSemaphore()", hide);
        const auto processed = lua.find(
            "Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_ECONOMIC_OVERVIEW, 0)", timer);
        if (close == std::string::npos || dequeue == std::string::npos ||
            registered == std::string::npos || escape == std::string::npos ||
            enter == std::string::npos || close_key == std::string::npos ||
            hide == std::string::npos || timer == std::string::npos ||
            processed == std::string::npos) {
            proof.blocker = "Selected EconomicOverview close lifecycle differs from verified source";
            return proof;
        }
        proof.verified = true;
        proof.lua_path = tabs.lua_path;
        proof.lua_sha256 = tabs.lua_sha256;
        proof.xml_path = tabs.xml_path;
    } catch (const std::exception &) {
        proof.blocker = "Selected EconomicOverview close source cannot be verified";
    }
    return proof;
}

bool EconomicOverviewClose::bind(const UiFrameSnapshot &frame,
    const PopupFrame &popups, const EconomicCloseSourceProof &source,
    const EconomicCloseMeasurement &measurement, bool context_visible,
    std::string &blocker) {
    blocker.clear();
    const auto &owner = measurement.owner;
    const auto context_count = std::count_if(popups.events.begin(),
        popups.events.end(), [](const PopupEvent &event) {
            return event.context_id == "EconomicOverview";
        });
    const bool event_matches = std::any_of(popups.events.begin(),
        popups.events.end(), [&](const PopupEvent &event) {
            return event.context_id == "EconomicOverview" &&
                   event.event_id == owner.event_id;
        });
    if (!source.verified || source.lua_sha256.empty() || source.xml_path.empty() ||
        !context_visible || !frame.active_player_id() || !frame.turn() ||
        owner.player_id != *frame.active_player_id() ||
        owner.turn != *frame.turn() || owner.frame != frame.frame() ||
        owner.player_id != popups.player_id || owner.turn != popups.turn ||
        owner.event_id < 0 || owner.owner_sequence == 0 ||
        owner.owner_sequence != popups.owner_sequence ||
        owner.lua_sha256 != source.lua_sha256 || owner.xml_path != source.xml_path ||
        owner.viewport_width <= 0 || owner.viewport_height <= 0 ||
        context_count != 1 || !event_matches ||
        !valid_rect(measurement.close_button, owner)) {
        measurement_.reset();
        blocker = "Current EconomicOverview popup or measured CloseButton is unavailable";
        return false;
    }
    if (attempted_owner_ && !same_popup(*attempted_owner_, owner))
        attempted_owner_.reset();
    measurement_ = measurement;
    return true;
}

EconomicCloseResult EconomicOverviewClose::activate(
    const EconomicTabOwner &current, EconomicCloseInput input,
    std::int32_t x, std::int32_t y, const EconomicCloseService &service) {
    EconomicCloseResult result;
    if (!measurement_ || !exact_owner(measurement_->owner, current)) {
        result.blocker = "EconomicOverview close owner is stale";
        return result;
    }
    if (input == EconomicCloseInput::button_click &&
        !in_rect(measurement_->close_button, x, y)) return result;
    result.consumed = true;
    if (attempted_owner_ && same_popup(*attempted_owner_, current)) {
        result.status = EconomicCloseStatus::uncertain;
        result.blocker = "EconomicOverview close was already attempted for this popup";
        return result;
    }
    if (!service) {
        result.blocker = "UIManager dequeue and ShowHide lifecycle service is unavailable";
        return result;
    }
    attempted_owner_ = current;
    try {
        const auto receipt = service(current);
        if (receipt.dequeued && receipt.context_hidden &&
            receipt.turn_timer_decremented && receipt.popup_processed) {
            measurement_.reset();
            result.status = EconomicCloseStatus::performed;
            return result;
        }
    } catch (const std::exception &) {
        // The callback may have performed some native effects before throwing.
    }
    result.status = EconomicCloseStatus::uncertain;
    result.blocker = "EconomicOverview close effects are incomplete or uncertain";
    return result;
}

} // namespace civ5::ui
