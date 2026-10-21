#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_food_population_checkpoint_capture.hpp"
#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/runtime/bnw_food_growth_import_admission.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/bnw_city_food_scalars.hpp"
#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_city_population.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_ui_adapter.hpp"
#include <tuple>
#include "civ5/runtime/bnw_city_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include "civ5/runtime/bnw_player_happiness_prefix_owner.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include <algorithm>
#include <climits>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool ok,const char* why){if(!ok)throw BnwCityFoodGrowthUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Growth reached native signed32 intermediate overflow");return static_cast<std::int32_t>(n);}
template<class C>auto& field(C& c,std::string_view name){
 using F=std::conditional_t<std::is_const_v<C>,const saves::CityCoreField,saves::CityCoreField>;F* found=nullptr;
 for(auto& f:c.core.fields)if(f.name&&std::string_view(f.name)==name){need(!found,"Growth duplicate core field");found=&f;}
 need(found&&found->encoding==saves::CityCoreEncoding::i32&&found->values.size()==1&&!found->wire_count&&found->bytes.empty()&&found->hashes.empty()&&found->row_keys.empty()&&found->row_counts.empty(),"Growth raw scalar shape unavailable");
 (void)native(found->values.front());return *found;
}
std::int32_t scalar(const saves::SavedCityObservation& c,std::string_view name){return native(field(c,name).values.front());}
saves::SavedCityObservation& city(saves::SavedSessionSnapshot& world,std::int32_t owner,std::int32_t id){
 need(world.all_players&&owner>=0&&owner<64&&world.all_players->players.size()==64,"Growth current roster missing");
 auto& player=world.all_players->players.at(static_cast<std::size_t>(owner));need(player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner,"Growth player allocation differs");
 saves::SavedCityObservation* out=nullptr;
 for(auto& c:player.cities.cities)if(scalar(c,"m_iID")==id){need(!out,"Growth duplicate full city ID");out=&c;}
 need(out&&scalar(*out,"m_eOwner")==owner,"Growth current full city identity absent");return *out;
}
bool section_same(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
template<class Rows>auto& named(Rows& rows,std::string_view name){
 using T=std::remove_reference_t<decltype(rows.front())>;T* out=nullptr;
 for(auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Growth duplicate reached field");out=&r;}
 need(out,"Growth reached field absent");return *out;
}
bool forced(const saves::SavedCityObservation& c){
 const auto& v=named(named(c.management.blocks,"citizens").fields,"m_bForceAvoidGrowth");
 need(v.value==0||v.value==1,"Growth forced-avoid native bool unavailable");return v.value!=0;
}
}
std::int32_t detail::bnw_growth_add_food(std::int32_t before,std::int32_t diff){return native(std::int64_t(before)+diff);}
std::int32_t detail::bnw_growth_add_kept(std::int32_t before,std::int32_t diff){return native(std::int64_t(before)+diff/100);}
std::int32_t detail::bnw_growth_clamp_kept(std::int32_t kept,std::int32_t threshold,std::int32_t percent){
 const auto upper=native(std::int64_t(threshold)*percent)/100;
 // CvGameCoreUtils.h19 Release range tests low FIRST, then high. Preserve
 // that branch order even for a negative upper bound (debug assert only).
 if(kept<0)return 0;if(kept>upper)return upper;return kept;
}
std::int32_t detail::bnw_growth_food_change(std::int32_t before,std::int32_t whole){
 const auto delta=native(std::int64_t(100)*whole);return native(std::int64_t(before)+delta);
}
struct BnwCityFoodGrowthRegistry::Impl {
 const std::thread::id thread=std::this_thread::get_id();bool retired=false;
 std::map<std::pair<std::int32_t,std::int32_t>,std::shared_ptr<BnwCityFoodGrowthPending>> pending;
};
struct BnwCityFoodGrowthPending::Impl {
 const std::thread::id thread=std::this_thread::get_id();bool retired=false,attempted=false;
 bool store_pending=false,capture_attempted=false,capture_failed=false;
 bool restored_growth_history=false;
 std::vector<std::int32_t> growth_threshold_reads;
 std::vector<BnwFoodPopulationStoreRecord> store_history;
 std::weak_ptr<const BnwFoodPopulationCheckpointCapture> capture;
 std::uint64_t first_store_revision=0;
 LiveSession* session=nullptr;std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<LiveSessionState> state;std::shared_ptr<const saves::SavedSession> source;
 std::shared_ptr<LiveSession::BnwGameDebugReadStorage> game;
 std::weak_ptr<BnwCityFoodGrowthRegistry> registry;
 SessionCheckpointIdentity identity;std::uint64_t generation=0,ui_revision=0;
 std::function<bool()> rules_current;saves::CivisSaveSection core_seal;
 BnwCityFoodGrowthProgress progress;
 std::optional<BnwCityFoodScalarMirror> mirror;
 std::shared_ptr<BnwCityPopulationPending> population;
 std::shared_ptr<BnwPlotYieldRefreshPending> population_plot;
 std::shared_ptr<BnwCityYieldRefreshPending> religion_city_child;
 std::vector<std::shared_ptr<BnwCityYieldRefreshRegistry>> archived_city_registries;
 std::optional<std::int32_t> religion_majority;
 std::unique_ptr<BnwCityReligionYieldOwner> religion_owner;
 std::unique_ptr<BnwPlayerHappinessPrefixOwner> player_happiness_owner;
 std::optional<std::uint8_t> religion_ui_yield;
 std::vector<std::shared_ptr<BnwPlotYieldRefreshPending>> archived_population_plots;
 std::array<std::int64_t,6> accepted_plot_cache{},accepted_plot_terrain{};
 std::uint8_t accepted_plot_writes=0,accepted_terrain_writes=0,accepted_plot_next_yield=0;
 std::uint64_t accepted_plot_operation=0,accepted_plot_ledger=0;
 BnwPlotYieldRefreshPhase accepted_plot_phase=BnwPlotYieldRefreshPhase::prepared;
 std::vector<BnwCityDirtyEvent> accepted_city_events;
 std::vector<BnwPlotGameplayEvent> accepted_plot_events;
 std::uint64_t accepted_city_occurrence=0,accepted_plot_occurrence=0;
 std::optional<saves::CivisSaveSection> population_production,population_management,population_players;
};
BnwCityFoodGrowthRegistry::BnwCityFoodGrowthRegistry():impl_(std::make_unique<Impl>()){}
BnwCityFoodGrowthRegistry::~BnwCityFoodGrowthRegistry(){retire();}
void BnwCityFoodGrowthRegistry::retire()noexcept{impl_->retired=true;for(const auto& [key,p]:impl_->pending){(void)key;p->retire();}}
BnwCityFoodGrowthPending::BnwCityFoodGrowthPending(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityFoodGrowthPending::~BnwCityFoodGrowthPending()=default;
void BnwCityFoodGrowthPending::retire()noexcept{
 impl_->retired=true;
 if(impl_->religion_city_child)impl_->religion_city_child->invalidate();
 if(impl_->religion_owner)impl_->religion_owner->retire();
 if(impl_->player_happiness_owner)impl_->player_happiness_owner->retire();
}
BnwCityFoodGrowthProgress BnwCityFoodGrowthPending::progress()const{
 need(impl_->thread==std::this_thread::get_id(),"Growth diagnostic foreign thread");auto out=impl_->progress;
 if(impl_->retired)out.phase=BnwCityFoodGrowthPhase::retired;return out;
}
void BnwCityFoodGrowthPending::require_source_registration()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired,"Growth owner thread/retirement differs");
 const auto registry=p.registry.lock();need(registry&&!registry->impl_->retired&&registry->impl_->thread==p.thread,"Growth registered journal retired");
 const auto host=p.host.lock();need(host&&&host->session()==p.session,"Growth genuine Host/session lifetime differs");
 const auto& s=*p.session;
 const auto same=[&]{return s.state_==p.state&&s.source_==p.source&&s.state_generation_==p.generation&&s.debug_==p.game&&s.bnw_game_debug_owner_current()&&s.food_growth_registry_==registry;};
 need(same()&&p.rules_current&&p.rules_current()&&same(),"Growth SAME rules/Game/world/generation retired");
 need(host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256&&registry->impl_->pending.at({p.progress.owner,p.progress.city}).get()==this,"Growth exact private registration/identity differs");
 }
