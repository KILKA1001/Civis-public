#pragma once
#include "civis_frontend_party_world_source.hpp"
#include <cstdint>
#include <functional>
#include <memory>

namespace civ5::terrain {class TerrainPieceLibrary;}
namespace civ5::materials {class MaterialAssets;}
namespace civ5::ui::detail {
// Owned selected-save loading INPUTS only. No TerrainWorld, native loading
// occurrence, frontend/menu completion or renderer-ready conversion exists.
class CivisSavedWorldLoadingOwner final {
    friend class CivisFrontendPartyWorldSource;
    struct State;
    struct Scope;
    const std::shared_ptr<State> state_;
    explicit CivisSavedWorldLoadingOwner(std::shared_ptr<State>);
    static std::shared_ptr<const CivisSavedWorldLoadingOwner> capture_owned(
        const CivisFrontendPartyWorldSource&);
public:
    class RendererLoadingReturn;
    std::shared_ptr<const RendererLoadingReturn> capture_renderer_loading_owned(
        const CivisFrontendPartyWorldSource::LoadingInputsLoan&,
        const terrain::TerrainPieceLibrary&,const materials::MaterialAssets&)const;
    class LoadingInputsLoan final {
        friend class CivisSavedWorldLoadingOwner;
        Scope& scope_;
        CivisFrontendPartyWorldSource::LoadingInputsLoan& source_;
        const std::uint64_t generation_;
        bool complete_=false;
        LoadingInputsLoan(Scope&,CivisFrontendPartyWorldSource::LoadingInputsLoan&);
        void finish_owned();
    public:
        ~LoadingInputsLoan();
        LoadingInputsLoan(const LoadingInputsLoan&)=delete;
        LoadingInputsLoan& operator=(const LoadingInputsLoan&)=delete;
        LoadingInputsLoan(LoadingInputsLoan&&)=delete;
        LoadingInputsLoan& operator=(LoadingInputsLoan&&)=delete;
        void require_current()const;
        const saves::SavedSessionSnapshot& snapshot_owned()const;
        const saves::SavedMapTerrainSource& terrain_source_owned()const;
        const saves::SavedMapTeamArtObservation& team_art_owned()const;
        std::span<const terrain::TerrainSourceCell> classified_owned()const;
        std::span<const terrain::TerrainAuxiliaryPlot> auxiliary_plots_owned()const;
        std::span<const terrain::TerrainRiverPlot> river_plots_owned()const;
    };
    ~CivisSavedWorldLoadingOwner();
    CivisSavedWorldLoadingOwner(const CivisSavedWorldLoadingOwner&)=delete;
    CivisSavedWorldLoadingOwner& operator=(const CivisSavedWorldLoadingOwner&)=delete;
    void require_current()const;
    void with_loading_inputs_owned(const std::function<void(const LoadingInputsLoan&)>&)const;
    // A real fresh source check then a concrete denied mandatory next edge.
    // This cannot return a WorldInput/World/ready token on a partial source.
    [[noreturn]] void advance_world_layout_450430_owned()const;
};
}
