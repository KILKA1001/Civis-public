#include "frontend_party_authored_lighting_source26.hpp"
#include "civis_frontend_party_world_source.hpp"
#include <stdexcept>
#include <optional>

namespace civ5::ui::detail {
namespace {
std::unique_ptr<FrontendPartyAuthoredLighting> capture_owned(
    const CivisFrontendPartyWorldSource& source,
    const std::shared_ptr<graphics::GlContext>& supplied_context,
    std::optional<graphics::DdsSamplingColor> spec_screen_color) {
    if (!supplied_context) throw std::invalid_argument("Authored lighting needs the actual GL owner");
    // After issuance, checks use the retained loan, never the source facade:
    // a callback may retire that facade while its actual storage stays alive.
    auto loan = source.begin_authored_material_inputs_owned();
    loan->require_current();
    const std::weak_ptr<graphics::GlContext> expected_context(supplied_context);
    loan->require_context_control_owned(expected_context);
    std::unique_ptr<FrontendPartyAuthoredLighting> result;
    {
        // Keep the genuine source controls through construction and destruction
        // of all local copies. The selected input remains held by this loan.
        const auto context = supplied_context;
        loan->require_current();
        loan->require_context_control_owned(expected_context);
        const auto bank = loan->bank_owned();
        loan->require_current();
        const auto window = loan->renderer_window_owned();
        loan->require_current();
        const auto& selected = loan->selected_owned();
        loan->require_current();
        if (spec_screen_color)
            result = FrontendPartyAuthoredLighting::capture_with_spec_screen(
                selected, bank, window, context, *spec_screen_color);
        else
            result = FrontendPartyAuthoredLighting::capture(selected, bank, window, context);
        loan->require_current();
        loan->require_context_control_owned(expected_context);
        result->require_boundary();
        loan->require_current();
    }
    // Copy/target destructors are also inside the final actual source POST.
    loan->require_current();
    loan->require_context_control_owned(expected_context);
    loan->finish_owned();
    return result;
}
} // namespace
std::unique_ptr<FrontendPartyAuthoredLighting> capture_frontend_party_authored_lighting(
    const CivisFrontendPartyWorldSource& source,
    const std::shared_ptr<graphics::GlContext>& context) {
    return capture_owned(source, context, std::nullopt);
}
std::unique_ptr<FrontendPartyAuthoredLighting> capture_frontend_party_authored_lighting_with_spec_screen(
    const CivisFrontendPartyWorldSource& source,
    const std::shared_ptr<graphics::GlContext>& context,
    graphics::DdsSamplingColor explicit_color) {
    // Invalid caller policy cannot open/quarantine a real load occurrence.
    if (explicit_color != graphics::DdsSamplingColor::linear &&
        explicit_color != graphics::DdsSamplingColor::srgb)
        throw std::invalid_argument("SpecScreen requires a supported explicit sampling policy");
    return capture_owned(source, context, explicit_color);
}
}