void BnwCityFoodGrowthPending::require_current()const{
 require_source_registration();const auto& p=*impl_;
 need(p.state->ui_source_revision==p.ui_revision,"Growth current source revision changed outside owned return");
 need(section_same(make_bnw_city_core_checkpoint(p.state->world,p.state->world,p.identity,true),p.core_seal),"Growth core changed outside exact owned scalar lineage");
 const auto* raw=resolve_bnw_current_city(*p.state,p.progress.owner,p.progress.city);need(raw,"Growth current city disappeared");
 if(p.population_production)need(section_same(make_bnw_city_production_checkpoint(p.state->world,p.state->world,p.identity,true),*p.population_production),"Population production prefix changed");
 if(p.population_management)need(section_same(make_bnw_city_management_checkpoint(p.state->world,p.state->world,p.identity,true),*p.population_management),"Population citizens prefix changed");
 if(p.population_players)need(section_same(make_bnw_player_core_checkpoint(p.state->world,p.state->world,p.identity,true),*p.population_players),"Population player prefix changed");
 if(p.population){p.population->check_owned_prefix();if(p.population->plot_attempted_&&p.population->progress_.phase==BnwCityPopulationPhase::failed)throw BnwCityPopulationUnavailable("Population failed plot call is quarantined; no replay");}
 if(p.mirror){need(scalar(*raw,"m_iFood")==p.mirror->food_times100&&scalar(*raw,"m_iFoodKept")==p.mirror->food_kept&&scalar(*raw,"m_iPopulation")==p.mirror->population,"Growth canonical raw/owned food mirror stale");need(p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).population==p.mirror->population,"Growth canonical population projection stale");}
}
std::shared_ptr<BnwCityFoodGrowthPending> BnwCityFoodGrowthPending::create(LiveSession& session,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,std::int64_t owner,std::int64_t id){
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id()&&host&&&host->session()==&session,"Growth SAME actual Host/session/thread required");
 (void)session.bnw_game_debug_mode();const auto* raw=resolve_bnw_current_city(*session.state_,native(owner),native(id));need(raw,"Growth actual full/index city absent");
 const auto key=std::pair{native(owner),scalar(*raw,"m_iID")};
 auto registry=session.food_growth_registry_;
 need(!registry||(!registry->impl_->retired&&!registry->impl_->pending.contains(key)),"Growth attempted call/once quarantine cannot replay");
 auto p=std::make_unique<Impl>();p->session=&session;p->host=host;p->state=session.state_;p->source=session.source_;p->game=session.debug_;p->generation=session.state_generation_;p->ui_revision=p->state->ui_source_revision;p->identity=session.debug_->identity;
 p->progress.owner=key.first;p->progress.city=key.second;p->progress.ui_revision=p->ui_revision;p->first_store_revision=p->ui_revision;
 p->rules_current=host->database().immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Growth actual immutable rules unavailable");
 p->core_seal=make_bnw_city_core_checkpoint(p->state->world,p->state->world,p->identity,true);
 if(!registry){registry=std::shared_ptr<BnwCityFoodGrowthRegistry>(new BnwCityFoodGrowthRegistry);session.food_growth_registry_=registry;}
 p->registry=registry;auto result=std::shared_ptr<BnwCityFoodGrowthPending>(new BnwCityFoodGrowthPending(std::move(p)));
 registry->impl_->pending.emplace(key,result);result->require_current();return result;
}
void BnwCityFoodGrowthPending::store(const char* name,std::int32_t value,bool conditional,BnwCityFoodGrowthPhase phase){
 require_current();auto& p=*impl_;auto& raw=city(p.state->world,p.progress.owner,p.progress.city);auto& f=field(raw,name);
 const bool writes=!conditional||f.values.front()!=value;
 need(!p.store_pending,"Growth interrupted native store history cannot replay");p.store_pending=true;
 BnwFoodPopulationStoreRecord event;event.kind=std::string_view(name)=="m_iFood"?BnwFoodPopulationStoreKind::food:BnwFoodPopulationStoreKind::food_kept;
 event.actor=p.progress.owner;event.full_object=p.progress.city;event.before=native(f.values.front());event.after=value;
 event.revision_before=p.ui_revision;event.revision_after=p.ui_revision;event.food_phase=phase;event.conditional=conditional;
 p.store_history.reserve(p.store_history.size()+1);p.store_history.push_back(event);const auto history_index=p.store_history.size()-1;
 if(writes){
  need(p.ui_revision<UINT64_MAX&&p.progress.native_scalar_assignments<UINT64_MAX,"Growth source revision exhausted before store");
  auto staged=p.state->world;field(city(staged,p.progress.owner,p.progress.city),name).values.front()=value;
  auto after=make_bnw_city_core_checkpoint(staged,staged,p.identity,true);require_current();
  // All potentially throwing preparation is before this native assignment.
  // Advance ONLY this reached field. No public snapshot/rebaseline authority.
  // Standalone imported-member mutation conservatively retires sibling
  // incomplete cursors. Their history is retained, never called completed.
  p.session->retire_bnw_city_entry();p.session->retire_plot_yield_refresh();p.session->retire_bnw_production_turn();
  p.state->city_strategy_runtime.clear();
  f.values.front()=value;p.core_seal.bytes.swap(after.bytes);p.core_seal.id=after.id;p.core_seal.flags=after.flags;
  ++p.state->ui_source_revision;++p.ui_revision;++p.progress.native_scalar_assignments;
 }
 if(std::string_view(name)=="m_iFood")p.mirror->food_times100=value;else p.mirror->food_kept=value;
 p.progress.food_times100=p.mirror->food_times100;p.progress.food_kept=p.mirror->food_kept;p.progress.ui_revision=p.ui_revision;p.progress.phase=phase;
 auto& earned=p.store_history[history_index];earned.stored=writes;earned.revision_after=p.ui_revision;earned.returned=true;p.store_pending=false;
}
std::int32_t BnwCityFoodGrowthPending::threshold()const{
 require_current();const auto host=impl_->host.lock();need(bool(host),"Growth threshold Host expired");
 // New real read owner AFTER our actual writes, on precisely the same current
 // world. It receives no arbitrary accepted snapshot or caller cache flag.
 const auto value=inspect_bnw_city_current_growth_threshold(host,impl_->progress.owner,impl_->progress.city).value();require_current();impl_->growth_threshold_reads.push_back(value);return value;
}
void BnwCityFoodGrowthPending::execute(){
 require_current();auto& p=*impl_;need(!p.attempted,"Growth call already attempted");p.attempted=true;
 try{
  auto& raw=city(p.state->world,p.progress.owner,p.progress.city);const auto razing=scalar(raw,"m_iRazingTurns");
  need(p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).razing==(razing>0),"Growth raw/owned IsRazing stale");
  if(razing>0){p.progress.phase=BnwCityFoodGrowthPhase::razing_returned;p.progress.returned=true;return;}
  std::int32_t diff=0;
  {
   const auto host=p.host.lock();need(bool(host),"Growth actual Host expired before foodDifference");
   const auto db=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
   const auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(db,p.identity.rules_sha256));
   const auto session=std::shared_ptr<const LiveSession>(host,&host->session());
   diff=inspect_bnw_city_current_food_difference(session,rules,p.progress.owner,p.progress.city,true).value_times100();
   // Strong read owners are local, never a Session -> Pending -> Host cycle.
  }
  require_current();p.progress.difference_times100=diff;p.progress.difference_observed=true;
  if(diff<0){
   // Exact SDK pNotifications branch BEFORE any food/population/threshold
   // getter. Imported Read owns presence; an absent pointer really skips Add.
   need(p.source->snapshot.all_players&&p.source->snapshot.all_players->players.size()==64,"Growth original notification Read roster unavailable");
   const auto& player=p.state->world.all_players->players.at(static_cast<std::size_t>(p.progress.owner));
   const auto& original=p.source->snapshot.all_players->players.at(static_cast<std::size_t>(p.progress.owner));
   const auto& ring=named(player.administration.blocks,"notifications");const auto& saved=named(original.administration.blocks,"notifications");
   const auto has=named(ring.fields,"has_notifications").value;
   need((has==0||has==1)&&has==named(saved.fields,"has_notifications").value&&p.state->notification_load_transients.current&&p.state->notification_load_transients.players.contains(p.progress.owner)==(has!=0),"Growth actual notification pointer Read owner unavailable");
   if(has){p.progress.phase=BnwCityFoodGrowthPhase::starvation_notification;return;}
  }
  p.mirror=BnwCityFoodScalarMirror{};p.mirror->owner=p.progress.owner;p.mirror->city=p.progress.city;
  p.mirror->source_core={raw.core.start_offset,raw.core.end_offset};p.mirror->population=scalar(raw,"m_iPopulation");p.mirror->razing_turns=razing;
  p.mirror->food_times100=scalar(raw,"m_iFood");p.mirror->food_kept=scalar(raw,"m_iFoodKept");
  store("m_iFood",detail::bnw_growth_add_food(p.mirror->food_times100,diff),true,BnwCityFoodGrowthPhase::food_stored);
  store("m_iFoodKept",detail::bnw_growth_add_kept(p.mirror->food_kept,diff),false,BnwCityFoodGrowthPhase::kept_stored);
  const auto limit=threshold();require_current();const auto percent=scalar(raw,"m_iMaxFoodKeptPercent");
  store("m_iFoodKept",detail::bnw_growth_clamp_kept(p.mirror->food_kept,limit,percent),false,BnwCityFoodGrowthPhase::kept_clamped);
  const auto compare=threshold();
  if(p.mirror->food_times100/100>=compare){
   require_current();
   if(forced(raw)){
    const auto value=native(std::int64_t(threshold())*100);
    store("m_iFood",value,true,BnwCityFoodGrowthPhase::forced_avoid_returned);p.progress.returned=true;return;
   }
   const auto growth=threshold();const auto minus=native(std::int64_t(growth)-p.mirror->food_kept);
   const auto kept=std::max(0,minus);need(kept!=INT32_MIN,"Growth food change native negation overflow");
   store("m_iFood",detail::bnw_growth_food_change(p.mirror->food_times100,-kept),true,BnwCityFoodGrowthPhase::population_growth);
   // Native changePopulation(1) follows this reached food store. Its area,
   // totals/citizens/religion/happiness/UI chain has no issuing owner here.
   return;
  }
  if(p.mirror->food_times100/100<0){
   const auto whole=p.mirror->food_times100/100;need(whole!=INT32_MIN,"Starvation food negation overflow");
   store("m_iFood",detail::bnw_growth_food_change(p.mirror->food_times100,-whole),true,BnwCityFoodGrowthPhase::kept_clamped);
   require_current();if(scalar(raw,"m_iPopulation")>1){p.progress.phase=BnwCityFoodGrowthPhase::population_starvation;return;}
  }
  p.progress.phase=BnwCityFoodGrowthPhase::ordinary_returned;p.progress.returned=true;
 }catch(...){p.progress.phase=BnwCityFoodGrowthPhase::failed;throw;}
}

