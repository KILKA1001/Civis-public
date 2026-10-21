#pragma once
#include "civ5/graphics/bnw_worldview_system_title_run.hpp"
namespace civ5::tests {
// Invoke ONLY with the issuer already admitted by the actual committed camera
// paint. It does not select a unit, create a title/portrait or fabricate quotes.
// This CPU/GDI hook creates and retires only its own measurement owners.
graphics::BnwWorldViewSystemTitleRunSnapshot exercise_current_worldview_system_title_run(
 std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>);
// Final negative fixture: caller performs a real publication/source retirement
// (e.g. FrameOwner notify or SAME scope retire). This callback only invalidates;
// it never supplies an authority/currentness assertion. Run after positives.
void exercise_worldview_system_title_source_retirement(
 std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>,const std::function<void()>&);
}
