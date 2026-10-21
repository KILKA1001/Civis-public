#pragma once
#include "civis_saved_world_loading_owner.hpp"
#include "civ5/scene/saved_scene_sources.hpp"
#include "civ5/materials/material_assets.hpp"
#include "civ5/terrain/terrain_world.hpp"
#include "civ5/terrain/terrain_auxiliary_owners.hpp"

namespace civ5::ui::detail {
// Functional alpha OWN x64 visual lineage. No Original thread snapshot,
// gameplay RNG, native pixel parity or accepted camera/depth frame is issued.
class CivisSavedWorldLoadingOwner::RendererLoadingReturn final {
    friend class CivisSavedWorldLoadingOwner;
    struct State;
    const std::shared_ptr<State> state_;
    explicit RendererLoadingReturn(std::shared_ptr<State>);
public:
    ~RendererLoadingReturn();
    RendererLoadingReturn(const RendererLoadingReturn&)=delete;
    RendererLoadingReturn& operator=(const RendererLoadingReturn&)=delete;
    void require_current()const;
    const scene::SavedSceneSources& saved_scene_owned()const;
    const terrain::TerrainAuxiliaryOwnerState& auxiliary_flags_owned()const;
    terrain::TerrainWorldInput input_owned()const;
    terrain::TerrainWorldStorage storage_owned()const;
    terrain::ReciprocalEstimate reciprocal_owned()const;
    terrain::ReciprocalSqrtEstimate reciprocal_sqrt_owned()const;
    terrain::CoastExecutionOrder execution_order_owned()const;
    std::uint32_t river_random_owned()const;
    terrain::X87Precision precision_owned()const;
    std::span<const terrain::TerrainFeature2Source> marsh_sources_owned()const;
    // SAME Source control and physical snapshot; no second loan issuance.
    void require_same_source_owned(const CivisFrontendPartyWorldSource::LoadingInputsLoan&)const;
};
}