void BnwCityFoodGrowthPending::require_population_child(const BnwCityPopulationPending& child)const{
 require_current();need(impl_->population.get()==&child&&child.parent_.lock().get()==this&&
  child.progress_.attempted,"Population actual food child registration differs");
}
std::shared_ptr<const BnwCityPopulationPending> BnwCityFoodGrowthPending::population_pending()const{
 require_current();return impl_->population;
}
std::shared_ptr<const BnwCityPopulationPending> BnwCityFoodGrowthPending::continue_population()const{
 require_current();auto& p=*impl_;
 need(p.attempted&&!p.progress.returned&&!p.population&&p.mirror&&
  (p.progress.phase==BnwCityFoodGrowthPhase::population_growth||p.progress.phase==BnwCityFoodGrowthPhase::population_starvation),
  "Population continuation requires exact unreplayed growth/starvation return site");
 const auto registry=p.registry.lock();need(bool(registry),"Population food registry absent");
 const auto actual=registry->impl_->pending.at({p.progress.owner,p.progress.city});need(actual.get()==this,"Population registered food issuer differs");
 auto child=std::shared_ptr<BnwCityPopulationPending>(new BnwCityPopulationPending(actual));
 p.population=child; // Retain before any reached effect/failure. Weak child backedge.
 child->progress_.owner=p.progress.owner;child->progress_.city=p.progress.city;
 child->progress_.change=p.progress.phase==BnwCityFoodGrowthPhase::population_growth?1:-1;
 child->progress_.attempted=true;child->progress_.ui_revision=p.ui_revision;
 try{
  // These are complete source beforeimages, not eager numerical operands.
  p.population_production=make_bnw_city_production_checkpoint(p.state->world,p.state->world,p.identity,true);
  p.population_management=make_bnw_city_management_checkpoint(p.state->world,p.state->world,p.identity,true);
  p.population_players=make_bnw_player_core_checkpoint(p.state->world,p.state->world,p.identity,true);
  require_population_child(*child);execute_population(*child);return child;
 }catch(...){child->progress_.phase=BnwCityPopulationPhase::failed;throw;}
}
void BnwCityFoodGrowthPending::execute_population(BnwCityPopulationPending& child)const{
 require_population_child(child);auto& p=*impl_;auto& out=child.progress_;
 auto& raw=city(p.state->world,p.progress.owner,p.progress.city);
 out.before=scalar(raw,"m_iPopulation");out.after=detail::bnw_population_add(out.before,out.change);
 const auto pop_change=native(std::int64_t(out.after)-out.before);
 need(pop_change==out.change,"Population change expression differs");
 // A closed internal publication helper. No caller callback or detached input
 // can reach this scope. One fixed native target per invocation, exact seals
 // prepared before stores; source snapshots and allocation generation unchanged.
 const auto publish=[&](BnwFoodPopulationStoreKind kind,std::int32_t object,std::int64_t& target,std::int32_t value,auto stage,
                         std::optional<saves::CivisSaveSection>* seal,
                         std::int64_t* projection,BnwCityPopulationPhase phase){
  require_population_child(child);
  need(p.ui_revision<UINT64_MAX&&out.assignments<UINT64_MAX,"Population revision/store count exhausted");
  need(!p.store_pending,"Population interrupted native store history cannot replay");p.store_pending=true;
  BnwFoodPopulationStoreRecord event;event.kind=kind;event.actor=p.progress.owner;event.full_object=object;event.population=true;
  event.before=native(target);event.after=value;event.revision_before=p.ui_revision;event.revision_after=p.ui_revision;event.population_phase=phase;
  event.projection=projection!=nullptr;if(projection)event.projection_before=native(*projection);
  auto staged=p.state->world;stage(staged,value);
  auto core=make_bnw_city_core_checkpoint(staged,staged,p.identity,true);
  (void)seal;
  // Existing codecs include companion-domain hashes. Refresh all four exact
  // private seals from this single-field staging clone; never accept unrelated
  // actual current state as a new baseline.
  auto production=make_bnw_city_production_checkpoint(staged,staged,p.identity,true);
  auto management=make_bnw_city_management_checkpoint(staged,staged,p.identity,true);
  auto players=make_bnw_player_core_checkpoint(staged,staged,p.identity,true);
  child.owned_writes_.reserve(child.owned_writes_.size()+(projection?2:1));
  p.store_history.reserve(p.store_history.size()+1);p.store_history.push_back(event);const auto history_index=p.store_history.size()-1;
  require_population_child(child);
  p.session->retire_bnw_city_entry();p.session->retire_plot_yield_refresh();p.session->retire_bnw_production_turn();
  p.state->city_strategy_runtime.clear();
  target=value;if(projection)*projection=value;
  child.owned_writes_.push_back({&target,value});if(projection)child.owned_writes_.push_back({projection,value});
  p.core_seal.bytes.swap(core.bytes);p.core_seal.id=core.id;p.core_seal.flags=core.flags;
  const auto swap_seal=[](auto& to,auto& from)noexcept{to->bytes.swap(from.bytes);to->id=from.id;to->flags=from.flags;};
  swap_seal(p.population_production,production);swap_seal(p.population_management,management);swap_seal(p.population_players,players);
  ++p.state->ui_source_revision;++p.ui_revision;++out.assignments;
  out.ui_revision=p.ui_revision;p.progress.ui_revision=p.ui_revision;out.phase=phase;
  auto& earned=p.store_history[history_index];earned.stored=true;earned.revision_after=p.ui_revision;earned.returned=true;p.store_pending=false;
 };
 auto& citizens=named(raw.management.blocks,"citizens");
 if(pop_change<0){
  // Native DoRemoveWorstCitizen(false,NO_SPECIALIST,newPopulation) precedes
  // the population write. Its early all-unassigned false return is real.
  const auto unassigned=native(named(citizens.fields,"m_iNumUnassignedCitizens").value);
  need(out.before>1,"Starvation lost native population>1 return site");
  if(unassigned!=out.before){
   const auto defaults=native(named(citizens.fields,"m_iNumDefaultSpecialists").value);
   if(defaults>0){
    const auto forced_defaults=native(named(citizens.fields,"m_iNumForcedDefaultSpecialists").value);
    if(defaults>forced_defaults||defaults>out.after){out.phase=BnwCityPopulationPhase::remove_default_specialist_required;return;}
   }
   // Existing Entry removal/scorer has a different genuine return site and
   // iCurrentCityPopulation; it cannot authorize this food-owned operation.
   out.phase=BnwCityPopulationPhase::remove_worst_worked_or_specialist_required;return;
  }
  out.unassigned_removal_returned=true;
  const auto delta=std::max(pop_change,-unassigned);const auto next=detail::bnw_population_add(unassigned,delta);
  auto& value=named(citizens.fields,"m_iNumUnassignedCitizens").value;
  publish(BnwFoodPopulationStoreKind::unassigned_citizens,p.progress.city,value,next,[&](auto& world,std::int32_t n){named(named(city(world,p.progress.owner,p.progress.city).management.blocks,"citizens").fields,"m_iNumUnassignedCitizens").value=n;},&p.population_management,nullptr,BnwCityPopulationPhase::prepared);
 }
 auto& owned=p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city);
 need(owned.population==out.before,"Population original raw/owned mirror differs");
 publish(BnwFoodPopulationStoreKind::city_population,p.progress.city,field(raw,"m_iPopulation").values.front(),out.after,[&](auto& world,std::int32_t n){field(city(world,p.progress.owner,p.progress.city),"m_iPopulation").values.front()=n;},nullptr,&owned.population,BnwCityPopulationPhase::population_stored);
 p.mirror->population=out.after;
 // Stale derived happiness is not a numeric gameplay update. It remains
 // unavailable until native later DoUpdateHappiness actually executes.
 p.session->bnw_unhappiness_current_=false;
 for(auto& [owner,player]:p.state->cities.players){(void)owner;player.excess_happiness.reset();}
 // CvPlayer firstCity/nextCity is forward FLTA occupied-slot order.
 std::map<std::int32_t,std::int32_t> slots;
 for(const auto& c:p.state->world.all_players->players.at(p.progress.owner).cities.cities){
  const auto id=scalar(c,"m_iID");need(resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,id)==&c,"Population owner FLTA current city differs");
  need(slots.emplace(id&8191,id).second,"Population owner duplicate city slot");
 }
 for(const auto& [slot,id]:slots){
  (void)slot;auto& c=city(p.state->world,p.progress.owner,id);
  auto& rank=named(named(c.production.blocks,"before_strategy").fields,"m_bPopulationRankValid").value;
  need(rank==0||rank==1,"Population rank native bool invalid");
  publish(BnwFoodPopulationStoreKind::population_rank,id,rank,0,[&](auto& world,std::int32_t n){named(named(city(world,p.progress.owner,id).production.blocks,"before_strategy").fields,"m_bPopulationRankValid").value=n;},&p.population_production,nullptr,BnwCityPopulationPhase::ranks_invalidated);
  ++out.rank_assignments;
 }
 const auto highest=scalar(raw,"m_iHighestPopulation");
 if(out.after>highest)publish(BnwFoodPopulationStoreKind::highest_population,p.progress.city,field(raw,"m_iHighestPopulation").values.front(),out.after,[&](auto& world,std::int32_t n){field(city(world,p.progress.owner,p.progress.city),"m_iHighestPopulation").values.front()=n;},nullptr,nullptr,BnwCityPopulationPhase::highest_population_stored);
 // area() is plot()->area(), not a cached city-area DTO.
 const auto x=scalar(raw,"m_iX"),y=scalar(raw,"m_iY");const auto& grid=p.state->world.map_header.grid;
 need(x>=0&&y>=0&&std::uint64_t(x)<grid.width&&std::uint64_t(y)<grid.height&&p.state->world.map_plots&&p.state->world.map_projection,"Population city plot unavailable");
 const auto plot=grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto& source_plot=p.state->world.map_plots->plots.at(plot);const auto& projected=p.state->world.map_projection->cells.at(plot);
 need(source_plot.index==plot&&source_plot.x==std::uint64_t(x)&&source_plot.y==std::uint64_t(y)&&projected.index==plot&&projected.x==std::uint64_t(x)&&projected.y==std::uint64_t(y),"Population city plot raw/projection differs");
 const auto area=native(named(source_plot.fields,"area").value);const auto* resolved=resolve_bnw_area_registry(p.state->world,area);need(resolved,"Population actual plot area missing");
 auto& areas=named(p.state->world.map_regions->containers,"areas");saves::MapRegionRecord* actual_area=nullptr;
 for(auto& a:areas.records)if(&a==resolved)actual_area=&a;need(actual_area,"Population actual mutable area identity missing");
 const auto full_area=native(named(actual_area->fields,"id").value);
 const auto* original_area=resolve_bnw_area_registry(p.source->snapshot,full_area);
 need(original_area&&original_area->version==actual_area->version&&original_area->start_offset==actual_area->start_offset&&original_area->end_offset==actual_area->end_offset,
  "Population actual area original source span unavailable");
 const auto mutate_area=[&](auto& world)->auto&{auto& records=named(world.map_regions->containers,"areas").records;for(auto& a:records)if(native(named(a.fields,"id").value)==native(named(actual_area->fields,"id").value))return a;throw BnwCityPopulationUnavailable("Population stage lost actual area fullID");};
 auto& area_total=named(actual_area->fields,"population").value;
 publish(BnwFoodPopulationStoreKind::area_population,full_area,area_total,detail::bnw_population_add(native(area_total),pop_change),[&](auto& world,std::int32_t n){named(mutate_area(world).fields,"population").value=n;},nullptr,nullptr,BnwCityPopulationPhase::area_stored);
 auto& area_players=named(actual_area->arrays,"population_per_player");
 need(area_players.columns==1&&p.state->world.map_regions->dimensions&&p.source->snapshot.map_regions->dimensions&&
  p.state->world.map_regions->dimensions->fixed_players==p.source->snapshot.map_regions->dimensions->fixed_players&&
  area_players.values.size()==p.state->world.map_regions->dimensions->fixed_players&&area_players.values.size()>std::size_t(p.progress.owner),"Population native area fixed player array unavailable");
 auto& area_player=area_players.values.at(p.progress.owner);
 publish(BnwFoodPopulationStoreKind::area_player_population,full_area,area_player,detail::bnw_population_add(native(area_player),pop_change),[&](auto& world,std::int32_t n){named(mutate_area(world).arrays,"population_per_player").values.at(p.progress.owner)=n;},nullptr,nullptr,BnwCityPopulationPhase::area_stored);
 auto& player=p.state->world.all_players->players.at(p.progress.owner);auto& total=named(player.core.fields,"m_iTotalPopulation");
 need(total.encoding==saves::PlayerCoreEncoding::i32&&total.values.size()==1&&p.state->cities.players.at(p.progress.owner).total_population==native(total.values.front()),"Population player total raw/owned mismatch");
 publish(BnwFoodPopulationStoreKind::player_population,p.progress.owner,total.values.front(),detail::bnw_population_add(native(total.values.front()),pop_change),[&](auto& world,std::int32_t n){named(world.all_players->players.at(p.progress.owner).core.fields,"m_iTotalPopulation").values.front()=n;},&p.population_players,&p.state->cities.players.at(p.progress.owner).total_population,BnwCityPopulationPhase::player_stored);
 const auto team=detail::bnw_player_team_read(*p.state,p.progress.owner);need(p.state->world.teams&&team>=0&&team<64&&p.state->world.teams->teams.size()==64,"Population team source unavailable");
 auto& team_row=p.state->world.teams->teams.at(team);need(team_row.index==std::uint64_t(team)&&named(team_row.fields,"id").value==team,"Population team actual identity differs");
 auto& team_total=named(team_row.fields,"population").value;
 publish(BnwFoodPopulationStoreKind::team_population,team,team_total,detail::bnw_population_add(native(team_total),pop_change),[&](auto& world,std::int32_t n){named(world.teams->teams.at(team).fields,"population").value=n;},nullptr,nullptr,BnwCityPopulationPhase::team_stored);
 auto* game=std::get_if<saves::Expansion2PrefixObservation>(&p.state->world.game_state.prefix);need(game,"Population BNW Game prefix missing");
 auto& game_total=game->scalars.at(10);
 publish(BnwFoodPopulationStoreKind::game_population,10,game_total,detail::bnw_population_add(native(game_total),pop_change),[&](auto& world,std::int32_t n){std::get<saves::Expansion2PrefixObservation>(world.game_state.prefix).scalars.at(10)=n;},nullptr,nullptr,BnwCityPopulationPhase::game_stored);
 require_population_child(child);
 // Existing normal plot owner cannot be replayed or renamed a population
 // child. Its exact call issuer/terrain return must be joined here privately,
 // preserving the current core/rank/player/area seals through each write.
 out.phase=BnwCityPopulationPhase::plot_yield_child_required;
}

