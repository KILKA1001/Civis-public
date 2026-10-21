#pragma once
#include "civ5/runtime/bnw_policy_adoption.hpp"
namespace civ5::tests {
// Called only with genuine SourceOwner-issued, actually executed policy prefix.
// The fixture has no hooks issuer/culture setter/synthetic session constructor.
void verify_actual_bnw_policy_adoption_prefix(const ui::SelectedBnwPreviewHost&,
 const runtime::BnwPolicyNativeObservation& before,
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&);
}
