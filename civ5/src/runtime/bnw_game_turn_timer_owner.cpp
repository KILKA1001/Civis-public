#include "civ5/runtime/bnw_game_turn_timer_owner.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/live_session.hpp"
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <optional>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace civ5::runtime {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::int64_t counter(){LARGE_INTEGER value{};need(QueryPerformanceCounter(&value)!=0,"BNW timer QPC unavailable");return value.QuadPart;}
std::int64_t frequency(){LARGE_INTEGER value{};need(QueryPerformanceFrequency(&value)!=0&&value.QuadPart>0,"BNW timer QPC frequency unavailable");return value.QuadPart;}
std::int64_t integer(const saves::PregameRecord& record,const char* name){
    const auto at=record.integers.find(name);need(at!=record.integers.end(),"BNW timer pregame identity absent");return at->second;
}
float elapsed(std::int64_t start,double reciprocal){
    const auto now=counter();
    // Reject unrepresentable signed subtraction; never clamp or invent a wrap.
    need(!((start<0&&now>std::numeric_limits<std::int64_t>::max()+start)||
           (start>0&&now<std::numeric_limits<std::int64_t>::min()+start)),"BNW timer QPC subtraction overflow");
    const float value=static_cast<float>(static_cast<double>(now-start)*reciprocal);
    need(std::isfinite(value),"BNW timer elapsed exceeds float range");return value;
}
void bnw_prefix(const saves::SavedSessionSnapshot& world){
    const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&world.game_state.prefix);
    need(prefix&&prefix->version==1&&world.pregame&&world.pregame->slots_version==3&&world.pregame->archive_version==6,
         "BNW timer unsupported Game/pregame wire version");
}
}
struct BnwGameTurnTimerUse::Data {
    std::weak_ptr<const void> owner;
    std::uint64_t operation=0;
    BnwGameTurnTimerReset reset=BnwGameTurnTimerReset::construction;
};
struct BnwGameTurnTimerOwner::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    mutable std::atomic<bool> poisoned{false};std::atomic<bool> retired{false};
    std::shared_ptr<const LiveSession> session;
    std::shared_ptr<LiveSession::RetainedFacadeLifetime> lifetime;
    std::shared_ptr<const LiveSessionState> state;
    std::uint64_t generation=0,next_operation=1;
    std::int64_t turn=0,hz=0,current_start=0,game_start=0;
    double reciprocal=0;
    float pause_delta=0,current_stopped=0,game_stopped=0;
    std::array<std::uint8_t,16> map_guid{},gamecore_guid{},engine_guid{};
    const saves::SavedSession* source=nullptr;
    const std::uint8_t* payload=nullptr;
    std::size_t payload_size=0;
    bool begin_attempted=false;
    // Actual standalone counterpart of the separate CvGame m_endTurnTimer;
    // its marks and semaphore are unsaved OS/runtime state, not map caches.
    std::int64_t end_start=0;std::int32_t end_semaphore=0;
    float end_stopped=0;bool end_running=false;
    std::uint64_t end_reset_occurrence=0;
    std::optional<std::int64_t> end_reset_attempted_turn;
    struct EndTurnTimerReturn {
     const BnwRetainedTurnCursor* cursor=nullptr;
     std::int64_t turn=0,start=0;std::uint64_t occurrence=0;
    };
    std::optional<EndTurnTimerReturn> end_return;
    std::shared_ptr<const BnwGameTurnTimerUse::Data> use;
    void check() const {
        if(std::this_thread::get_id()!=thread){poisoned.store(true);throw std::runtime_error("BNW timer foreign thread");}
        need(!retired.load()&&!poisoned.load(),"BNW timer retired/poisoned");
        try {
        need(bool(lifetime),"BNW timer unissued facade lifetime");lifetime->require_current();
        need(session&&session->retained_facade_lifetime_==lifetime&&lifetime->facade==session.get(),
             "BNW timer genuine facade lifetime differs");
        const auto current=session->read_state_lease();
        need(current&&current.get()==state.get()&&session->state_generation()==generation,"BNW timer session allocation/generation changed");
        // Check the state lease first: a moved-from facade has no source.
        const auto& retained=session->source();
        need(&retained==source&&retained.source.bytes.data()==payload&&retained.source.bytes.size()==payload_size,
             "BNW timer retained source allocation changed");
        bnw_prefix(state->world);
        need(state->setup.profile==content::TechnologyProfile::bnw&&state->setup.game_turn==turn&&state->setup.game_type==0&&
             state->world.game_state.layout==saves::GameStateLayout::expansion2_version1&&state->world.pregame&&
             state->world.pregame->layout==saves::GameStateLayout::expansion2_version1&&
             state->world.map_header.guid==map_guid,"BNW timer live game identity changed");
        const auto& p=*state->world.pregame;
        need(p.gamecore_guid==gamecore_guid&&p.engine_guid==engine_guid&&
             integer(p.archive,"game_turn")==turn&&integer(p.archive,"game_type")==0,"BNW timer pregame identity changed");
        }catch(...){poisoned.store(true);throw;}
    }
    void start_current(){current_start=counter();current_stopped=0.0f;}
    void start_game(){game_start=counter();game_stopped=0.0f;}
};
std::shared_ptr<BnwGameTurnTimerOwner> BnwGameTurnTimerOwner::create(std::shared_ptr<const LiveSession> session){
    need(static_cast<bool>(session),"BNW timer session absent");
    const auto lifetime=session->retain_bnw_facade_lifetime();
    need(session->plot_yield_refresh_thread_==std::this_thread::get_id(),
         "BNW timer foreign session constructor thread");
    const auto state=session->read_state_lease();need(state&&state->world.pregame,"BNW timer live pregame absent");
    need(session->source().snapshot.pregame&& !session->source().source.bytes.empty(),"BNW timer retained source absent");
    const auto& source=session->source().snapshot;
    bnw_prefix(source);
    need(source.game_state.layout==saves::GameStateLayout::expansion2_version1&&
         source.pregame->layout==saves::GameStateLayout::expansion2_version1&&
         source.map_header.guid==state->world.map_header.guid&&
         source.pregame->gamecore_guid==state->world.pregame->gamecore_guid&&
         source.pregame->engine_guid==state->world.pregame->engine_guid,"BNW timer foreign source game identity");
    auto i=std::make_shared<Impl>();i->session=std::move(session);i->lifetime=lifetime;i->state=state;i->generation=i->session->state_generation();
    i->turn=state->setup.game_turn;i->map_guid=state->world.map_header.guid;
    i->gamecore_guid=state->world.pregame->gamecore_guid;i->engine_guid=state->world.pregame->engine_guid;
    i->source=&i->session->source();i->payload=i->source->source.bytes.data();
    i->payload_size=i->source->source.bytes.size();i->check();
    i->hz=frequency();i->reciprocal=1.0/static_cast<double>(i->hz);
    auto use=std::make_shared<BnwGameTurnTimerUse::Data>();use->owner=i;
    // Sticky unsaved history BEFORE the first FTimer Start, including the
    // direct owner route that does not construct a retained turn cursor.
    i->session->mark_bnw_retained_timer_history();
    // FTimer constructor calls Start. No external consumer exists until publish.
    i->start_current();i->start_game();i->use=std::move(use);
    // A concrete new standalone timer owner executes the constructor reset
    // lifecycle: real Start/Stop like CvGame.cpp138-139. No imported QPC flag.
    i->end_start=counter();i->end_stopped=0.0f;i->end_running=true;
    i->end_stopped=elapsed(i->end_start,i->reciprocal);i->end_running=false;
    return std::shared_ptr<BnwGameTurnTimerOwner>(new BnwGameTurnTimerOwner(std::move(i)));
}
BnwGameTurnTimerOwner::~BnwGameTurnTimerOwner(){retire();}
void BnwGameTurnTimerOwner::retire() noexcept {impl_->retired.store(true);}
void BnwGameTurnTimerOwner::require_current() const {const auto i=impl_;i->check();}
BnwGameTurnTimerUse BnwGameTurnTimerOwner::current_use() const {const auto i=impl_;i->check();return BnwGameTurnTimerUse(i->use);}
BnwGameTurnTimerUse BnwGameTurnTimerOwner::reset_for_engine_begin(){return reset(BnwGameTurnTimerReset::engine_begin);}
BnwGameTurnTimerUse BnwGameTurnTimerOwner::reset_for_current_player(){return reset(BnwGameTurnTimerReset::current_player);}
BnwGameTurnTimerUse BnwGameTurnTimerOwner::reset(BnwGameTurnTimerReset kind){
    const auto i=impl_;i->check();
    need(kind!=BnwGameTurnTimerReset::engine_begin||!i->begin_attempted,"BNW timer engine begin already attempted");
    need(i->next_operation!=std::numeric_limits<std::uint64_t>::max(),"BNW timer operation exhausted");
    auto use=std::make_shared<BnwGameTurnTimerUse::Data>();use->owner=i;use->operation=i->next_operation;use->reset=kind;
    if(kind==BnwGameTurnTimerReset::engine_begin)i->begin_attempted=true;
    try {
        i->start_current();
        i->pause_delta=0.0f;
        if(kind==BnwGameTurnTimerReset::engine_begin)i->start_game();
        i->check();
    }catch(...){i->poisoned.store(true);throw;}
    ++i->next_operation;i->use=std::move(use);return BnwGameTurnTimerUse(i->use);
}
BnwGameTurnTimerSample BnwGameTurnTimerOwner::read(const BnwGameTurnTimerUse& use) const {
    const auto i=impl_;i->check();
    need(use.data_&&use.data_->owner.lock().get()==i.get()&&use.data_==i->use,"BNW timer source-use foreign/stale");
    BnwGameTurnTimerSample sample;
    try {
        sample.current_elapsed=elapsed(i->current_start,i->reciprocal);
        sample.game_turn_elapsed=elapsed(i->game_start,i->reciprocal);
        i->check();
    }catch(...){i->poisoned.store(true);throw;}
    sample.pause_delta=i->pause_delta;sample.current_stopped_elapsed=i->current_stopped;sample.game_turn_stopped_elapsed=i->game_stopped;
    sample.current_start=i->current_start;sample.game_turn_start=i->game_start;sample.frequency=i->hz;
    sample.operation=i->use->operation;sample.reset=i->use->reset;return sample;
}
} // namespace civ5::runtime

