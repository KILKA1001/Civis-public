#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/ui_runtime.hpp"
#include "civ5/ui/world_view.hpp"
#include <memory>
namespace civ5::ui {
struct CivisResourceBankUiDocument {
    std::string requested_key;
    assets::AssetCandidate candidate;
    runtime::FileIdentity payload_identity;
};
// Authored UI data owned by one genuine Civis bank. No VM/window/frame/font
// publication, gameplay action, or Original completed bank is granted here.
class CivisResourceBankUiSnapshot final {
 public:
    static std::shared_ptr<const CivisResourceBankUiSnapshot> capture(
        const std::shared_ptr<assets::CivisResourceBankOwner>&);
    CivisResourceBankUiSnapshot(const CivisResourceBankUiSnapshot&) = delete;
    CivisResourceBankUiSnapshot& operator=(const CivisResourceBankUiSnapshot&) = delete;
    std::shared_ptr<const CivisResourceBankUiSnapshot> retain_owner() const;
    void require_current() const;
    void require_boundary() const;
    const UiContextRegistry& registry() const;
    const WorldViewSkin& world_view_skin() const;
    const std::vector<CivisResourceBankUiDocument>& document_sources() const;
    void require_same_bank(const assets::CivisResourceBankOwner&) const;
 private:
    CivisResourceBankUiSnapshot(std::shared_ptr<const assets::CivisResourceBankOwner>,
        UiContextRegistry, WorldViewSkin, std::vector<CivisResourceBankUiDocument>);
    std::weak_ptr<const CivisResourceBankUiSnapshot> canonical_self_;
    const std::shared_ptr<const assets::CivisResourceBankOwner> bank_;
    const assets::CivisResourceBankCapability capability_;
    const UiContextRegistry registry_;
    const WorldViewSkin world_view_skin_;
    const std::vector<CivisResourceBankUiDocument> sources_;
};
} // namespace civ5::ui
