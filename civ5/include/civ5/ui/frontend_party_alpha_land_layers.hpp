#pragma once
#include "civ5/graphics/gl_land_material.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include <memory>
#include <thread>

namespace civ5::ui::detail {class CivisFrontendPartyWorldSource;}
namespace civ5::ui {
// Explicit own x64 Alpha material policy. These real cleared textures do not
// claim Original overlay/shadow production, a native branch, or a game frame.
// Retain this owner through draw; destroy on its GL thread before the window.
class FrontendPartyAlphaLandLayers final {
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    std::shared_ptr<State> state_;
    explicit FrontendPartyAlphaLandLayers(std::shared_ptr<State>);
public:
    enum class OverlayPolicy { OverlayNone };
    enum class ShadowPolicy { ShadowDisabled };
    static std::unique_ptr<FrontendPartyAlphaLandLayers> capture(
        const detail::CivisFrontendPartyWorldSource& actual_source,
        const std::shared_ptr<graphics::GlContext>& actual_context,
        OverlayPolicy,ShadowPolicy);
    ~FrontendPartyAlphaLandLayers();
    FrontendPartyAlphaLandLayers(const FrontendPartyAlphaLandLayers&)=delete;
    FrontendPartyAlphaLandLayers& operator=(const FrontendPartyAlphaLandLayers&)=delete;
    void require_current() const; // Read-only owner/WGL checks; no callbacks/IO/GL effects.
    void require_boundary() const; // Actual Bank/window/file checks outside callbacks.
    const graphics::GlTexture& overlay() const;
    const graphics::GlTexture& shadow() const;
    // Only slots1/3 and shadow_fade/projection are changed. The existing actual
    // world/view, height, depth, picking and other frame state stay with caller.
    // Finite transformed coordinates/intermediates remain the renderer's duty.
    void apply_to(materials::LandMaterialConstants&,graphics::GlLandMaterialInputs&) const;
};
}
