#include "civ5/ui/bnw_owned_notification_publication_owner.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <limits>
#include <algorithm>
#include <stdexcept>
namespace civ5::ui {
BnwOwnedNotificationPublicationOwner::BnwOwnedNotificationPublicationOwner(
 std::weak_ptr<const LiveUiHost> ui,std::weak_ptr<const SelectedBnwPreviewHost> host,
 std::weak_ptr<const runtime::LiveSessionState> world,std::weak_ptr<const BnwEngineUiTurnOwner> engine,std::uint64_t generation)
 :ui_(std::move(ui)),host_(std::move(host)),world_(std::move(world)),engine_(std::move(engine)),generation_(generation){}
void BnwOwnedNotificationPublicationOwner::merge_frame(LiveUiFrame& frame,const LiveUiHost& ui)const{
 if(creator_!=std::this_thread::get_id()||progress_.failed||!busy_||frame.owner!=&ui||frame.sequence!=ui.sequence_||
    ui.notification_publication_owner_.get()!=this||!frame.snapshot.active_player_id()||!frame.snapshot.turn())
  throw std::runtime_error("Notification frame merge lacks actual receiving UI/frame");
 const auto actor=*frame.snapshot.active_player_id();
 BnwOwnedNotificationFrameFeed feed;feed.actor=actor;feed.model_revision=progress_.model_revision;feed.received_prefix=progress_.received_prefix;
 std::vector<std::int64_t> removals;
 for(const auto& [key,e]:entries_)if(key.first==actor)feed.entries.push_back(e);
 for(const auto& key:removed_)if(key.first==actor)removals.push_back(key.second);
 std::sort(feed.entries.begin(),feed.entries.end(),[](const auto&a,const auto&b){return a.occurrence<b.occurrence;});
 auto next=LiveNotificationFrameMerge::merge(frame.snapshot,feed.entries,removals);
 const auto published=std::make_shared<const BnwOwnedNotificationFrameFeed>(std::move(feed));
 next=next.with<BnwOwnedNotificationFrameFeed>(published);
 frame.snapshot=std::move(next);
 issued_frame_feed_=published;issued_ui_sequence_=frame.sequence;issued_turn_=frame.snapshot.turn();
 // Actual value capture history precedes later provider bookends. Failed
 // late bookends quarantine this owner; binding cannot then be issued.
 // This actual captured UI value does not earn original native FX/minimap,
 // input availability, layout-art fidelity or physical presentation.
}
}

