#pragma once
#include "civis_frontend_party_material_storage.hpp"
#include <memory>
#include <thread>

namespace civ5::ui::detail {
// OWN functional alpha ocean: authored colour + the SAME generated height
// field and current camera. Reflection/animation/Original parity not issued.
class CivisBasicWaterSurface final {
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    explicit CivisBasicWaterSurface(std::shared_ptr<State>);
public:
    using FrameInputsLoan=CivisFrontendPartyMaterialStorage::FrameInputsLoan;
    // Capture once before LAND raster. Source FULL callbacks occur only at
    // explicit loan boundaries; every draw accessor is a pure borrowed read.
    static std::unique_ptr<CivisBasicWaterSurface> capture_owned(const FrameInputsLoan&);
    ~CivisBasicWaterSurface();
    CivisBasicWaterSurface(const CivisBasicWaterSurface&)=delete;
    CivisBasicWaterSurface& operator=(const CivisBasicWaterSurface&)=delete;
    // Model must bind its actual LAND colour/depth target before this call.
    // Draw writes that same depth; unchanged canonical picking consumes it.
    void draw_owned(const FrameInputsLoan&);
};
}
