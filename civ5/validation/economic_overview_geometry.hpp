#pragma once

#include "civ5/ui/economic_overview_scene.hpp"

namespace civ5::validation {

// Measure only the verified Expansion2 hierarchy. The popup owner and source
// proofs are checked again by EconomicOverviewScene::bind_popup.
inline std::optional<civ5::ui::EconomicOverviewInputRects>
measure_economic_overview_buttons(
    const civ5::assets::ResourceMountTable &resources,
    const civ5::ui::EconomicTabSourceProof &source,
    const civ5::ui::EconomicTabOwner &owner) {
    if (!source.verified || source.xml_path.find("Expansion2") ==
            std::string::npos || source.xml_path != owner.xml_path ||
        owner.viewport_width < 1000 || owner.viewport_height < 730)
        return std::nullopt;
    const auto selected = resources.resolve(civ5::assets::AssetKey(
        "Assets/UI/InGame/Popups/EconomicOverview.xml"));
    const auto *asset = std::get_if<civ5::assets::AssetHandle>(&selected.asset);
    if (!asset) return std::nullopt;
    const auto &provenance = asset->candidate().provenance;
    const auto selected_path = provenance.archive_relative_path
        ? *provenance.archive_relative_path + ":" + provenance.source_relative_path
        : provenance.source_relative_path;
    if (selected_path != source.xml_path) return std::nullopt;
    try {
        const auto bytes = asset->read_bytes(8ULL * 1024 * 1024);
        const auto xml = civ5::content::read_xml_metadata(
            std::string(bytes.begin(), bytes.end()));
        if (xml.name != "Context") return std::nullopt;
        const civ5::content::XmlNode *panel = nullptr;
        for (const auto &child : xml.children)
            if (child.name == "Grid" && child.attribute("Size") == "1000,658" &&
                child.attribute("Anchor") == "C,C" &&
                child.attribute("Offset") == "0,36" &&
                child.attribute("Style") == "Grid9DetailFive140") {
                if (panel) return std::nullopt;
                panel = &child;
            }
        if (!panel) return std::nullopt;
        bool tabs = false, close = false;
        for (const auto &box : panel->children) {
            if (box.name != "Box") continue;
            if (box.attribute("Anchor") == "C,T" &&
                box.attribute("Size") == "955,32" &&
                box.attribute("Offset") == "0,44") {
                for (const auto &stack : box.children) {
                    if (stack.name != "Stack" ||
                        stack.attribute("Size") != "971,32" ||
                        stack.attribute("Anchor") != "C,T" ||
                        stack.attribute("StackGrowth") != "Right") continue;
                    std::size_t next = 0;
                    for (const auto &button : stack.children) {
                        if (button.name != "Button") continue;
                        const auto expected = next == 0 ? "GeneralInfoButton" :
                            "HappinessButton";
                        if (next >= 2 || button.attribute("ID") != expected ||
                            button.attribute("Anchor") != "L,T" ||
                            button.attribute("Size") != "300,24" ||
                            button.attribute("Offset") != "0,0")
                            return std::nullopt;
                        ++next;
                    }
                    tabs = next == 2;
                }
            }
            if (box.attribute("Anchor") == "C,B" &&
                box.attribute("AnchorSide") == "I.I" &&
                box.attribute("Offset") == "0,54" &&
                box.attribute("Size") == "960,56")
                for (const auto &button : box.children)
                    if (button.name == "GridButton" &&
                        button.attribute("ID") == "CloseButton" &&
                        button.attribute("Anchor") == "L,B" &&
                        button.attribute("Size") == "150,32" &&
                        button.attribute("Offset") == "14,0") close = true;
        }
        if (!tabs || !close) return std::nullopt;
        const auto x = (owner.viewport_width - 1000) / 2;
        const auto y = (owner.viewport_height - 658) / 2 + 36;
        const auto tab_x = x + (1000 - 955) / 2 + (955 - 971) / 2;
        const auto tab_y = y + 44;
        civ5::ui::EconomicOverviewInputRects result;
        result.owner = owner;
        result.general_button = {tab_x, tab_y, 300, 24};
        result.happiness_button = {tab_x + 300, tab_y, 300, 24};
        result.close_button = {x + (1000 - 960) / 2 + 14,
                               y + 658 - 54 - 32, 150, 32};
        if (result.close_button.y + 32 > owner.viewport_height)
            return std::nullopt;
        return result;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

inline std::optional<bool> live_happiness_enabled(
    const civ5::runtime::LiveSession &session) {
    std::optional<bool> enabled;
    for (const auto &option : session.state().setup.game_options) {
        if (option.name != "GAMEOPTION_NO_HAPPINESS") continue;
        if (enabled || (option.value != 0 && option.value != 1))
            return std::nullopt;
        enabled = option.value == 0;
    }
    return enabled;
}

} // namespace civ5::validation
