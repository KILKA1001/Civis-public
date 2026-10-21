#pragma once
#include <memory>
#include <span>
namespace civ5::runtime {class BnwMajorTeamHasTechPending;class BnwMajorTechTransportConsumption;}
namespace civ5::ui {class SelectedBnwPreviewHost;class LiveUiHost;struct LiveUiFrame;}
namespace civ5::tests {
// Called only by the natural selected-client driver with its genuine acted
// core/receipts and actual begin_frame values. This function issues no authority.
// No standalone positive/synthetic frame or hook factory is provided.
void verify_major_tech_dirty_fixture(const ui::SelectedBnwPreviewHost&,
 const ui::LiveUiHost&,const ui::LiveUiFrame& before,const ui::LiveUiFrame& after,
 const runtime::BnwMajorTeamHasTechPending&,
 std::span<const std::shared_ptr<const runtime::BnwMajorTechTransportConsumption>>);
}
