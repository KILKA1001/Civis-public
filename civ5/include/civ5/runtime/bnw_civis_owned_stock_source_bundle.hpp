#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/civis_resource_bank_ui_snapshot.hpp"
#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
#include <thread>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwSelectedStockLuaSourceOwner;
namespace detail {class BnwFontResourceLoan;}
// Staged own-bank source lineage only. No Original completed-resource,
// active VM, GUI/window, input, or event completion authority is issued.
class BnwCivisOwnedStockSourceBundle final {
 public:
    BnwCivisOwnedStockSourceBundle(const BnwCivisOwnedStockSourceBundle&)=delete;
    void require_current()const;
    void require_boundary()const;
    void require_same_host(const ui::SelectedBnwPreviewHost&)const;
    std::shared_ptr<const ui::SelectedBnwPreviewHost> retain_host()const;
    const assets::CivisResourceBankSourceRecipe& recipe()const;
    const assets::ResourceMountTable& resources()const;
    assets::CivisLoadedResource loaded(std::string_view)const;
    std::string text(std::string_view,std::string* sha=nullptr)const;
    FileIdentity source_identity(const std::filesystem::path&)const;
    content::XmlNode source_xml(const std::filesystem::path&)const;
    const ui::CivisResourceBankUiSnapshot& ui_snapshot()const;
    const ui::CivisResourceBankFontColorSnapshot& font_color_snapshot()const;
 private:
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwSelectedStockLuaBootstrap;
    friend class detail::BnwFontResourceLoan;
    friend class BnwSelectedWorldViewContextOwner;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::weak_ptr<const ui::SelectedBnwPreviewHost> host_;
    const std::shared_ptr<assets::CivisResourceBankOwner> bank_;
    const assets::CivisResourceBankCapability capability_;
    const std::shared_ptr<const ui::CivisResourceBankUiSnapshot> ui_;
    const std::shared_ptr<const ui::CivisResourceBankFontColorSnapshot> font_;
    const assets::CivisResourceBankSourceRecipe recipe_;
    BnwCivisOwnedStockSourceBundle(std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::shared_ptr<assets::CivisResourceBankOwner>,
        std::shared_ptr<const ui::CivisResourceBankUiSnapshot>,
        std::shared_ptr<const ui::CivisResourceBankFontColorSnapshot>);
    std::string key(const std::filesystem::path&)const;
};
}
