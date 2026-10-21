#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/ui/civis_measured_civilopedia_controls.hpp"

namespace civ5::ui {
using CivisCivilopediaScrollMeasurementProvider = std::function<
    CivisMeasuredScrollSnapshot(const CivilopediaLayoutOwnerKey &,
        std::string_view panel_id, const CivisControlGeometry &current_viewport)>;

// Binds actual Lua UIManager/GetSize/setter/CalculateInternalSize operations.
// The control tree and renderer callback borrows must outlive the host. The
// callback reads a complete resolved renderer snapshot for this owner's frame;
// it must not fabricate visibility/anchoring from unresolved XML rectangles.
// Missing measurement callback is an explicit failure at the first scroll call.
runtime::ScriptUiLayoutProvider bind_civis_measured_civilopedia_layout(
    CivisMeasuredCivilopediaControlTree &controls,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaMeasuredLayout &measured,
    CivisCivilopediaScrollMeasurementProvider renderer_measurements);
} // namespace civ5::ui
