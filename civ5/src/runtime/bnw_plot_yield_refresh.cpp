#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_population.hpp"
#include <tuple>
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_unowned_plot_yield.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <set>
#include <map>
#include <stdexcept>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Plot refresh native int overflow");return static_cast<std::int32_t>(n);}
auto& named(auto& rows,std::string_view name){using T=std::remove_reference_t<decltype(rows.front())>;T* out=nullptr;for(auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Plot refresh duplicate source field");out=&r;}need(out,"Plot refresh missing source field");return *out;}
std::int32_t scalar(const auto& rows,std::string_view name){const auto& f=named(rows,name);need(f.values.size()==1,"Plot refresh source scalar shape differs");return i32(f.values.front());}
using SixShort=std::array<std::int16_t,6>;
std::int16_t narrow_short(std::int32_t value){
 const auto bits=static_cast<std::uint16_t>(value);
 return static_cast<std::int16_t>(bits<=INT16_MAX?bits:static_cast<std::int32_t>(bits)-65536);
}
struct Ledger {std::atomic<std::uint64_t> attempt_revision{0};std::atomic<bool> uncertain{false},failed{false};};
struct Operands {
 std::int64_t plot=-1,area=-1,terrain=-1,potential=-1,working_owner=-1,working_id=-1;
 std::array<std::int64_t,6> cached{},terrain_yields{};
 std::optional<std::pair<std::int64_t,std::int64_t>> city;
 std::pair<std::uint64_t,std::uint64_t> city_span{},citizens_span{};
 std::vector<std::int64_t> worked_flags;
 std::int32_t city_x=0,city_y=0;
 std::array<std::uint8_t,16> map_guid{};
 std::array<std::uint64_t,8> map_binding{};
 std::vector<std::int64_t> area_metadata,area_next_free,area_ids;
 std::vector<std::pair<std::uint64_t,std::uint64_t>> area_spans;
 bool has_area=false,worked=false,city_inputs=false;
};
Operands capture(const LiveSessionState& s,std::int64_t index,bool retain_city_inputs=false,
 const SixShort* targets=nullptr,bool normal=false){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&s.world.map_plots&&s.world.map_plots->layout==bnw&&s.world.map_projection&&index>=0&&static_cast<std::uint64_t>(index)<s.world.map_plots->plots.size(),"Plot refresh requires real BNW map/allocation");
 const auto& raw=s.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto& map=*s.world.map_projection;const auto& cell=map.cells.at(static_cast<std::size_t>(index));
 need(map.grid.size()==s.world.map_plots->plots.size()&&map.cells.size()==s.world.map_plots->plots.size()&&s.world.map_header.layout==bnw,"Plot refresh native grid/profile shape stale");
 const auto xy=map.grid.coordinates(static_cast<std::uint64_t>(index));need(raw.index==static_cast<std::uint64_t>(index)&&cell.index==raw.index&&raw.x==xy[0]&&raw.y==xy[1]&&cell.x==raw.x&&cell.y==raw.y&&map.grid.width==s.world.map_header.grid.width&&map.grid.height==s.world.map_header.grid.height&&map.wrap_x==s.world.map_header.wrap_x&&map.wrap_y==s.world.map_header.wrap_y,"Plot refresh map/projection identity stale");
 Operands o;o.plot=index;o.area=i32(named(raw.fields,"area").value);o.map_guid=s.world.map_header.guid;
 o.map_binding={map.grid.width,map.grid.height,static_cast<std::uint64_t>(map.wrap_x),static_cast<std::uint64_t>(map.wrap_y),raw.start_offset,raw.end_offset,s.world.map_header.start_offset,s.world.map_header.end_offset};
 if(o.area==-1)return o; // Native INVALID_INDEX returns NULL before touching the FLTA.
 const auto* current_area=resolve_bnw_area_registry(s.world,i32(o.area));
 const auto& areas=named(s.world.map_regions->containers,"areas");
 // The shared pure resolver validates the same complete FLTA before records.
 // Keep the exact original retained metadata/IDs/spans and source order.
 o.area_metadata=areas.legacy_metadata;o.area_next_free=areas.next_free;
 for(const auto& a:areas.records){o.area_ids.push_back(i32(named(a.fields,"id").value));o.area_spans.emplace_back(a.start_offset,a.end_offset);}
 o.has_area=current_area!=nullptr;
 if(!o.has_area)return o; // CvPlot::updateYield returns before calculateYield/getYield.
 o.terrain=named(raw.fields,"terrain_type").value;o.potential=named(raw.fields,"potential_city_work").value;
 need(cell.terrain_type&&*cell.terrain_type==o.terrain&&cell.potential_city_work&&o.potential>=0&&o.potential<=1&&*cell.potential_city_work==(o.potential!=0),"Plot refresh terrain/workability raw projection stale");
 if(!normal)need(bnw_plot_early_zero_yield(s,index,0).has_value(),"Plot refresh full calculateYield producer unavailable for normal plot");
 const auto& yields=named(raw.arrays,"yields");need(yields.columns==1&&yields.values.size()==6&&cell.cached_yields&&cell.cached_yields->size()==6,"Plot refresh native/projected six-yield cache missing");
 for(std::size_t y=0;y<6;++y){need(yields.values[y]>=INT16_MIN&&yields.values[y]<=INT16_MAX&&yields.values[y]==cell.cached_yields->at(static_cast<std::int64_t>(y)),"Plot refresh native/projected short cache stale");o.cached[y]=yields.values[y];}
 // The first normal capture authenticates area/cache without reading a city
 // before the genuine six calculations establish which short values differ.
 if(normal&&!targets)return o;
 o.city_inputs=retain_city_inputs;
 for(std::size_t y=0;y<6;++y)if(o.cached[y]!=(targets?targets->at(y):0))o.city_inputs=true;
 if(!o.city_inputs)return o;
 o.working_owner=named(raw.fields,"working_city_owner").value;o.working_id=named(raw.fields,"working_city_id").value;
 need(cell.yield_city&&cell.yield_city->owner==o.working_owner&&cell.yield_city->id==o.working_id,"Plot refresh working-city raw/projection stale");
 const auto* city=resolve_bnw_current_city(s,i32(o.working_owner),i32(o.working_id));if(!city)return o;
 const auto id=scalar(city->core.fields,"m_iID");o.city=std::pair{o.working_owner,std::int64_t(id)};o.city_span={city->core.start_offset,city->core.end_offset};
 const auto& owned=s.cities.players.at(o.working_owner).cities.at(id);
 o.city_x=scalar(city->core.fields,"m_iX");o.city_y=scalar(city->core.fields,"m_iY");need(owned.x==o.city_x&&owned.y==o.city_y&&o.city_x>=0&&o.city_y>=0&&static_cast<std::uint64_t>(o.city_x)<map.grid.width&&static_cast<std::uint64_t>(o.city_y)<map.grid.height,"Plot refresh live city coordinate stale");
 const auto& citizens=named(city->management.blocks,"citizens");need(city->management.layout==bnw,"Plot refresh citizen profile missing");o.citizens_span={citizens.start_offset,citizens.end_offset};
 const auto& flags=named(citizens.arrays,"m_pabWorkingPlot");need(flags.columns==1&&flags.values.size()==37,"Plot refresh native working flags unavailable");o.worked_flags=flags.values;for(auto v:o.worked_flags)need(v==0||v==1,"Plot refresh working flag not bool");
 const auto slot=detail::bnw_citizen_plot_slot(map,o.city_x,o.city_y,i32(static_cast<std::int64_t>(cell.x)),i32(static_cast<std::int64_t>(cell.y)));o.worked=slot>=0&&o.worked_flags.at(static_cast<std::size_t>(slot))!=0;
 if(o.worked){const auto& base=named(city->core.fields,"m_aiBaseYieldRateFromTerrain");need(base.encoding==saves::CityCoreEncoding::vector_i32&&base.values.size()==6,"Plot refresh raw terrain yield array unavailable");for(std::size_t y=0;y<6;++y){o.terrain_yields[y]=i32(base.values[y]);const auto it=owned.yields.find(static_cast<std::int64_t>(y));const auto live=it==owned.yields.end()?0:it->second.terrain;need(live==base.values[y],"Plot refresh raw/live terrain yields stale");}}
 return o;
}
bool same(const Operands& a,const Operands& b){return a.map_guid==b.map_guid&&a.map_binding==b.map_binding&&a.area_metadata==b.area_metadata&&a.area_next_free==b.area_next_free&&a.area_ids==b.area_ids&&a.area_spans==b.area_spans&&a.city_inputs==b.city_inputs&&a.plot==b.plot&&a.area==b.area&&a.has_area==b.has_area&&a.terrain==b.terrain&&a.potential==b.potential&&a.cached==b.cached&&a.working_owner==b.working_owner&&a.working_id==b.working_id&&a.city==b.city&&a.city_span==b.city_span&&a.citizens_span==b.citizens_span&&a.worked_flags==b.worked_flags&&a.city_x==b.city_x&&a.city_y==b.city_y&&a.worked==b.worked&&a.terrain_yields==b.terrain_yields;}
}
struct BnwPlotYieldRefreshIssuer::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};
 std::weak_ptr<const LiveSessionState> state;std::uint64_t generation=0,next_operation=0;
 std::weak_ptr<const saves::SavedSession> source;
 std::shared_ptr<const LiveSession::BnwGameDebugReadStorage> game;
 std::map<std::int64_t,std::uint64_t> claims;
 // Monotone private city CALL history. Latest claim changes, old calls remain.
 std::map<std::uint64_t,std::tuple<std::int64_t,std::uint64_t,std::uint8_t>> child_calls;
 std::map<std::uint64_t,std::tuple<std::int64_t,std::uint64_t,std::uint64_t>> policy_calls;
 std::map<std::uint64_t,std::tuple<std::int64_t,std::int32_t,std::int32_t>> population_calls;
};
struct BnwPlotYieldRefreshPending::Impl {
 std::weak_ptr<BnwPlotYieldRefreshIssuer> issuer;std::shared_ptr<LiveSessionState> state;
 std::shared_ptr<Ledger> ledger=std::make_shared<Ledger>();std::atomic<bool> invalid{false};
 std::thread::id thread=std::this_thread::get_id();Operands expected;
 std::array<std::int64_t,6> initial_terrain{};
 std::map<std::int64_t,CityYieldComponents> initial_owned;
 std::uint8_t terrain_written_mask=0;
 std::array<std::int64_t,6> initial_cached{};
 SixShort targets{};std::optional<BnwNormalPlotYieldRules> normal_rules;
 std::shared_ptr<const saves::SavedSession> source;SessionCheckpointIdentity identity;
 BnwPlotYieldRefreshProgress progress;std::uint64_t generation=0;bool quarantine_staged=false;
 // Detached map nodes are allocated before source writes, never after a cache store.
 std::map<std::int64_t,CityYieldComponents>::node_type insertions[6];
};
struct BnwPlotYieldRefreshCheckpoint::Impl {
 SixShort targets{};std::optional<BnwNormalPlotYieldRules> normal_rules;
 std::shared_ptr<const saves::SavedSession> source;SessionCheckpointIdentity identity;
 Operands expected;BnwPlotYieldRefreshProgress progress;std::shared_ptr<Ledger> ledger;std::uint64_t observed_revision=0;bool observed_uncertain=false,forced_quarantine=false;};
