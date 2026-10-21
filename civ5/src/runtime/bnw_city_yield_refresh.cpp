#include "civ5/runtime/bnw_city_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_population.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_ui_adapter.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <atomic>
#include <climits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
#include <tuple>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"City yield refresh native32 operand unavailable");return static_cast<std::int32_t>(n);}
const auto& named(const auto& rows,std::string_view name){
 const typename std::remove_cvref_t<decltype(rows)>::value_type* found=nullptr;
 for(const auto& row:rows)if(row.name&&std::string_view(row.name)==name){need(!found,"City yield refresh duplicate source field");found=&row;}
 need(found,"City yield refresh required source field absent");return *found;
}
std::int32_t scalar(const auto& rows,std::string_view name){const auto& f=named(rows,name);need(f.values.size()==1,"City yield refresh source scalar shape differs");return native(f.values.front());}
bool text_equal(const char* a,const char* b){return (!a&&!b)||(a&&b&&std::string_view(a)==b);}
bool same_node(const saves::CityTailNode& a,const saves::CityTailNode& b){
 if(!text_equal(a.name,b.name)||!text_equal(a.encoding,b.encoding)||a.start_offset!=b.start_offset||
  a.end_offset!=b.end_offset||a.wire_count!=b.wire_count||a.values!=b.values||a.children.size()!=b.children.size())return false;
 for(std::size_t i=0;i<a.children.size();++i)if(!same_node(a.children[i],b.children[i]))return false;
 return true;
}
struct Binding {
 std::int32_t owner=-1,city=-1,x=0,y=0;
 std::uint64_t version=0,start=0,end=0,width=0,height=0;
 bool wrap_x=false,wrap_y=false;
 std::array<std::uint8_t,16> guid{};
 saves::CityTailNode religion;
};
Binding capture(const LiveSessionState& s,const saves::SavedSession& original,std::int32_t owner,std::int32_t city){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&
  s.world.map_projection&&s.world.map_plots&&s.world.map_plots->layout==bnw&&s.world.map_header.layout==bnw,
  "City yield refresh actual BNW map/profile absent");
 const auto* c=resolve_bnw_city_registry(s.world,s.cities,owner,city);
 need(c&&c->core.layout==bnw&&c->core.version,"City yield refresh canonical current city absent");
 const auto* source=resolve_bnw_city_registry(original.snapshot,s.cities,owner,city);
 need(source&&source->core.layout==bnw&&source->core.version==c->core.version&&
  source->core.start_offset==c->core.start_offset&&source->core.end_offset==c->core.end_offset,
  "City yield refresh immutable original city provenance differs");
 Binding b;b.owner=scalar(c->core.fields,"m_eOwner");b.city=scalar(c->core.fields,"m_iID");
 need(b.owner==owner&&b.city==city,"City yield refresh actual full city identity differs");
 b.x=scalar(c->core.fields,"m_iX");b.y=scalar(c->core.fields,"m_iY");
 const auto& owned=s.cities.players.at(owner).cities.at(city);
 const auto& map=*s.world.map_projection;
 need(owned.x==b.x&&owned.y==b.y&&b.x>=0&&b.y>=0&&static_cast<std::uint64_t>(b.x)<map.grid.width&&
  static_cast<std::uint64_t>(b.y)<map.grid.height,"City yield refresh raw/owned coordinates differ");
 need(map.grid.width==s.world.map_header.grid.width&&map.grid.height==s.world.map_header.grid.height&&
  map.wrap_x==s.world.map_header.wrap_x&&map.wrap_y==s.world.map_header.wrap_y&&
  map.cells.size()==map.grid.size()&&s.world.map_plots->plots.size()==map.cells.size(),
  "City yield refresh native map/header/projection dimensions differ");
 b.version=*c->core.version;b.start=c->core.start_offset;b.end=c->core.end_offset;
 b.width=map.grid.width;b.height=map.grid.height;b.wrap_x=map.wrap_x;b.wrap_y=map.wrap_y;b.guid=s.world.map_header.guid;
 // UpdateReligion's majority argument is evaluated before entering updateYield.
 // Preserve that actual status input; do not obtain a supplied or late majority.
 need(c->tail.layout==bnw,"City yield refresh religion source layout absent");
 b.religion=named(c->tail.blocks,"religions");return b;
}
bool same(const Binding& a,const Binding& b){return a.owner==b.owner&&a.city==b.city&&a.x==b.x&&a.y==b.y&&
 a.version==b.version&&a.start==b.start&&a.end==b.end&&a.width==b.width&&a.height==b.height&&
 a.wrap_x==b.wrap_x&&a.wrap_y==b.wrap_y&&a.guid==b.guid&&same_node(a.religion,b.religion);}
