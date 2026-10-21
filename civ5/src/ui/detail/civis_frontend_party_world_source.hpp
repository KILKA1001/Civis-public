#pragma once
#include "civis_frontend_party_bank_handoff.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/terrain/terrain_source_grid.hpp"
#include "civ5/terrain/terrain_auxiliary.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"
#include "civ5/graphics/gl_context.hpp"
#include <memory>
#include <span>
#include <thread>

namespace civ5::ui::detail {
class CivisSavedWorldLoadingOwner;
class CivisFrontendPartyMaterialStorage;
class CivisFrontendAlphaLandFrame;
// Owns the genuine early saved inputs. This source stage issues neither a
// completed terrain generation nor a game frame or frontend completion.
class CivisFrontendPartyWorldSource final {
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    explicit CivisFrontendPartyWorldSource(std::shared_ptr<State>);
    friend class CivisSavedWorldLoadingOwner;
    friend class CivisFrontendPartyMaterialStorage;
public:
    class LoadingInputsLoan;
    class LoadingControl final {
        friend class CivisFrontendPartyWorldSource;
        friend class LoadingInputsLoan;
        friend class CivisSavedWorldLoadingOwner;
        friend class CivisFrontendPartyMaterialStorage;
        friend class CivisFrontendAlphaLandFrame;
        const std::thread::id creator_;
        const std::weak_ptr<State> state_;
        const std::weak_ptr<const CivisFrontendPartyWorldSource> facade_;
        explicit LoadingControl(const std::shared_ptr<State>&);
        void require_current_owned()const;
    };
    class LoadingInputsLoan final {
        friend class CivisFrontendPartyWorldSource;
        friend class CivisSavedWorldLoadingOwner;
        friend class CivisFrontendPartyMaterialStorage;
        const std::thread::id creator_;
        const std::shared_ptr<State> state_;
        const std::uint64_t generation_;
        bool returned_=false;
        explicit LoadingInputsLoan(std::shared_ptr<State>);
        void first()const;
        std::shared_ptr<const LoadingControl> loading_control_owned_without_callbacks()const;
        std::weak_ptr<const CivisFrontendPartyWorldSource> source_control_owned_without_callbacks()const;
    public:
        ~LoadingInputsLoan();
        LoadingInputsLoan(const LoadingInputsLoan&)=delete;
        LoadingInputsLoan& operator=(const LoadingInputsLoan&)=delete;
        LoadingInputsLoan(LoadingInputsLoan&&)=delete;
        void require_current()const;
        void require_context_control_owned(const std::weak_ptr<graphics::GlContext>&)const;
        const FrontendSelectedSaveInput& selected_owned()const;
        const std::shared_ptr<assets::CivisResourceBankOwner>& bank_owned()const;
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& renderer_window_owned()const;
        const saves::SavedSessionSnapshot& snapshot_owned()const;
        const saves::SavedMapTerrainSource& terrain_source_owned()const;
        const saves::SavedMapTeamArtObservation& team_art_owned()const;
        std::span<const terrain::TerrainSourceCell> classified_owned()const;
        std::span<const terrain::TerrainAuxiliaryPlot> auxiliary_plots_owned()const;
        std::span<const terrain::TerrainRiverPlot> river_plots_owned()const;
        // Call after the consumer's callback/target destructors, before return.
        void finish_owned();
    };
    static std::shared_ptr<CivisFrontendPartyWorldSource> capture(
        std::unique_ptr<CivisFrontendPartyBankHandoff>&& actual_selected_handoff,
        const std::shared_ptr<assets::CivisResourceBankOwner>& actual_bank,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& actual_window,
        const std::shared_ptr<graphics::GlContext>& actual_context);
    ~CivisFrontendPartyWorldSource();
    CivisFrontendPartyWorldSource(const CivisFrontendPartyWorldSource&)=delete;
    CivisFrontendPartyWorldSource& operator=(const CivisFrontendPartyWorldSource&)=delete;
    void require_current()const;
    std::shared_ptr<const CivisSelectedBnwRulesPreparation> selected_rules_owned()const;
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> renderer_window_owned()const;
    std::shared_ptr<const CivisSavedWorldLoadingOwner> loading_inputs_owned()const;
    // Same closed issuer for authored resource construction; this capability
    // authenticates Input/Bank/context, not unfinished terrain loading.
    std::unique_ptr<LoadingInputsLoan> begin_authored_material_inputs_owned()const;
    // Real Source29 child call is defined in its disjoint entry fragment.
    std::shared_ptr<const CivisSavedWorldLoadingOwner> capture_loading_inputs_owned()const;
private:
    std::unique_ptr<LoadingInputsLoan> begin_loading_inputs_owned()const;
    static std::unique_ptr<LoadingInputsLoan> begin_loading_inputs_from_control_owned(const LoadingControl&);
    static std::unique_ptr<LoadingInputsLoan> issue_loading_owned(const std::shared_ptr<State>&);
};
}
