#pragma once
#include "civis_frontend_party_bank_handoff.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/materials/material_world.hpp"
#include <memory>

namespace civ5::ui::detail {
// Actual saved-scene CPU/Bank suppliers for the existing handoff. World
// construction, GL draw and presentation still belong to the renderer.
class CivisFrontendPartySceneSource final {
    struct State;
    std::shared_ptr<State> state_;
    explicit CivisFrontendPartySceneSource(std::shared_ptr<State>);
public:
    static std::unique_ptr<CivisFrontendPartySceneSource> select(
        const FrontendCatalogContentAdapter&,const FrontendRequest&,
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const std::shared_ptr<const materials::MaterialWorld>&,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,
        const std::shared_ptr<const CivisSelectedBnwRulesPreparation>&);
    ~CivisFrontendPartySceneSource();
    CivisFrontendPartySceneSource(const CivisFrontendPartySceneSource&)=delete;
    CivisFrontendPartySceneSource& operator=(const CivisFrontendPartySceneSource&)=delete;
    CivisFrontendPartyBankHandoff& handoff()const;
    const std::filesystem::path& scene_save()const;
    std::int64_t active_player()const;
    const graphics::BnwRendererSceneReceipt& scene_receipt()const;
    const saves::SavedMapProjection& projection()const;
    const saves::SavedMapTeamArtObservation& team_art()const;
    const std::shared_ptr<const materials::MaterialWorld>& material_world()const;
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& renderer_window()const;
    const runtime::BnwStockLuaClientExtentProvider& client_provider()const;
    // Produces the actual reconciliation callback from this decoded scene.
    // It accepts no substitute callback or metadata-ready predicate.
    BnwPrepareRendererScene prepare_scene()const;
};
}
