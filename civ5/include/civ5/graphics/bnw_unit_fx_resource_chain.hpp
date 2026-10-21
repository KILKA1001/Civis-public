#pragma once
#include "civ5/graphics/bnw_unit_art_scale_caller.hpp"
#include "civ5/graphics/bnw_unit_fx_source_declarations.hpp"
namespace civ5::graphics {
struct BnwUnitFxSelectedResource {
    std::string request,sha256;
    std::optional<std::uint64_t> mount;
    assets::Provenance provenance;
    assets::AssetHandle handle;
};
// SAME retained caller/SQL quote -> selected FXSXML -> source texture/GR2/DGE
// handles. This does not construct a second loader/controller/backend.
class BnwUnitFxResourceChain {
 public:
    static std::shared_ptr<const BnwUnitFxResourceChain> create(
        std::shared_ptr<const BnwUnitArtScaleCaller>,BnwUnitCompiledMemberScaleQuote);
    const BnwUnitFxSelectedResource& definition()const;
    const std::vector<BnwUnitFxFileDeclaration>& declarations()const;
    const std::vector<BnwUnitFxSelectedResource>& selected_loader_dependencies()const;
    const BnwUnitFxSelectedResource& warrior_geometry_source()const;
    const BnwUnitFxSelectedResource& warrior_idle_animation_source()const;
    const BnwUnitCompiledMemberScaleQuote& compiled_scale_quote()const;
    void validate_current()const;
 private:
    struct Impl;std::shared_ptr<const Impl> impl_;
    explicit BnwUnitFxResourceChain(std::shared_ptr<const Impl>);
};
} // namespace civ5::graphics
