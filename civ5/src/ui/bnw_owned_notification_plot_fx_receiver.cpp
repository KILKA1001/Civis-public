#include "civ5/ui/bnw_owned_notification_plot_fx_receiver.hpp"
#include "civ5/ui/bnw_notification_graphics_publication.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/runtime/bnw_live_game_context.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <vector>
namespace civ5::ui {
struct BnwOwnedNotificationPlotFxReceiver::State final {
 enum class Phase {acquired,queried,released,appended};
 struct History {BnwEngineUiNotificationEffect effect;Phase phase;std::int32_t team;std::uint32_t sequence;};
 struct Record {std::array<std::uint8_t,0x60> bytes;};
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 bool busy=false;
 const std::weak_ptr<const BnwClientSceneFrame> scene;
 std::weak_ptr<BnwOwnedNotificationPlotFxReceiver> canonical;
 const terrain::IntMatrix4 matrix;
 const std::size_t maximum;
 std::vector<History> history;
 std::vector<Record> records;
 std::mutex record_lock;
 std::uint32_t sequence=0;
 State(std::weak_ptr<const BnwClientSceneFrame> s,terrain::IntMatrix4 m,std::size_t n)
  :scene(std::move(s)),matrix(std::move(m)),maximum(n) {
  if(!n||n>history.max_size()||n>records.max_size())throw std::invalid_argument("Notification FX OWN capacity invalid");
  history.reserve(n);records.reserve(n);
 }
};
namespace {
void word(std::array<std::uint8_t,0x60>& b,std::size_t offset,std::uint32_t v) {
 for(std::size_t i=0;i<4;++i)b[offset+i]=static_cast<std::uint8_t>(v>>(i*8));
}
std::array<std::uint8_t,0x60> command(const terrain::IntMatrix4& m,
 std::int32_t x,std::int32_t y,std::uint32_t plot,std::uint32_t sequence) {
 const auto r=static_cast<std::uint32_t>(y);
 const auto q=static_cast<std::uint32_t>(x)-static_cast<std::uint32_t>(y>>1);
 const auto s=0u-q-r;
 const auto lane=[&](std::size_t i){return static_cast<std::uint32_t>(m[12+i])+
  q*static_cast<std::uint32_t>(m[i])+s*static_cast<std::uint32_t>(m[8+i])+r*static_cast<std::uint32_t>(m[4+i]);};
 const auto a=lane(0),b=lane(1);
 // OWN zero initialization of unused tail, not original-byte padding parity.
 std::array<std::uint8_t,0x60> bytes{};
 word(bytes,0,0x60);word(bytes,4,0x8f);
 const std::array<std::uint32_t,13> payload{UINT32_MAX,plot,UINT32_MAX,0,UINT32_MAX,0,UINT32_MAX,0,UINT32_MAX,0,0,0,0};
 for(std::size_t i=0;i<payload.size();++i)word(bytes,0x10+i*4,payload[i]);
 word(bytes,0x44,sequence);
 word(bytes,0x48,UINT32_MAX); // Actual selected SDK GameplayDoFX default iFXType=-1.
 word(bytes,0x4c,(a&0xffffu)|((b&0xffffu)<<16));
 word(bytes,0x50,a); // Exact fourth copied DWORD; no invented field semantics.
 return bytes;
}
}
BnwOwnedNotificationPlotFxReceiver::BnwOwnedNotificationPlotFxReceiver(
 std::weak_ptr<const BnwClientSceneFrame> scene,terrain::IntMatrix4 matrix,std::size_t maximum)
 :state_(std::make_shared<State>(std::move(scene),std::move(matrix),maximum)){}
BnwOwnedNotificationPlotFxReceiver::~BnwOwnedNotificationPlotFxReceiver(){state_->alive.store(false);}
void BnwOwnedNotificationPlotFxReceiver::fail(const std::shared_ptr<State>& s)noexcept{if(s)s->failed.store(true);}
void BnwOwnedNotificationPlotFxReceiver::pure(const std::shared_ptr<State>& s){
 if(!s||s->creator!=std::this_thread::get_id()||!s->alive.load()||s->failed.load()||s->scene.expired()||s->canonical.expired())
  throw std::runtime_error("Notification FX receiver retired, failed or foreign");
}
std::shared_ptr<BnwOwnedNotificationPlotFxReceiver> BnwOwnedNotificationPlotFxReceiver::create_for_actual_scene(
 const SceneFactoryKey& key,const terrain::WorldLayout& actual,std::size_t maximum){
 if(key.creator_!=std::this_thread::get_id())throw std::runtime_error("Notification FX factory foreign creator");
 if(key.scene_.expired())throw std::runtime_error("Notification FX factory Scene retired");
 auto result=std::shared_ptr<BnwOwnedNotificationPlotFxReceiver>(new BnwOwnedNotificationPlotFxReceiver(key.scene_,actual.source_grid.second_matrix,maximum));
 result->state_->canonical=result;pure(result->state_);return result;
}
void BnwOwnedNotificationPlotFxReceiver::dispatch_current_owned(
 const BnwNotificationPublicationLender& loan,const terrain::WorldLayout& actual)const {
 if(creator_!=std::this_thread::get_id()){state_->failed.store(true);throw std::runtime_error("Notification FX receiver foreign creator");}
 const auto saved=state_;pure(saved);
 if(loan.creator_!=std::this_thread::get_id()){fail(saved);throw std::runtime_error("Notification FX lender foreign creator");}
 const auto source=loan.storage_; // No raw loan/this reuse after callbackful guards.
 if(saved->busy){fail(saved);BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(source);throw std::runtime_error("Notification FX receiver reentered");}
 saved->busy=true;
 struct Busy {State& s;~Busy(){s.busy=false;}} busy{*saved};
 try {
  {const auto canonical=saved->canonical.lock();if(!canonical||canonical.get()!=this)throw std::runtime_error("Notification FX receiver substituted");}
  pure(saved);
  BnwNotificationPublicationLender::full(source);pure(saved);
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(source,saved->scene);
  const auto effect=BnwNotificationPublicationLender::pending_effect_from_storage(source);pure(saved);
  if(effect.kind!=BnwEngineUiNotificationEffectKind::gameplay_fx_null||!effect.generation||!effect.occurrence)
   throw std::invalid_argument("Notification FX actual pending effect differs");
  if(saved->history.size()>=saved->maximum||(!saved->history.empty()&&effect.occurrence<=saved->history.back().effect.occurrence))
   throw std::runtime_error("Notification FX source repeated or OWN history full");
  if(actual.source_grid.second_matrix!=saved->matrix)throw std::runtime_error("Notification FX actual runtime matrix changed");
  State::History prepared{effect,State::Phase::acquired,-1,0};pure(saved);
  std::unique_ptr<runtime::BnwLiveGameContext::GameReference> game;
  {const auto scene=saved->scene.lock();if(!scene||!scene->selected_host_)throw std::runtime_error("Notification FX canonical Scene Host missing");
   game=runtime::BnwLiveGameContext::retain_for_actual_host(scene->selected_host_)->get_game();}
  static_assert(std::is_nothrow_move_constructible_v<State::History>);
  saved->history.push_back(std::move(prepared)); // Reserved no-throw acquire history before guards.
  pure(saved);BnwNotificationPublicationLender::pure(source);
  auto& history=saved->history.back();
  const auto team=game->get_active_team();
  history.team=team;history.phase=State::Phase::queried;
  pure(saved);BnwNotificationPublicationLender::pure(source);
  std::optional<State::Record> record;
  {const auto scene=saved->scene.lock();if(!scene||!scene->scene_state_)throw std::runtime_error("Notification FX actual current world missing");
   const auto& world=scene->scene_state_->world;const auto& h=world.map_header;
   const runtime::MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,h.layout};
   const auto plot=geometry.plot(effect.x,effect.y); // Sentinel -(INT_MAX); wrap is genuine even for -1.
   if(plot){
    if(!world.map_projection||world.map_projection->grid.width!=h.grid.width||world.map_projection->grid.height!=h.grid.height)
     throw std::runtime_error("Notification FX SAME map projection missing");
    const auto& cell=world.map_projection->cells.at(static_cast<std::size_t>(*plot));
    if(!cell.revealed_teams)throw std::runtime_error("Notification FX actual IsRevealed data missing");
    if(team<0)throw std::runtime_error("Notification FX nonNULL IsRevealed invalid team");
    if(cell.revealed_teams->contains(team)){
     if(*plot>INT32_MAX||saved->sequence==UINT32_MAX||saved->records.size()>=saved->maximum)
      throw std::runtime_error("Notification FX OWN queue or native index exhausted");
     const auto xy=h.grid.coordinates(static_cast<std::uint64_t>(*plot));
     if(xy[0]>INT32_MAX||xy[1]>INT32_MAX)throw std::runtime_error("Notification FX GetPosition outside native width");
     record=State::Record{command(saved->matrix,static_cast<std::int32_t>(xy[0]),static_cast<std::int32_t>(xy[1]),static_cast<std::uint32_t>(*plot),saved->sequence+1)};
    }
   }
  }
  pure(saved);BnwNotificationPublicationLender::pure(source);
  if(record){
   const std::lock_guard<std::mutex> lock(saved->record_lock);
   saved->records.push_back(std::move(*record));++saved->sequence;
   history.sequence=saved->sequence;history.phase=State::Phase::appended; // Physical prefix recorded before providers.
  }
  pure(saved);BnwNotificationPublicationLender::pure(source);
  game->destroy();history.phase=State::Phase::released;pure(saved);
  {const auto scene=saved->scene.lock();if(!scene||!scene->selected_host_)throw std::runtime_error("Notification FX Host retired after Destroy");
   game->require_destroyed_for(*scene->selected_host_);}
  game.reset(); // Release actual Game Host/context pins before callbackful source POST.
  pure(saved);BnwNotificationPublicationLender::full(source);pure(saved);
 }catch(...){fail(saved);BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(source);throw;}
}
}
