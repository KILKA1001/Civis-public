#include "civ5/runtime/bnw_local_client_scalar_owner.hpp"
#include "civ5/runtime/bnw_plot_reveal_read.hpp"
#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_displayed_plot_yield.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include <climits>
#include <atomic>
#include <thread>
#include <string_view>
namespace civ5::runtime {
namespace {
void need(bool ok,const char* why){if(!ok)throw BnwLuaCalculatedPlotYieldUnavailable(why);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwLuaCalculatedPlotYieldUnavailable&){throw;}
 catch(const std::exception& e){throw BnwLuaCalculatedPlotYieldUnavailable(e.what());}
}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Lua calculated yield native index unavailable");return static_cast<std::int32_t>(n);}
}
struct BnwLuaCalculatedPlotYieldOwner::VmLease {
 const std::thread::id thread=std::this_thread::get_id();
 const void* actual_state;
 std::atomic<bool> retired{false};
 explicit VmLease(const void* state):actual_state(state){}
};
struct BnwLuaCalculatedPlotYieldOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const BnwNormalPlotYieldRules> rules;
 std::shared_ptr<const LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 SessionCheckpointIdentity identity;
 std::uint64_t generation=0;
 std::shared_ptr<BnwPlotYieldRefreshIssuer> read_control;
 // Only SourceOwner can bind its actual ctor lease + weak stock epoch.
 // It throws on retirement/reentry; no public caller currentness predicate.
 std::function<void()> stock_epoch;
 std::weak_ptr<const VmLease> vm;
 bool vm_bound=false;
 std::weak_ptr<const BnwLocalClientScalarOwner> client;bool client_bound=false;
};
BnwLuaCalculatedPlotYieldOwner::BnwLuaCalculatedPlotYieldOwner(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwLuaCalculatedPlotYieldOwner::~BnwLuaCalculatedPlotYieldOwner(){if(impl_&&impl_->read_control)impl_->read_control->retire();}
void BnwLuaCalculatedPlotYieldOwner::require_same_world(const std::shared_ptr<const LiveSessionState>& actual)const{
 require_current();need(actual&&impl_->state==actual,"Lua calculated map/Game differs from actual retained GameCore world");require_current();
}
void BnwLuaCalculatedPlotYieldOwner::bind_initial(){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&p.session&&p.rules,"Lua calculated yield real owning session/rules absent or foreign");
 p.rules->require_current(); // BEFORE actual Game/source/world.
 need(p.session->bnw_game_debug_owner_current(),"Lua calculated yield actual Game import owner unavailable");
 p.identity=p.session->debug_->identity;p.rules->require_current(p.identity.rules_sha256);
 p.state=p.session->state_;p.source=p.session->source_;p.generation=p.session->state_generation_;
 (void)p.session->bnw_game_debug_mode();
 p.read_control=std::shared_ptr<BnwPlotYieldRefreshIssuer>(new BnwPlotYieldRefreshIssuer(p.session->state_,p.generation));
 p.read_control->bind_game_debug(p.session->debug_,p.source);
 p.rules->require_current(p.identity.rules_sha256);
 (void)p.read_control->require_game_debug(*p.state,*p.source,p.identity);
}
std::shared_ptr<BnwLuaCalculatedPlotYieldOwner::VmLease>
BnwLuaCalculatedPlotYieldOwner::make_vm_lease(const void* actual_state){
 need(actual_state!=nullptr,"Lua calculated yield VM allocation absent");
 return std::make_shared<VmLease>(actual_state);
}
void BnwLuaCalculatedPlotYieldOwner::retire_vm_lease(const std::shared_ptr<VmLease>& vm)noexcept{
 if(vm)vm->retired.store(true);
}
void BnwLuaCalculatedPlotYieldOwner::bind_vm_lease(const std::shared_ptr<VmLease>& vm)const{
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&vm&&vm->thread==p.thread&&!vm->retired.load()&&!p.vm_bound,
  "Lua calculated yield must bind once to SAME actual VM incarnation");
 p.vm=vm;p.vm_bound=true;
}
void BnwLuaCalculatedPlotYieldOwner::bind_stock_epoch(std::function<void()> guard){
 need(impl_->thread==std::this_thread::get_id()&&!impl_->stock_epoch&&static_cast<bool>(guard),"Lua calculated yield stock epoch can bind only once");
 impl_->stock_epoch=std::move(guard); // No invocation while stock bootstrap is not running yet.
}
void BnwLuaCalculatedPlotYieldOwner::require_current()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Lua calculated yield foreign constructor thread");
 const auto vm=p.vm.lock();need(vm&&vm->actual_state&&!vm->retired.load()&&vm->thread==p.thread,
  "Lua calculated yield actual VM incarnation retired/unbound");
 if(p.stock_epoch)p.stock_epoch(); // Actual VM guard BEFORE Game/world/rules borrow.
 need(p.session&&p.rules&&p.read_control,"Lua calculated yield real owner absent");
 p.rules->require_current(p.identity.rules_sha256);
 need(p.session->bnw_game_debug_owner_current(),"Lua calculated yield actual Game owner retired");
 need(p.session->state_==p.state&&p.session->source_==p.source&&p.session->state_generation_==p.generation,
  "Lua calculated yield source/state allocation or generation retired");
 need(p.session->debug_->identity.payload_sha256==p.identity.payload_sha256&&p.session->debug_->identity.rules_sha256==p.identity.rules_sha256,
  "Lua calculated yield actual source identity retired");
 (void)p.read_control->require_game_debug(*p.state,*p.source,p.identity);
}
std::int32_t BnwLuaCalculatedPlotYieldOwner::value(std::int64_t plot,std::int64_t yield,bool display)const{return typed([&]{
 require_current();const auto& p=*impl_;const auto index=native(plot),y=native(yield);
 need(index>=0&&y>=0&&y<6,"Lua calculated yield native plot/YieldInfo domain unavailable");
 std::int32_t result=0;
 if(display){const auto query=inspect_bnw_displayed_plot_yield(p.session,p.rules,index,y);result=query.value();}
 else result=p.rules->calculate_current_normal(*p.read_control,*p.state,*p.source,p.identity,index,y);
 require_current();return result;
});}
std::int32_t BnwLuaCalculatedPlotYieldOwner::active_player()const{return typed([&]{
 require_current();const auto result=detail::bnw_game_active_player_read(*impl_->state);
 require_current();return result;
});}
std::int32_t BnwLuaCalculatedPlotYieldOwner::active_team()const{return typed([&]{
 require_current();const auto result=detail::bnw_game_active_team_read(*impl_->state);
 require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::player_is_turn_active(std::int64_t player)const{return typed([&]{
 require_current();const auto& p=*impl_;const auto id=native(player);const auto& state=*p.state;
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(id>=0&&id<64&&state.setup.profile==content::TechnologyProfile::bnw&&state.world.game_state.layout==bnw&&
  state.world.all_players&&state.world.all_players->layout==bnw&&state.world.all_players->context.player_count==64&&
  state.world.all_players->players.size()==64,"Lua Player turn read actual BNW native64 roster absent");
 const auto slot=static_cast<std::size_t>(id);const auto& raw=state.world.all_players->players[slot];
 need(raw.index==slot&&raw.saved_player_id==id&&raw.core.layout==bnw,"Lua Player turn read actual player identity differs");
 const saves::PlayerCoreField* identity=nullptr;const saves::PlayerCoreField* turn=nullptr;
 for(const auto& field:raw.core.fields)if(field.name){
  if(std::string_view(field.name)=="m_eID"){need(!identity,"Lua Player turn duplicate native ID");identity=&field;}
  if(std::string_view(field.name)=="m_bTurnActive"){need(!turn,"Lua Player turn duplicate native bool");turn=&field;}
 }
 need(identity&&identity->encoding==saves::PlayerCoreEncoding::i32&&identity->values.size()==1&&identity->values[0]==id&&
  !identity->wire_count&&identity->bytes.empty()&&identity->row_counts.empty()&&identity->hashes.empty(),
  "Lua Player turn read raw CvPlayer ID field unavailable");
 need(turn&&turn->encoding==saves::PlayerCoreEncoding::boolean&&turn->values.size()==1&&
  (turn->values[0]==0||turn->values[0]==1)&&!turn->wire_count&&turn->bytes.empty()&&turn->row_counts.empty()&&turn->hashes.empty(),
  "Lua Player turn read raw m_bTurnActive bool unavailable");
 const bool result=turn->values[0]!=0;require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::pregame_is_multiplayer_game()const{return typed([&]{
 require_current();const auto& state=*impl_->state;
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(state.setup.profile==content::TechnologyProfile::bnw&&state.world.game_state.layout==bnw&&
  state.world.pregame&&state.world.pregame->layout==bnw,
  "Lua PreGame read actual BNW native pregame absent");
 const auto& integers=state.world.pregame->archive.integers;const auto type=integers.find("game_type");
 need(type!=integers.end()&&type->second>=0&&type->second<=3&&type->second==state.setup.game_type,
  "Lua PreGame read native game type unavailable or disagrees with restored setup");
 // Original EXE428C70 -> CvDllPreGame vtable+D4 -> native
 // CvPreGame::isNetworkMultiplayerGame, not hotseat/email isGameMultiPlayer.
 const bool result=type->second==1;require_current();return result;
});}
void BnwLuaCalculatedPlotYieldOwner::bind_local_client(std::shared_ptr<const BnwLocalClientScalarOwner> client){
 require_current();auto& p=*impl_;need(!p.client_bound&&client,"Lua client must bind once to actual Source client");
 client->require_same_world(p.state);p.client=client;p.client_bound=true;
 require_current();client->require_same_world(p.state);
}
bool BnwLuaCalculatedPlotYieldOwner::local_client_has_sent_turn_complete()const{return typed([&]{
 require_current();const auto& p=*impl_;const auto client=p.client.lock();
 need(p.client_bound&&client,"Lua Network actual SP client reset owner absent/retired");
 client->require_same_world(p.state);const auto active=detail::bnw_game_active_player_read(*p.state);
 const bool result=client->has_sent_turn_complete(active);client->require_same_world(p.state);require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::local_client_has_sent_turn_all_complete()const{return typed([&]{
 require_current();const auto& p=*impl_;const auto client=p.client.lock();
 need(p.client_bound&&client,"Lua Network actual SP client reset owner absent/retired");
 client->require_same_world(p.state);const auto active=detail::bnw_game_active_player_read(*p.state);
 const bool result=client->has_sent_turn_all_complete(active);client->require_same_world(p.state);require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::local_client_is_processing_messages()const{return typed([&]{
 require_current();const auto& p=*impl_;const auto client=p.client.lock();
 need(p.client_bound&&client,"Lua Game actual SP client processing owner absent/retired");
 client->require_same_world(p.state);const bool result=client->is_processing_messages();
 client->require_same_world(p.state);require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::game_is_option(std::int32_t selector)const{return typed([&]{
 require_current();const auto& p=*impl_;const auto client=p.client.lock();
 need(p.client_bound&&client,"Lua Game.IsOption actual imported client owner absent/retired");
 client->require_same_world(p.state);const bool result=client->read_game_is_option_owned(selector);
 client->require_same_world(p.state);require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::game_is_option(const char* requested)const{return typed([&]{
 require_current();const auto& p=*impl_;const auto client=p.client.lock();
 need(p.client_bound&&client,"Lua Game.IsOption actual imported client owner absent/retired");
 client->require_same_world(p.state);const bool result=client->read_game_is_option_owned(requested);
 client->require_same_world(p.state);require_current();return result;
});}
bool BnwLuaCalculatedPlotYieldOwner::is_revealed(std::int64_t plot,std::int64_t team,bool debug)const{return typed([&]{
 require_current();const auto& p=*impl_;const auto index=native(plot),t=native(team);
 const auto& world=p.state->world;constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(index>=0&&world.game_state.layout==bnw&&world.map_header.layout==bnw&&world.map_plots&&
  world.map_plots->layout==bnw&&world.map_projection,
  "Lua reveal actual BNW plot source absent");
 const auto& grid=world.map_header.grid;const auto& projected=*world.map_projection;
 need(grid.width>0&&grid.height>0&&projected.grid.width==grid.width&&projected.grid.height==grid.height&&
  world.map_plots->plots.size()==grid.size()&&projected.cells.size()==grid.size()&&
  static_cast<std::uint64_t>(index)<grid.size(),"Lua reveal actual plot native grid unavailable");
 const auto slot=static_cast<std::size_t>(index);const auto& raw=world.map_plots->plots[slot];const auto& cell=projected.cells[slot];
 const auto xy=grid.coordinates(static_cast<std::uint64_t>(index));
 need(raw.index==slot&&raw.x==xy[0]&&raw.y==xy[1]&&cell.index==slot&&cell.x==xy[0]&&cell.y==xy[1],
  "Lua reveal actual raw/projected plot identity differs");
 // GetInstance identity precedes the source debug branch, but reveal/team
 // bit operands must remain AFTER it. bDebug is only a native argument.
 if(debug&&p.read_control->require_game_debug(*p.state,*p.source,p.identity)){
  require_current();return true;
 }
 const bool result=detail::bnw_plot_revealed_read(world,static_cast<std::uint64_t>(index),t);
 require_current();return result;
});}
ScriptMapProvider BnwLuaCalculatedPlotYieldOwner::current_map_borrow()const{
 require_current();const auto& p=*impl_;
 need(p.state->world.map_projection.has_value(),"Lua calculated yield actual map absent");
 const auto& pass=bnw_live_plot_impassability(*p.state);
 return lua_map_services(*p.state->world.map_projection,pass);
}
std::shared_ptr<BnwLuaCalculatedPlotYieldOwner> make_bnw_lua_calculated_plot_yield_owner(
 std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwNormalPlotYieldRules> rules){return typed([&]{
 auto p=std::make_unique<BnwLuaCalculatedPlotYieldOwner::Impl>();p->session=std::move(session);p->rules=std::move(rules);
 auto result=std::shared_ptr<BnwLuaCalculatedPlotYieldOwner>(new BnwLuaCalculatedPlotYieldOwner(std::move(p)));
 result->bind_initial();return result;
});}
ScriptMapProvider bnw_lua_calculated_plot_services(std::shared_ptr<const BnwLuaCalculatedPlotYieldOwner> owner){
 need(static_cast<bool>(owner),"Lua calculated map owner absent");
 // Wrappers hold the real parent; each fresh borrow lasts only this C++ call.
 // Never capture a projection/passability reference across replacement.
 ScriptMapProvider result;
 result.grid_size=[owner]{const auto map=owner->current_map_borrow();auto v=map.grid_size();owner->require_current();return v;};
 result.wraps=[owner]{const auto map=owner->current_map_borrow();auto v=map.wraps();owner->require_current();return v;};
 result.plot=[owner](auto x,auto y){const auto map=owner->current_map_borrow();auto v=map.plot(x,y);owner->require_current();return v;};
 result.plot_by_index=[owner](auto index){const auto map=owner->current_map_borrow();auto v=map.plot_by_index(index);owner->require_current();return v;};
 result.direction=[owner](auto x,auto y,auto d){const auto map=owner->current_map_borrow();auto v=map.direction(x,y,d);owner->require_current();return v;};
 result.distance=[owner](auto x,auto y,auto x2,auto y2){const auto map=owner->current_map_borrow();auto v=map.distance(x,y,x2,y2);owner->require_current();return v;};
 result.plot_value=[owner](auto index,auto op,auto team){const auto map=owner->current_map_borrow();auto v=map.plot_value(index,op,team);owner->require_current();return v;};
 result.calculated_yield=std::move(owner);return result;
}
}
