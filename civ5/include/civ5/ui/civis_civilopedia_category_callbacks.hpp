#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/ui/civis_measured_ui_lifecycle.hpp"

namespace civ5::ui {
// Separate lifetime owner: callbacks cease to be valid when this owner closes,
// expires, or observes a different selected source/frame/session identity.
class CivisCivilopediaCallbackOwner {
  public:
    explicit CivisCivilopediaCallbackOwner(CivilopediaLayoutOwnerKey owner);
    bool current(const CivilopediaLayoutOwnerKey &owner) const;
    void observe(const CivilopediaLayoutOwnerKey &owner);
    void close();
  private:
    CivilopediaLayoutOwnerKey owner_;
    bool valid_ = true;
};

// Adds only category Button IDs verified in the selected installed XML and
// exact source registration loop. The event token is the caller's declared
// Civis pointer mapping, not an assertion of the original numeric Mouse ABI.
// No Mouse global is installed implicitly: the caller supplies its enum table.
std::vector<std::string> bind_civis_civilopedia_category_callbacks(
    runtime::ScriptUiLayoutProvider &layout,
    const std::filesystem::path &game_root,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaLayoutOwnerKey &identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,
    std::int64_t left_click_event);
// Extends an already declared category capability to authored top-level
// Button/GridButton:RegisterCallback(Mouse.eLClick, closure) calls. Other control kinds
// are exposed as source-verified identities without invented callback APIs.
std::vector<std::string> bind_civis_civilopedia_top_level_button_callbacks(
    runtime::ScriptUiLayoutProvider &layout,const std::filesystem::path &game_root,
    const SelectedBnwCivilopediaLayoutSources &sources,const CivilopediaLayoutOwnerKey &identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,std::int64_t left_click_event);
} // namespace civ5::ui