namespace civ5::runtime {
void BnwGameTurnTimerOwner::adopt_actual_game_turn_clock_return(const BnwRetainedTurnCursor& cursor){
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&!p->retired.load()&&!p->poisoned.load(),
  "BNW timer actual clock adoption foreign/retired");
 // This is an identity continuation, not FTimer::Start or a timer ACK. The
 // old cached turn cannot be checked through ordinary currentness after the
 // native clock has legitimately changed. Only its genuine retained cursor
 // may prove the exact stored old->new source return without callbacks.
 need(p->session&&p->state&&p->lifetime&&p->session->retained_facade_lifetime_==p->lifetime&&
  p->lifetime->facade==p->session.get()&&p->use&&p->use->owner.lock().get()==p.get(),
  "BNW timer actual allocation/lifetime/use lineage absent");
 cursor.require_actual_timer_clock_adoption(*this,*p->session,*p->state,p->generation,p->turn);
 p->turn=p->state->setup.game_turn;
}
}

namespace civ5::runtime {
BnwEndTurnTimerSample BnwGameTurnTimerOwner::read_end_turn_timer()const{
 const auto p=impl_;p->check();BnwEndTurnTimerSample result;
 result.start=p->end_start;result.frequency=p->hz;result.semaphore=p->end_semaphore;
 result.stopped_elapsed=p->end_stopped;result.running=p->end_running;result.reset_occurrence=p->end_reset_occurrence;
 try{result.elapsed=p->end_running?elapsed(p->end_start,p->reciprocal):p->end_stopped;p->check();}
 catch(...){p->poisoned.store(true);throw;}return result;
}
BnwEndTurnTimerSample BnwGameTurnTimerOwner::reset_actual_end_turn_timer(const BnwRetainedTurnCursor& cursor){
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"EndTurnTimer actual reset foreign creator");
 p->check();cursor.require_actual_end_turn_timer_reset_source(*this,*p->session,*p->state,p->generation,p->turn);
 need((!p->end_reset_attempted_turn||*p->end_reset_attempted_turn!=p->turn)&&p->end_reset_occurrence<UINT64_MAX,
  "EndTurnTimer actual reset repeated/occurrence exhausted");
 // Sticky reservation BEFORE the first sem store and real OS query. A QPC
 // failure preserves that attempted native prefix and poisons this owner.
 p->end_reset_attempted_turn=p->turn;
 try{
  // CvGame::endTurnTimerReset native order: semaphore=0, then FTimer::Start.
  // QueryPerformanceCounter is a real OS effect. It cannot acknowledge UI,
  // restart the other two timers, consume Lua, or finish the whole game turn.
  p->end_semaphore=0;
  const auto actual_start=counter();p->end_start=actual_start;p->end_stopped=0.0f;p->end_running=true;
  ++p->end_reset_occurrence;p->end_return=Impl::EndTurnTimerReturn{&cursor,p->turn,actual_start,p->end_reset_occurrence};
  BnwEndTurnTimerSample actual;actual.start=actual_start;actual.frequency=p->hz;actual.semaphore=0;
  actual.stopped_elapsed=0.0f;actual.elapsed=0.0f;actual.reset_occurrence=p->end_reset_occurrence;actual.running=true;
  require_actual_end_turn_timer_return(cursor,actual);return actual;
 }catch(...){p->poisoned.store(true);throw;}
}
void BnwGameTurnTimerOwner::require_actual_end_turn_timer_return(const BnwRetainedTurnCursor& cursor,const BnwEndTurnTimerSample& sample)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&!p->retired.load()&&!p->poisoned.load(),"EndTurnTimer actual return foreign/retired");
 p->check();cursor.require_actual_end_turn_timer_reset_source(*this,*p->session,*p->state,p->generation,p->turn);
 need(p->end_return&&p->end_reset_attempted_turn==std::optional{p->turn}&&p->end_return->cursor==&cursor&&p->end_return->turn==p->turn&&
  p->end_return->start==p->end_start&&p->end_return->occurrence==p->end_reset_occurrence&&p->end_reset_occurrence!=0&&p->end_semaphore==0&&p->end_running&&p->end_stopped==0.0f&&
  sample.start==p->end_start&&sample.frequency==p->hz&&sample.semaphore==0&&sample.running&&sample.stopped_elapsed==0.0f&&sample.elapsed==0.0f&&sample.reset_occurrence==p->end_reset_occurrence,
  "EndTurnTimer concrete semaphore/QPC Start return differs");
}
}
