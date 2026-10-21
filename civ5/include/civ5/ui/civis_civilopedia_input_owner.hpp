#pragma once
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include <thread>

namespace civ5::ui {
// Captured Civis input state for one selected XML/source/frame owner. Native
// window code supplies visibility and UTF-8 edits; no Lua engine data is faked.
class CivisCivilopediaInputOwner:public std::enable_shared_from_this<CivisCivilopediaInputOwner> {
  public:
    static std::shared_ptr<CivisCivilopediaInputOwner> open(
        const std::filesystem::path &game_root,const SelectedBnwCivilopediaLayoutSources &sources,
        CivilopediaLayoutOwnerKey identity,std::shared_ptr<CivisCivilopediaCallbackOwner> owner,
        bool initially_hidden);
    runtime::ScriptUiTextEntryProvider text_provider();
    runtime::ScriptUiContextProvider context_provider();
    std::string text() const;
    bool hidden() const;
    std::size_t max_length() const {return max_length_;}
    void close();
  private:
    CivisCivilopediaInputOwner(CivilopediaLayoutOwnerKey,std::shared_ptr<CivisCivilopediaCallbackOwner>,std::size_t,bool);
    void require_current() const;
    void replace(std::string_view);
    CivilopediaLayoutOwnerKey identity_;
    std::weak_ptr<CivisCivilopediaCallbackOwner> owner_;
    std::size_t max_length_;
    bool hidden_,closed_=false;
    std::string text_;
    std::thread::id thread_;
};
} // namespace civ5::ui