void BnwCityFoodGrowthPending::require_population_plot_unissued(const BnwCityPopulationPending& child,
 const BnwPlotYieldRefreshIssuer& issuer,const LiveSessionState& state,const saves::SavedSession& source,
 const SessionCheckpointIdentity& identity,std::int64_t plot)const{
 require_population_child(child);const auto& p=*impl_;
 need(child.plot_attempted_&&child.progress_.phase==BnwCityPopulationPhase::plot_call&&!p.population_plot&&
  p.session->plot_yield_refresh_issuer_.get()==&issuer&&p.state.get()==&state&&p.source.get()==&source&&
  identity.payload_sha256==p.identity.payload_sha256&&identity.rules_sha256==p.identity.rules_sha256,
  "Population unissued call is not the actual native return/issuer/source");
 const auto& raw=city(p.state->world,p.progress.owner,p.progress.city);
 const auto x=scalar(raw,"m_iX"),y=scalar(raw,"m_iY");const auto& grid=p.state->world.map_header.grid;
 need(x>=0&&y>=0&&std::uint64_t(x)<grid.width&&std::uint64_t(y)<grid.height&&
  grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y))==static_cast<std::uint64_t>(plot),
  "Population unissued plot is not this city's actual native plot");
 (void)issuer.require_game_debug(state,source,identity);
}
void BnwCityFoodGrowthPending::require_population_plot_binding(const BnwCityPopulationPending& child,
 const BnwPlotYieldRefreshPending& plot)const{
 // Unlike require_current, this metadata guard is legal during our exact
 // child writes, before old terrain/core/revision seals have been accepted.
 require_source_registration();const auto& p=*impl_;
 need(p.population.get()==&child&&child.parent_.lock().get()==this&&child.plot_attempted_&&
  child.progress_.attempted&&p.population_plot.get()==&plot,
  "Population plot does not belong to this actual food return");
 const auto issued=plot.issued_identity();
 const auto found=p.session->plot_yield_refresh_pending_.find(issued.first);
 need(found!=p.session->plot_yield_refresh_pending_.end()&&found->second.get()==&plot&&
  issued.second==p.accepted_plot_operation,"Population plot exact latest session operation differs");
}
void BnwCityFoodGrowthPending::accept_population_plot_prefix(BnwCityPopulationPending& child)const{
 auto& p=*impl_;need(bool(p.population_plot),"Population actual plot child absent");
 const auto facts=child.religion_attempted_?
  p.population_plot->population_city_prefix_source(*p.religion_city_child,child):
  p.population_plot->population_prefix_source(child);const auto& terrain=facts.terrain;
 need(terrain.state.get()==p.state.get()&&terrain.source==p.source&&terrain.generation==p.generation&&
  terrain.operation==p.accepted_plot_operation&&terrain.identity.payload_sha256==p.identity.payload_sha256&&
  terrain.identity.rules_sha256==p.identity.rules_sha256,"Population plot source allocation/identity differs");
 need(facts.progress.plot_writes>=p.accepted_plot_writes&&
  facts.progress.terrain_writes>=p.accepted_terrain_writes,"Population plot prefix counter went backwards");
 auto normalized=p.state->world;
 auto& cached=named(normalized.map_plots->plots.at(static_cast<std::size_t>(facts.progress.plot)).arrays,"yields").values;
 need(cached.size()==6,"Population plot actual cache shape differs");
 for(std::size_t y=0;y<6;++y){need(cached[y]==facts.current_cached[y],"Population actual raw cache differs from child");cached[y]=p.accepted_plot_cache[y];}
 if(terrain.city){
  auto& current=city(normalized,terrain.city->first,terrain.city->second);
  auto& values=named(current.core.fields,"m_aiBaseYieldRateFromTerrain").values;
  need(values.size()==6,"Population plot actual terrain shape differs");
  for(std::size_t y=0;y<6;++y){need(values[y]==terrain.current_terrain[y],"Population actual raw terrain differs from child");values[y]=p.accepted_plot_terrain[y];}
 }
 // Normalize ONLY the child-owned cache/terrain fields to the previously
 // accepted prefix. All other raw domains must equal the old private seals.
 need(section_same(make_bnw_city_core_checkpoint(normalized,normalized,p.identity,true),p.core_seal)&&
  section_same(make_bnw_city_production_checkpoint(normalized,normalized,p.identity,true),*p.population_production)&&
  section_same(make_bnw_city_management_checkpoint(normalized,normalized,p.identity,true),*p.population_management)&&
  section_same(make_bnw_player_core_checkpoint(normalized,normalized,p.identity,true),*p.population_players),
  "Population plot contains an unrelated core/production/citizens/player change");
 const auto& state=*p.state;
 need(state.city_dirty_events.size()>=p.accepted_city_events.size()&&
  state.plot_gameplay_events.size()>=p.accepted_plot_events.size(),"Population plot event history was erased");
 need(std::equal(p.accepted_city_events.begin(),p.accepted_city_events.end(),state.city_dirty_events.begin()),
  "Population plot previous city events were replaced");
 const auto same_event=[](const BnwPlotGameplayEvent& a,const BnwPlotGameplayEvent& b){
  return std::tie(a.occurrence,a.generation,a.operation,a.target_revision,a.map_guid,a.plot,a.x,a.y,a.yields,a.kind)==
         std::tie(b.occurrence,b.generation,b.operation,b.target_revision,b.map_guid,b.plot,b.x,b.y,b.yields,b.kind);};
 for(std::size_t i=0;i<p.accepted_plot_events.size();++i)need(same_event(p.accepted_plot_events[i],state.plot_gameplay_events[i]),
  "Population plot previous symbols events were replaced");
 const auto dirty=state.city_dirty_events.size()-p.accepted_city_events.size();
 const auto symbols=state.plot_gameplay_events.size()-p.accepted_plot_events.size();
 need(dirty<=1&&symbols<=1&&!(dirty&&symbols)&&
  state.city_dirty_next_occurrence==p.accepted_city_occurrence+dirty&&
  state.plot_gameplay_next_occurrence==p.accepted_plot_occurrence+symbols,
  "Population plot event occurrence/phase prefix differs");
 const auto required=p.accepted_plot_phase==BnwPlotYieldRefreshPhase::city_screen_predicate?1U+static_cast<unsigned>(dirty):
  (p.accepted_plot_phase==BnwPlotYieldRefreshPhase::city_screen_dirty||p.accepted_plot_phase==BnwPlotYieldRefreshPhase::symbols?1U:0U);
 need(p.accepted_plot_ledger<=UINT64_MAX-required&&terrain.ledger_revision==p.accepted_plot_ledger+required,
  "Population plot mandatory effect attempt ledger differs from native phase");
 const auto writes=static_cast<std::uint64_t>(facts.progress.plot_writes-p.accepted_plot_writes)+
  static_cast<std::uint64_t>(facts.progress.terrain_writes-p.accepted_terrain_writes)+dirty+symbols;
 need(p.ui_revision<=UINT64_MAX-writes&&state.ui_source_revision==p.ui_revision+writes,
  "Population plot current revision is not its exact reached writes/events");
 if(dirty){
  const auto& e=state.city_dirty_events.back();
  need(terrain.city&&e.occurrence==p.accepted_city_occurrence&&e.generation==p.generation&&
   e.owner==terrain.city->first&&e.city==terrain.city->second&&e.plot==facts.progress.plot&&
   e.yield==static_cast<std::int8_t>(p.accepted_plot_next_yield)&&e.kind==BnwCityDirtyKind::city_screen&&e.value,
   "Population city dirty event is not the reached native child notification");
 }
 if(symbols){
  const auto& e=state.plot_gameplay_events.back();const auto xy=state.world.map_header.grid.coordinates(static_cast<std::uint64_t>(facts.progress.plot));
  need(e.occurrence==p.accepted_plot_occurrence&&e.generation==p.generation&&e.operation==terrain.operation&&
   e.target_revision==state.ui_source_revision&&e.map_guid==state.world.map_header.guid&&e.plot==facts.progress.plot&&
   e.x==xy[0]&&e.y==xy[1]&&e.kind==BnwPlotGameplayEventKind::yield_might_have_changed,
   "Population symbols event is not the reached native child notification");
  for(std::size_t y=0;y<6;++y)need(e.yields[y]==facts.current_cached[y],"Population symbols event cache differs");
 }
 // Allocations can still fail here: the caller quarantines the retained call,
 // never rolls back or retries an already reached native child effect.
 auto core=make_bnw_city_core_checkpoint(state.world,state.world,p.identity,true);
 auto production=make_bnw_city_production_checkpoint(state.world,state.world,p.identity,true);
 auto management=make_bnw_city_management_checkpoint(state.world,state.world,p.identity,true);
 auto players=make_bnw_player_core_checkpoint(state.world,state.world,p.identity,true);
 auto city_events=state.city_dirty_events;auto plot_events=state.plot_gameplay_events;
 require_population_plot_binding(child,*p.population_plot);
 const auto seal=[](auto& dst,auto& src)noexcept{dst.bytes.swap(src.bytes);dst.id=src.id;dst.flags=src.flags;};
 seal(p.core_seal,core);seal(*p.population_production,production);seal(*p.population_management,management);seal(*p.population_players,players);
 p.accepted_plot_cache=facts.current_cached;p.accepted_plot_terrain=terrain.current_terrain;
 p.accepted_plot_writes=facts.progress.plot_writes;p.accepted_terrain_writes=facts.progress.terrain_writes;
 p.accepted_plot_next_yield=facts.progress.next_yield;p.accepted_plot_ledger=terrain.ledger_revision;p.accepted_plot_phase=facts.progress.phase;
 p.accepted_city_events.swap(city_events);p.accepted_plot_events.swap(plot_events);
 p.accepted_city_occurrence=state.city_dirty_next_occurrence;p.accepted_plot_occurrence=state.plot_gameplay_next_occurrence;
 p.ui_revision=state.ui_source_revision;p.progress.ui_revision=p.ui_revision;child.progress_.ui_revision=p.ui_revision;
 require_population_child(child);
}
void BnwCityFoodGrowthPending::continue_population_plot(const BnwCityPopulationPending& actual)const{
 require_population_child(actual);auto& p=*impl_;auto& child=*p.population;
 need(&child==&actual&&(child.progress_.phase==BnwCityPopulationPhase::plot_yield_child_required||
  child.progress_.phase==BnwCityPopulationPhase::plot_call),
  "Population actual plot call is already returned/quarantined");
 try{
  const auto host=p.host.lock();need(bool(host),"Population plot actual Host expired");
  if(!child.plot_attempted_){
   need(child.progress_.phase==BnwCityPopulationPhase::plot_yield_child_required&&!p.population_plot,
    "Population has not reached its actual plot update call");
   child.plot_attempted_=true;child.progress_.phase=BnwCityPopulationPhase::plot_call;
   // Every previous scalar write retired siblings. This is a NEW native call
   // from the actual population return, not replay of a standalone cursor.
   const auto& raw=city(p.state->world,p.progress.owner,p.progress.city);
   const auto x=scalar(raw,"m_iX"),y=scalar(raw,"m_iY");const auto& grid=p.state->world.map_header.grid;
   need(x>=0&&y>=0&&std::uint64_t(x)<grid.width&&std::uint64_t(y)<grid.height,"Population native city plot unavailable");
   const auto plot=static_cast<std::int64_t>(grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y)));
   auto rules=BnwNormalPlotYieldRules::load(host->database(),p.identity.rules_sha256);
   require_population_child(child);
   if(!p.session->plot_yield_refresh_issuer_)p.session->plot_yield_refresh_issuer_=
    std::shared_ptr<BnwPlotYieldRefreshIssuer>(new BnwPlotYieldRefreshIssuer(p.state,p.generation));
   const auto issuer=p.session->plot_yield_refresh_issuer_;issuer->bind_game_debug(p.game,p.source);
   std::map<std::int64_t,std::shared_ptr<BnwPlotYieldRefreshPending>> staging;
   const auto found=p.session->plot_yield_refresh_pending_.find(plot);
   if(found==p.session->plot_yield_refresh_pending_.end())staging.emplace(plot,nullptr);
   auto node=staging.empty()?decltype(staging)::node_type{}:staging.extract(staging.begin());
   p.archived_population_plots.reserve(p.archived_population_plots.size()+(found==p.session->plot_yield_refresh_pending_.end()?0:1));
   auto pending=BnwPlotYieldRefreshPending::prepare_normal_population(issuer,p.state,p.source,p.identity,rules,plot,child);
   require_population_child(child);
   const auto old=found==p.session->plot_yield_refresh_pending_.end()?nullptr:found->second;
   // Retain the actual old object/ledger. No erase, reset or old world query.
   if(old){p.archived_population_plots.push_back(old);old->invalidate();}
   if(found!=p.session->plot_yield_refresh_pending_.end())found->second=pending;
   else{node.mapped()=pending;p.session->plot_yield_refresh_pending_.insert(std::move(node));}
   p.population_plot=pending;p.accepted_plot_operation=pending->issued_identity().second;
   const auto before=pending->population_prefix_source(child);
   p.accepted_plot_cache=before.current_cached;p.accepted_plot_terrain=before.terrain.current_terrain;
   p.accepted_plot_next_yield=before.progress.next_yield;
   p.accepted_city_events=p.state->city_dirty_events;p.accepted_plot_events=p.state->plot_gameplay_events;
   p.accepted_city_occurrence=p.state->city_dirty_next_occurrence;p.accepted_plot_occurrence=p.state->plot_gameplay_next_occurrence;
   pending->advance_prefix();accept_population_plot_prefix(child);
  }else{
   need(bool(p.population_plot),"Population attempted plot call lacks retained actual child");
   require_population_plot_binding(child,*p.population_plot);
   const auto plot=p.population_plot->issued_identity().first;
   auto adapter=ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(host);
   (void)adapter->continue_plot(plot);
   accept_population_plot_prefix(child);
  }
  const auto phase=p.population_plot->progress().phase;
  if(phase==BnwPlotYieldRefreshPhase::complete||phase==BnwPlotYieldRefreshPhase::skipped_no_area)
   child.progress_.phase=BnwCityPopulationPhase::update_religion_required;
  // Only the genuine retained child's actual return crosses plot.updateYield.
  // UpdateReligion(GetReligiousMajority()) is still the next mandatory call;
  // an Entry-owned 37-slot owner cannot be substituted for this return site.
 }catch(...){if(p.population_plot)p.population_plot->invalidate();child.progress_.phase=BnwCityPopulationPhase::failed;throw;}
}

