#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/graphics/gl_color_keys.hpp"
#include "civ5/graphics/gl_distance_fog.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/frontend_selected_save_input.hpp"
#include <memory>
#include <thread>

namespace civ5::ui::detail {
// Held authored assets and real textures for the actual selected Bank/window.
// This component is not a WorldSource, scene binding, launch or frame receipt.
// Capture/boundary operations run outside client/render callbacks. Immutable
// getters use read-only owner/WGL checks; storage must survive synchronous
// consumers and retire on the creating GL thread before its window is retired.
class FrontendPartyAuthoredLighting final {
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    std::shared_ptr<State> state_;
    explicit FrontendPartyAuthoredLighting(std::shared_ptr<State>);
public:
    static std::unique_ptr<FrontendPartyAuthoredLighting> capture(
        const FrontendSelectedSaveInput&,
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,
        const std::shared_ptr<graphics::GlContext>&);
    // Adds the actual authored specscreen DDS. Sampling decode is an explicit
    // caller x64 policy, not an inferred Original sampler/parity declaration.
    static std::unique_ptr<FrontendPartyAuthoredLighting> capture_with_spec_screen(
        const FrontendSelectedSaveInput&,
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,
        const std::shared_ptr<graphics::GlContext>&,
        graphics::DdsSamplingColor explicit_color);
    ~FrontendPartyAuthoredLighting();
    FrontendPartyAuthoredLighting(const FrontendPartyAuthoredLighting&)=delete;
    FrontendPartyAuthoredLighting& operator=(const FrontendPartyAuthoredLighting&)=delete;
    void require_current() const; // Creator/Bank/identity and read-only WGL queries; no effects.
    void require_boundary() const; // Explicit file/window checks, never per-paint.
    const materials::LightingCubeAssets& lighting_assets() const;
    const materials::DistanceFogAssets& fog_assets() const;
    const materials::ColorKeyAssets& color_key_assets() const;
    const graphics::GlLightingCubes& lighting_cubes() const;
    const graphics::GlDistanceFog& distance_fog() const;
    const graphics::GlColorKeys& color_keys() const;
    // Same GPU object/control as color_keys(), created by one real allocation.
    // Retain this component/context until shared consumers retire on its GL thread.
    std::shared_ptr<const graphics::GlColorKeys> color_keys_owned() const;
    std::array<GLuint,5> color_correct() const;
    // Absent only for the old four-argument factory; these getters then throw.
    // The component must remain held through use of the borrowed typed owners.
    const graphics::GlTexture& spec_screen() const;
    const assets::AssetHandle& spec_screen_asset() const;
    const assets::DdsData& spec_screen_data() const;
    graphics::DdsSamplingColor spec_screen_sampling_color() const;
};
}
