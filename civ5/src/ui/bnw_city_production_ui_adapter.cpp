#include "civ5/ui/bnw_city_production_ui_adapter.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <climits>
#include <algorithm>
#include <tuple>
#include <functional>
#include <stdexcept>
#include <thread>
namespace civ5::ui {
namespace {void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}}
struct BnwCityProductionUiAdapter::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const SelectedBnwPreviewHost> host;
 std::shared_ptr<const runtime::LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 std::function<bool()> rules_current;
 runtime::SessionCheckpointIdentity identity;std::uint64_t generation=0;
};
BnwCityProductionUiAdapter::BnwCityProductionUiAdapter(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityProductionUiAdapter::~BnwCityProductionUiAdapter()=default;
std::shared_ptr<BnwCityProductionUiAdapter> BnwCityProductionUiAdapter::from_selected_host(std::shared_ptr<const SelectedBnwPreviewHost> host){
 need(bool(host),"Production UI actual Host absent");auto p=std::make_unique<Impl>();p->host=std::move(host);const auto& session=p->host->session();
 need(session.plot_yield_refresh_thread_==p->thread,"Production UI foreign session thread");
 p->rules_current=p->host->database().immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Production UI SAME immutable rules retired");
 need(session.bnw_game_debug_owner_current(),"Production UI genuine imported Game absent");p->identity=session.debug_->identity;
 need(p->identity.payload_sha256==p->host->identity().payload_sha256&&p->identity.rules_sha256==p->host->identity().rules_sha256&&p->host->database().gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Production UI source/Host/rules identity differs");
 (void)session.bnw_game_debug_mode();p->state=session.state_;p->source=session.source_;p->generation=session.state_generation_;
 auto out=std::shared_ptr<BnwCityProductionUiAdapter>(new BnwCityProductionUiAdapter(std::move(p)));out->require_current();return out;
}
void BnwCityProductionUiAdapter::require_current()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Production UI foreign constructor thread");
 need(p.host&&p.rules_current&&p.rules_current(),"Production UI source rules retired");const auto& session=p.host->session();
 need(session.bnw_game_debug_owner_current()&&session.state_==p.state&&session.source_==p.source&&session.state_generation_==p.generation,"Production UI actual source/allocation/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&session.debug_->identity.rules_sha256==p.identity.rules_sha256,"Production UI actual import identity changed");(void)session.bnw_game_debug_mode();
 for(const auto& event:p.state->city_dirty_events)need(event.generation==p.generation,"Production UI restored dirty queue requires actual reconciliation");
}

void detail::require_production_city_screen_delta(
 const runtime::BnwCityProductionTurnProgress& old,const runtime::BnwCityProductionTurnProgress& now,
 std::uint64_t old_revision,std::uint64_t new_revision,
 std::uint64_t old_occurrence,std::uint64_t new_occurrence,std::uint64_t generation,
 std::span<const runtime::BnwCityDirtyEvent> before,std::span<const runtime::BnwCityDirtyEvent> after){
 using Phase=runtime::BnwCityProductionTurnPhase;
 need((old.phase==Phase::city_screen_predicate_pending||old.phase==Phase::city_screen_dirty_pending)&&now.phase==Phase::banner_event_pending,"Stock CityScreen native phase transition differs");
 need(std::tie(old.turn,old.owner,old.city,old.process,old.overflow_before,old.feature_before,old.difference_times100,old.project_before,old.production_needed,old.raw_writes)==std::tie(now.turn,now.owner,now.city,now.process,now.overflow_before,now.feature_before,now.difference_times100,now.project_before,now.production_needed,now.raw_writes),"Stock CityScreen changed production operands/stores");
 need(after.size()>=before.size()&&std::equal(before.begin(),before.end(),after.begin()),"Stock CityScreen replaced earlier ordered queue");
 if(after.size()==before.size()){
  need(old.phase==Phase::city_screen_predicate_pending&&new_revision==old_revision&&new_occurrence==old_occurrence&&std::uint32_t(now.native_calls)==std::uint32_t(old.native_calls)+1,"Stock CityScreen skipped predicate does not preserve exact source state");
  return;
 }
 need(after.size()-before.size()==1&&old_revision<UINT64_MAX&&old_occurrence<UINT64_MAX&&new_revision==old_revision+1&&new_occurrence==old_occurrence+1,"Stock CityScreen has unexpected publication/occurrence/revision delta");
 const auto calls=old.phase==Phase::city_screen_predicate_pending?2U:1U;
 need(std::uint32_t(now.native_calls)==std::uint32_t(old.native_calls)+calls,"Stock CityScreen native calls changed");
 const runtime::BnwCityDirtyEvent expected{old_occurrence,generation,old.owner,old.city,-1,-1,runtime::BnwCityDirtyKind::city_screen,true};
 need(after.back()==expected,"Stock CityScreen appended event identity differs");
}
runtime::BnwCityProductionTurnKey BnwCityProductionUiAdapter::require_same_stock_pending(
 const SelectedBnwPreviewHost* host,const std::shared_ptr<const runtime::BnwCityProductionTurnPending>& supplied)const{
 require_current();need(host&&host==impl_->host.get()&&supplied,"Stock CityScreen requires SAME actual adapter Host/cursor");
 const auto& session=impl_->host->session();const auto key=supplied->source_key();
 const auto found=session.production_turn_pending_.find(key);
 need(found!=session.production_turn_pending_.end()&&found->second&&found->second.get()==supplied.get(),"Stock CityScreen cursor is not exact registered SAME pending");
 supplied->require_current();return key;
}
void BnwCityProductionUiAdapter::fail_stock_pending(const std::shared_ptr<const runtime::BnwCityProductionTurnPending>& pending)noexcept{
 if(!pending||impl_->thread!=std::this_thread::get_id())return;
 try{
  const auto& session=impl_->host->session();
  for(const auto& [key,actual]:session.production_turn_pending_){
   (void)key;if(actual&&actual.get()==pending.get()){actual->fail_ui_effect();return;}
  }
  // Restore/move retires the old registered cursor itself. No arbitrary
  // supplied const object is cast into a mutable owner on a stale generation.
 }catch(...){/* The enclosing stock operation still retains uncertainty. */}
}

runtime::BnwCityProductionTurnProgress BnwCityProductionUiAdapter::continue_city(std::shared_ptr<const runtime::BnwCityProductionTurnPending> supplied){
 require_current();need(bool(supplied),"Production UI actual registered cursor absent");const auto& session=impl_->host->session();
 const auto found=session.production_turn_pending_.find(supplied->source_key());need(found!=session.production_turn_pending_.end()&&found->second&&found->second.get()==supplied.get(),"Production UI requires exact registered SAME cursor");
 const auto pending=found->second;pending->require_current();auto facts=pending->progress();using Phase=runtime::BnwCityProductionTurnPhase;
 if(facts.phase==Phase::banner_event_pending)throw std::runtime_error("Production UI actual SAME stock/world SpecificCityInfoDirty BANNER return unavailable");
 need(facts.phase==Phase::city_screen_predicate_pending||facts.phase==Phase::city_screen_dirty_pending,"Production UI cannot replay completed/non-tail operation");
 try{
  if(facts.phase==Phase::city_screen_predicate_pending){
   pending->begin_ui_effect(); // BEFORE genuine Game active-player getter
   const auto active=runtime::detail::bnw_game_active_player_read(*impl_->state);bool dirty=false;
   if(facts.owner==active){
    const auto selection=session.city_selection_.lock();need(bool(selection),"Production UI actual canonical selection owner unavailable");
    selection->require_current();dirty=selection->is_selected(facts.owner,facts.city);selection->require_current();
   }
   require_current();pending->accept_city_screen_predicate(dirty);
   if(!dirty){return pending->progress();}
   facts=pending->progress();
  }
  need(facts.phase==Phase::city_screen_dirty_pending,"Production UI CityScreen source phase differs");auto& state=*session.state_;
  need(state.city_dirty_next_occurrence<UINT64_MAX&&state.ui_source_revision<UINT64_MAX,"Production UI native notification ordinal/revision exhausted");
  auto events=state.city_dirty_events;events.push_back({state.city_dirty_next_occurrence,impl_->generation,facts.owner,facts.city,-1,-1,runtime::BnwCityDirtyKind::city_screen,true});
  require_current();pending->require_current();pending->begin_ui_effect();
  state.city_dirty_events.swap(events);++state.city_dirty_next_occurrence;++state.ui_source_revision;
  pending->accept_city_screen_publication();require_current();return pending->progress();
 }catch(...){pending->fail_ui_effect();throw;}
}
}
