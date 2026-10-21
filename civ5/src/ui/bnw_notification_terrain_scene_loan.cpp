#include "civ5/ui/bnw_notification_terrain_scene_loan.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/ui/minimap_world_bridge.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/terrain/terrain_world.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>
namespace civ5::ui {
struct BnwNotificationTerrainSceneLoan::State final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 bool checking=false,invoking=false;
 const std::shared_ptr<BnwWorldViewRendererRunLoan::State> run;
 const UiScreenGlRenderer* const renderer;
 const std::weak_ptr<BnwWorldViewRendererRunLoan::RendererLifetime> renderer_life;
 const void* const renderer_scope;
 const std::weak_ptr<const terrain::TerrainWorld> terrain;
 const std::weak_ptr<const BnwClientSceneFrame> scene;
 const std::shared_ptr<const BnwClientFrame> frame;
 const std::weak_ptr<BnwNotificationPublicationLender::Storage> lender;
 const BnwEngineUiNotificationEffect effect;
 State(const BnwWorldViewRendererProjectionLoan& p,
     const std::shared_ptr<BnwNotificationPublicationLender::Storage>& actual,
     BnwEngineUiNotificationEffect e)
  :run(p.run_),renderer(p.renderer_),renderer_life(p.lifetime_),
   renderer_scope(p.lifetime_->active_scope),terrain(p.run_->terrain_owner),
   scene(p.scene_),frame(p.frame_),lender(actual),effect(std::move(e)){}
};
void BnwNotificationTerrainSceneLoan::fail(const std::shared_ptr<State>& s)noexcept{
 s->failed.store(true);
 if(const auto actual=s->lender.lock())
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(actual);
}
void BnwNotificationTerrainSceneLoan::pure(const std::shared_ptr<State>& s){
 // Saved independent life FIRST, before mutable/raw producer/scope reads.
 if(!s||s->creator!=std::this_thread::get_id()||!s->alive.load()||s->failed.load()){
  if(s)fail(s);
  throw std::runtime_error("Notification terrain Scene loan retired, failed or foreign");
 }
 try{
  const auto actual=s->lender.lock();
  if(!actual)throw std::runtime_error("Notification terrain publication scope retired");
  BnwNotificationPublicationLender::pure(actual);
  s->run->require_current(s->renderer);
  const auto life=s->renderer_life.lock();
  if(!life||life->creator!=s->creator||!life->alive||life->failed||
     !life->active||life->renderer!=s->renderer||!s->renderer_scope||
     life->active_scope!=s->renderer_scope)
   throw std::runtime_error("Notification terrain actual renderer scope changed");
  if(!s->frame||s->frame->stamp.identity.map_guid!=s->run->map_guid||
     s->frame->world.map_header.guid!=s->run->map_guid||
     s->frame->world.map_header.grid.width!=s->run->map_width||
     s->frame->world.map_header.grid.height!=s->run->map_height)
   throw std::runtime_error("Notification terrain current frame map differs");
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(actual,s->scene);
  if(s->effect.kind!=BnwEngineUiNotificationEffectKind::gameplay_fx_null&&
     s->effect.kind!=BnwEngineUiNotificationEffectKind::gameplay_minimap_notification)
   throw std::runtime_error("Notification terrain effect is outside physical FX/minimap scope");
  if(BnwNotificationPublicationLender::pending_effect_from_storage(actual)!=s->effect)
   throw std::runtime_error("Notification terrain actual pending effect changed");
  {
   // All borrowed Terrain/Scene identity pins end before any callbackful POST.
   const auto terrain=s->terrain.lock();
   const auto scene=s->scene.lock();
   const auto bound_scene=s->run->scene_owner.lock();
   if(!terrain||s->terrain.use_count()<=1||!scene||!bound_scene||
      scene.get()!=bound_scene.get()||scene.owner_before(bound_scene)||bound_scene.owner_before(scene)||
      scene->frame().get()!=s->frame.get()||scene->frame().owner_before(s->frame)||
      s->frame.owner_before(scene->frame())||
      terrain->source().layout.source_grid.width!=static_cast<std::int64_t>(s->run->map_width)||
      terrain->source().layout.source_grid.height!=static_cast<std::int64_t>(s->run->map_height)||
      s->frame->world.map_header.wrap_x!=(terrain->source().map_mode==1)||
      s->frame->world.map_header.wrap_y)
    throw std::runtime_error("Notification terrain actual run source origin differs");
  }
  // No internally earned Terrain pin can hide outside terrain retirement.
  if(s->terrain.expired()||s->scene.expired())
   throw std::runtime_error("Notification terrain outside source owner retired");
  BnwNotificationPublicationLender::pure(actual);
  s->run->require_current(s->renderer);
 }catch(...){fail(s);throw;}
}
void BnwNotificationTerrainSceneLoan::full(const std::shared_ptr<State>& s){
 pure(s);
 if(s->checking){fail(s);throw std::runtime_error("Notification terrain current checker reentered");}
 s->checking=true;struct End{State& s;~End(){s.checking=false;}} end{*s};
 try{
  const auto actual=s->lender.lock();
  if(!actual)throw std::runtime_error("Notification terrain actual lender retired");
  BnwNotificationPublicationLender::full(actual);
  pure(s); // FIRST after every callbackful supplier.
 }catch(...){fail(s);throw;}
}
std::shared_ptr<BnwNotificationTerrainSceneLoan::State>
BnwNotificationTerrainSceneLoan::bind(const BnwWorldViewRendererProjectionLoan& projection,
 const BnwNotificationPublicationLender& lender){
 // Root renderer must obtain projection from its SAME actual active scope.
 lender.require_origin_without_callbacks();
 projection.require_current_without_callbacks();
 const auto actual=lender.storage_;
 BnwNotificationPublicationLender::pure(actual);
 auto effect=BnwNotificationPublicationLender::pending_effect_from_storage(actual);
 BnwNotificationPublicationLender::pure(actual);
 auto result=std::make_shared<State>(projection,actual,std::move(effect));
 try{pure(result);return result;}catch(...){result->alive.store(false);throw;}
}
BnwNotificationTerrainSceneLoan::BnwNotificationTerrainSceneLoan(
 const BnwWorldViewRendererProjectionLoan& p,const BnwNotificationPublicationLender& lender)
 :state_(bind(p,lender)){
 const auto saved=state_;
 try{full(saved);}catch(...){saved->alive.store(false);throw;}
}
BnwNotificationTerrainSceneLoan::~BnwNotificationTerrainSceneLoan(){state_->alive.store(false);}
std::shared_ptr<BnwNotificationTerrainSceneLoan::State>
BnwNotificationTerrainSceneLoan::saved_state_without_callbacks()const{
 if(creator_!=std::this_thread::get_id())
  throw std::runtime_error("Notification terrain loan foreign creator");
 const auto saved=state_;pure(saved);return saved;
}
void BnwNotificationTerrainSceneLoan::require_current()const{
 const auto saved=saved_state_without_callbacks();full(saved);
}
BnwNotificationTerrainSceneLoan::Geometry
BnwNotificationTerrainSceneLoan::read_geometry_without_callbacks()const{
 const auto saved=saved_state_without_callbacks();
 try{
  std::optional<Geometry> result;
  {
   const auto actual=saved->terrain.lock();
   if(!actual||saved->terrain.use_count()<=1)
    throw std::runtime_error("Notification terrain geometry outside owner retired");
   // Existing genuine bridge validates aligned dimensions; finite scalars
   // are checked explicitly, matching the existing queue producer contract.
   const auto transform=minimap_world_transform(actual->source().layout,actual->owner().parameters);
   if(!std::all_of(transform.cube_to_world.begin(),transform.cube_to_world.end(),
       [](float x){return std::isfinite(x);})||!std::isfinite(transform.scale_x)||
      !std::isfinite(transform.scale_y)||transform.scale_x==0||transform.scale_y==0||
      !std::isfinite(transform.offset_x)||!std::isfinite(transform.offset_y))
    throw std::runtime_error("Notification actual terrain height transform invalid");
   result.emplace(Geometry{actual->source().layout,actual->owner().parameters,
      actual->source().map_mode==1});
  }
  pure(saved);return *result;
 }catch(...){fail(saved);throw;}
}
std::array<std::int32_t,3>
BnwNotificationTerrainSceneLoan::transformed_effect_coordinates_without_callbacks()const{
 const auto saved=saved_state_without_callbacks();
 try{
  std::array<std::int32_t,3> result;
  {
   const auto actual=saved->terrain.lock();
   if(!actual||saved->terrain.use_count()<=1)
    throw std::runtime_error("Notification integer transform outside terrain owner retired");
   const auto& m=actual->source().layout.source_grid.second_matrix;
   const auto q=static_cast<std::uint32_t>(saved->effect.x)-
      static_cast<std::uint32_t>(static_cast<std::int64_t>(saved->effect.y)>>1);
   const auto r=static_cast<std::uint32_t>(saved->effect.y);
   const auto t=0U-q-r;
   for(std::size_t i=0;i<result.size();++i){
    const auto value=static_cast<std::uint32_t>(m[12+i])+
       q*static_cast<std::uint32_t>(m[i])+
       t*static_cast<std::uint32_t>(m[8+i])+
       r*static_cast<std::uint32_t>(m[4+i]);
    result[i]=std::bit_cast<std::int32_t>(value);
   }
  }
  pure(saved);return result;
 }catch(...){fail(saved);throw;}
}
BnwNotificationTerrainSceneLoan::PlotRead
BnwNotificationTerrainSceneLoan::read_effect_plot_without_callbacks()const{
 const auto saved=saved_state_without_callbacks();
 try{
  const auto& f=*saved->frame;
  if(f.world.map_header.layout!=saves::GameStateLayout::expansion2_version1||
     !f.world.map_projection||f.stamp.identity.team<0||f.stamp.identity.team>=64)
   throw std::runtime_error("Notification actual BNW current map/team unavailable");
  const auto& map=*f.world.map_projection;
  if(map.grid.width!=f.world.map_header.grid.width||map.grid.height!=f.world.map_header.grid.height||
     map.wrap_x!=f.world.map_header.wrap_x||
     map.wrap_y!=f.world.map_header.wrap_y||map.cells.size()!=map.grid.size())
   throw std::runtime_error("Notification actual current map projection differs");
  runtime::MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,f.world.map_header.layout};
  const auto index=geometry.plot(saved->effect.x,saved->effect.y);
  PlotRead result{index,std::nullopt,f.stamp.identity.team};
  if(index){
   const auto& cell=map.cells.at(static_cast<std::size_t>(*index));
   if(cell.index!=static_cast<std::uint64_t>(*index)||!cell.revealed_teams||
      std::any_of(cell.revealed_teams->begin(),cell.revealed_teams->end(),
       [](std::int64_t team){return team<0||team>=64;}))
    throw std::runtime_error("Notification actual plot revealed-team source unavailable");
   result.revealed=cell.revealed_teams->contains(f.stamp.identity.team);
  }
  // NULL plot is real lookup result, not a minimap early-skip/return receipt.
  // published_team is NOT a GameContext GetActiveTeam execution receipt.
  pure(saved);return result;
 }catch(...){fail(saved);throw;}
}
void BnwNotificationTerrainSceneLoan::invoke_owned(
 const std::function<void(BnwNotificationTerrainSceneLoan&)>& target){
 const auto saved=saved_state_without_callbacks();
 if(saved->invoking){fail(saved);throw std::runtime_error("Notification terrain target reentered");}
 saved->invoking=true;struct End{State& s;~End(){s.invoking=false;}} end{*saved};
 std::unique_ptr<std::function<void(BnwNotificationTerrainSceneLoan&)>> owned;
 try{
  full(saved);pure(saved);
  owned=std::make_unique<std::function<void(BnwNotificationTerrainSceneLoan&)>>(target);
  pure(saved);full(saved);pure(saved);
  if(!*owned)throw std::invalid_argument("Notification terrain target absent");
  (*owned)(*this);pure(saved);full(saved);pure(saved);
  owned.reset();pure(saved);full(saved);pure(saved);
 }catch(...){fail(saved);owned.reset();throw;}
}
}
