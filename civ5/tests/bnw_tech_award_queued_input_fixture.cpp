#include "bnw_tech_award_queued_input_fixture.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/ui/bnw_tech_award_context_snapshot.hpp"
#include "civ5/ui/bnw_tech_award_modal_snapshot.hpp"
#include <stdexcept>
namespace civ5::tests {
void verify_actual_tech_award_queued_input(ui::LiveUiHost& host,const ui::LiveUiFrame& before,
 const std::shared_ptr<const ui::BnwTechAwardInputDispatch>& input,const ui::LiveUiFrame& after) {
 const auto need=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 need(bool(input),"UNAVAILABLE: genuine queued input dispatch absent");
 const auto* old=before.snapshot.find<ui::BnwTechAwardContextSnapshot>();
 const auto* old_modal=before.snapshot.find<ui::BnwTechAwardModalSnapshot>();
 const auto* modal=after.snapshot.find<ui::BnwTechAwardModalSnapshot>();
 const auto* stamp=after.snapshot.find<ui::BnwClientFrameStamp>();
 need(old&&old_modal&&modal&&stamp,"Genuine source popup before/new modal frame missing");
 need(old->context.visible&&old->context.queued&&old_modal->shown_received&&old_modal->timer_semaphore==1,
      "Queued input fixture did not start from actual original ShowHide/open popup");
 need(input->phase()==ui::BnwTechAwardInputPhase::pending_frame,
      "Actual original registered input/Dequeue/ShowHide dispatch not complete");
 need(!modal->modal_queued&&!modal->source_showhide_pending&&modal->timer_semaphore==0&&
      modal->processed.size()==old_modal->processed.size()+1&&modal->occurrence==old->occurrence&&
      !after.snapshot.find<ui::BnwTechAwardContextSnapshot>(),
      "Queued input omitted ordered timer/processed/closed-context effects");
 const auto& a=old->publication.identity;const auto& z=stamp->identity;
 need(a.session==z.session&&a.player==z.player&&a.team==z.team&&a.turn==z.turn&&a.map_guid==z.map_guid&&a.ui_source_revision==z.ui_source_revision,
      "Popup-only input changed actual world identity/earned technology revision");
 host.complete_tech_award_input_after_frame(input,after);
 need(input->phase()==ui::BnwTechAwardInputPhase::completed,"Actual new frame did not close input publication");
 bool replay_refused=false;try{host.complete_tech_award_input_after_frame(input,after);}catch(const std::exception&){replay_refused=true;}
 need(replay_refused,"Queued popup input frame completion replay accepted");
}
}

namespace civ5::tests {
namespace {void input_check(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
void verify_actual_tech_award_double_reservation(ui::LiveUiHost& host,const ui::LiveUiFrame& frame,
 const std::shared_ptr<const ui::BnwTechAwardInputDispatch>& first) {
 input_check(first&&first->phase()==ui::BnwTechAwardInputPhase::reserved,"UNAVAILABLE: genuine unexecuted first input reservation required");
 first->require_current();bool refused=false;
 try{(void)host.prepare_tech_award_input(frame,ui::BnwTechAwardInput::close_button);}catch(const std::exception&){refused=true;}
 input_check(refused&&first->phase()==ui::BnwTechAwardInputPhase::reserved,"Double click replaced/consumed first real token");
 first->require_current();
 const auto* modal=frame.snapshot.find<ui::BnwTechAwardModalSnapshot>();
 input_check(modal&&modal->modal_queued&&modal->timer_semaphore==1,"Double reservation changed native modal state before dispatch");
}
void verify_actual_tech_award_stale_input(ui::LiveUiHost& host,const ui::LiveUiFrame& old,const ui::LiveUiFrame& after) {
 const auto* prior=old.snapshot.find<ui::BnwTechAwardContextSnapshot>();
 const auto* closed=after.snapshot.find<ui::BnwTechAwardModalSnapshot>();
 input_check(prior&&closed&&after.sequence>old.sequence&&!closed->modal_queued&&!closed->source_showhide_pending&&closed->timer_semaphore==0,
             "UNAVAILABLE: actual open-to-closed UI frame pair required");
 bool stale=false,closed_refused=false;
 try{(void)host.prepare_tech_award_input(old,ui::BnwTechAwardInput::continue_button);}catch(const std::exception&){stale=true;}
 try{(void)host.prepare_tech_award_input(after,ui::BnwTechAwardInput::return_key);}catch(const std::exception&){closed_refused=true;}
 input_check(stale&&closed_refused&&host.is_current_frame(after),"Old/closed context input accepted or current closed frame damaged");
}
void verify_actual_tech_award_failed_input(ui::LiveUiHost& host,
 const std::shared_ptr<const ui::BnwTechAwardInputDispatch>& failed,const ui::LiveUiFrame& actual_frame) {
 input_check(failed&&failed->phase()==ui::BnwTechAwardInputPhase::uncertain,"UNAVAILABLE: naturally failed genuine Lua/SourceOwner token required");
 bool refused=false;
 try{host.complete_tech_award_input_after_frame(failed,actual_frame);}catch(const std::exception&){refused=true;}
 input_check(refused&&failed->phase()==ui::BnwTechAwardInputPhase::uncertain,"Actual failed input was credited as completed frame");
}
}
