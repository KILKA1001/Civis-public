#include "civ5/runtime/bnw_displayed_plot_yield.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_unowned_plot_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class T>const T& named(const std::vector<T>& rows,std::string_view name){const T* found=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Duplicate genuine fixture source");found=&r;}need(found,"Actual fixture source absent");return *found;}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwDisplayedPlotYieldUnavailable&){denied=true;}need(denied,"Displayed yield admitted retired/generic source authority");}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Actual immutable selected host absent");
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());
 auto rules=std::make_shared<const BnwNormalPlotYieldRules>(BnwNormalPlotYieldRules::load(host->database(),host->identity().rules_sha256));
 auto actual_normal=make_bnw_lua_calculated_plot_yield_owner(session,rules);
 // One actual VM incarnation binds the genuine normal reader; display and
 // normal use this SAME real session/rules, never a detached debug grant.
 auto actual_vm=std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
  ScriptSaveDataProvider{},ScriptPregameProvider{},bnw_lua_calculated_plot_services(actual_normal));
 need(!host->session().bnw_game_debug_mode(),"Original import no longer reaches actual false-debug branch");
 const auto& state=host->session().state();const auto active=state.world.pregame->archive.integers.at("active_player");const auto team=state.setup.players.at(active).team;
 need(active>=0&&active<64&&team>=0&&team<64&&state.world.map_plots->dimensions.teams==80,"Actual selected display actor/storage changed");
 // Actual coherent Root diagnostic proved 592 is JUNGLE and 673 is FOREST.
 // Keep jungle as a separate generic ordinary-feature source scenario.
 for(const auto fixture:{std::pair<std::size_t,const char*>{673,"FEATURE_FOREST"},std::pair<std::size_t,const char*>{592,"FEATURE_JUNGLE"}}){
  const auto* info=host->types().find(fixture.second);const auto& cell=state.world.map_projection->cells.at(fixture.first);
  need(info&&info->table=="Features"&&cell.feature_type_hash==info->hash,"Actual selected feature fixture identity changed");
 }
 // Legacy PUBLIC raw-state normal math still authenticates source/history on
 // every call. No display issuer or fresh-yield authority is fabricated.
 std::optional<std::int32_t> early_plot;
 for(std::size_t i=0;i<state.world.map_projection->cells.size();++i)if(bnw_plot_early_zero_yield(state,static_cast<std::int64_t>(i),0)){early_plot=static_cast<std::int32_t>(i);break;}
 need(early_plot.has_value(),"Actual unchanged selected map has no early-zero regression fixture");
 need(calculate_bnw_normal_plot_yield(state,host->session().source(),host->identity(),*rules,*early_plot,0)==0,"Actual normal early-zero changed");
 const auto normal_refuses=[&](const saves::SavedSession& source,const SessionCheckpointIdentity& identity){
  bool denied=false;try{(void)calculate_bnw_normal_plot_yield(state,source,identity,*rules,*early_plot,0);}catch(const BnwNormalPlotYieldUnavailable&){denied=true;}
  need(denied,"Public normal early-zero bypassed payload/history binding");
 };
 auto wrong_identity=host->identity();wrong_identity.payload_sha256="mismatched-selected-payload";
 normal_refuses(host->session().source(),wrong_identity);
 auto wrong_history=host->session().source();++wrong_history.snapshot.history.start_offset;
 normal_refuses(wrong_history,host->identity());
 std::optional<BnwDisplayedPlotYield> retained;std::size_t comparisons=0;
 for(const std::int32_t plot:{632,673,593,592}){
  const auto& raw=state.world.map_plots->plots.at(static_cast<std::size_t>(plot));const auto& cell=state.world.map_projection->cells.at(static_cast<std::size_t>(plot));
  const auto& owners=named(raw.arrays,"revealed_owners");need(owners.values.size()==80&&cell.owner&&owners.values.at(static_cast<std::size_t>(team))==*cell.owner,"Selected actual display owner differs from normal comparator");
  const auto& route=named(raw.arrays,"revealed_routes");need(cell.route&&route.values.at(static_cast<std::size_t>(team))==cell.route->type,"Selected actual display route differs from normal comparator");
  const auto& improvements=named(raw.arrays,"revealed_improvement_hashes");
  const bool actual_none=(cell.improvement_type&&*cell.improvement_type==-1&&!cell.improvement_type_hash)||(!cell.improvement_type&&cell.improvement_type_hash&&*cell.improvement_type_hash==0);
  need(raw.version&&*raw.version>=6&&actual_none&&improvements.values.size()==80&&improvements.values.at(static_cast<std::size_t>(team))==0,"Selected actual display improvement differs from normal comparator");
  // Genuine source city reveal is a required comparison precondition; this
  // is not a request to change it or a fabricated issuing source.
  for(const auto& ref:{cell.plot_city,cell.yield_city})if(ref&&ref->owner!=-1){const auto* c=resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(ref->owner),static_cast<std::int32_t>(ref->id));need(c,"Actual display city absent");const auto& reveal=named(c->core.fields,"m_abRevealed");need(reveal.values.size()==80&&reveal.values.at(static_cast<std::size_t>(team))==1,"Actual comparator city is not source revealed");}
  for(std::int32_t y=0;y<6;++y){auto q=inspect_bnw_displayed_plot_yield(session,rules,plot,y);const auto value=q.value();
   const auto normal=actual_normal->value(plot,y,false);need(value==normal,"Genuine revealed/display and actual-owner source results differ");
   if(plot==632&&y==0)need(value>0,"Actual road-home displayed FOOD is zero");
   if(!retained)retained=std::move(q);++comparisons;
  }
 }
 // Exercise a genuine fallback actor branch without changing source inputs.
 bool fallback=false;
 for(std::size_t i=0;i<state.world.map_projection->cells.size();++i){const auto& cell=state.world.map_projection->cells[i];const auto& raw=state.world.map_plots->plots[i];if(!cell.owner||*cell.owner!=-1||!cell.potential_city_work||!*cell.potential_city_work||!cell.terrain_type||*cell.terrain_type<0)continue;
  const auto& reveal=named(raw.arrays,"revealed_owners");if(reveal.values.at(static_cast<std::size_t>(team))!=-1)continue;
  // A bare ordinary plot excludes unsupported wonder/all-tech branches.
  const auto none=[](const auto& id,const auto& hash){return (id&&*id==-1&&!hash)||(!id&&hash&&*hash==0);};
  if(!none(cell.feature_type,cell.feature_type_hash)||!none(cell.resource_type,cell.resource_type_hash)||!none(cell.improvement_type,cell.improvement_type_hash))continue;
  auto q=inspect_bnw_displayed_plot_yield(session,rules,static_cast<std::int64_t>(i),0);need(q.value()>=0,"Actual fallback actor branch missed source clamp");fallback=true;break;
 }
 need(retained&&comparisons==24&&fallback,"Genuine display/fallback source coverage absent");
 // Numeric comparison is finished. Retire the ONE actual VM before its
 // calculated owner, so neither can retain aliases during lifetime checks.
 actual_vm.reset();actual_normal.reset();
 bool worker_query=false,worker_bind=false;std::thread worker([&]{try{(void)retained->value();}catch(const BnwDisplayedPlotYieldUnavailable&){worker_query=true;}try{(void)inspect_bnw_displayed_plot_yield(session,rules,632,0);}catch(const BnwDisplayedPlotYieldUnavailable&){worker_bind=true;}});worker.join();need(worker_query&&worker_bind,"Foreign thread touched display world");
 auto detached=state; // It remains a DTO, including when copied from actual state.
 detached.world.map_plots->plots.at(632).arrays.clear();
 auto generic=std::make_shared<const LiveSession>(LiveSession::from_restored_bnw(host->session().source(),std::move(detached)));
 refuses([&]{(void)inspect_bnw_displayed_plot_yield(generic,rules,632,0);});generic.reset();
 auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);refuses([&]{(void)retained->value();});retained=inspect_bnw_displayed_plot_yield(session,rules,632,0);
 auto mutable_session=std::make_shared<LiveSession>(std::move(host->session()));
 refuses([&]{(void)retained->value();});refuses([&]{(void)inspect_bnw_displayed_plot_yield(session,rules,632,0);});
 retained=inspect_bnw_displayed_plot_yield(mutable_session,rules,632,0);
 const auto initial=retained->value();
 std::weak_ptr<ui::SelectedBnwPreviewHost> weak_host=host;
 std::weak_ptr<LiveSession> weak_session=mutable_session;
 std::weak_ptr<const BnwNormalPlotYieldRules> weak_rules=rules;
 host.reset();session.reset();rules.reset();mutable_session.reset();
 need(!boot.preview.host&&weak_host.expired()&&weak_session.use_count()==1&&weak_rules.use_count()==1,
  "External factory/host/session/rules/VM parents survived retained display lifetime test");
 need(retained->value()==initial,"Actual sole retained-query strong parent lifetime failed");
 // Obtain a new sanctioned writer only AFTER the sole-owner read passed.
 auto revoke_owner=weak_session.lock();need(bool(revoke_owner),"Retained query lost actual mutable session parent");
 auto& revoked=revoke_owner->mutable_state(); // Actual sanctioned revoke precedes tampering.
 revoked.world.map_plots->plots.at(632).arrays.clear();refuses([&]{(void)retained->value();});
 need(identify_file(save).sha256==before,"Original selected save changed");
 std::cout<<"PASS genuine displayed source numeric queries="<<comparisons<<", source city/reveal/active fallback, strong parents/thread/generation/generic/revocation; no true-debug writer, symbol consumer, refresh or turn grant\n";
}
}
static_assert(!std::is_default_constructible_v<BnwDisplayedPlotYield>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected original save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
