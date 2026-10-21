#include "civ5/ui/economic_overview_tabs.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <climits>
#include <filesystem>
#include <string_view>

namespace civ5::ui {
namespace {

bool same_owner(const EconomicTabOwner &a, const EconomicTabOwner &b) {
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

bool valid_rect(const OverviewRect &r, const EconomicTabOwner &owner) {
    return r.x >= 0 && r.y >= 0 && r.width > 0 && r.height > 0 &&
           std::int64_t(r.x) + r.width <= owner.viewport_width &&
           std::int64_t(r.y) + r.height <= owner.viewport_height;
}

bool contains(const OverviewRect &r, std::int32_t x, std::int32_t y) {
    return x >= r.x && y >= r.y &&
           std::int64_t(x) < std::int64_t(r.x) + r.width &&
           std::int64_t(y) < std::int64_t(r.y) + r.height;
}

bool overlaps(const OverviewRect &a, const OverviewRect &b) {
    return std::int64_t(a.x) < std::int64_t(b.x) + b.width &&
           std::int64_t(b.x) < std::int64_t(a.x) + a.width &&
           std::int64_t(a.y) < std::int64_t(b.y) + b.height &&
           std::int64_t(b.y) < std::int64_t(a.y) + a.height;
}

bool has_control(const OverviewSource &source, std::string_view id,
                 std::string_view element) {
    return std::count_if(source.controls.begin(), source.controls.end(),
        [&](const OverviewControl &c) { return c.id == id && c.element == element; }) == 1;
}

bool callback_body(std::string_view lua, std::string_view name,
                   std::string_view first, std::string_view second,
                   std::string_view third, std::string_view fourth,
                   std::string_view selected) {
    const auto start = lua.find(std::string("function ") + std::string(name) + "()");
    if (start == std::string_view::npos) return false;
    const auto end = lua.find("\nend", start);
    if (end == std::string_view::npos) return false;
    const auto body = lua.substr(start, end - start);
    std::size_t cursor = 0;
    for (const auto token : {first, second, third, fourth, selected}) {
        const auto found = body.find(token, cursor);
        if (found == std::string_view::npos) return false;
        cursor = found + token.size();
    }
    return true;
}

} // namespace

EconomicTabSourceProof inspect_economic_tab_source(
    const assets::ResourceMountTable &resources, const OverviewSource &source) {
    EconomicTabSourceProof proof;
    if (source.context_id != "EconomicOverview" ||
        source.selected_source_path.empty() ||
        !has_control(source, "GeneralInfoButton", "Button") ||
        !has_control(source, "HappinessButton", "Button") ||
        !has_control(source, "GeneralInfoSelectHighlight", "Grid") ||
        !has_control(source, "HappinessSelectHighlight", "Grid") ||
        !has_control(source, "EconomicGeneralInfo", "LuaContext") ||
        !has_control(source, "HappinessInfo", "LuaContext")) {
        proof.blocker = "Selected EconomicOverview XML lacks unique tab controls";
        return proof;
    }
    constexpr auto key = "Assets/UI/InGame/Popups/EconomicOverview.lua";
    const auto resolution = resources.resolve(assets::AssetKey(key));
    const auto *handle = std::get_if<assets::AssetHandle>(&resolution.asset);
    if (!handle) {
        proof.blocker = "Selected EconomicOverview Lua is missing or ambiguous";
        return proof;
    }
    try {
        const auto bytes = handle->read_bytes(1024 * 1024);
        const std::string lua(bytes.begin(), bytes.end());
        const bool general = callback_body(lua, "OnGeneralInfoButton",
            "GeneralInfoSelectHighlight:SetHide(false)",
            "HappinessSelectHighlight:SetHide(true)",
            "EconomicGeneralInfo:SetHide( false )", "HappinessInfo:SetHide( true )",
            "m_CurrentPanel = Controls.EconomicGeneralInfo");
        const bool happiness = callback_body(lua, "OnHappinessButton",
            "GeneralInfoSelectHighlight:SetHide(true)",
            "HappinessSelectHighlight:SetHide(false)",
            "EconomicGeneralInfo:SetHide( true )", "HappinessInfo:SetHide( false )",
            "m_CurrentPanel = Controls.HappinessInfo");
        if (!general || !happiness ||
            lua.find("GeneralInfoButton:RegisterCallback( Mouse.eLClick, OnGeneralInfoButton )") == std::string::npos ||
            lua.find("HappinessButton:RegisterCallback( Mouse.eLClick, OnHappinessButton )") == std::string::npos) {
            proof.blocker = "Selected EconomicOverview Lua tab callback differs from verified source";
            return proof;
        }
        const auto &provenance = handle->candidate().provenance;
        proof.lua_path = provenance.archive_relative_path
            ? *provenance.archive_relative_path + ":" + provenance.source_relative_path
            : provenance.source_relative_path;
        proof.lua_sha256 = runtime::identify_bytes(bytes).sha256;
        proof.xml_path = source.selected_source_path;
        proof.verified = true;
    } catch (const std::exception &) {
        proof.blocker = "Selected EconomicOverview Lua cannot be verified";
    }
    return proof;
}

bool EconomicOverviewTabs::bind(const UiFrameSnapshot &frame,
    const PopupFrame &popups, const EconomicTabSourceProof &source,
    const EconomicTabMeasurement &measurement,
    std::optional<bool> happiness_enabled, std::string &blocker) {
    blocker.clear();
    const auto &owner = measurement.owner;
    const auto matches = std::count_if(popups.events.begin(), popups.events.end(),
        [&](const PopupEvent &event) {
            return event.context_id == "EconomicOverview";
        });
    const bool matching_event = std::any_of(popups.events.begin(), popups.events.end(),
        [&](const PopupEvent &event) {
            return event.context_id == "EconomicOverview" &&
                   event.event_id == owner.event_id;
        });
    if (!source.verified || source.lua_sha256.empty() || source.xml_path.empty() ||
        source.lua_path.empty() || owner.event_id < 0 ||
        !frame.active_player_id() || !frame.turn() ||
        owner.player_id != *frame.active_player_id() || owner.turn != *frame.turn() ||
        owner.frame != frame.frame() || owner.player_id != popups.player_id ||
        owner.turn != popups.turn || owner.owner_sequence == 0 ||
        owner.owner_sequence != popups.owner_sequence || matches != 1 ||
        !matching_event ||
        owner.lua_sha256 != source.lua_sha256 || owner.xml_path != source.xml_path ||
        owner.viewport_width <= 0 || owner.viewport_height <= 0 ||
        !valid_rect(measurement.general_button, owner) ||
        !valid_rect(measurement.happiness_button, owner) ||
        overlaps(measurement.general_button, measurement.happiness_button)) {
        measurement_.reset();
        blocker = "Current EconomicOverview popup or measured tab controls are unavailable";
        return false;
    }
    if (!measurement_ || !same_popup(measurement_->owner, owner))
        selected_ = EconomicTab::general;
    measurement_ = measurement;
    happiness_enabled_ = happiness_enabled;
    if (happiness_enabled_ != true) selected_ = EconomicTab::general;
    return true;
}

EconomicTabView EconomicOverviewTabs::view(const EconomicTabOwner &current) const {
    EconomicTabView result;
    if (!measurement_ || !same_owner(measurement_->owner, current)) {
        result.blocker = "EconomicOverview tab owner is stale";
        return result;
    }
    result.available = true;
    result.selected = selected_;
    result.general_visible = result.general_highlight = selected_ == EconomicTab::general;
    result.happiness_visible = result.happiness_highlight = selected_ == EconomicTab::happiness;
    return result;
}

EconomicTabClick EconomicOverviewTabs::click(const EconomicTabOwner &current,
    std::int32_t x, std::int32_t y) {
    EconomicTabClick result;
    result.selected = selected_;
    if (!view(current).available) {
        result.blocker = "EconomicOverview tab owner is stale";
        return result;
    }
    if (contains(measurement_->general_button, x, y)) {
        selected_ = EconomicTab::general;
    } else if (contains(measurement_->happiness_button, x, y)) {
        result.consumed = true;
        if (happiness_enabled_ != true) {
            result.blocker = "Live GAMEOPTION_NO_HAPPINESS state is unavailable or enabled";
            return result;
        }
        selected_ = EconomicTab::happiness;
    } else {
        return result;
    }
    result.consumed = result.performed = true;
    result.selected = selected_;
    return result;
}

} // namespace civ5::ui
