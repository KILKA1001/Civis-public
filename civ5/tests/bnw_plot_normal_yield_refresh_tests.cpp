#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <iostream>
#include <optional>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f,std::string_view text={}){
 bool caught=false;try{f();}catch(const std::exception& e){caught=true;check(text.empty()||std::string_view(e.what()).find(text)!=std::string_view::npos,"Wrong normal refresh refusal");}
 check(caught,"Invalid normal issuing operation accepted");
}
const auto& named(const auto& rows,std::string_view name){
 const typename std::remove_reference_t<decltype(rows)>::value_type* result=nullptr;
 for(const auto& r:rows)if(r.name&&name==r.name){check(!result,"Duplicate fixture field");result=&r;}
 check(result,"Actual fixture field missing");return *result;
}
using Six=std::array<std::int64_t,6>;
Six cache(const runtime::LiveSessionState& state,std::int32_t plot){
 const auto& raw=named(state.world.map_plots->plots.at(static_cast<std::size_t>(plot)).arrays,"yields");
 const auto& cell=state.world.map_projection->cells.at(static_cast<std::size_t>(plot));
 check(raw.columns==1&&raw.values.size()==6&&cell.cached_yields&&cell.cached_yields->size()==6,"Actual six short cache absent");
 Six result{};for(std::size_t y=0;y<6;++y){result[y]=raw.values[y];check(result[y]>=INT16_MIN&&result[y]<=INT16_MAX&&cell.cached_yields->at(static_cast<std::int64_t>(y))==result[y],"Actual raw/projection short cache differs");}return result;
}
std::int64_t short_value(std::int32_t n){const auto bits=static_cast<std::uint16_t>(n);return bits<=INT16_MAX?bits:static_cast<std::int32_t>(bits)-65536;}
Six target(const runtime::LiveSession& session,const runtime::SessionCheckpointIdentity& identity,
           const runtime::BnwNormalPlotYieldRules& rules,std::int32_t plot){
 Six out{};
 for(std::int32_t y=0;y<6;++y){
  out.at(static_cast<std::size_t>(y))=
   short_value(runtime::calculate_bnw_normal_plot_yield(session.state(),session.source(),identity,rules,plot,y));
 }
 return out;
}
bool none(const std::optional<std::int64_t>& id,const std::optional<std::uint64_t>& hash){return (id&&*id==-1&&!hash)||(!id&&hash&&*hash==0);}
void selected(const std::filesystem::path& save,const std::filesystem::path& installation){
 const auto save_hash=runtime::identify_file(save).sha256;
 auto bootstrap=ui::bootstrap_selected_bnw_preview(save,installation,0,true);
 if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
 check(static_cast<bool>(bootstrap.preview.host),"Genuine immutable selected host missing");
 const auto& host=*bootstrap.preview.host;
 const auto source_hash=runtime::identify_bytes(host.session().source().source.bytes).sha256;
 const auto rules=runtime::BnwNormalPlotYieldRules::load(host.database(),host.identity().rules_sha256);
 const auto pair_rules=runtime::BnwPlotTerrainRulesSnapshot::from_rules(host.database());
 const auto city_rules=runtime::load_city_yield_rules(host.database(),content::TechnologyProfile::bnw);
 const auto maintenance=runtime::load_maintenance_rules(host.database(),content::TechnologyProfile::bnw,host.types());
 const auto treasury=runtime::load_treasury_rules(host.database(),content::TechnologyProfile::bnw,host.types());
 const runtime::LiveSessionImportRules imports{host.database(),host.types(),host.technologies(),host.defines(),
  city_rules,host.production_rules(),maintenance,treasury,host.identity(),{}};
 // Every issuing session is actually restored from the unchanged original
 // decoded source. No const_cast, mutable fixture field or caller identity
 // mints an import owner. Detached getter tests remain a separate math scope.
 auto fresh=[&]{return runtime::LiveSession::restore_bnw(host.session().source(),imports);};
 std::array<bool,3> observed{},chosen{};std::optional<std::int32_t> first_supported;
 std::uint32_t calculations=0;
 std::uint64_t checked=0;
 const auto& actual=*host.session().state().world.map_projection;
 for(std::size_t index=0;index<actual.cells.size();++index){
  const auto& cell=actual.cells[index];
  if(index>static_cast<std::size_t>(INT32_MAX)||!cell.terrain_type||*cell.terrain_type<0||
     !cell.potential_city_work||!*cell.potential_city_work||!cell.route||
     !none(cell.feature_type,cell.feature_type_hash)||!none(cell.resource_type,cell.resource_type_hash)||
     !none(cell.improvement_type,cell.improvement_type_hash))continue;
  const auto plot=static_cast<std::int32_t>(index);
  if(calculations==24)break; // Bounded pure producer scan, no fabricated inputs.
  ++calculations;
  Six calculated{};try{calculated=target(host.session(),host.identity(),rules,plot);}catch(const runtime::BnwNormalPlotYieldUnavailable&){continue;}
  const auto imported_cache=cache(host.session().state(),plot);
  const auto& ref=cell.yield_city;
  // Diversify using actual changed values and actual working-city presence.
  // Presence does not assert IsWorkingPlot: the issued cursor proves that.
  const bool has_city=ref&&runtime::resolve_bnw_current_city(host.session().state(),
   static_cast<std::int32_t>(ref->owner),static_cast<std::int32_t>(ref->id));
  const std::size_t bucket=imported_cache==calculated?0:(has_city?2:1);
  if(chosen[bucket])continue;chosen[bucket]=true;
  auto session=fresh();const auto old=cache(session.state(),plot);
  check(target(session,host.identity(),rules,plot)==calculated,"Fresh real import normal calculation differs");
  const auto retained=session.read_state_lease();
  std::shared_ptr<const runtime::BnwPlotYieldRefreshPending> pending;
  try{pending=session.begin_bnw_plot_normal_yield_refresh(plot,rules);}catch(const std::exception& e){
   // A genuinely absent area/working operand may prevent this candidate.
   // Do not turn that refusal into an artificial successful receipt.
   check(cache(session.state(),plot)==old,"Refused actual prepare changed native caches");
   std::cout<<"Actual candidate "<<plot<<" refused before stores: "<<e.what()<<'\n';continue;
  }
  const auto progress=pending->progress();check(progress.operation!=0&&!progress.effect_started,"Normal cursor forged a delivered effect");
  refuses([&]{(void)runtime::make_bnw_plot_terrain_checkpoint_pair(session,host.identity(),pair_rules);},"normal calculation history");
  if(progress.phase==runtime::BnwPlotYieldRefreshPhase::skipped_no_area){check(cache(session.state(),plot)==old&&!progress.fresh_six_yields,"NULL area changed caches/freshness");continue;}
  std::size_t first=6,changes=0;for(std::size_t y=0;y<6;++y)if(old[y]!=calculated[y]){first=std::min(first,y);++changes;}
  const auto after=cache(session.state(),plot);
  std::size_t kind=0;
  if(!changes){check(progress.phase==runtime::BnwPlotYieldRefreshPhase::complete&&progress.next_yield==6&&progress.plot_writes==0&&progress.terrain_writes==0&&progress.fresh_six_yields&&after==old,"Natural unchanged normal values issued effects");}
  else if(progress.phase==runtime::BnwPlotYieldRefreshPhase::symbols){kind=1;check(after==calculated&&progress.next_yield==6&&progress.plot_writes==changes&&progress.terrain_writes==0&&!progress.fresh_six_yields,"Natural unworked changed prefix bypassed symbols/order");}
  else{kind=2;check(progress.phase==runtime::BnwPlotYieldRefreshPhase::city_screen_predicate&&progress.next_yield==first&&progress.plot_writes==1&&progress.terrain_writes==1&&!progress.fresh_six_yields,"Natural worked prefix bypassed UI/source cursor");
   for(std::size_t y=0;y<6;++y)check(after[y]==(y==first?calculated[y]:old[y]),"Worked store continued past mandatory UI");
   const auto& reference=retained->world.map_projection->cells[index].yield_city;check(reference.has_value(),"Actual working handle absent");
   const auto* original=runtime::resolve_bnw_current_city(host.session().state(),static_cast<std::int32_t>(reference->owner),static_cast<std::int32_t>(reference->id));
   const auto* changed=runtime::resolve_bnw_current_city(session.state(),static_cast<std::int32_t>(reference->owner),static_cast<std::int32_t>(reference->id));
   check(original&&changed,"Actual canonical working city disappeared");
   const auto& base=named(original->core.fields,"m_aiBaseYieldRateFromTerrain").values;
   const auto& live=named(changed->core.fields,"m_aiBaseYieldRateFromTerrain").values;
   for(std::size_t y=0;y<6;++y)check(live[y]==base[y]+(y==first?calculated[y]-old[y]:0),"Actual native terrain delta/source order differs");
  }
  observed[kind]=true;++checked;if(!first_supported)first_supported=plot;
  const auto post=session.checkpoint();const auto generation=session.state_generation();session.restore(post);
  check(session.state_generation()==generation+1&&cache(session.state(),plot)==after,"Normal in-memory checkpoint lost raw/projected prefix");
  refuses([&]{(void)pending->progress();},"retired/expired");
  auto restored=session.bnw_plot_yield_refresh_pending(plot);const auto restored_progress=restored->progress();
  refuses([&]{(void)runtime::make_bnw_plot_terrain_checkpoint_pair(session,host.identity(),pair_rules);},"normal calculation history");
  check(restored_progress.phase==progress.phase&&restored_progress.next_yield==progress.next_yield&&restored_progress.operation==progress.operation&&restored_progress.plot_writes==progress.plot_writes&&restored_progress.terrain_writes==progress.terrain_writes,"Normal checkpoint lost target/cursor/once slot");
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(plot,rules);});
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(plot);});
  check(cache(session.state(),plot)==after,"Cross-API duplicate replayed native stores");
  bool foreign=false;std::thread worker([&]{try{(void)session.begin_bnw_plot_normal_yield_refresh(plot,rules);}catch(const std::exception&){foreign=true;}});worker.join();check(foreign,"Foreign normal begin read actual state");
  check(!session.readiness().can_continue()&&!restored->full_city_refresh_ready(),"Normal prefix granted complete turn");
  if(checked>=3)break; // Bounded actual-save evidence, no broad gameplay gate.
 }
 check(first_supported.has_value(),"Selected actual save has no accepted bounded normal refresh candidate");
 {
  auto generic=runtime::LiveSession::from_restored_bnw(host.session().source(),host.session().state());const auto old=cache(generic.state(),*first_supported);
  refuses([&]{(void)generic.begin_bnw_plot_normal_yield_refresh(*first_supported,rules);},"private BNW import identity");
  check(cache(generic.state(),*first_supported)==old,"Generic restored world fabricated issuing authority");
 }
 {
  auto session=fresh();const auto old=cache(session.state(),*first_supported);(void)session.mutable_state();
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(*first_supported,rules);},"private BNW import identity");
  check(cache(session.state(),*first_supported)==old,"Revoked real import identity was bypassed");
 }
 {
  auto session=fresh();auto abandoned=rules;const auto retained_rules=std::move(abandoned);(void)retained_rules;
  const auto old=cache(session.state(),*first_supported);
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(*first_supported,abandoned);},"SAME rules/thread");
  check(cache(session.state(),*first_supported)==old,"Moved-from rules reached stores");
 }
 {
  auto session=fresh();std::shared_ptr<const runtime::BnwPlotYieldRefreshPending> pending;
  {const auto temporary=runtime::BnwNormalPlotYieldRules::load(host.database(),host.identity().rules_sha256);pending=session.begin_bnw_plot_normal_yield_refresh(*first_supported,temporary);}
  (void)pending->progress(); // Rules facade released; SAME internal producer retained.
 }
 {
  std::shared_ptr<const runtime::BnwPlotYieldRefreshPending> pending;
  {auto session=fresh();pending=session.begin_bnw_plot_normal_yield_refresh(*first_supported,rules);}
  refuses([&]{(void)pending->progress();},"retired/expired"); // No borrowed facade dereference.
 }
 check(runtime::identify_file(save).sha256==save_hash&&runtime::identify_bytes(host.session().source().source.bytes).sha256==source_hash,"Original source save/payload changed");
 check(!host.session().readiness().can_continue(),"Selected host continuation incorrectly enabled");
 std::cout<<"Actual normal refresh cases="<<checked<<" bounded producer queries="<<calculations<<" observed unchanged/symbols/worked="<<observed[0]<<'/'<<observed[1]<<'/'<<observed[2]<<"; unobserved branches remain unverified; synthetic overflow/wrap/rule-retirement scenarios deferred to genuine command owners\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwNormalPlotYieldRules>);
static_assert(!std::is_default_constructible_v<runtime::BnwPlotYieldRefreshPending>);
int main(int argc,char** argv){try{check(argc==3,"Supply actual selected BNW save and shared installation view");selected(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL normal refresh: "<<e.what()<<'\n';return 1;}}
