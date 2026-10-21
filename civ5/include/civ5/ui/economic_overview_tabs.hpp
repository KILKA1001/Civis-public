#pragma once

#include "civ5/ui/overview_screens.hpp"
#include "civ5/ui/popup_screens.hpp"
#include <optional>

namespace civ5::ui {

struct EconomicTabSourceProof {
    bool verified = false;
    std::string lua_path, lua_sha256, xml_path, blocker;
};

// Checks the selected Lua callback bodies and the six XML-owned controls.
EconomicTabSourceProof inspect_economic_tab_source(
    const assets::ResourceMountTable &resources, const OverviewSource &source);

struct EconomicTabOwner {
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0, owner_sequence = 0;
    std::string lua_sha256, xml_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
};

struct EconomicTabMeasurement {
    EconomicTabOwner owner;
    OverviewRect general_button, happiness_button;
};

enum class EconomicTab { general, happiness };

struct EconomicTabView {
    bool available = false;
    EconomicTab selected = EconomicTab::general;
    bool general_visible = false, happiness_visible = false;
    bool general_highlight = false, happiness_highlight = false;
    std::string blocker;
};

struct EconomicTabClick {
    bool consumed = false, performed = false;
    EconomicTab selected = EconomicTab::general;
    std::string blocker;
};

// This performs only the source-authored local SetHide transition. The popup
// event and measured button rectangles must be supplied by the current owner.
class EconomicOverviewTabs {
  public:
    bool bind(const UiFrameSnapshot &frame, const PopupFrame &popups,
              const EconomicTabSourceProof &source,
              const EconomicTabMeasurement &measurement,
              std::optional<bool> happiness_enabled, std::string &blocker);
    EconomicTabView view(const EconomicTabOwner &current) const;
    EconomicTabClick click(const EconomicTabOwner &current,
                           std::int32_t x, std::int32_t y);

  private:
    std::optional<EconomicTabMeasurement> measurement_;
    EconomicTab selected_ = EconomicTab::general;
    std::optional<bool> happiness_enabled_;
};

} // namespace civ5::ui
