#pragma once
namespace civ5::ui {class BnwTechAwardContextOwner;}
namespace civ5::tests {
// Call inside the genuine producer operation after its actual stock OnPopup
// layout receiver. This verifies data and never grants paint/queue authority.
void verify_actual_tech_award_context_layout(const ui::BnwTechAwardContextOwner&);
}
