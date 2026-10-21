#pragma once
#include "civ5/ui/frontend_selected_save_input.hpp"
#include "civ5/ui/bnw_live_preview_launch.hpp"
#include <memory>

namespace civ5::ui { struct FrontendRequest; }
namespace civ5::ui::detail {
class CivisFrontendPartyWorldSource;
class CivisSelectedBnwRulesPreparation;
// Selection/storage custody for one existing Civis renderer launch. This
// does not issue a native namespace mutation, frontend App or gameplay seal.
class CivisFrontendPartyBankHandoff final {
    friend class CivisFrontendPartyWorldSource;
    const FrontendSelectedSaveInput& selected_world_input_owned(const assets::CivisResourceBankOwner&)const;
    std::shared_ptr<const CivisSelectedBnwRulesPreparation> selected_rules_owned(const assets::CivisResourceBankOwner&)const;
    struct State;
    std::shared_ptr<State> state_;
    explicit CivisFrontendPartyBankHandoff(std::shared_ptr<State>);
public:
    static std::unique_ptr<CivisFrontendPartyBankHandoff> capture(
        FrontendSelectedSaveInput&&,
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const std::shared_ptr<const CivisResourceBankUiSnapshot>&,
        const std::shared_ptr<const CivisResourceBankFontColorSnapshot>&,
        const std::shared_ptr<BnwWorldViewFontSourceScope>&,
        std::shared_ptr<const CivisSelectedBnwRulesPreparation> prepared={});
    static std::unique_ptr<CivisFrontendPartyBankHandoff> select(
        const FrontendCatalogContentAdapter&,const FrontendRequest&,
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const std::shared_ptr<const CivisResourceBankUiSnapshot>&,
        const std::shared_ptr<const CivisResourceBankFontColorSnapshot>&,
        const std::shared_ptr<BnwWorldViewFontSourceScope>&,
        std::shared_ptr<const CivisSelectedBnwRulesPreparation> prepared);
    ~CivisFrontendPartyBankHandoff();
    CivisFrontendPartyBankHandoff(const CivisFrontendPartyBankHandoff&)=delete;
    CivisFrontendPartyBankHandoff& operator=(const CivisFrontendPartyBankHandoff&)=delete;
    // The caller still owns the actual renderer/window and decoded scene.
    // One attempt consumes this handoff, including a refused launch. Camera
    // scope retirement is not reversed; no fallback or implicit recapture.
    BnwLivePreviewOutcome execute(
        std::int64_t active_player,
        const std::filesystem::path& scene_save,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,
        const runtime::BnwStockLuaClientExtentProvider&,
        const BnwObservedCivisStockUiSourceCameraRender&,
        const graphics::BnwRendererSceneReceipt&,
        const BnwPrepareRendererScene&,
        const std::function<void(const SelectedBnwBootstrapResult&)>& observe={},
        const std::shared_ptr<runtime::BnwCivisApplicationStateOwner>& application={});
};
}
