#pragma once
#include <memory>
namespace civ5::ui {class LiveUiHost;class BnwTechAwardInputDispatch;struct LiveUiFrame;}
namespace civ5::tests {
// Called with actual before frame, SourceOwner-dispatched once token and new
// real UiHost-issued frame. It creates no owner, VM, callback or synthetic input.
void verify_actual_tech_award_queued_input(ui::LiveUiHost&,
 const ui::LiveUiFrame& before,const std::shared_ptr<const ui::BnwTechAwardInputDispatch>&,
 const ui::LiveUiFrame& after);
// Called before SourceOwner dispatch while the genuine first reservation is
// alive. A second click must fail without creating another source operation.
void verify_actual_tech_award_double_reservation(ui::LiveUiHost&,const ui::LiveUiFrame&,
 const std::shared_ptr<const ui::BnwTechAwardInputDispatch>&);
// Real old/new frames after actual closing, not a fabricated stale DTO.
void verify_actual_tech_award_stale_input(ui::LiveUiHost&,const ui::LiveUiFrame& old,
 const ui::LiveUiFrame& after);
// Requires a naturally failed SourceOwner/Lua token. Does not inject a listener,
// mutate a closure/VM, manufacture an exception or claim failure was exercised.
void verify_actual_tech_award_failed_input(ui::LiveUiHost&,
 const std::shared_ptr<const ui::BnwTechAwardInputDispatch>&,const ui::LiveUiFrame&);
}
