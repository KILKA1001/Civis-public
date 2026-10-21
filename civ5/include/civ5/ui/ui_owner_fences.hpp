#pragma once
#include "civ5/ui/ui_runtime.hpp"
#include <functional>
#include <vector>
namespace civ5::ui {
// Borrowed owner guards live as long as the surrounding synchronous host.
// The same captured UI payload is checked before draw, input and presentation.
struct UiOwnerFences {
    std::vector<std::function<bool()>> checks;
    bool current() const noexcept {
        try { for (const auto &check : checks) if (!check || !check()) return false; }
        catch (...) { return false; }
        return true;
    }
};
inline bool ui_owners_current(const UiFrameSnapshot &frame) noexcept {
    const auto *fences=frame.find<UiOwnerFences>();
    return !fences || fences->current();
}
} // namespace civ5::ui