BnwPlotYieldRefreshIssuer::BnwPlotYieldRefreshIssuer(const std::shared_ptr<LiveSessionState>& s,std::uint64_t gen):impl_(std::make_shared<Impl>()){need(s&&gen,"Plot refresh issuer allocation/generation absent");impl_->state=s;impl_->generation=gen;}
BnwPlotYieldRefreshIssuer::~BnwPlotYieldRefreshIssuer(){retire();}
void BnwPlotYieldRefreshIssuer::retire()noexcept{if(impl_)impl_->retired.store(true);}
void BnwPlotYieldRefreshIssuer::bind_game_debug(
 const std::shared_ptr<LiveSession::BnwGameDebugReadStorage>& game,
 const std::shared_ptr<const saves::SavedSession>& source){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load(),"Normal Game read control retired/foreign");
 need(game&&source&&game->thread==p.thread&&!game->retired.load()&&p.generation&&p.state.lock(),"Normal Game actual storage/source unavailable");
 need(game->raw_mode==game->cached_mode&&!game->identity.payload_sha256.empty()&&!game->identity.rules_sha256.empty(),"Normal Game actual raw/cache/source mismatch");
 if(p.game){need(p.game==game&&p.source.lock()==source,"Normal Game actual storage/source replaced without new control");return;}
 p.source=source;p.game=game; // No copied bool or session backedge.
}
bool BnwPlotYieldRefreshIssuer::require_game_debug(const LiveSessionState& state,
 const saves::SavedSession& source,const SessionCheckpointIdentity& identity)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&!p->retired.load(),"Normal Game read control retired/foreign");
 // Stable constructor thread/control precede actual Game/source/world reads.
 need(p->generation&&p->state.lock().get()==&state&&p->source.lock().get()==&source&&p->game,
  "Normal Game requires SAME real source/state allocation; early issuer is not a normal owner");
 const auto game=p->game;need(game->thread==p->thread&&!game->retired.load(),"Normal Game actual field owner retired/foreign");
 need(game->identity.payload_sha256==identity.payload_sha256&&game->identity.rules_sha256==identity.rules_sha256,
  "Normal Game actual source identity differs");
 need(game->raw_mode==game->cached_mode,"Normal Game actual raw/cache transition unavailable");
 return game->cached_mode;
}
SixShort BnwPlotYieldRefreshPending::normal_targets(const BnwPlotYieldRefreshIssuer& issuer,
 const LiveSessionState& state,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,
 const BnwNormalPlotYieldRules& rules,std::int64_t plot){
 SixShort out{};const auto index=i32(plot);
 for(std::int32_t y=0;y<6;++y)out.at(static_cast<std::size_t>(y))=
  narrow_short(rules.calculate_current_normal(issuer,state,source,identity,index,y));
 return out;
}
BnwPlotYieldRefreshPending::BnwPlotYieldRefreshPending(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwPlotYieldRefreshPending::~BnwPlotYieldRefreshPending()=default;
void BnwPlotYieldRefreshPending::require_current()const{
 auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Plot refresh foreign thread");
 need(!p.quarantine_staged,"Plot refresh quarantine candidate not committed");
 const auto issuer=p.issuer.lock();need(issuer&&!issuer->impl_->retired.load()&&!p.invalid.load(),"Plot refresh issuer retired/expired or old allocation");
 need(issuer->impl_->thread==p.thread&&issuer->impl_->generation==p.generation&&issuer->impl_->state.lock().get()==p.state.get(),"Plot refresh SAME issuer allocation/generation lost");
 need(issuer->impl_->claims.contains(p.progress.plot)&&issuer->impl_->claims.at(p.progress.plot)==p.progress.operation,"Plot refresh private once slot differs");
 need(!p.ledger->failed.load(),"Plot refresh terminal failure; no retry/reset");
 try{
  if(p.normal_rules){
   p.normal_rules->require_current(p.identity.rules_sha256); // BEFORE any world/source borrow.
   need(static_cast<bool>(p.source),"Normal plot refresh retained source absent");
   (void)issuer->require_game_debug(*p.state,*p.source,p.identity);
   if(p.expected.has_area)need(normal_targets(*issuer,*p.state,*p.source,p.identity,*p.normal_rules,p.expected.plot)==p.targets,
    "Normal plot refresh current calculated short targets stale");
  }
  need(same(p.expected,capture(*p.state,p.expected.plot,p.expected.city_inputs,
   p.normal_rules?&p.targets:nullptr,p.normal_rules.has_value())),"Plot refresh current raw/live prefix stale");
  if(p.normal_rules){p.normal_rules->require_current(p.identity.rules_sha256);(void)issuer->require_game_debug(*p.state,*p.source,p.identity);}
 }catch(...){p.ledger->failed.store(true);throw;}
}
void BnwPlotYieldRefreshPending::require_policy_parent_source(const BnwPolicyAdoptionPending& parent)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Policy plot child foreign parent thread");const auto issuer=p.issuer.lock();need(issuer&&issuer->impl_->thread==p.thread&&!issuer->impl_->retired.load()&&issuer->impl_->generation==p.generation&&issuer->impl_->state.lock()==p.state,"Policy plot child original issuer/state lost");
 const auto call=parent.private_policy_plot_identity();const auto& owner=*issuer->impl_;need(owner.claims.contains(p.progress.plot)&&owner.claims.at(p.progress.plot)==p.progress.operation&&owner.policy_calls.contains(p.progress.operation)&&owner.policy_calls.at(p.progress.operation)==std::tuple{p.progress.plot,call.first,call.second},"Policy plot child is not the original issued parent/serial/uint64 ordinal");
 require_current();
}
BnwPlotYieldRefreshProgress BnwPlotYieldRefreshPending::progress()const{require_current();return impl_->progress;}
void BnwPlotYieldRefreshPending::prepare_insertions(Impl& p){
 // Every actual cache/terrain store invalidates existing UI publications.
 // Reserve the entire remaining finite prefix before any first store.
 std::uint64_t writes=0;
 for(std::size_t y=p.progress.next_yield;y<6;++y)
  if(p.expected.cached[y]!=p.targets[y])writes+=p.expected.worked?2U:1U;
 need(p.state->ui_source_revision<=UINT64_MAX-writes,"Plot refresh UI source revision overflow");
 if(!p.expected.worked)return;
 auto& owned=p.state->cities.players.at(p.expected.city->first).cities.at(p.expected.city->second);
 for(std::size_t y=p.progress.next_yield;y<6;++y)if(p.expected.cached[y]!=p.targets[y]){
  const auto delta=i32(std::int64_t(p.targets[y])-p.expected.cached[y]);
  (void)i32(p.expected.terrain_yields[y]+delta);
  if(!owned.yields.contains(static_cast<std::int64_t>(y))){
   std::map<std::int64_t,CityYieldComponents> staged;
   staged.emplace(static_cast<std::int64_t>(y),CityYieldComponents{});
   p.insertions[y]=staged.extract(staged.begin());
  }
 }
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::prepare(const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& s,std::int64_t plot){
 need(issuer&&s&&issuer->impl_->thread==std::this_thread::get_id()&&!issuer->impl_->retired.load()&&issuer->impl_->state.lock().get()==s.get(),"Plot refresh private issuer not current");
 auto& owner=*issuer->impl_;need(owner.next_operation<UINT64_MAX&&!owner.claims.contains(plot),"Plot refresh SAME issuer/plot already claimed; cannot replay pending effects");
 auto p=std::make_unique<Impl>();p->issuer=issuer;p->state=s;p->generation=owner.generation;p->expected=capture(*s,plot);p->progress.plot=plot;p->progress.operation=owner.next_operation+1;
 prepare_insertions(*p);
 auto out=std::shared_ptr<BnwPlotYieldRefreshPending>(new BnwPlotYieldRefreshPending(std::move(p)));
 owner.claims.emplace(plot,out->impl_->progress.operation);++owner.next_operation;return out;
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::prepare_normal(
 const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& s,
 const std::shared_ptr<const saves::SavedSession>& source,const SessionCheckpointIdentity& identity,
 const BnwNormalPlotYieldRules& rules,std::int64_t plot){
 return prepare_normal_bound(issuer,s,source,identity,rules,plot,nullptr);
}

std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::prepare_normal_policy(
 const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source,const SessionCheckpointIdentity& identity,
 const BnwNormalPlotYieldRules& rules,std::int64_t plot,const BnwPolicyAdoptionPending& policy){
 return prepare_normal_bound(issuer,state,source,identity,rules,plot,nullptr,nullptr,&policy);
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::prepare_normal_child(
 const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& s,
 const std::shared_ptr<const saves::SavedSession>& source,const SessionCheckpointIdentity& identity,
 const BnwNormalPlotYieldRules& rules,BnwCityYieldRefreshPending& parent){
 const auto plot=parent.require_unissued_slot();
 return prepare_normal_bound(issuer,s,source,identity,rules,plot,&parent);
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::prepare_normal_population(
 const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source,const SessionCheckpointIdentity& identity,
 const BnwNormalPlotYieldRules& rules,std::int64_t plot,const BnwCityPopulationPending& parent){
 return prepare_normal_bound(issuer,state,source,identity,rules,plot,nullptr,&parent);
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::prepare_normal_bound(
 const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& s,
 const std::shared_ptr<const saves::SavedSession>& source,const SessionCheckpointIdentity& identity,
 const BnwNormalPlotYieldRules& rules,std::int64_t plot,const BnwCityYieldRefreshPending* parent,const BnwCityPopulationPending* population,const BnwPolicyAdoptionPending* policy){
 rules.require_current(identity.rules_sha256); // BEFORE actual issuer/world/source binding reads.
 need(issuer&&s&&source&&issuer->impl_->thread==std::this_thread::get_id()&&!issuer->impl_->retired.load()&&issuer->impl_->state.lock().get()==s.get(),"Plot refresh private issuer not current");
 auto& owner=*issuer->impl_;need(owner.next_operation<UINT64_MAX&&(parent||population||policy||!owner.claims.contains(plot)),"Plot refresh SAME issuer/plot already claimed; cannot replay pending effects");
 need(static_cast<int>(parent!=nullptr)+static_cast<int>(population!=nullptr)+static_cast<int>(policy!=nullptr)<=1,"Plot child cannot have two actual parents");
 if(policy)policy->require_unissued_policy_plot_source(*issuer,*s,*source,identity,plot);
 if(population)population->require_unissued_plot_source(*issuer,*s,*source,identity,plot);
 if(parent){parent->require_child_source(*issuer,*s,*source,identity);need(parent->require_unissued_slot()==plot,"Plot child actual native slot differs");}
 (void)issuer->require_game_debug(*s,*source,identity); // BEFORE no-area capture/world.
 auto p=std::make_unique<Impl>();p->issuer=issuer;p->state=s;p->source=source;p->identity=identity;p->normal_rules=rules;p->generation=owner.generation;
 p->expected=capture(*s,plot,false,nullptr,true); // Native NULL area returns before calculation/caches.
 if(p->expected.has_area){
  p->targets=normal_targets(*issuer,*s,*source,identity,rules,plot);
  p->expected=capture(*s,plot,false,&p->targets,true);
 }
 rules.require_current(identity.rules_sha256);(void)issuer->require_game_debug(*s,*source,identity);
 p->initial_cached=p->expected.cached;
 p->initial_terrain=p->expected.terrain_yields;
 if(p->expected.worked)p->initial_owned=s->cities.players.at(p->expected.city->first).cities.at(p->expected.city->second).yields;
 p->progress.plot=plot;p->progress.operation=owner.next_operation+1;prepare_insertions(*p);
 auto out=std::shared_ptr<BnwPlotYieldRefreshPending>(new BnwPlotYieldRefreshPending(std::move(p)));
 if(!parent&&!population&&!policy){owner.claims.emplace(plot,out->impl_->progress.operation);++owner.next_operation;return out;}

 if(policy){
  const auto call=policy->private_policy_plot_identity();
  std::map<std::int64_t,std::uint64_t> claims;if(!owner.claims.contains(plot))claims.emplace(plot,out->impl_->progress.operation);
  auto claim=claims.empty()?decltype(claims)::node_type{}:claims.extract(claims.begin());
  std::map<std::uint64_t,std::tuple<std::int64_t,std::uint64_t,std::uint64_t>> history;
  history.emplace(out->impl_->progress.operation,std::tuple{plot,call.first,call.second});auto record=history.extract(history.begin());
  policy->require_unissued_policy_plot_source(*issuer,*s,*source,identity,plot);rules.require_current(identity.rules_sha256);
  if(owner.claims.contains(plot))owner.claims.at(plot)=out->impl_->progress.operation;else owner.claims.insert(std::move(claim));
  owner.policy_calls.insert(std::move(record));++owner.next_operation;return out;
 }
 if(population){
  std::map<std::int64_t,std::uint64_t> claims;
  if(!owner.claims.contains(plot))claims.emplace(plot,out->impl_->progress.operation);
  auto claim=claims.empty()?decltype(claims)::node_type{}:claims.extract(claims.begin());
  const auto observation=population->progress();
  std::map<std::uint64_t,std::tuple<std::int64_t,std::int32_t,std::int32_t>> history;
  history.emplace(out->impl_->progress.operation,std::tuple{plot,observation.owner,observation.city});
  auto record=history.extract(history.begin());
  population->require_unissued_plot_source(*issuer,*s,*source,identity,plot);rules.require_current(identity.rules_sha256);
  if(owner.claims.contains(plot))owner.claims.at(plot)=out->impl_->progress.operation;
  else owner.claims.insert(std::move(claim));
  owner.population_calls.insert(std::move(record));++owner.next_operation;return out;
 }
 const auto call=parent->private_call_identity();
 std::map<std::int64_t,std::uint64_t> claim_staging;
 if(!owner.claims.contains(plot))claim_staging.emplace(plot,out->impl_->progress.operation);
 auto claim_node=claim_staging.empty()?decltype(claim_staging)::node_type{}:claim_staging.extract(claim_staging.begin());
 std::map<std::uint64_t,std::tuple<std::int64_t,std::uint64_t,std::uint8_t>> history_staging;
 history_staging.emplace(out->impl_->progress.operation,std::tuple{plot,call.first,call.second});
 auto history_node=history_staging.extract(history_staging.begin());
 parent->require_child_source(*issuer,*s,*source,identity);rules.require_current(identity.rules_sha256);
 if(owner.claims.contains(plot))owner.claims.at(plot)=out->impl_->progress.operation;
 else owner.claims.insert(std::move(claim_node));
 owner.child_calls.insert(std::move(history_node));++owner.next_operation;return out;
}
std::pair<std::int64_t,std::uint64_t> BnwPlotYieldRefreshPending::issued_identity()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Plot issued identity foreign thread");
 const auto issuer=p.issuer.lock();need(issuer&&!issuer->impl_->retired.load()&&!p.invalid.load()&&
  issuer->impl_->generation==p.generation&&issuer->impl_->state.lock()==p.state&&
  issuer->impl_->claims.contains(p.progress.plot)&&issuer->impl_->claims.at(p.progress.plot)==p.progress.operation,
  "Plot issued identity actual control/claim retired");
 return {p.progress.plot,p.progress.operation};
}
BnwPlotYieldRefreshPending::TerrainPrefixSource BnwPlotYieldRefreshPending::bound_terrain_prefix_source()const{
 require_current();const auto& p=*impl_;
 TerrainPrefixSource out;out.state=p.state;out.source=p.source;out.identity=p.identity;
 out.generation=p.generation;out.operation=p.progress.operation;out.ledger_revision=p.ledger->attempt_revision.load();
 out.written_yields=p.terrain_written_mask;out.terrain_write_count=p.progress.terrain_writes;out.next_yield=p.progress.next_yield;
 need(!p.progress.effect_started&&!p.ledger->uncertain.load(),"City terrain prefix cannot acknowledge an uncertain effect");
 if(p.expected.worked){
  need(p.expected.city.has_value(),"City terrain prefix actual working city absent");
  out.city=std::pair{i32(p.expected.city->first),i32(p.expected.city->second)};
  out.initial_terrain=p.initial_terrain;out.current_terrain=p.expected.terrain_yields;
  out.initial_owned=p.initial_owned;out.current_owned=p.state->cities.players.at(out.city->first).cities.at(out.city->second).yields;
  auto expected=p.initial_owned;std::uint8_t writes=0;
  for(std::size_t y=0;y<6;++y)if((p.terrain_written_mask&(1U<<y))!=0){
   ++writes;expected[static_cast<std::int64_t>(y)].terrain=p.expected.terrain_yields[y];
  }
  need(writes==p.progress.terrain_writes&&expected.size()==out.current_owned.size(),"City terrain prefix actual store ledger/owned shape differs");
  auto values=[](const CityYieldComponents& c){return std::tie(c.terrain,c.buildings,c.specialists,c.misc,c.religion,
   c.per_population,c.per_religion,c.rate_modifier,c.resource_modifier,c.process_modifier);};
  for(const auto& [key,value]:expected){const auto found=out.current_owned.find(key);
   need(found!=out.current_owned.end()&&values(value)==values(found->second),"City terrain prefix unrelated owned component replaced");}
 }else need(!p.terrain_written_mask&&!p.progress.terrain_writes,"City terrain prefix has writes without actual worked city");
 require_current();return out;
}
BnwPlotYieldRefreshPending::TerrainPrefixSource BnwPlotYieldRefreshPending::terrain_prefix_source(
 const BnwCityYieldRefreshPending& parent,const BnwCityEntryPending& entry)const{
 parent.require_entry_child_prefix(entry,*this);auto out=bound_terrain_prefix_source();
 parent.require_entry_child_prefix(entry,*this);require_current();return out;
}
BnwPlotYieldRefreshPending::PopulationPlotPrefixSource BnwPlotYieldRefreshPending::population_prefix_source(
 const BnwCityPopulationPending& parent)const{
 parent.require_plot_binding(*this);require_current();const auto& p=*impl_;
 need(p.normal_rules.has_value(),"Population plot child must be genuine normal source calculation");
 PopulationPlotPrefixSource out;out.terrain=bound_terrain_prefix_source();out.progress=p.progress;
 out.initial_cached=p.initial_cached;out.current_cached=p.expected.cached;
 std::uint8_t writes=0;
 for(std::size_t y=0;y<6;++y)if(out.initial_cached[y]!=out.current_cached[y]){
  ++writes;need(out.current_cached[y]==p.targets[y],"Population plot actual cache write differs from calculated target");
 }
 need(writes==p.progress.plot_writes,"Population plot actual cache write count differs");
 parent.require_plot_binding(*this);require_current();return out;
}
BnwPlotYieldRefreshPending::PopulationPlotPrefixSource BnwPlotYieldRefreshPending::population_city_prefix_source(
 const BnwCityYieldRefreshPending& city,const BnwCityPopulationPending& parent)const{
 city.require_population_child_prefix(parent,*this);require_current();const auto& p=*impl_;
 need(p.normal_rules.has_value(),"Population plot child must be genuine normal source calculation");
 PopulationPlotPrefixSource out;out.terrain=bound_terrain_prefix_source();out.progress=p.progress;
 out.initial_cached=p.initial_cached;out.current_cached=p.expected.cached;
 std::uint8_t writes=0;
 for(std::size_t y=0;y<6;++y)if(out.initial_cached[y]!=out.current_cached[y]){
  ++writes;need(out.current_cached[y]==p.targets[y],"Population plot actual cache write differs from calculated target");
 }
 need(writes==p.progress.plot_writes,"Population plot actual cache write count differs");
 city.require_population_child_prefix(parent,*this);require_current();return out;
}
void BnwPlotYieldRefreshPending::advance_prefix(){
 require_current();auto& p=*impl_;need(p.progress.phase==BnwPlotYieldRefreshPhase::prepared&&!p.progress.effect_started,"Plot refresh source cursor cannot replay");
 if(!p.expected.has_area){p.progress.phase=BnwPlotYieldRefreshPhase::skipped_no_area;return;}
 try{
  auto& s=*p.state;auto& raw=s.world.map_plots->plots.at(static_cast<std::size_t>(p.expected.plot));auto& projection=s.world.map_projection->cells.at(static_cast<std::size_t>(p.expected.plot));auto& ys=named(raw.arrays,"yields").values;
  while(p.progress.next_yield<6){const auto y=p.progress.next_yield;if(p.expected.cached[y]==p.targets[y]){++p.progress.next_yield;continue;}
   const auto old=p.expected.cached[y];const auto value=p.targets[y];
   need(s.ui_source_revision<=UINT64_MAX-(p.expected.worked?2U:1U),
    "Plot refresh UI source revision overflow before cache store");
   saves::SavedCityObservation* city=nullptr;
   std::vector<std::int64_t>* base=nullptr;CityYieldComponents* owned_yield=nullptr;
   std::int32_t next=0;
   if(p.expected.worked){
    const auto* resolved=resolve_bnw_current_city(s,i32(p.expected.city->first),i32(p.expected.city->second));
    need(resolved,"Plot refresh working city disappeared before cache store");
    // Resolve mutable storage through the actual current roster, not a const cast.
    for(auto& player:s.world.all_players->players)for(auto& candidate:player.cities.cities)
     if(&candidate==resolved)city=&candidate;
    need(city,"Plot refresh resolved city is outside actual mutable roster");
    base=&named(city->core.fields,"m_aiBaseYieldRateFromTerrain").values;
    auto& owned=s.cities.players.at(p.expected.city->first).cities.at(p.expected.city->second);
    next=i32(p.expected.terrain_yields[y]+i32(std::int64_t(value)-old));
    if(!owned.yields.contains(y)){
     need(!p.insertions[y].empty(),"Plot refresh preallocated terrain mirror node missing");
     owned.yields.insert(std::move(p.insertions[y]));
    }
    owned_yield=&owned.yields.at(y);
   }
   ys[y]=value;projection.cached_yields->at(y)=value;p.expected.cached[y]=value;
   ++p.progress.plot_writes;++s.ui_source_revision;
   if(p.expected.worked){
    (*base)[y]=next;owned_yield->terrain=next;p.expected.terrain_yields[y]=next;
    ++p.progress.terrain_writes;p.terrain_written_mask=static_cast<std::uint8_t>(p.terrain_written_mask|(1U<<y));++s.ui_source_revision;
    p.progress.phase=BnwPlotYieldRefreshPhase::city_screen_predicate;return;
   }
   ++p.progress.next_yield;
  }
  p.progress.phase=p.progress.plot_writes?BnwPlotYieldRefreshPhase::symbols:BnwPlotYieldRefreshPhase::complete;
  p.progress.fresh_six_yields=p.progress.phase==BnwPlotYieldRefreshPhase::complete;
 }catch(...){p.ledger->failed.store(true);p.progress.phase=BnwPlotYieldRefreshPhase::terminal_failure;throw;}
}
BnwPlotYieldRefreshPending::UiTailSource BnwPlotYieldRefreshPending::ui_tail_source()const{
 require_current();const auto& p=*impl_;
 need(p.progress.phase==BnwPlotYieldRefreshPhase::city_screen_predicate||
  p.progress.phase==BnwPlotYieldRefreshPhase::city_screen_dirty||
  p.progress.phase==BnwPlotYieldRefreshPhase::symbols,"Plot UI tail cannot replay completed/non-tail operation");
 UiTailSource out;
 out.state=p.state;out.generation=p.generation;out.operation=p.progress.operation;
 out.plot=p.expected.plot;out.map_guid=p.expected.map_guid;
 const auto xy=p.state->world.map_projection->grid.coordinates(static_cast<std::uint64_t>(out.plot));
 out.x=xy[0];out.y=xy[1];
 for(std::size_t y=0;y<6;++y)out.yields[y]=static_cast<std::int16_t>(p.expected.cached[y]);
 if(p.expected.city)out.city=std::pair{i32(p.expected.city->first),i32(p.expected.city->second)};
 out.next_yield=p.progress.next_yield;out.phase=p.progress.phase;return out;
}
void BnwPlotYieldRefreshPending::begin_required_effect(){require_current();auto& p=*impl_;need(!p.progress.effect_started&&(p.progress.phase==BnwPlotYieldRefreshPhase::city_screen_predicate||p.progress.phase==BnwPlotYieldRefreshPhase::city_screen_dirty||p.progress.phase==BnwPlotYieldRefreshPhase::symbols),"Plot refresh required effect cannot replay");need(p.ledger->attempt_revision.load()<UINT64_MAX,"Plot refresh effect revision overflow");p.ledger->attempt_revision.fetch_add(1);p.ledger->uncertain.store(true);p.progress.effect_started=true;}
void BnwPlotYieldRefreshPending::complete_city_screen_predicate(bool dirty){require_current();auto& p=*impl_;need(p.progress.phase==BnwPlotYieldRefreshPhase::city_screen_predicate&&p.progress.effect_started,"Plot refresh predicate receipt not started/current");p.progress.effect_started=false;p.ledger->uncertain.store(false);if(dirty){p.progress.phase=BnwPlotYieldRefreshPhase::city_screen_dirty;return;}++p.progress.next_yield;p.progress.phase=BnwPlotYieldRefreshPhase::prepared;advance_prefix();}
void BnwPlotYieldRefreshPending::complete_required_effect(){require_current();auto& p=*impl_;need(p.progress.effect_started&&(p.progress.phase==BnwPlotYieldRefreshPhase::city_screen_dirty||p.progress.phase==BnwPlotYieldRefreshPhase::symbols),"Plot refresh completion cannot skip required effect");p.progress.effect_started=false;p.ledger->uncertain.store(false);if(p.progress.phase==BnwPlotYieldRefreshPhase::symbols){p.progress.phase=BnwPlotYieldRefreshPhase::complete;p.progress.fresh_six_yields=true;return;}++p.progress.next_yield;p.progress.phase=BnwPlotYieldRefreshPhase::prepared;advance_prefix();}
void BnwPlotYieldRefreshPending::fail_required_effect()noexcept{impl_->ledger->failed.store(true);}
void BnwPlotYieldRefreshPending::invalidate()noexcept{impl_->invalid.store(true);}
void BnwPlotYieldRefreshPending::commit_quarantine()noexcept{if(impl_->quarantine_staged){impl_->ledger->failed.store(true);impl_->quarantine_staged=false;}}
BnwPlotYieldRefreshCheckpoint::BnwPlotYieldRefreshCheckpoint(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> BnwPlotYieldRefreshPending::checkpoint()const{
 auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Plot refresh checkpoint foreign thread");
 const auto issuer=p.issuer.lock();const bool retired=!issuer||issuer->impl_->retired.load()||p.invalid.load()||issuer->impl_->thread!=p.thread||issuer->impl_->generation!=p.generation||issuer->impl_->state.lock().get()!=p.state.get();
 // Retired history exports OWNED metadata only, never old-world reads.
 // It can preserve a once slot, but cannot grant permission to resume it.
 // Healthy checkpoints validate actual raw/mirror operands as before.
 if(!retired&&!p.ledger->failed.load())require_current();
 auto cp=std::make_shared<BnwPlotYieldRefreshCheckpoint::Impl>();cp->targets=p.targets;cp->normal_rules=p.normal_rules;cp->source=p.source;cp->identity=p.identity;cp->expected=p.expected;cp->progress=p.progress;cp->ledger=p.ledger;cp->observed_revision=p.ledger->attempt_revision.load();cp->observed_uncertain=p.ledger->uncertain.load();cp->forced_quarantine=retired;
 return std::shared_ptr<const BnwPlotYieldRefreshCheckpoint>(new BnwPlotYieldRefreshCheckpoint(std::move(cp)));
}
std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> BnwPlotYieldRefreshPending::checkpoint_history()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Plot metadata history foreign thread");
 auto cp=std::make_shared<BnwPlotYieldRefreshCheckpoint::Impl>();
 cp->targets=p.targets;cp->normal_rules=p.normal_rules;cp->source=p.source;cp->identity=p.identity;
 cp->expected=p.expected;cp->progress=p.progress;cp->ledger=p.ledger;
 cp->observed_revision=p.ledger->attempt_revision.load();cp->observed_uncertain=p.ledger->uncertain.load();cp->forced_quarantine=true;
 return std::shared_ptr<const BnwPlotYieldRefreshCheckpoint>(new BnwPlotYieldRefreshCheckpoint(std::move(cp)));
}
std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> BnwPlotYieldRefreshPending::quarantined_history(const BnwPlotYieldRefreshCheckpoint& history){
 need(bool(history.impl_),"Plot metadata checkpoint absent");
 auto cp=std::make_shared<BnwPlotYieldRefreshCheckpoint::Impl>(*history.impl_);cp->forced_quarantine=true;
 return std::shared_ptr<const BnwPlotYieldRefreshCheckpoint>(new BnwPlotYieldRefreshCheckpoint(std::move(cp)));
}
bool BnwPlotYieldRefreshPending::preflight_restore_rules(const BnwPlotYieldRefreshCheckpoint& cp,
 const std::optional<LiveSession::BnwGameDebugReadState>& snapshot){
 need(static_cast<bool>(cp.impl_),"Plot refresh checkpoint metadata absent");
 const auto& c=*cp.impl_;
 const bool quarantine=c.forced_quarantine||c.progress.effect_started||c.observed_uncertain||c.ledger->failed.load()||c.ledger->uncertain.load()||c.ledger->attempt_revision.load()!=c.observed_revision;
 if(quarantine||!c.normal_rules)return false; // OWNED metadata; no old Game/DB/world queries.
 c.normal_rules->require_current(c.identity.rules_sha256);
 // The session's private snapshot is restoration DATA, never a prior token.
 need(snapshot&&snapshot->raw_mode==snapshot->cached_mode&&
  snapshot->identity.payload_sha256==c.identity.payload_sha256&&snapshot->identity.rules_sha256==c.identity.rules_sha256,
  "Healthy normal checkpoint has no matching actual Game restoration data");
 return true;
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::restore_checkpoint(const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& state,const BnwPlotYieldRefreshCheckpoint& cp){
 need(cp.impl_&&issuer&&state&&issuer->impl_->thread==std::this_thread::get_id()&&!issuer->impl_->retired.load()&&issuer->impl_->state.lock().get()==state.get(),"Plot refresh checkpoint has no new genuine issuer/allocation");
 need(!issuer->impl_->claims.contains(cp.impl_->expected.plot),"Plot refresh checkpoint plot already issued");
 const bool quarantine=cp.impl_->forced_quarantine||cp.impl_->progress.effect_started||cp.impl_->observed_uncertain||cp.impl_->ledger->failed.load()||cp.impl_->ledger->uncertain.load()||cp.impl_->ledger->attempt_revision.load()!=cp.impl_->observed_revision;
 if(!quarantine){
  const auto& c=*cp.impl_;
  if(c.normal_rules){
   c.normal_rules->require_current(c.identity.rules_sha256); // Before restored-world capture.
   need(static_cast<bool>(c.source),"Normal plot refresh checkpoint retained source absent");
   (void)issuer->require_game_debug(*state,*c.source,c.identity); // NEW staged owner BEFORE any capture.
   if(c.expected.has_area)need(normal_targets(*issuer,*state,*c.source,c.identity,*c.normal_rules,c.expected.plot)==c.targets,
    "Normal plot refresh checkpoint calculated short targets differ");
  }
  need(same(c.expected,capture(*state,c.expected.plot,c.expected.city_inputs,
   c.normal_rules?&c.targets:nullptr,c.normal_rules.has_value())),"Plot refresh checkpoint current restored prefix differs");
  if(c.normal_rules){c.normal_rules->require_current(c.identity.rules_sha256);(void)issuer->require_game_debug(*state,*c.source,c.identity);}
 }
 auto p=std::make_unique<Impl>();p->issuer=issuer;p->state=state;p->generation=issuer->impl_->generation;p->targets=cp.impl_->targets;p->normal_rules=cp.impl_->normal_rules;p->source=cp.impl_->source;p->identity=cp.impl_->identity;p->expected=cp.impl_->expected;p->progress=cp.impl_->progress;p->ledger=cp.impl_->ledger;
 if(quarantine){p->quarantine_staged=true;p->progress.phase=BnwPlotYieldRefreshPhase::terminal_failure;p->progress.fresh_six_yields=false;}else prepare_insertions(*p);
 auto out=std::shared_ptr<BnwPlotYieldRefreshPending>(new BnwPlotYieldRefreshPending(std::move(p)));issuer->impl_->claims.emplace(out->impl_->expected.plot,out->impl_->progress.operation);issuer->impl_->next_operation=std::max(issuer->impl_->next_operation,out->impl_->progress.operation);return out;
}
bool BnwPlotYieldRefreshPending::has_irreversible_history()const{
 need(impl_->thread==std::this_thread::get_id(),"Plot refresh history foreign thread");
 return impl_->ledger->attempt_revision.load()!=0||impl_->ledger->failed.load();
}
void BnwPlotYieldRefreshPending::require_early_zero_checkpoint_mode()const{
 need(impl_->thread==std::this_thread::get_id(),"Plot refresh checkpoint mode foreign thread");
 // Normal mode is actual private retained producer state, even for no writes,
 // zero results or quarantine. Refuse before any normal world/DB query.
 need(!impl_->normal_rules,"Plot/terrain pair cannot capture normal calculation history");
 require_current();
}
std::shared_ptr<BnwPlotYieldRefreshPending> BnwPlotYieldRefreshPending::quarantine_for_new_allocation(const std::shared_ptr<BnwPlotYieldRefreshIssuer>& issuer,const std::shared_ptr<LiveSessionState>& state,const BnwPlotYieldRefreshPending& old){
 need(old.has_irreversible_history(),"Plot refresh absent checkpoint slot has no irreversible history");
 need(issuer&&state&&issuer->impl_->thread==std::this_thread::get_id()&&!issuer->impl_->retired.load()&&issuer->impl_->state.lock().get()==state.get()&&!issuer->impl_->claims.contains(old.impl_->progress.plot),"Plot refresh quarantine issuer/allocation differs");
 auto p=std::make_unique<Impl>();p->issuer=issuer;p->state=state;p->generation=issuer->impl_->generation;p->targets=old.impl_->targets;p->normal_rules=old.impl_->normal_rules;p->source=old.impl_->source;p->identity=old.impl_->identity;p->expected=old.impl_->expected;p->progress=old.impl_->progress;p->ledger=old.impl_->ledger;p->progress.phase=BnwPlotYieldRefreshPhase::terminal_failure;p->progress.fresh_six_yields=false;p->quarantine_staged=true;
 auto out=std::shared_ptr<BnwPlotYieldRefreshPending>(new BnwPlotYieldRefreshPending(std::move(p)));
 // Stage only: Root commits the shared ledger after every candidate allocation.
 issuer->impl_->claims.emplace(out->impl_->progress.plot,out->impl_->progress.operation);issuer->impl_->next_operation=std::max(issuer->impl_->next_operation,out->impl_->progress.operation);return out;
}
}