struct Ledger {
 std::atomic<bool> retired{false};
 std::atomic<std::uint64_t> attempts{0};
 std::atomic<std::uint8_t> next{0},skipped{0},returned{0};
};
struct ReturnedCall {
 std::uint8_t slot=0;std::int64_t plot=-1;std::uint64_t operation=0;
 std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> history;
};
}
struct BnwCityYieldRefreshRegistry::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};std::uint64_t next_operation=0;
 std::map<std::pair<std::int32_t,std::int32_t>,std::shared_ptr<BnwCityYieldRefreshPending>> cities;
};
struct BnwCityYieldRefreshPending::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::weak_ptr<BnwCityYieldRefreshRegistry> registry;
 std::weak_ptr<BnwPlotYieldRefreshIssuer> issuer;
 std::weak_ptr<BnwCityEntryPending> entry;
 std::weak_ptr<BnwCityPopulationPending> population;
 std::shared_ptr<LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 SessionCheckpointIdentity identity;std::uint64_t generation=0;
 std::optional<BnwNormalPlotYieldRules> rules;Binding expected;
 std::array<std::optional<std::int64_t>,37> positions{};
 std::shared_ptr<Ledger> ledger=std::make_shared<Ledger>();
 BnwCityYieldRefreshProgress progress;
 std::shared_ptr<BnwPlotYieldRefreshPending> child;
 std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> quarantined_child_history;
 std::array<std::optional<ReturnedCall>,37> returned;
};
struct BnwCityYieldRefreshCheckpoint::Impl {
 struct City {BnwCityYieldRefreshProgress progress;std::shared_ptr<Ledger> ledger;
  std::array<std::optional<ReturnedCall>,37> returned;
  std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> current_child_history;};
 std::uint64_t next_operation=0;std::map<std::pair<std::int32_t,std::int32_t>,City> cities;
};
struct BnwCityYieldRefresh::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<BnwCityYieldRefreshPending> pending;
};
BnwCityYieldRefreshPending::BnwCityYieldRefreshPending(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityYieldRefreshPending::~BnwCityYieldRefreshPending()=default;
BnwCityYieldRefresh::BnwCityYieldRefresh(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityYieldRefresh::~BnwCityYieldRefresh()=default;
BnwCityYieldRefreshRegistry::BnwCityYieldRefreshRegistry():impl_(std::make_shared<Impl>()){}
BnwCityYieldRefreshCheckpoint::BnwCityYieldRefreshCheckpoint(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityYieldRefreshPending::require_current()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"City yield refresh foreign source thread");
 need(!p.ledger->retired.load()&&p.progress.phase!=BnwCityYieldRefreshPhase::quarantined&&
  p.progress.phase!=BnwCityYieldRefreshPhase::terminal_failure,"City yield refresh retired/quarantined; no replay");
 const auto registry=p.registry.lock();need(registry&&registry->impl_->thread==p.thread&&!registry->impl_->retired.load(),
  "City yield refresh actual registration retired");
 const auto registered=registry->impl_->cities.find({p.progress.owner,p.progress.city});
 need(registered!=registry->impl_->cities.end()&&registered->second.get()==this&&p.progress.operation,
  "City yield refresh exact registered parent missing");
 need(p.rules.has_value(),"City yield refresh current immutable rules absent");
 p.rules->require_current(p.identity.rules_sha256); // BEFORE current world/source reads.
 const auto issuer=p.issuer.lock();need(issuer&&p.state&&p.source,"City yield refresh source allocation/control retired");
 (void)issuer->require_game_debug(*p.state,*p.source,p.identity);
 need(same(p.expected,capture(*p.state,*p.source,p.progress.owner,p.progress.city)),
  "City yield refresh city/map/religion status changed outside actual call");
 p.rules->require_current(p.identity.rules_sha256);(void)issuer->require_game_debug(*p.state,*p.source,p.identity);
}
BnwCityYieldRefreshProgress BnwCityYieldRefreshPending::progress()const{
 need(impl_->thread==std::this_thread::get_id(),"City yield refresh progress foreign thread");
 auto out=impl_->progress;
 if(impl_->ledger->retired.load()){if(out.phase!=BnwCityYieldRefreshPhase::terminal_failure)out.phase=BnwCityYieldRefreshPhase::quarantined;}
 else require_current();return out;
}
std::shared_ptr<BnwCityYieldRefreshPending> BnwCityYieldRefreshPending::prepare(LiveSession& session,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,
 const std::shared_ptr<BnwCityYieldRefreshRegistry>& registry,std::int64_t owner,std::int64_t handle,
 const std::shared_ptr<BnwCityEntryPending>& entry,const std::shared_ptr<BnwCityPopulationPending>& population){
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id(),"City yield refresh factory foreign thread");
 need(!entry||!population,"City refresh cannot have two issuing parents");
 if(population)population->require_current();
 need(host&& &host->session()==&session&&session.bnw_game_debug_owner_current(),"City yield refresh needs SAME genuine Host/session");
 (void)session.bnw_game_debug_mode();
 const auto id=native(owner);const auto h=native(handle);
 auto rules=BnwNormalPlotYieldRules::load(host->database(),host->identity().rules_sha256);
 rules.require_current(host->identity().rules_sha256);
 need(registry&&registry->impl_->thread==session.plot_yield_refresh_thread_&&!registry->impl_->retired.load(),
  "City yield refresh actual registry retired");
 need(host->identity().payload_sha256==session.debug_->identity.payload_sha256&&
  host->identity().rules_sha256==session.debug_->identity.rules_sha256,"City yield refresh Host/import identity differs");
 const auto* current=resolve_bnw_city_registry(session.state_->world,session.state_->cities,id,h);
 need(current,"City yield refresh actual FLTA city not found");const auto full=scalar(current->core.fields,"m_iID");
 if(entry){entry->require_current();need(session.city_entry_pending_.contains(entry->source_key())&&
  session.city_entry_pending_.at(entry->source_key())==entry,"City yield refresh Entry caller is not registered");
  // The actual private Entry caller, not this component factory, proves its
  // UpdateReligion source phase and seals GetReligiousMajority beforehand.
  const auto key=entry->source_key();need(std::get<1>(key)==id&&std::get<2>(key)==full,
   "City yield refresh Entry caller city differs");}
 auto p=std::make_unique<Impl>();p->registry=registry;p->state=session.state_;p->source=session.source_;
 p->issuer=session.plot_yield_refresh_issuer_;p->entry=entry;p->population=population;p->rules=std::move(rules);
 p->identity=host->identity();p->generation=session.state_generation_;
 p->expected=capture(*p->state,*p->source,id,full);
 const MapGeometry geometry{p->state->world.map_projection->grid,p->expected.wrap_x,p->expected.wrap_y,bnw};
 p->positions=detail::bnw_citizen_plot_positions(geometry,p->expected.x,p->expected.y);
 need(registry->impl_->next_operation<UINT64_MAX&&!registry->impl_->cities.contains({id,full}),
  "City yield refresh source component already owns this city/generation");
 for(const auto& [key,other]:registry->impl_->cities){(void)key;need(other&&
  other->impl_->progress.phase==BnwCityYieldRefreshPhase::source_loop_returned&&!other->impl_->ledger->retired.load(),
  "City yield refresh previous native city call has not returned");}
 p->progress.owner=id;p->progress.city=full;p->progress.operation=registry->impl_->next_operation+1;
 auto result=std::shared_ptr<BnwCityYieldRefreshPending>(new BnwCityYieldRefreshPending(std::move(p)));
 registry->impl_->cities.emplace(std::pair{id,full},result);++registry->impl_->next_operation;
 result->require_current();return result;
}
std::int64_t BnwCityYieldRefreshPending::require_unissued_slot()const{
 require_current();const auto& p=*impl_;need(!p.child&&p.progress.phase==BnwCityYieldRefreshPhase::native_slot&&
  p.progress.next_slot<37&&p.positions[p.progress.next_slot],"City yield refresh has no actual non-null unissued slot");
 return *p.positions[p.progress.next_slot];
}
std::pair<std::uint64_t,std::uint8_t> BnwCityYieldRefreshPending::private_call_identity()const{
 (void)require_unissued_slot();return {impl_->progress.operation,impl_->progress.next_slot};
}
void BnwCityYieldRefreshPending::require_child_source(const BnwPlotYieldRefreshIssuer& issuer,
 const LiveSessionState& state,const saves::SavedSession& source,const SessionCheckpointIdentity& identity)const{
 (void)require_unissued_slot();need(impl_->issuer.lock().get()==&issuer&&impl_->state.get()==&state&&
  impl_->source.get()==&source&&impl_->identity.payload_sha256==identity.payload_sha256&&
  impl_->identity.rules_sha256==identity.rules_sha256,"City yield child is not SAME actual parent source/control");
}
void BnwCityYieldRefreshPending::bind_child(const std::shared_ptr<BnwPlotYieldRefreshPending>& child){
 const auto plot=require_unissued_slot();need(bool(child),"City yield refresh child missing");
 const auto actual=child->issued_identity();need(actual.first==plot&&actual.second,"City yield refresh child actual plot/operation differs");
 impl_->child=child;impl_->progress.current_child_operation=actual.second;
 impl_->progress.phase=BnwCityYieldRefreshPhase::plot_call;
}
void BnwCityYieldRefreshPending::require_exact_child(const BnwPlotYieldRefreshPending& child)const{
 require_current();const auto& p=*impl_;need(p.child.get()==&child&&p.progress.phase==BnwCityYieldRefreshPhase::plot_call&&
  p.progress.next_slot<37&&p.progress.current_child_operation,"City yield refresh exact current child/slot absent");
 const auto actual=child.progress();need(actual.plot==*p.positions[p.progress.next_slot]&&
  actual.operation==*p.progress.current_child_operation,"City yield refresh current child operation changed");
}
void BnwCityYieldRefreshPending::accept_child_return(const BnwPlotYieldRefreshPending& child){
 require_exact_child(child);auto& p=*impl_;const auto actual=child.progress();
 need(actual.phase==BnwPlotYieldRefreshPhase::complete||actual.phase==BnwPlotYieldRefreshPhase::skipped_no_area,
  "City yield refresh cannot skip required child calculation/store/UI/symbol effects");
 // Capture immutable child history WHILE current, before a later call changes
 // city terrain. Historical calls are never recaptured against that later world.
 auto history=child.checkpoint();need(p.progress.attempts<UINT64_MAX,"City yield refresh history exhausted");
 require_exact_child(child);++p.progress.attempts;p.ledger->attempts.store(p.progress.attempts);
 p.returned[p.progress.next_slot]=ReturnedCall{p.progress.next_slot,actual.plot,actual.operation,std::move(history)};
 ++p.progress.returned_calls;++p.progress.next_slot;p.ledger->returned.store(p.progress.returned_calls);
 p.ledger->next.store(p.progress.next_slot);p.child.reset();p.progress.current_child_operation.reset();
 p.progress.phase=p.progress.next_slot==37?BnwCityYieldRefreshPhase::source_loop_returned:BnwCityYieldRefreshPhase::native_slot;
}
void BnwCityYieldRefreshPending::advance(LiveSession& session,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host){
 require_current();auto& p=*impl_;
 need(host&& &host->session()==&session&&session.state_==p.state&&session.source_==p.source&&
  session.state_generation_==p.generation&&session.city_yield_refresh_registry_==p.registry.lock(),
  "City yield refresh continuation requires SAME current Host/session/generation");
 if(p.progress.phase==BnwCityYieldRefreshPhase::source_loop_returned)return;
 try{
  if(p.child){
   require_exact_child(*session.current_bnw_city_plot_child(*this));
   auto child=session.current_bnw_city_plot_child(*this);const auto actual=child->progress();
   if(actual.phase!=BnwPlotYieldRefreshPhase::complete&&actual.phase!=BnwPlotYieldRefreshPhase::skipped_no_area)
    session.continue_bnw_city_plot_yield_child(*this,host);
   require_exact_child(*session.current_bnw_city_plot_child(*this));
   const auto after=child->progress();if(after.phase==BnwPlotYieldRefreshPhase::complete||after.phase==BnwPlotYieldRefreshPhase::skipped_no_area)
    accept_child_return(*child);
   return;
  }
  need(p.progress.next_slot<37&&p.progress.attempts<UINT64_MAX,"City yield refresh native cursor/history exhausted");
  if(!p.positions[p.progress.next_slot]){
   // Genuine plotCity NULL; no plot cache, area, normal formula or UI query.
   ++p.progress.attempts;p.ledger->attempts.store(p.progress.attempts);++p.progress.null_skips;++p.progress.next_slot;
   p.ledger->skipped.store(p.progress.null_skips);p.ledger->next.store(p.progress.next_slot);
   if(p.progress.next_slot==37)p.progress.phase=BnwCityYieldRefreshPhase::source_loop_returned;return;
  }
  ++p.progress.attempts;p.ledger->attempts.store(p.progress.attempts);
  session.begin_bnw_city_plot_yield_child(*this); // publishes/binds BEFORE advance_prefix.
  require_exact_child(*session.current_bnw_city_plot_child(*this));
  const auto child=session.current_bnw_city_plot_child(*this);const auto actual=child->progress();
  if(actual.phase==BnwPlotYieldRefreshPhase::complete||actual.phase==BnwPlotYieldRefreshPhase::skipped_no_area)accept_child_return(*child);
 }catch(...){p.progress.phase=BnwCityYieldRefreshPhase::terminal_failure;p.ledger->retired.store(true);throw;}
}
void BnwCityYieldRefreshPending::require_entry_child_prefix(const BnwCityEntryPending& entry,const BnwPlotYieldRefreshPending& child)const{
 require_exact_child(child);const auto actual=impl_->entry.lock();
 need(actual&&actual.get()==&entry,"City terrain prefix has no SAME actual Entry caller");
 // Do NOT call Entry.require_current here: its old terrain/core seals are
 // precisely the beforeimages the trusted Entry join must reconcile next.
 const auto key=entry.source_key();
 need(std::get<1>(key)==impl_->progress.owner&&std::get<2>(key)==impl_->progress.city&&
  std::get<3>(key)==impl_->expected.start&&std::get<4>(key)==impl_->expected.end,
  "City terrain prefix Entry source identity differs");
}
void BnwCityYieldRefreshPending::require_completed_for_entry(const BnwCityEntryPending& entry)const{
 require_current();const auto actual=impl_->entry.lock();need(actual&&actual.get()==&entry,
  "Standalone city yield component cannot acknowledge an Entry predecessor");entry.require_current();
 need(impl_->progress.phase==BnwCityYieldRefreshPhase::source_loop_returned&&impl_->progress.next_slot==37&&
  !impl_->child&&impl_->progress.null_skips+impl_->progress.returned_calls==37,
  "City yield refresh actual native37 loop has not returned");
}
void BnwCityYieldRefreshPending::require_population_child_prefix(const BnwCityPopulationPending& population,
 const BnwPlotYieldRefreshPending& child)const{
 require_exact_child(child);const auto actual=impl_->population.lock();
 need(actual&&actual.get()==&population,"City terrain prefix lacks SAME actual population caller");
 population.require_city_binding(*this);
}
void BnwCityYieldRefreshPending::require_completed_for_population(const BnwCityPopulationPending& population)const{
 require_current();const auto actual=impl_->population.lock();need(actual&&actual.get()==&population,
  "Standalone city refresh cannot acknowledge population predecessor");population.require_city_binding(*this);population.require_current();
 need(impl_->progress.phase==BnwCityYieldRefreshPhase::source_loop_returned&&impl_->progress.next_slot==37&&
  !impl_->child&&impl_->progress.null_skips+impl_->progress.returned_calls==37,
  "Population religion actual native37 has not returned");
}
void BnwCityYieldRefreshPending::require_population_registration(const BnwCityPopulationPending& population,const LiveSession& session)const{
 require_current();need(impl_->population.lock().get()==&population&&session.city_yield_refresh_registry_==impl_->registry.lock()&&
  session.state_==impl_->state&&session.source_==impl_->source&&session.state_generation_==impl_->generation,
  "Population city source registry/allocation/generation replaced");
}
void BnwCityYieldRefreshPending::invalidate()noexcept{impl_->ledger->retired.store(true);}
void BnwCityYieldRefreshRegistry::retire()noexcept{
 impl_->retired.store(true);
 if(impl_->thread!=std::this_thread::get_id())return; // no foreign map/Node borrow
 for(const auto& [key,p]:impl_->cities){(void)key;if(p)p->invalidate();}
}
bool BnwCityYieldRefreshRegistry::has_history()const{
 need(impl_->thread==std::this_thread::get_id(),"City yield history foreign thread");return !impl_->cities.empty();
}
std::shared_ptr<const BnwCityYieldRefreshCheckpoint> BnwCityYieldRefreshRegistry::checkpoint()const{
 need(impl_->thread==std::this_thread::get_id(),"City yield checkpoint foreign thread");
 auto cp=std::make_shared<BnwCityYieldRefreshCheckpoint::Impl>();cp->next_operation=impl_->next_operation;
 for(const auto& [key,p]:impl_->cities){need(bool(p),"City yield checkpoint unissued parent");
  if(!impl_->retired.load()&&!p->impl_->ledger->retired.load()){
   p->require_current();if(p->impl_->child)p->require_exact_child(*p->impl_->child);
  }
  const auto child_history=p->impl_->child?p->impl_->child->checkpoint_history():p->impl_->quarantined_child_history;
  cp->cities.emplace(key,BnwCityYieldRefreshCheckpoint::Impl::City{p->impl_->progress,p->impl_->ledger,p->impl_->returned,child_history});}
 return std::shared_ptr<const BnwCityYieldRefreshCheckpoint>(new BnwCityYieldRefreshCheckpoint(std::move(cp)));
}
std::shared_ptr<BnwCityYieldRefreshRegistry> BnwCityYieldRefreshRegistry::restore_quarantined(
 const std::shared_ptr<const BnwCityYieldRefreshCheckpoint>& cp,const std::shared_ptr<BnwCityYieldRefreshRegistry>& current){
 // Stage metadata ONLY. No old Game/rules/DB/VM/world/query callback.
 auto out=std::shared_ptr<BnwCityYieldRefreshRegistry>(new BnwCityYieldRefreshRegistry());
 auto add=[&](const auto& key,const auto& history){
  auto p=std::make_unique<BnwCityYieldRefreshPending::Impl>();p->progress=history.progress;p->ledger=history.ledger;p->returned=history.returned;
  p->quarantined_child_history=history.current_child_history;
  need(bool(p->ledger),"City yield checkpoint ledger absent");p->progress.attempts=std::max(p->progress.attempts,p->ledger->attempts.load());
  p->progress.next_slot=std::max(p->progress.next_slot,p->ledger->next.load());
  p->progress.null_skips=std::max(p->progress.null_skips,p->ledger->skipped.load());
  p->progress.returned_calls=std::max(p->progress.returned_calls,p->ledger->returned.load());
  p->progress.phase=BnwCityYieldRefreshPhase::quarantined;p->registry=out;
  out->impl_->cities.emplace(key,std::shared_ptr<BnwCityYieldRefreshPending>(new BnwCityYieldRefreshPending(std::move(p))));
 };
 if(cp){need(bool(cp->impl_),"City yield checkpoint metadata absent");out->impl_->next_operation=cp->impl_->next_operation;for(const auto& [key,h]:cp->impl_->cities)add(key,h);}
 if(current){need(current->impl_->thread==std::this_thread::get_id(),"City yield restore foreign thread");
  out->impl_->next_operation=std::max(out->impl_->next_operation,current->impl_->next_operation);
  for(const auto& [key,p]:current->impl_->cities){need(bool(p),"City yield current history absent");
   const auto current_history=p->impl_->child?p->impl_->child->checkpoint_history():p->impl_->quarantined_child_history;
   if(!out->impl_->cities.contains(key))add(key,BnwCityYieldRefreshCheckpoint::Impl::City{p->impl_->progress,p->impl_->ledger,p->impl_->returned,current_history});
   else{
    auto& staged=*out->impl_->cities.at(key)->impl_;
    need(staged.progress.operation==p->impl_->progress.operation&&staged.ledger==p->impl_->ledger,
     "City yield checkpoint operation/once ledger differs from actual history");
    for(std::size_t i=0;i<37;++i)if(p->impl_->returned[i]){
     if(staged.returned[i])need(staged.returned[i]->operation==p->impl_->returned[i]->operation,
      "City yield checkpoint native slot history differs");
     staged.returned[i]=p->impl_->returned[i];
    }
    staged.quarantined_child_history=current_history;
    staged.progress.attempts=std::max(staged.progress.attempts,p->impl_->progress.attempts);
    staged.progress.next_slot=std::max(staged.progress.next_slot,p->impl_->progress.next_slot);
    staged.progress.returned_calls=std::max(staged.progress.returned_calls,p->impl_->progress.returned_calls);
    staged.progress.null_skips=std::max(staged.progress.null_skips,p->impl_->progress.null_skips);
    staged.progress.current_child_operation=p->impl_->progress.current_child_operation;
   }
  }
 }
 return out;
}
void BnwCityYieldRefreshRegistry::commit_quarantine()noexcept{retire();}
std::shared_ptr<BnwCityYieldRefresh> BnwCityYieldRefresh::from_selected_host(
 std::shared_ptr<ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){
 need(bool(host),"City yield refresh needs an actual selected Host");auto p=std::make_unique<Impl>();p->host=std::move(host);
 p->pending=p->host->session().begin_bnw_city_yield_refresh(p->host,owner,city,{});
 return std::shared_ptr<BnwCityYieldRefresh>(new BnwCityYieldRefresh(std::move(p)));
}
void BnwCityYieldRefresh::advance(){need(impl_->thread==std::this_thread::get_id(),"City yield handle foreign thread");
 impl_->pending->advance(impl_->host->session(),impl_->host);}
