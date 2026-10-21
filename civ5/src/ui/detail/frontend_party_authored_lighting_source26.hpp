#pragma once
#include "frontend_party_authored_lighting.hpp"

namespace civ5::ui::detail {
class CivisFrontendPartyWorldSource;
// Concrete consumer of the actual Source26 input issuer. Captures authored
// assets/textures only; it grants no terrain, frame or party completion.
std::unique_ptr<FrontendPartyAuthoredLighting> capture_frontend_party_authored_lighting(
    const CivisFrontendPartyWorldSource& actual_source,
    const std::shared_ptr<graphics::GlContext>& actual_context);
// Explicit own-renderer sampling policy; no default/original sampler claim.
std::unique_ptr<FrontendPartyAuthoredLighting> capture_frontend_party_authored_lighting_with_spec_screen(
    const CivisFrontendPartyWorldSource& actual_source,
    const std::shared_ptr<graphics::GlContext>& actual_context,
    graphics::DdsSamplingColor explicit_color);
}