void BnwCityFoodGrowthPending::require_population_city_binding(const BnwCityPopulationPending& child,
 const BnwCityYieldRefreshPending& city_child)const{
 require_source_registration();const auto& p=*impl_;
 need(p.population.get()==&child&&child.parent_.lock().get()==this&&child.religion_attempted_&&
  child.progress_.phase!=BnwCityPopulationPhase::failed&&p.religion_city_child.get()==&city_child,
  "Population religion city child is not actual registered return");
 need(p.session->city_yield_refresh_registry_&&p.religion_majority.has_value(),
  "Population religion original majority or city registry absent");
 city_child.require_population_registration(child,*p.session);
}
void BnwCityFoodGrowthPending::bind_population_city_plot(const BnwCityPopulationPending& child,
 const BnwCityYieldRefreshPending& parent,const std::shared_ptr<BnwPlotYieldRefreshPending>& plot)const{
 require_population_city_binding(child,parent);need(bool(plot),"Population religion plot child absent");
 parent.require_population_child_prefix(child,*plot);auto& p=*impl_;
 // Preallocate archival capacity before binding the next genuine native call.
 // Previous completed calls are retained metadata, never queried or reset.
 p.archived_population_plots.reserve(p.archived_population_plots.size()+(p.population_plot?1:0));
 const auto facts=plot->population_city_prefix_source(parent,child);
 auto city_events=p.state->city_dirty_events;auto plot_events=p.state->plot_gameplay_events;
 if(p.population_plot)p.archived_population_plots.push_back(p.population_plot);
 p.population_plot=plot;p.accepted_plot_operation=facts.terrain.operation;
 p.accepted_plot_cache=facts.current_cached;p.accepted_plot_terrain=facts.terrain.current_terrain;
 p.accepted_plot_writes=facts.progress.plot_writes;p.accepted_terrain_writes=facts.progress.terrain_writes;
 p.accepted_plot_next_yield=facts.progress.next_yield;p.accepted_plot_ledger=facts.terrain.ledger_revision;
 p.accepted_plot_phase=facts.progress.phase;p.accepted_city_events.swap(city_events);p.accepted_plot_events.swap(plot_events);
 p.accepted_city_occurrence=p.state->city_dirty_next_occurrence;p.accepted_plot_occurrence=p.state->plot_gameplay_next_occurrence;
}
void BnwCityFoodGrowthPending::accept_population_city_plot(const BnwCityPopulationPending& child,
 const BnwCityYieldRefreshPending& parent,const BnwPlotYieldRefreshPending& plot)const{
 require_population_city_binding(child,parent);parent.require_population_child_prefix(child,plot);
 need(impl_->population_plot.get()==&plot,"Population religion latest retained plot pointer differs");
 accept_population_plot_prefix(*impl_->population);
}
void BnwCityFoodGrowthPending::require_population_religion_return()const{
 require_current();const auto& p=*impl_;
 need(p.population&&p.religion_city_child&&p.population->religion_attempted_&&
  (p.population->progress_.phase==BnwCityPopulationPhase::religion_yields||
   ((p.population->progress_.phase==BnwCityPopulationPhase::player_update_religion_required||
     p.population->progress_.phase==BnwCityPopulationPhase::player_happiness||
     p.population->progress_.phase==BnwCityPopulationPhase::player_founder_religion_required)&&
    p.religion_owner&&p.religion_owner->player_happiness_tail_reached()&&!p.religion_ui_yield)),
  "Population religion setters lack genuine UpdateReligion city37 return");
 p.religion_city_child->require_completed_for_population(*p.population);
}
void BnwCityFoodGrowthPending::require_population_player_return()const{
 require_population_religion_return();const auto& p=*impl_;
 need(p.population&&p.religion_owner&&!p.religion_ui_yield&&
  (p.population->progress_.phase==BnwCityPopulationPhase::player_update_religion_required||
   p.population->progress_.phase==BnwCityPopulationPhase::player_happiness||
   p.population->progress_.phase==BnwCityPopulationPhase::player_founder_religion_required),
  "Population player religion lacks exact returned city-religion call");
 const auto host=p.host.lock();need(bool(host),"Population player religion Host expired");
 p.religion_owner->require_player_happiness_tail(*this,host);
}
void BnwCityFoodGrowthPending::continue_population_player_religion(const BnwCityPopulationPending& actual)const{
 require_population_child(actual);require_population_player_return();auto& p=*impl_;auto& child=*p.population;
 try{
  const auto host=p.host.lock();need(bool(host),"Population player religion Host expired");
  if(!child.progress_.player_religion_attempted){
   need(!p.player_happiness_owner&&child.progress_.player_happiness_stores==0,
    "Population player happiness call already issued");
   // First-attempt marker precedes Rules materialization and reached getters.
   child.progress_.player_religion_attempted=true;
   child.progress_.phase=BnwCityPopulationPhase::player_happiness;
   p.player_happiness_owner=BnwPlayerHappinessPrefixOwner::from_population_religion_tail(
    *this,*p.religion_owner,host,p.state,p.source);
  }
  need(p.player_happiness_owner&&p.population_players,
   "Population player happiness lacks retained owner/player beforeimage");
  auto& owner=*p.player_happiness_owner;auto step=owner.prepare_next(*this,host);
  need(step.stage==child.progress_.player_happiness_stores&&step.stage<6,
   "Population player happiness source order differs");
  auto world=p.state->world;auto& staged=world.all_players->players.at(static_cast<std::size_t>(p.progress.owner));
  named(staged.core.fields,"m_iHappiness").values.front()=step.after;
  auto next_players=make_bnw_player_core_checkpoint(world,world,p.identity,true);
  auto& raw=p.state->world.all_players->players.at(static_cast<std::size_t>(p.progress.owner));
  auto* target=&named(raw.core.fields,"m_iHappiness").values.front();
  auto* mirror=&p.state->unhappiness.players.at(p.progress.owner).happiness;
  auto* excess=&p.state->cities.players.at(p.progress.owner).excess_happiness;
  require_population_player_return();owner.validate_before_publication(step,*this,host);
  need(p.ui_revision<UINT64_MAX,"Population player happiness revision exhausted before store");
  // Exact old ID24 was validated above. Allocate its single-field successor
  // before publication; no general rebaseline or restored-cache authority.
  *target=step.after;*mirror=step.after;excess->reset();
  p.population_players->bytes.swap(next_players.bytes);
  p.population_players->id=next_players.id;p.population_players->flags=next_players.flags;
  ++p.state->ui_source_revision;++p.ui_revision;++child.progress_.player_happiness_stores;
  owner.commit_published(std::move(step),p.ui_revision);
  child.progress_.phase=child.progress_.player_happiness_stores==6?
   BnwCityPopulationPhase::player_founder_religion_required:BnwCityPopulationPhase::player_happiness;
  p.progress.ui_revision=p.ui_revision;child.progress_.ui_revision=p.ui_revision;
  require_population_child(child);
 }catch(...){
  // Earlier six stores remain actual facts on any later operand/Lua failure.
  // Failed cursors retain the same history and cannot be resumed or replayed.
  if(p.player_happiness_owner)p.player_happiness_owner->retire();
  if(p.religion_owner)p.religion_owner->retire();
  if(p.religion_city_child)p.religion_city_child->invalidate();
  child.progress_.phase=BnwCityPopulationPhase::failed;throw;
 }
}

