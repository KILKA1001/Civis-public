#pragma once

#include "civ5/ui/economic_overview_tabs.hpp"
#include <functional>

namespace civ5::ui {

struct EconomicCloseSourceProof {
    bool verified = false;
    std::string lua_path, lua_sha256, xml_path, blocker;
};

EconomicCloseSourceProof inspect_economic_close_source(
    const assets::ResourceMountTable &resources, const OverviewSource &source);

struct EconomicCloseMeasurement {
    EconomicTabOwner owner;
    OverviewRect close_button;
};

struct EconomicCloseReceipt {
    bool dequeued = false;
    bool context_hidden = false;
    bool turn_timer_decremented = false;
    bool popup_processed = false;
};

// The owner must execute UIManager:DequeuePopup and its resulting ShowHide
// effects, then report all four completed operations. No service means no
// close. A partial/throwing service is never retried for the same popup.
using EconomicCloseService = std::function<EconomicCloseReceipt(
    const EconomicTabOwner &)>;

enum class EconomicCloseInput { button_click, escape_key, return_key };
enum class EconomicCloseStatus { unavailable, performed, uncertain };
struct EconomicCloseResult {
    bool consumed = false;
    EconomicCloseStatus status = EconomicCloseStatus::unavailable;
    std::string blocker;
};

class EconomicOverviewClose {
  public:
    bool bind(const UiFrameSnapshot &frame, const PopupFrame &popups,
              const EconomicCloseSourceProof &source,
              const EconomicCloseMeasurement &measurement,
              bool context_visible, std::string &blocker);
    EconomicCloseResult activate(const EconomicTabOwner &current,
                                 EconomicCloseInput input,
                                 std::int32_t x, std::int32_t y,
                                 const EconomicCloseService &service);

  private:
    std::optional<EconomicCloseMeasurement> measurement_;
    std::optional<EconomicTabOwner> attempted_owner_;
};

} // namespace civ5::ui