namespace civ5::ui {
void BnwOwnedNotificationPublicationOwner::require_frame_binding_without_callbacks(const LiveUiFrame& frame)const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification model binding foreign creator");
 if(progress_.failed||busy_||!issued_frame_feed_)throw std::runtime_error("Notification model binding failed/busy/unissued");
 const auto self=self_.lock();const auto ui=ui_.lock();const auto engine=engine_.lock();const auto host=host_.lock();const auto world=world_.lock();
 if(!self||self.get()!=this||!ui||!engine||!host||!world||ui->notification_publication_owner_.get()!=this||
  ui->engine_ui_turn_owner_.get()!=engine.get()||ui->plot_yield_ui_thread_!=creator_||
  ui->notification_publication_owner_.owner_before(self)||self.owner_before(ui->notification_publication_owner_)||
  ui->engine_ui_turn_owner_.owner_before(engine)||engine.owner_before(ui->engine_ui_turn_owner_)||
  ui->selected_owner_lifetime_.owner_before(ui_)||ui_.owner_before(ui->selected_owner_lifetime_))
  throw std::runtime_error("Notification model binding canonical UI/Engine/owner retired");
 // Existing session lease/generation reads are pure owner checks, never
 // Rules/Lua/Scene/native-window callbacks or copied function targets.
 const auto actual_world=host->session().read_state_lease();
 if(actual_world.get()!=world.get()||actual_world.owner_before(world)||world.owner_before(actual_world)||
  host->session().state_generation()!=generation_||frame.owner!=ui.get()||frame.sequence!=issued_ui_sequence_||frame.sequence!=ui->sequence_||
  frame.snapshot.active_player_id()!=host->active_player_id()||frame.snapshot.turn()!=world->setup.game_turn||issued_turn_!=frame.snapshot.turn()||
  frame.snapshot.find<BnwOwnedNotificationFrameFeed>()!=issued_frame_feed_.get()||
  issued_frame_feed_->actor!=host->active_player_id()||issued_frame_feed_->model_revision!=progress_.model_revision||
  issued_frame_feed_->received_prefix!=progress_.received_prefix)
  throw std::runtime_error("Notification model binding is not the current genuinely issued frame/feed");
}
std::unique_ptr<BnwNotificationOwnerModelLoan>
BnwOwnedNotificationPublicationOwner::capture_notification_model_for_graphics_without_callbacks(const LiveUiFrame& frame)const{
 require_frame_binding_without_callbacks(frame);
 auto result=std::unique_ptr<BnwNotificationOwnerModelLoan>(new BnwNotificationOwnerModelLoan(*this,frame));
 require_frame_binding_without_callbacks(frame);result->require_origin_without_callbacks();return result;
}
BnwNotificationOwnerModelLoan::BnwNotificationOwnerModelLoan(const BnwOwnedNotificationPublicationOwner& owner,const LiveUiFrame& frame)
 :owner_(owner.self_.lock()),ui_(owner.ui_),engine_(owner.engine_),host_(owner.host_),world_(owner.world_),feed_(owner.issued_frame_feed_),
  generation_(owner.generation_),ui_sequence_(frame.sequence){
 if(!owner_||owner_.get()!=&owner||creator_!=owner.creator_||owner.model_loan_pins_==std::numeric_limits<std::size_t>::max())
  throw std::runtime_error("Notification authentic model borrower absent/foreign/exhausted");
 ++owner.model_loan_pins_;counted_=true;
 try{require_origin_without_callbacks();}
 catch(...){--owner.model_loan_pins_;counted_=false;throw;}
}
BnwNotificationOwnerModelLoan::~BnwNotificationOwnerModelLoan(){
 if(creator_!=std::this_thread::get_id())std::terminate();
 if(counted_){if(!owner_||owner_->model_loan_pins_==0)std::terminate();--owner_->model_loan_pins_;counted_=false;}
 owner_.reset();
}
void BnwNotificationOwnerModelLoan::require_origin_without_callbacks()const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification retained model binding foreign creator");
 if(!owner_||!counted_)throw std::runtime_error("Notification model borrower uncounted/retired");
 owner_->require_retained_model_without_callbacks(*this);
}
void BnwOwnedNotificationPublicationOwner::require_retained_model_without_callbacks(const BnwNotificationOwnerModelLoan& loan)const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification model owner foreign creator");
 if(loan.creator_!=creator_||loan.owner_.get()!=this||!loan.counted_||model_loan_pins_==0||
  model_loan_pins_>=static_cast<std::size_t>(std::numeric_limits<long>::max())||loan.owner_.use_count()<=static_cast<long>(model_loan_pins_)||
  progress_.failed||busy_||generation_!=loan.generation_||issued_ui_sequence_!=loan.ui_sequence_||issued_frame_feed_.get()!=loan.feed_.get()||
  issued_frame_feed_.owner_before(loan.feed_)||loan.feed_.owner_before(issued_frame_feed_)||
  ui_.owner_before(loan.ui_)||loan.ui_.owner_before(ui_)||engine_.owner_before(loan.engine_)||loan.engine_.owner_before(engine_)||
  host_.owner_before(loan.host_)||loan.host_.owner_before(host_)||world_.owner_before(loan.world_)||loan.world_.owner_before(world_))
  throw std::runtime_error("Notification retained model binding source/payload retired");
 const auto ui=ui_.lock();const auto host=host_.lock();const auto world=world_.lock();const auto engine=engine_.lock();
 if(!ui||!host||!world||!engine||ui->notification_publication_owner_.get()!=this||ui->engine_ui_turn_owner_.get()!=engine.get()||
  ui->notification_publication_owner_.owner_before(loan.owner_)||loan.owner_.owner_before(ui->notification_publication_owner_)||
  ui->engine_ui_turn_owner_.owner_before(engine)||engine.owner_before(ui->engine_ui_turn_owner_)||
  ui->selected_owner_lifetime_.owner_before(ui_)||ui_.owner_before(ui->selected_owner_lifetime_)||
  ui->plot_yield_ui_thread_!=creator_||ui->sequence_!=issued_ui_sequence_||!loan.feed_||
  loan.feed_->actor!=host->active_player_id()||loan.feed_->model_revision!=progress_.model_revision||loan.feed_->received_prefix!=progress_.received_prefix||
  issued_turn_!=world->setup.game_turn||host->session().state_generation()!=generation_)
  throw std::runtime_error("Notification retained model binding canonical current owner lost");
 const auto current=host->session().read_state_lease();
 if(current.get()!=world.get()||current.owner_before(world)||world.owner_before(current))
  throw std::runtime_error("Notification retained model binding SAME session allocation changed");
 // Current model binding deliberately cannot attest Scene/GUI/HWND, supplied
 // Source/Lease, nullable plot FX, minimap, authored draw or AFTER_SWAP.
}
void BnwNotificationOwnerModelLoan::require_same_frame_payload_without_callbacks(const LiveUiFrame& frame)const{
 require_origin_without_callbacks();owner_->require_frame_binding_without_callbacks(frame);require_origin_without_callbacks();
}
void BnwNotificationOwnerModelLoan::require_source_origin_without_callbacks(
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,const runtime::BnwStockLuaAttemptLease& lease)const{
 require_origin_without_callbacks();
 const auto engine=engine_.lock();if(!engine)throw std::runtime_error("Notification model Engine retired before source admission");
 // Qualified Engine69 delegate is required; no copied private StockLease
 // implementation or inaccessible Source helper is invented by the owner.
 engine->require_notification_model_origin_without_callbacks(*owner_,pump,lease);
 require_origin_without_callbacks();
}
BnwEngineUiNotificationEffect BnwNotificationOwnerModelLoan::pending_effect()const{
 require_origin_without_callbacks();
 const auto engine=engine_.lock();if(!engine)throw std::runtime_error("Notification model Engine retired before native head query");
 auto effect=engine->notification_model_head_without_callbacks(*owner_);
 require_origin_without_callbacks();return effect;
}
}

namespace civ5::ui {
void BnwNotificationOwnerModelLoan::quarantine_without_callbacks()const noexcept{
 // A foreign creator attempt must not race/poison a legitimate entered UI.
 // Legitimate creator-side late failure remains sticky after retirement.
 if(creator_!=std::this_thread::get_id())return;
 if(owner_&&counted_)owner_->quarantine_model_loan_without_callbacks(*this);
}
void BnwOwnedNotificationPublicationOwner::quarantine_model_loan_without_callbacks(
 const BnwNotificationOwnerModelLoan& loan)const noexcept{
 if(creator_!=std::this_thread::get_id()||loan.creator_!=creator_)return;
 // No currentness callback, weak-owner resolution, effects, throw or reset.
 // The actual counted token is retained by its private immutable factory.
 if(loan.owner_.get()!=this||!loan.counted_||model_loan_pins_==0)return;
 progress_.failed=true;
}
}