void BnwCityFoodGrowthPending::continue_population_religion(const BnwCityPopulationPending& actual)const{
 require_population_child(actual);auto& p=*impl_;auto& child=*p.population;
 need(&actual==&child&&(child.progress_.phase==BnwCityPopulationPhase::update_religion_required||
  child.progress_.phase==BnwCityPopulationPhase::religion_city_call||
  child.progress_.phase==BnwCityPopulationPhase::religion_yields||
  child.progress_.phase==BnwCityPopulationPhase::player_update_religion_required||
  child.progress_.phase==BnwCityPopulationPhase::player_happiness||
  child.progress_.phase==BnwCityPopulationPhase::player_founder_religion_required),
  "Population religion continuation not at actual source call");
 // This is the actual Player.UpdateReligion call, after the returned city
 // religion cursor and its synchronous mandatory UI tail. Never replay city37.
 if(child.progress_.phase==BnwCityPopulationPhase::player_update_religion_required||
    child.progress_.phase==BnwCityPopulationPhase::player_happiness||
    child.progress_.phase==BnwCityPopulationPhase::player_founder_religion_required){
  continue_population_player_religion(actual);return;
 }
 try{
  const auto host=p.host.lock();need(bool(host),"Population religion actual Host expired");
  if(!child.religion_attempted_){
   need(!p.religion_city_child,"Population religion city call already issued");
   child.religion_attempted_=true; // before majority getters, allocation or effect
   p.religion_majority=BnwCityReligionYieldOwner::capture_majority_for_population(*this,host,p.state,p.source);
   child.progress_.phase=BnwCityPopulationPhase::religion_city_call;
   auto registry=std::shared_ptr<BnwCityYieldRefreshRegistry>(new BnwCityYieldRefreshRegistry());
   auto old=p.session->city_yield_refresh_registry_;
   p.archived_city_registries.reserve(p.archived_city_registries.size()+(old?1:0));
   // Earlier native scalar writes retired the old city/plot cursors. This is
   // a distinct call at the actual UpdateReligion site; preserve old history.
   if(old){p.archived_city_registries.push_back(old);old->retire();}
   p.session->city_yield_refresh_registry_=registry;
   p.religion_city_child=BnwCityYieldRefreshPending::prepare(*p.session,host,registry,
    p.progress.owner,p.progress.city,{},p.population);
   return;
  }
  need(bool(p.religion_city_child),"Population attempted religion call lacks actual native37 child");
  require_population_city_binding(child,*p.religion_city_child);
  if(child.progress_.phase==BnwCityPopulationPhase::religion_city_call){
   p.religion_city_child->advance(*p.session,host);require_population_child(child);
   const auto actual_city=p.religion_city_child->progress();
   child.progress_.religion_city_slots=actual_city.next_slot;child.progress_.religion_city_null_skips=actual_city.null_skips;
   child.progress_.religion_city_returned_calls=actual_city.returned_calls;
   if(p.religion_city_child->progress().phase==BnwCityYieldRefreshPhase::source_loop_returned){
    p.religion_city_child->require_completed_for_population(child);
    // The native37 owner captured each completed call while current. Its last
    // latest-plot handle must not recapture the later religion-mutated world.
    if(p.population_plot)p.population_plot->invalidate();
    child.progress_.phase=BnwCityPopulationPhase::religion_yields;
   }
   return;
  }
  require_population_religion_return();
  if(!p.religion_owner)p.religion_owner=BnwCityReligionYieldOwner::after_population_city_yield_child(
   *this,host,p.state,p.source,*p.religion_majority);
  auto& owner=*p.religion_owner;owner.require_current(*this,host);
  if(p.religion_ui_yield){
   const auto yi=*p.religion_ui_yield;const auto city_team=detail::bnw_player_team_read(*p.state,p.progress.owner);
   const auto active_team=detail::bnw_game_active_team_read(*p.state);
   auto events=p.state->city_dirty_events;auto next=p.state->city_dirty_next_occurrence;bool emitted=false;
   if(city_team==active_team){
    const auto control=p.session->city_selection_.lock();
    need(bool(control),"Population religion actual selection owner absent");control->require_current();
    if(control->is_selected(p.progress.owner,p.progress.city)){
     need(next<UINT64_MAX&&events.size()<events.max_size(),"Population religion dirty queue exhausted");
     events.push_back({next++,p.generation,p.progress.owner,p.progress.city,-1,
      static_cast<std::int8_t>(yi),BnwCityDirtyKind::city_screen,true});emitted=true;
    }
    control->require_current();
   }
   require_population_religion_return();owner.require_current(*this,host);
   if(emitted)need(p.ui_revision<UINT64_MAX,"Population religion dirty revision exhausted");
   p.state->city_dirty_events.swap(events);p.state->city_dirty_next_occurrence=next;
   if(emitted){++p.state->ui_source_revision;++p.ui_revision;}
   p.religion_ui_yield.reset();p.progress.ui_revision=p.ui_revision;child.progress_.ui_revision=p.ui_revision;
   return;
  }
  if(owner.player_happiness_tail_reached()){
   child.progress_.phase=BnwCityPopulationPhase::player_update_religion_required;return;
  }
  std::optional<BnwCitySpecialistCounts> counts;
  if(owner.needs_specialist_total())counts.emplace(BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city));
  const auto step=owner.prepare_next(*this,host,counts?&*counts:nullptr);
  if(!step.writes){require_population_religion_return();owner.validate_before_publication(step,*this,host);owner.commit_published(step);child.progress_.religion_yield=step.after.yield;return;}
  auto world=p.state->world;auto& staged=city(world,p.progress.owner,p.progress.city);
  auto& staged_field=named(staged.core.fields,step.field);
  const auto index=step.vector_yield<0?std::size_t{0}:static_cast<std::size_t>(step.vector_yield);
  staged_field.values.at(index)=step.after_value;
  auto yields=p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).yields;
  if(step.vector_yield>=0)yields[step.vector_yield].religion=step.after_value;
  auto seal=make_bnw_city_core_checkpoint(world,world,p.identity,true);
  auto& raw=city(p.state->world,p.progress.owner,p.progress.city);auto* target=&named(raw.core.fields,step.field).values.at(index);
  auto* mirror=&p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).yields;
  require_population_religion_return();owner.validate_before_publication(step,*this,host);
  need(p.ui_revision<UINT64_MAX&&child.progress_.religion_assignments<UINT64_MAX,"Population religion write history exhausted");
  // No throwing code after the actual source setter starts. Whole old seals
  // were checked above; replace ONLY this field and its authoritative mirror.
  *target=step.after_value;mirror->swap(yields);p.core_seal.bytes.swap(seal.bytes);
  p.core_seal.id=seal.id;p.core_seal.flags=seal.flags;
  ++p.state->ui_source_revision;++p.ui_revision;++child.progress_.religion_assignments;
  if(child.progress_.religion_reset_assignments<6)++child.progress_.religion_reset_assignments;
  owner.commit_published(step);child.progress_.religion_yield=step.after.yield;
  if(step.base_city_screen)p.religion_ui_yield=static_cast<std::uint8_t>(step.vector_yield);
  p.progress.ui_revision=p.ui_revision;child.progress_.ui_revision=p.ui_revision;
  require_population_child(child);
 }catch(...){
  // Actual partial terrain/cache/religion stores remain facts, never replayed.
  if(p.religion_city_child)p.religion_city_child->invalidate();
  if(p.religion_owner)p.religion_owner->retire();
  child.progress_.phase=BnwCityPopulationPhase::failed;throw;
 }
}


std::shared_ptr<const BnwFoodPopulationCheckpointCapture> BnwCityFoodGrowthPending::capture_population_checkpoint_before_plot()const{
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired,"Food checkpoint capture creator retired");
 need(!p.capture_attempted,"Food checkpoint capture already attempted; once history cannot replay");p.capture_attempted=true;
 try{
  // Acquire actual controls before COPY/callbacks. The token is external; the
  // registered Pending retains only a weak token, never a strong Host cycle.
  const auto registry=p.registry.lock();const auto host=p.host.lock();
  need(registry&&host&&registry->impl_->pending.size()==1,"Food checkpoint capture requires exactly one actual once entry");
  const auto parent=registry->impl_->pending.at({p.progress.owner,p.progress.city});const auto child=p.population;
  need(parent.get()==this&&child,"Food checkpoint capture genuine parent/child absent");require_current();
  need(p.attempted&&!p.progress.returned&&p.progress.difference_observed&&p.mirror&&!p.store_pending&&
   (p.progress.phase==BnwCityFoodGrowthPhase::population_growth||p.progress.phase==BnwCityFoodGrowthPhase::population_starvation)&&
   child->progress_.attempted&&child->progress_.phase==BnwCityPopulationPhase::plot_yield_child_required&&
   !child->plot_attempted_&&!child->religion_attempted_&&!p.population_plot&&!p.religion_city_child&&!p.religion_owner&&!p.player_happiness_owner&&
   p.archived_population_plots.empty()&&p.archived_city_registries.empty()&&!p.state->game_clock.pending,
   "Food checkpoint capture requires exact earned pre-plot return; descendants/partial stores unsupported");
  need(!p.session->bnw_retained_turn_history_&&!p.session->plot_yield_refresh_issuer_&&p.session->plot_yield_refresh_pending_.empty()&&
   p.session->city_entry_pending_.empty()&&p.session->production_turn_pending_.empty()&&!p.session->city_yield_refresh_registry_&&!p.session->unit_move_registry_,
   "Food checkpoint capture other native once-prefix histories unsupported");
  auto components=make_bnw_population_totals_checkpoint_bundle(p.state->world,p.source->snapshot,p.identity);
  auto clock=make_bnw_game_clock_checkpoint(*p.state,p.source->snapshot,p.identity);
  auto token=std::shared_ptr<const BnwFoodPopulationCheckpointCapture>(new BnwFoodPopulationCheckpointCapture(
   parent,child,host,std::move(components),std::move(clock),p.store_history,p.identity,p.progress,child->progress_,p.generation,p.ui_revision,
   p.state->setup.game_turn,p.state->game_clock.sent_auto_moves,p.state->random.map.state(),p.state->random.gameplay.state()));
  p.capture=token;
  require_current();require_population_capture_current_without_callbacks(*token);return token;
 }catch(...){p.capture_failed=true;throw;}
}
void BnwCityFoodGrowthPending::require_population_capture_current_without_callbacks(const BnwFoodPopulationCheckpointCapture& token)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&token.thread_==p.thread&&!p.retired&&p.capture_attempted&&!p.capture_failed&&!p.store_pending,
  "Food checkpoint capture retired/foreign/interrupted");
 const auto registry=p.registry.lock();const auto host=p.host.lock();const auto capture=p.capture.lock();
 need(registry&&!registry->impl_->retired&&registry->impl_->thread==p.thread&&registry->impl_->pending.size()==1&&
  registry->impl_->pending.at({p.progress.owner,p.progress.city}).get()==this&&capture&&capture.get()==&token&&
  !p.capture.owner_before(capture)&&!capture.owner_before(p.capture)&&host&&host.get()==token.host_.get()&&
  !host.owner_before(token.host_)&&!token.host_.owner_before(host)&&token.parent_.get()==this&&p.population==token.child_&&!p.population.owner_before(token.child_)&&!token.child_.owner_before(p.population)&&
  p.population->parent_.lock().get()==this&&!p.population->parent_.owner_before(token.parent_)&&!token.parent_.owner_before(p.population->parent_)&&
  !registry->impl_->pending.at({p.progress.owner,p.progress.city}).owner_before(token.parent_)&&!token.parent_.owner_before(registry->impl_->pending.at({p.progress.owner,p.progress.city}))&&
  &host->session()==p.session&&p.session->food_growth_registry_==registry&&
  p.session->state_==p.state&&!p.session->state_.owner_before(p.state)&&!p.state.owner_before(p.session->state_)&&
  p.session->source_==p.source&&!p.session->source_.owner_before(p.source)&&!p.source.owner_before(p.session->source_)&&p.session->state_generation_==p.generation&&p.session->debug_==p.game&&
  p.generation==token.generation_&&p.identity.payload_sha256==token.identity_.payload_sha256&&p.identity.rules_sha256==token.identity_.rules_sha256&&
  host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256,
  "Food checkpoint capture actual registry/Host/source/control substituted");
 need(!p.session->bnw_retained_turn_history_&&!p.session->plot_yield_refresh_issuer_&&p.session->plot_yield_refresh_pending_.empty()&&
  p.session->city_entry_pending_.empty()&&p.session->production_turn_pending_.empty()&&!p.session->city_yield_refresh_registry_&&!p.session->unit_move_registry_,
  "Food checkpoint capture other native once-prefix histories changed");
 const auto& f=token.food_;const auto& c=token.population_;const auto& now=p.population->progress_;
 need(p.attempted&&!p.progress.returned&&p.progress.owner==f.owner&&p.progress.city==f.city&&p.progress.phase==f.phase&&
  p.progress.difference_observed==f.difference_observed&&p.progress.difference_times100==f.difference_times100&&
  p.progress.food_times100==f.food_times100&&p.progress.food_kept==f.food_kept&&p.progress.native_scalar_assignments==f.native_scalar_assignments&&
  p.population->progress_.phase==BnwCityPopulationPhase::plot_yield_child_required&&!p.population->plot_attempted_&&!p.population->religion_attempted_&&
  !p.population_plot&&!p.religion_city_child&&!p.religion_owner&&!p.player_happiness_owner&&p.archived_population_plots.empty()&&p.archived_city_registries.empty()&&
  now.owner==c.owner&&now.city==c.city&&now.change==c.change&&now.before==c.before&&now.after==c.after&&now.attempted&&
  now.assignments==c.assignments&&now.rank_assignments==c.rank_assignments&&now.unassigned_removal_returned==c.unassigned_removal_returned&&
  now.religion_reset_assignments==0&&now.religion_yield==0&&now.religion_city_slots==0&&now.religion_city_null_skips==0&&
  now.religion_city_returned_calls==0&&now.religion_assignments==0&&now.player_happiness_stores==0&&!now.player_religion_attempted&&
  p.store_history==token.stores_,"Food checkpoint capture actual ordered return/descendant history changed");
 need(p.state->ui_source_revision==token.revision_&&p.ui_revision==token.revision_&&p.progress.ui_revision==token.revision_&&now.ui_revision==token.revision_&&
  p.state->setup.game_turn==token.turn_&&!p.state->game_clock.pending&&p.state->game_clock.sent_auto_moves==token.sent_auto_moves_&&
  p.state->random.map.algorithm()==RandomAlgorithm::bnw&&p.state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&
  p.state->random.map.state()==token.map_rng_&&p.state->random.gameplay.state()==token.game_rng_&&
  !p.session->bnw_unhappiness_current_&&p.state->city_strategy_runtime.empty(),"Food checkpoint capture clock/RNG/derived freshness changed");
 for(const auto& [owner,player]:p.state->cities.players){(void)owner;need(!player.excess_happiness,"Food checkpoint capture imported happiness cache was promoted");}
 std::uint64_t revision=p.first_store_revision,food_count=0,pop_count=0;
 for(const auto& entry:p.store_history){need(entry.returned&&entry.actor==f.owner&&entry.revision_before==revision&&
   entry.revision_after==entry.revision_before+(entry.stored?1U:0U),"Food checkpoint capture incomplete/out-of-order native store history");
  revision=entry.revision_after;if(entry.stored){if(entry.population)++pop_count;else ++food_count;}}
 need(revision==token.revision_&&food_count==f.native_scalar_assignments&&pop_count==c.assignments,
  "Food checkpoint capture native assignment/revision history changed");
 p.population->check_owned_prefix();
 need(section_same(make_bnw_game_clock_checkpoint(*p.state,p.source->snapshot,p.identity),token.clock_),"Food checkpoint capture physical raw/owned clock changed");
 // Existing SAME codecs provide semantic full-roster/object/schema/companion
 // validation, not a second guessed parser. No callbacks in this final seal.
 const auto actual=make_bnw_population_totals_checkpoint_bundle(p.state->world,p.source->snapshot,p.identity);const auto& saved=token.components_;
 need(section_same(actual.totals,saved.totals)&&section_same(actual.players,saved.players)&&
  section_same(actual.cities.core,saved.cities.core)&&section_same(actual.cities.production,saved.cities.production)&&
  section_same(actual.cities.management,saved.cities.management)&&section_same(actual.cities.tail,saved.cities.tail),
  "Food checkpoint capture current full-roster population/city companions changed");
}

