#pragma once
#include "civ5/runtime/bnw_policy_adoption.hpp"
#include <memory>
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;}
namespace civ5::ui {class LiveUiHost;}
namespace civ5::tests {
enum class ActualPolicyReformationResourceScenario {reformation,resource};
// Out-of-line genuine production caller. The selected saved game and normal
// command must naturally reach the requested effect; no eligibility edit.
std::shared_ptr<const runtime::BnwPolicyAdoptionPending>
run_actual_policy_reformation_resource_probe(runtime::BnwSelectedStockLuaSourceOwner&,
 const ui::SelectedBnwPreviewHost&,const runtime::BnwPolicyAdoptionCommand&,
 std::shared_ptr<const ui::LiveUiHost>,ActualPolicyReformationResourceScenario);
}
