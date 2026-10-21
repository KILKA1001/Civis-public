#pragma once

#include "civ5/ui/economic_overview_close.hpp"

namespace civ5::ui {

struct EconomicCloseOwnerServices {
    // Must sample the current popup queue and frame at invocation time. A
    // cached owner from the measured hit is insufficient.
    std::function<std::optional<EconomicTabOwner>()> current_owner;
    // Return true only after the actual turn-timer owner decrements its
    // semaphore for this popup's ShowHide transition.
    std::function<bool(const EconomicTabOwner &)> decrement_turn_timer;
    // Return true only after BUTTONPOPUP_ECONOMIC_OVERVIEW Processed is sent
    // synchronously to the actual UI event owner.
    std::function<bool(const EconomicTabOwner &)> publish_processed;
};

// Uses UiScreenController as Civis' local UIManager queue. The other two
// native ShowHide effects have no current in-tree provider and are mandatory.
// A missing service returns an empty callback, keeping close unavailable.
// The controller must outlive the returned callback.
EconomicCloseService make_economic_close_service(
    UiScreenController &controller, UiFrameSnapshot bound_frame,
    EconomicCloseOwnerServices services);

} // namespace civ5::ui