std::shared_ptr<const BnwCityFoodGrowthPending> LiveSession::begin_bnw_city_food_growth_prefix(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t id){
 need(!source_->source.live_bnw_food_growth_journal,"Restored Food47 call cannot execute the already earned growth prefix again");
 auto p=BnwCityFoodGrowthPending::create(*this,host,owner,id);p->execute();return p;
}
std::shared_ptr<const BnwCityFoodGrowthPending> LiveSession::bnw_city_food_growth_pending(std::int64_t owner,std::int64_t id)const{
 need(plot_yield_refresh_thread_==std::this_thread::get_id(),"Growth journal lookup foreign thread");
 if(!food_growth_registry_)return {};const auto* raw=resolve_bnw_current_city(*state_,native(owner),native(id));need(raw,"Growth journal city absent");
 const auto found=food_growth_registry_->impl_->pending.find({native(owner),scalar(*raw,"m_iID")});return found==food_growth_registry_->impl_->pending.end()?nullptr:found->second;
}
void LiveSession::retire_bnw_food_growth()noexcept{if(food_growth_registry_)food_growth_registry_->retire();if(bnw_food_growth_import_&&bnw_food_growth_import_->attempted_)bnw_food_growth_import_->retired_=true;}

std::int32_t BnwCityFoodGrowthPending::growth_checkpoint_threshold_for(const BnwFoodPopulationCheckpointCapture& token)const{
 require_population_capture_current_without_callbacks(token);const auto& p=*impl_;
 need(p.progress.phase==BnwCityFoodGrowthPhase::population_growth&&p.progress.difference_times100>=0&&
  p.population&&p.population->progress_.change==1&&p.growth_threshold_reads.size()==3&&p.growth_threshold_reads.front()>0&&
  std::all_of(p.growth_threshold_reads.begin(),p.growth_threshold_reads.end(),[&](auto n){return n==p.growth_threshold_reads.front();}),
  "Food journal supports only actual growth+1/three unchanged threshold reads");
 return p.growth_threshold_reads.front();
}
std::shared_ptr<const BnwCityFoodGrowthPending> BnwCityFoodGrowthPending::restore_growth_before_plot_from_unpublished_host(
 std::shared_ptr<ui::SelectedBnwPreviewHost> host,const saves::CivisSaveSection& journal,
 const BnwPopulationTotalsCheckpointBundle& bundle,const saves::CivisSaveSection& clock,const saves::CivisSaveSection& random){
 // This private method is callable only by the actual unpublished Host factory.
 // Numeric47/46 decoders never invoke it and cannot issue a Pending.
 need(host&&host->saved_database_thread_==std::this_thread::get_id(),"Food restore foreign actual Host creator");
 need(!host->activation_owner_&&!host->bnw_food_canonical_self_.expired()&&
  !host->bnw_food_canonical_self_.owner_before(host)&&!host.owner_before(host->bnw_food_canonical_self_),
  "Food restore requires SAME unpublished canonical Host control");
 auto& session=*host->session_;need(session.plot_yield_refresh_thread_==std::this_thread::get_id()&&session.state_&&session.source_&&
  !session.food_growth_registry_&&!session.bnw_retained_turn_history_&&!session.plot_yield_refresh_issuer_&&session.plot_yield_refresh_pending_.empty()&&
  session.city_entry_pending_.empty()&&session.production_turn_pending_.empty()&&!session.city_yield_refresh_registry_&&!session.unit_move_registry_,
  "Food restore requires pristine real once registries");
 const auto admission=session.bnw_food_growth_import_;
 need(admission&&admission->creator_==std::this_thread::get_id()&&!admission->retired_&&!admission->attempted_&&!admission->restored_&&
  admission->generation_==session.state_generation_&&admission->source_.lock().get()==session.source_.get()&&
  !admission->source_.owner_before(session.source_)&&!session.source_.owner_before(admission->source_)&&
  admission->state_.lock().get()==session.state_.get()&&!admission->state_.owner_before(session.state_)&&!session.state_.owner_before(admission->state_)&&
  admission->journal_.id==journal.id&&admission->journal_.flags==journal.flags&&admission->journal_.bytes==journal.bytes&&
  admission->identity_.payload_sha256==host->identity().payload_sha256&&admission->identity_.rules_sha256==host->identity().rules_sha256,
  "Food restore requires actual closed SAME import staging owner");
 const auto source=session.source_;const auto state=session.state_;const auto generation=session.state_generation_;const auto identity=host->identity();
 const auto& input=source->source;
 need(input.container==saves::SaveContainerKind::civis&&input.source_sha256&&input.source_prefix&&!input.source_prefix->empty()&&
  input.inflate.stream_end&&input.inflate.available_bytes==input.bytes.size()&&identify_bytes(input.bytes).sha256==identity.payload_sha256&&
  input.live_bnw_food_growth_journal&&*input.live_bnw_food_growth_journal==journal.bytes&&input.live_bnw_population_totals&&
  *input.live_bnw_population_totals==bundle.totals.bytes&&clock.id==16&&clock.flags==0&&random.id==13&&random.flags==0,
  "Food restore actual imported OriginalPayload/journal lineage unavailable");
 const auto represented=saves::make_civis_import_document(input,*input.source_sha256);
 for(const auto& s:represented.sections){const auto id=s.id;need(id==1||id==4||id==5||id==6||id==42||id==13||id==16||
  (id>=24&&id<=28)||id==46||id==47,"Food restore unsupported prior mutable section/owner history");}
 validate_bnw_food_growth_journal_companions(represented);
 auto record=BnwFoodPopulationGrowthCheckpoint::decode(journal);
 need(record.identity.payload_sha256==identity.payload_sha256&&record.identity.rules_sha256==identity.rules_sha256&&
  record.first_revision==state->ui_source_revision&&record.turn==state->setup.game_turn&&!state->game_clock.pending&&
  state->random.map.algorithm()==RandomAlgorithm::bnw&&state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&
  record.map_rng==state->random.map.state()&&record.game_rng==state->random.gameplay.state()&&
  identify_bytes(clock.bytes).sha256==record.companions[6]&&identify_bytes(random.bytes).sha256==record.companions[7]&&
  section_same(clock,make_bnw_game_clock_checkpoint(*state,source->snapshot,identity))&&
  section_same(random,make_simulation_random_checkpoint(state->random,identity)),
  "Food restore clock/RNG/initial once ordinal mismatch");
 const auto pristine=make_bnw_population_totals_baseline_checkpoint_bundle(state->world,identity);
 const auto original=make_bnw_population_totals_baseline_checkpoint_bundle(source->snapshot,identity);
 const auto six=[](const auto& b){return std::array<const saves::CivisSaveSection*,6>{&b.totals,&b.players,&b.cities.core,&b.cities.production,&b.cities.management,&b.cities.tail};};
 for(std::size_t n=0;n<6;++n)need(section_same(*six(pristine)[n],*six(original)[n]),"Food restore expected OriginalPayload fields were already overlaid");
 admission->attempted_=true;
 const auto food=create(session,host,record.actor,record.city);auto& p=*food->impl_;
 // Keep failure/once reservation before every callback; never retry a failed
 // imported restore as begin/execute_growth on the same world.
 p.attempted=true;
 try{
  const auto db=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
  const auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(db,identity.rules_sha256));
  const auto lease=std::shared_ptr<const LiveSession>(host,&session);
  const auto difference=inspect_bnw_city_current_food_difference(lease,rules,record.actor,record.city,true).value_times100();food->require_current();
  const auto threshold=inspect_bnw_city_current_growth_threshold(host,record.actor,record.city).value();food->require_current();
  BnwFoodPopulationGrowthCheckpoint::validate_growth(record,source->snapshot,bundle,difference,threshold);
  // Typed codecs validate persisted data only. Authority is still this real
  // Host/Session and the successful original numeric reads above.
  const auto decoded=restore_bnw_population_totals_checkpoint_bundle(bundle,source->snapshot,identity);
  auto staged=state->world;
  struct Install {std::int64_t* target;std::int64_t before,after;};std::vector<Install> writes;
  auto child=std::shared_ptr<BnwCityPopulationPending>(new BnwCityPopulationPending(food));
  child->owned_writes_.reserve(record.stores.size()+2);writes.reserve(record.stores.size()+2);
  std::map<std::int64_t*,std::pair<std::int64_t,std::int64_t>> final;
  const auto target=[&](auto& w,const BnwFoodPopulationStoreRecord&e)->std::int64_t&{
   switch(e.kind){
    case BnwFoodPopulationStoreKind::food:return field(city(w,e.actor,e.full_object),"m_iFood").values.front();
    case BnwFoodPopulationStoreKind::food_kept:return field(city(w,e.actor,e.full_object),"m_iFoodKept").values.front();
    case BnwFoodPopulationStoreKind::city_population:return field(city(w,e.actor,e.full_object),"m_iPopulation").values.front();
    case BnwFoodPopulationStoreKind::population_rank:return named(named(city(w,e.actor,e.full_object).production.blocks,"before_strategy").fields,"m_bPopulationRankValid").value;
    case BnwFoodPopulationStoreKind::highest_population:return field(city(w,e.actor,e.full_object),"m_iHighestPopulation").values.front();
    case BnwFoodPopulationStoreKind::area_population:
    case BnwFoodPopulationStoreKind::area_player_population:{auto& rows=named(w.map_regions->containers,"areas").records;saves::MapRegionRecord* found=nullptr;for(auto& a:rows)if(named(a.fields,"id").value==e.full_object){need(!found,"Food restore duplicate original area");found=&a;}need(found,"Food restore actual original area absent");if(e.kind==BnwFoodPopulationStoreKind::area_population)return named(found->fields,"population").value;return named(found->arrays,"population_per_player").values.at(static_cast<std::size_t>(e.actor));}
    case BnwFoodPopulationStoreKind::player_population:return named(w.all_players->players.at(static_cast<std::size_t>(e.actor)).core.fields,"m_iTotalPopulation").values.front();
    case BnwFoodPopulationStoreKind::team_population:return named(w.teams->teams.at(static_cast<std::size_t>(e.full_object)).fields,"population").value;
    case BnwFoodPopulationStoreKind::game_population:return std::get<saves::Expansion2PrefixObservation>(w.game_state.prefix).scalars.at(10);
    case BnwFoodPopulationStoreKind::unassigned_citizens:throw BnwCityFoodGrowthUnavailable("Food growth47 cannot restore starvation citizen removal");
   }throw BnwCityFoodGrowthUnavailable("Food restore unknown semantic target");
  };
  for(const auto&e:record.stores){auto& scratch=target(staged,e);need(scratch==e.before,"Food restore original field before/history differs");scratch=e.after;auto* live=&target(state->world,e);
   if(e.stored){auto [i,new_target]=final.emplace(live,std::pair{*live,std::int64_t(e.after)});if(!new_target)i->second.second=e.after;}
   if(e.population){need(e.stored,"Food restore incomplete population history");child->owned_writes_.push_back({live,e.after});}
  }
  auto next_core=make_bnw_city_core_checkpoint(staged,staged,identity,true);
  auto next_production=make_bnw_city_production_checkpoint(staged,staged,identity,true);
  auto next_management=make_bnw_city_management_checkpoint(staged,staged,identity,true);
  auto next_players=make_bnw_player_core_checkpoint(staged,staged,identity,true);
  const auto staged_parts=make_bnw_population_totals_checkpoint_bundle(staged,source->snapshot,identity);
  const auto decoded_parts=make_bnw_population_totals_checkpoint_bundle(decoded,source->snapshot,identity);
  for(std::size_t n=0;n<6;++n)need(section_same(*six(staged_parts)[n],*six(decoded_parts)[n]),"Food restore projection/staged fields differ from companions");
  auto& live_city=state->cities.players.at(record.actor).cities.at(record.city);
  auto& live_player=state->cities.players.at(record.actor);
  const auto& old_raw=city(state->world,record.actor,record.city);
  const auto before_population=scalar(old_raw,"m_iPopulation");const auto after_population=detail::bnw_population_add(before_population,1);
  need(live_city.population==before_population&&live_player.total_population==native(named(state->world.all_players->players.at(static_cast<std::size_t>(record.actor)).core.fields,"m_iTotalPopulation").values.front()),"Food restore original raw/owned projection differs");
  child->owned_writes_.push_back({&live_city.population,after_population});
  const auto total_after=native(named(staged.all_players->players.at(static_cast<std::size_t>(record.actor)).core.fields,"m_iTotalPopulation").values.front());
  child->owned_writes_.push_back({&live_player.total_population,total_after});
  BnwCityFoodScalarMirror mirror;mirror.owner=record.actor;mirror.city=record.city;mirror.population=after_population;mirror.razing_turns=0;mirror.source_core={old_raw.core.start_offset,old_raw.core.end_offset};mirror.food_times100=scalar(city(staged,record.actor,record.city),"m_iFood");mirror.food_kept=scalar(city(staged,record.actor,record.city),"m_iFoodKept");
  BnwCityFoodGrowthProgress progress;progress.owner=record.actor;progress.city=record.city;progress.difference_times100=difference;progress.difference_observed=true;progress.food_times100=mirror.food_times100;progress.food_kept=mirror.food_kept;progress.ui_revision=record.revision;progress.phase=BnwCityFoodGrowthPhase::population_growth;
  BnwCityPopulationProgress population;population.owner=record.actor;population.city=record.city;population.change=1;population.before=before_population;population.after=after_population;population.attempted=true;population.ui_revision=record.revision;population.phase=BnwCityPopulationPhase::plot_yield_child_required;
  for(const auto&e:record.stores){if(e.population){++population.assignments;if(e.kind==BnwFoodPopulationStoreKind::population_rank)++population.rank_assignments;}else if(e.stored)++progress.native_scalar_assignments;}
  p.growth_threshold_reads={threshold,threshold,threshold};auto history=record.stores;
  for(const auto& [ptr,values]:final)writes.push_back({ptr,values.first,values.second});
  // Allocate optional seals before any native installation. No allocation,
  // provider, registry construction or scalar arithmetic follows the seal.
  std::optional<saves::CivisSaveSection> production(std::move(next_production)),management(std::move(next_management)),players(std::move(next_players));
  food->require_current();
  need(session.state_==state&&session.source_==source&&session.state_generation_==generation&&p.game==session.debug_&&
   session.bnw_game_debug_owner_current()&&!host->activation_owner_&&!host->bnw_food_canonical_self_.owner_before(host)&&!host.owner_before(host->bnw_food_canonical_self_)&&
   state->ui_source_revision==record.first_revision&&state->setup.game_turn==record.turn&&!state->game_clock.pending&&
   state->random.map.algorithm()==RandomAlgorithm::bnw&&state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&state->random.map.state()==record.map_rng&&state->random.gameplay.state()==record.game_rng&&
   section_same(clock,make_bnw_game_clock_checkpoint(*state,source->snapshot,identity))&&section_same(random,make_simulation_random_checkpoint(state->random,identity)),
   "Food restore final actual Host/source/clock/RNG boundary changed");
  need(session.bnw_food_growth_import_==admission&&!session.bnw_food_growth_import_.owner_before(admission)&&!admission.owner_before(session.bnw_food_growth_import_)&&
   admission->attempted_&&!admission->restored_&&!admission->retired_&&admission->generation_==generation&&admission->source_.lock().get()==source.get()&&admission->state_.lock().get()==state.get(),
   "Food restore imported staging owner changed during callbacks");
  const auto last=make_bnw_population_totals_baseline_checkpoint_bundle(state->world,identity);for(std::size_t n=0;n<6;++n)need(section_same(*six(last)[n],*six(pristine)[n]),"Food restore callback changed original field/roster");
  for(const auto&e:record.stores)if(e.stored){auto* actual=&target(state->world,e);need(final.contains(actual),"Food restore original physical target allocation changed");}
  need(&state->cities.players.at(record.actor).cities.at(record.city).population==&live_city.population&&
   &state->cities.players.at(record.actor).total_population==&live_player.total_population,"Food restore physical projection allocation changed");
  for(const auto& w:writes)need(w.target&&*w.target==w.before,"Food restore physical original target changed");
  need(live_city.population==before_population&&live_player.total_population==total_after-1,"Food restore final paired projections changed");
  // This is snapshot installation of a validated prior execution, NOT a call
  // to execute_growth/population and NOT new effects or RNG history.
  for(const auto& w:writes)*w.target=w.after;
  live_city.population=after_population;live_player.total_population=total_after;
  state->ui_source_revision=record.revision;session.bnw_unhappiness_current_=false;state->city_strategy_runtime.clear();for(auto&[owner,player]:state->cities.players){(void)owner;player.excess_happiness.reset();}
  p.ui_revision=record.revision;p.first_store_revision=record.first_revision;p.progress=progress;p.mirror=mirror;p.population=std::move(child);p.population->progress_=population;
  p.store_history.swap(history);p.core_seal.bytes.swap(next_core.bytes);p.core_seal.id=next_core.id;p.core_seal.flags=next_core.flags;
  p.population_production.swap(production);p.population_management.swap(management);p.population_players.swap(players);p.restored_growth_history=true;admission->restored_=true;
  // No provider/callback or allocation after snapshot stores. The real
  // registered parent/child and original physical bindings remain held.
  const auto registered=p.registry.lock();
  need(p.thread==std::this_thread::get_id()&&!p.retired&&!p.capture_failed&&!p.store_pending&&
   p.session==&session&&p.state==state&&p.source==source&&p.generation==generation&&
   session.state_==state&&session.source_==source&&session.state_generation_==generation&&
   registered&&session.food_growth_registry_==registered&&!registered->impl_->retired&&
   registered->impl_->pending.at({record.actor,record.city}).get()==food.get()&&
   p.game==session.debug_&&session.bnw_game_debug_owner_current()&&
   session.bnw_food_growth_import_==admission&&admission->attempted_&&admission->restored_&&!admission->retired_&&
   p.restored_growth_history&&p.ui_revision==record.revision&&state->ui_source_revision==record.revision&&
   p.progress.phase==BnwCityFoodGrowthPhase::population_growth&&p.population&&
   p.population->parent_.lock().get()==food.get()&&p.population->progress_.phase==BnwCityPopulationPhase::plot_yield_child_required&&
   !p.population->plot_attempted_&&!p.population->religion_attempted_&&
   state->random.map.state()==record.map_rng&&state->random.gameplay.state()==record.game_rng,
   "Food47 pure restored owner/history seal failed after installation");
  for(const auto& w:writes)need(w.target&&*w.target==w.after,"Food47 restored physical field store differs");
  need(live_city.population==after_population&&live_player.total_population==total_after,"Food47 restored raw/owned pair differs");
  p.population->check_owned_prefix();return food;
 }catch(...){admission->retired_=true;p.capture_failed=true;p.progress.phase=BnwCityFoodGrowthPhase::failed;if(p.population)p.population->progress_.phase=BnwCityPopulationPhase::failed;throw;}
}