BnwCityYieldRefreshProgress BnwCityYieldRefresh::progress()const{
 need(impl_->thread==std::this_thread::get_id(),"City yield handle progress foreign thread");return impl_->pending->progress();}
// Root private declarations are supplied separately; definitions here own the
// actual registration/child loop. The Plot owner supplies prepare_normal_child.
std::shared_ptr<BnwCityYieldRefreshPending> LiveSession::begin_bnw_city_yield_refresh(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,std::int64_t owner,std::int64_t city,
 const std::shared_ptr<BnwCityEntryPending>& entry){
 need(plot_yield_refresh_thread_==std::this_thread::get_id(),"City yield registration foreign thread");
 need(host&& &host->session()==this&&bnw_game_debug_owner_current(),"City yield registration needs SAME actual Host");
 (void)bnw_game_debug_mode();
 if(!city_yield_refresh_registry_){need(plot_yield_refresh_pending_.empty(),"City yield component cannot reuse standalone plot history");
  city_yield_refresh_registry_=std::shared_ptr<BnwCityYieldRefreshRegistry>(new BnwCityYieldRefreshRegistry());}
 if(!plot_yield_refresh_issuer_)plot_yield_refresh_issuer_=std::shared_ptr<BnwPlotYieldRefreshIssuer>(new BnwPlotYieldRefreshIssuer(state_,state_generation_));
 plot_yield_refresh_issuer_->bind_game_debug(debug_,source_);
 return BnwCityYieldRefreshPending::prepare(*this,host,city_yield_refresh_registry_,owner,city,entry);
}
void LiveSession::begin_bnw_city_plot_yield_child(BnwCityYieldRefreshPending& parent){
 const auto plot=parent.require_unissued_slot();
 need(parent.impl_->registry.lock()==city_yield_refresh_registry_&&parent.impl_->state==state_&&
  parent.impl_->source==source_&&parent.impl_->generation==state_generation_,"City plot child actual registered parent differs");
 std::map<std::int64_t,std::shared_ptr<BnwPlotYieldRefreshPending>> staging;
 const auto existing=plot_yield_refresh_pending_.find(plot);
 if(existing==plot_yield_refresh_pending_.end())staging.emplace(plot,nullptr);
 auto node=staging.empty()?decltype(staging)::node_type{}:staging.extract(staging.begin());
 auto child=BnwPlotYieldRefreshPending::prepare_normal_child(plot_yield_refresh_issuer_,state_,source_,
  parent.impl_->identity,*parent.impl_->rules,parent);
 // All map allocations precede issued pointer publication. Never recapture
 // archived old calls; their immutable checkpoints already retain history.
 for(const auto& [old_plot,old]:plot_yield_refresh_pending_){(void)old_plot;if(old)old->invalidate();}
 if(existing!=plot_yield_refresh_pending_.end())existing->second=child;
 else{node.mapped()=child;plot_yield_refresh_pending_.insert(std::move(node));}
 parent.bind_child(child); // actual pointer/op retained BEFORE first store.
 if(const auto population=parent.impl_->population.lock())population->bind_city_plot(parent,child);
 child->advance_prefix();
 if(const auto population=parent.impl_->population.lock())population->accept_city_plot(parent,*child);
 if(const auto entry=parent.impl_->entry.lock()){
  need(city_entry_pending_.contains(entry->source_key())&&city_entry_pending_.at(entry->source_key())==entry,
   "City plot prefix actual Entry registration replaced");
  entry->accept_city_plot_child_prefix(parent,*child); // genuine definition supplied by Entry owner
  entry->require_current();
 }
}
std::shared_ptr<BnwPlotYieldRefreshPending> LiveSession::current_bnw_city_plot_child(const BnwCityYieldRefreshPending& parent)const{
 parent.require_current();need(parent.impl_->registry.lock()==city_yield_refresh_registry_&&parent.impl_->child,
  "City plot child actual parent/slot absent");
 const auto found=plot_yield_refresh_pending_.find(*parent.impl_->positions[parent.impl_->progress.next_slot]);
 need(found!=plot_yield_refresh_pending_.end()&&found->second==parent.impl_->child,"City plot latest lookup is not bound actual child");
 parent.require_exact_child(*found->second);return found->second;
}
void LiveSession::continue_bnw_city_plot_yield_child(BnwCityYieldRefreshPending& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host){
 const auto child=current_bnw_city_plot_child(parent);parent.require_exact_child(*child);
 auto adapter=ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(host);
 (void)adapter->continue_plot(child->progress().plot);parent.require_exact_child(*current_bnw_city_plot_child(parent));
 if(const auto population=parent.impl_->population.lock())population->accept_city_plot(parent,*child);
 if(const auto entry=parent.impl_->entry.lock()){
  need(city_entry_pending_.contains(entry->source_key())&&city_entry_pending_.at(entry->source_key())==entry,
   "City plot continuation actual Entry registration replaced");
  entry->accept_city_plot_child_prefix(parent,*child);
  entry->require_current();
 }
}
}
