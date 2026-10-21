#pragma once
#include "civ5/ui/bnw_client_text_policy.hpp"
#include "civ5/ui/civis_locale_text_owner.hpp"
namespace civ5::ui {
// Retains BOTH existing locale owners. It does not lookup/translate text or
// manufacture a language/default/owner identity. Stale guards belong to them.
BnwLabelLocaleSelectionProvider bnw_text_policy_selected_locale(
 std::shared_ptr<SelectedCivilopediaLocale>,std::shared_ptr<CivisLocaleTextOwner>,
 CivilopediaLayoutOwnerKey);
}