bool LiveSession::has_bnw_food_growth_import_quarantine()const{
 need(plot_yield_refresh_thread_==std::this_thread::get_id(),"Food import quarantine foreign creator");
 if(!source_||!source_->source.live_bnw_food_growth_journal)return bool(bnw_food_growth_import_);
 const auto admission=bnw_food_growth_import_;
 if(!admission||admission->creator_!=std::this_thread::get_id()||admission->retired_||!admission->attempted_||!admission->restored_||
  admission->generation_!=state_generation_||admission->source_.lock().get()!=source_.get()||admission->state_.lock().get()!=state_.get()||
  admission->journal_.bytes!=*source_->source.live_bnw_food_growth_journal||!food_growth_registry_||food_growth_registry_->impl_->retired||food_growth_registry_->impl_->pending.size()!=1)return true;
 const auto& pending=food_growth_registry_->impl_->pending.begin()->second;
 if(!pending)return true;const auto& p=*pending->impl_;
 return p.thread!=std::this_thread::get_id()||p.retired||!p.restored_growth_history||!p.attempted||p.store_pending||p.capture_failed||
  p.session!=this||p.state!=state_||p.source!=source_||p.generation!=state_generation_||!p.population||
  p.progress.phase==BnwCityFoodGrowthPhase::failed||p.population->progress_.phase==BnwCityPopulationPhase::failed||
  p.host.expired()||p.population->parent_.lock().get()!=pending.get();
}
void BnwCityFoodGrowthPending::require_restored_growth_publication_current()const{
 require_current();const auto& p=*impl_;const auto admission=p.session->bnw_food_growth_import_;
 need(admission&&!p.session->has_bnw_food_growth_import_quarantine()&&p.restored_growth_history&&
  p.progress.phase==BnwCityFoodGrowthPhase::population_growth&&p.population&&p.population->progress_.phase==BnwCityPopulationPhase::plot_yield_child_required&&
  !p.population->plot_attempted_&&!p.population->religion_attempted_&&!p.population_plot&&!p.religion_owner&&!p.religion_city_child&&
  p.state->ui_source_revision==p.ui_revision&&p.ui_revision==BnwFoodPopulationGrowthCheckpoint::decode(admission->journal_).revision,
  "Food47 final publication requires exact restored unissued Plot call");
 const auto actual=make_bnw_population_totals_checkpoint_bundle(p.state->world,p.source->snapshot,p.identity);const auto& saved=admission->bundle_;
 need(section_same(actual.totals,saved.totals)&&section_same(actual.players,saved.players)&&section_same(actual.cities.core,saved.cities.core)&&
  section_same(actual.cities.production,saved.cities.production)&&section_same(actual.cities.management,saved.cities.management)&&section_same(actual.cities.tail,saved.cities.tail)&&
  section_same(make_bnw_game_clock_checkpoint(*p.state,p.source->snapshot,p.identity),admission->clock_)&&
  section_same(make_simulation_random_checkpoint(p.state->random,p.identity),admission->random_),
  "Food47 restored current fields/clock/RNG differ before Host publication");
}

}
