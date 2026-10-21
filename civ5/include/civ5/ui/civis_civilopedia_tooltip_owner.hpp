#pragma once
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include "civ5/ui/ui_texture_asset.hpp"
#include <thread>

namespace civ5::ui {
struct CivisCivilopediaTooltipControlState {
    std::string id,kind,texture,blocker;
    bool hidden=false;
    std::int64_t offset_x=0,offset_y=0;
    std::optional<UiTextureAsset> selected_texture;
    std::optional<assets::AssetHandle> retained_source,retained_index;
};
// Real selected XML controls and captured Civis state. Texture selection is
// optional at registration; no renderer receipt/readiness is manufactured.
class CivisCivilopediaTooltipOwner:public std::enable_shared_from_this<CivisCivilopediaTooltipOwner> {
  public:
    static std::shared_ptr<CivisCivilopediaTooltipOwner> open(
        const std::filesystem::path &,const SelectedBnwCivilopediaLayoutSources &,
        CivilopediaLayoutOwnerKey,std::shared_ptr<CivisCivilopediaCallbackOwner>,
        const assets::ResourceMountTable *texture_resources=nullptr);
    runtime::ScriptUiTooltipProvider provider();
    const std::string &type_name()const{return type_name_;}
    std::vector<CivisCivilopediaTooltipControlState> snapshot()const;
    std::size_t registrations()const{return registrations_;}
    bool render_receipt_verified()const{return false;}
    void close();
  private:
    CivisCivilopediaTooltipOwner(CivilopediaLayoutOwnerKey,std::shared_ptr<CivisCivilopediaCallbackOwner>,const assets::ResourceMountTable *);
    void require_current()const;
    CivisCivilopediaTooltipControlState &control(std::string_view);
    void texture(std::string_view,std::string_view);
    void offset(std::string_view,std::int64_t,std::int64_t);
    CivilopediaLayoutOwnerKey identity_;
    std::weak_ptr<CivisCivilopediaCallbackOwner> owner_;
    const assets::ResourceMountTable *resources_;
    std::thread::id thread_;
    bool closed_=false;
    std::size_t registrations_=0;
    std::string type_name_;
    std::vector<CivisCivilopediaTooltipControlState> controls_;
};
} // namespace civ5::ui
