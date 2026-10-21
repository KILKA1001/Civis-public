#pragma once
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/frontend_selected_save_input.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <memory>
#include <thread>

namespace civ5::ui::detail {
struct CivisRulesPreparationSink;
// The once-created output of the existing Civis rules producer. It contains
// real activated rules, the final Host clone/caches and an owned feature
// snapshot. It publishes no Host, LiveSession, renderer or readiness flag.
class CivisSelectedBnwRulesPreparation final {
    friend struct CivisRulesPreparationSink;
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    explicit CivisSelectedBnwRulesPreparation(std::shared_ptr<State>);
    SelectedBnwBootstrapResult adopt_activated_owned(
        std::unique_ptr<content::RulesDatabase>&&,
        std::optional<content::LooseResourcePlan>&&,
        SelectedBnwBootstrapResult&&, BnwSourceDecodeContract&&,
        const runtime::FileIdentity&, const saves::SaveHeader&);
public:
    static std::shared_ptr<const CivisSelectedBnwRulesPreparation> capture_owned(
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const FrontendSelectedSaveInput&);
    ~CivisSelectedBnwRulesPreparation();
    CivisSelectedBnwRulesPreparation(const CivisSelectedBnwRulesPreparation&)=delete;
    CivisSelectedBnwRulesPreparation& operator=(const CivisSelectedBnwRulesPreparation&)=delete;
    std::shared_ptr<const CivisSelectedBnwRulesPreparation> retain_owner()const;
    void require_bank_owned(const assets::CivisResourceBankOwner&)const;
    void require_selected_owned(const assets::CivisResourceBankOwner&,
        const FrontendSelectedSaveInput&)const;
    saves::SavedTerrainFeatureResolver terrain_features_owned(
        const assets::CivisResourceBankOwner&,const FrontendSelectedSaveInput&)const;
    // Reuses the prepared Host and its exact clone/types. Only the remaining
    // original decode/restore/publication phase runs, once, before the Camera.
    SelectedBnwBootstrapResult restore_host_owned(
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        std::int64_t actual_active_player)const;
};
}
