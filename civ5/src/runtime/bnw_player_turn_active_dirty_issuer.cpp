#include "civ5/runtime/bnw_player_turn_active_dirty_issuer.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include <thread>
#include <stdexcept>
#include <exception>
namespace civ5::runtime {
namespace {void dirty_need(bool b,const char* m){if(!b)throw std::runtime_error(m);}}
struct BnwPlayerTurnActiveDirtyIssuer::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<BnwSelectedStockLuaSourceOwner> source;std::weak_ptr<BnwRetainedTurnCursor> cursor;
 std::weak_ptr<const void> bootstrap,control;std::weak_ptr<const LiveSessionState> world;
 std::uint64_t generation=0,serial=0,occurrence=0,delivery_serial=0;
 const ui::BnwEngineUiEndTurnQueue* queue=nullptr;const ui::BnwEngineUiEndTurnQueue::Event* event=nullptr;
 bool queued=false,publisher_returned=false,delivery_attempted=false,delivered=false;
 void thread()const {dirty_need(creator==std::this_thread::get_id(),"Player dirty issuer foreign creator");}
 void pure_thread()const noexcept{if(creator!=std::this_thread::get_id())std::terminate();}
};
BnwPlayerTurnActiveDirtyIssuer::BnwPlayerTurnActiveDirtyIssuer(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwPlayerTurnActiveDirtyIssuer::~BnwPlayerTurnActiveDirtyIssuer()=default;
std::shared_ptr<const BnwPlayerTurnActiveDirtyIssuer> BnwPlayerTurnActiveDirtyIssuer::create_owned(
 std::shared_ptr<BnwSelectedStockLuaSourceOwner> source,std::shared_ptr<BnwRetainedTurnCursor> cursor,
 std::weak_ptr<const void> bootstrap,std::weak_ptr<const void> control,std::shared_ptr<const LiveSessionState> world,
 std::uint64_t generation,std::uint64_t serial){
 dirty_need(source&&cursor&&world&&!bootstrap.expired()&&!control.expired()&&serial,"Player dirty factory incomplete");
 auto p=std::make_shared<Impl>();p->source=source;p->cursor=cursor;p->bootstrap=std::move(bootstrap);p->control=std::move(control);
 p->world=world;p->generation=generation;p->serial=serial;
 return std::shared_ptr<const BnwPlayerTurnActiveDirtyIssuer>(new BnwPlayerTurnActiveDirtyIssuer(std::move(p)));
}
void BnwPlayerTurnActiveDirtyIssuer::require_source_lineage_without_callbacks(const BnwSelectedStockLuaSourceOwner& source,
 const BnwRetainedTurnCursor& cursor,const void* bootstrap,const void* control,const LiveSessionState& world,
 std::uint64_t generation,std::uint64_t serial)const{
 const auto p=impl_;p->thread();const auto s=p->source.lock();const auto c=p->cursor.lock();const auto b=p->bootstrap.lock();
 const auto op=p->control.lock();const auto w=p->world.lock();
 dirty_need(s&&s.get()==&source&&c&&c.get()==&cursor&&b&&b.get()==bootstrap&&op&&op.get()==control&&w&&w.get()==&world&&
  generation==p->generation&&serial>=p->serial,"Player dirty actual source/control/world lineage changed");
}
void BnwPlayerTurnActiveDirtyIssuer::require_before_publication_without_callbacks(const ui::BnwEngineUiTurnOwner& engine,
 const BnwStockLuaAttemptLease& lease)const{
 const auto p=impl_;p->thread();const auto s=p->source.lock();const auto c=p->cursor.lock();
 dirty_need(s&&c&&!p->publisher_returned&&!p->delivery_attempted&&!p->delivered,"Player dirty publisher already returned/retired");
 s->require_player_turn_dirty_source_without_callbacks(*this,*c,lease,true);
 c->require_player_turn_dirty_operands_without_callbacks(*this,engine,*s);
}
void BnwPlayerTurnActiveDirtyIssuer::require_before_delivery_without_callbacks(const ui::BnwEngineUiTurnOwner& engine,
 const BnwStockLuaAttemptLease& lease)const{
 const auto p=impl_;p->thread();const auto s=p->source.lock();const auto c=p->cursor.lock();
 dirty_need(s&&c&&p->queued&&p->publisher_returned&&!p->delivery_attempted&&!p->delivered,"Player dirty delivery lacks returned publisher");
 s->require_player_turn_dirty_source_without_callbacks(*this,*c,lease,false);
 c->require_player_turn_dirty_operands_without_callbacks(*this,engine,*s);
}
void BnwPlayerTurnActiveDirtyIssuer::record_queued_without_callbacks(const ui::BnwEngineUiEndTurnQueue& queue,
 const ui::BnwEngineUiEndTurnQueue::Event& event)const noexcept{
 const auto p=impl_;p->pure_thread();if(p->queued||event.player_dirty_issuer.get()!=this||event.operation!=p->serial)std::terminate();
 p->queue=&queue;p->event=&event;p->occurrence=event.occurrence;p->queued=true;
}
void BnwPlayerTurnActiveDirtyIssuer::record_publisher_return_without_callbacks(const ui::BnwEngineUiEndTurnQueue& queue,
 const ui::BnwEngineUiEndTurnQueue::Event& event)const noexcept{
 const auto p=impl_;p->pure_thread();if(!p->queued||p->publisher_returned||p->queue!=&queue||p->event!=&event||
  event.player_dirty_issuer.get()!=this||event.occurrence!=p->occurrence||!event.setter_returned)std::terminate();
 p->publisher_returned=true;
}
void BnwPlayerTurnActiveDirtyIssuer::record_delivery_attempt_without_callbacks(const ui::BnwEngineUiEndTurnQueue& queue,
 const ui::BnwEngineUiEndTurnQueue::Event& event)const noexcept{
 const auto p=impl_;p->pure_thread();if(!p->publisher_returned||p->delivery_attempted||p->queue!=&queue||p->event!=&event||
  event.player_dirty_issuer.get()!=this||event.occurrence!=p->occurrence)std::terminate();p->delivery_attempted=true;
}
void BnwPlayerTurnActiveDirtyIssuer::record_delivered_without_callbacks(const ui::BnwEngineUiEndTurnQueue& queue,
 const ui::BnwEngineUiEndTurnQueue::Event& event,const ui::BnwEngineUiEndTurnQueue::Completion& completion,std::uint64_t serial)const noexcept{
 const auto p=impl_;p->pure_thread();if(!p->delivery_attempted||p->delivered||p->queue!=&queue||p->event!=&event||
  completion.event_!=&event||completion.occurrence_!=p->occurrence||event.player_dirty_issuer.get()!=this||serial<=p->serial)std::terminate();
 p->delivery_serial=serial;p->delivered=true; // Actual protected Completion, before any queue POST callbacks/pop.
}
bool BnwPlayerTurnActiveDirtyIssuer::queued_without_callbacks()const noexcept{impl_->pure_thread();return impl_->queued;}
bool BnwPlayerTurnActiveDirtyIssuer::publisher_returned_without_callbacks()const noexcept{impl_->pure_thread();return impl_->publisher_returned;}
bool BnwPlayerTurnActiveDirtyIssuer::delivery_attempted_without_callbacks()const noexcept{impl_->pure_thread();return impl_->delivery_attempted;}
bool BnwPlayerTurnActiveDirtyIssuer::delivered_without_callbacks()const noexcept{impl_->pure_thread();return impl_->delivered;}
}
