#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Unavailable city-entry source admitted");}
template<class F>std::string refusal_message(F&& f){try{f();}catch(const std::exception& e){return e.what();}throw std::runtime_error("Unavailable file-pair history admitted");}
std::int64_t value(const auto& c,std::string_view name){
 const saves::CityCoreField* out=nullptr;
 for(const auto& f:c.core.fields)if(f.name&&name==f.name){need(!out,"Actual fixture duplicate core field");out=&f;}
 need(out&&out->values.size()==1,"Actual fixture scalar absent");return out->values.front();
}
std::array<bool,3> flags(const auto& c){
 std::optional<bool> sold;
 for(const auto& b:c.production.blocks)if(b.name&&std::string_view(b.name)=="buildings")
  for(const auto& f:b.fields)if(f.name&&std::string_view(f.name)=="m_bSoldBuildingThisTurn"){
   need(!sold&&(f.value==0||f.value==1),"Actual fixture sold boolean unavailable");sold=f.value!=0;
  }
 need(sold.has_value(),"Actual fixture buildings flag absent");
 return {value(c,"m_bDrafted")!=0,value(c,"m_bMadeAttack")!=0,*sold};
}
const auto& city(const LiveSession& s,std::int64_t owner,std::int64_t id){
 const saves::SavedCityObservation* out=nullptr;
 for(const auto& p:s.state().world.all_players->players)if(p.saved_player_id==owner)
  for(const auto& c:p.cities.cities)if(value(c,"m_iID")==id){need(!out,"Actual fixture duplicate city");out=&c;}
 need(out,"Actual fixture city absent");return *out;
}
auto host(const std::filesystem::path& save,const std::filesystem::path& game){
 auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 return std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=identify_file(save).sha256;
 {
  // Pure positions/cache math has no live issuer or caller grant.
  MapGeometry geometry{{5,5},false,false,saves::GameStateLayout::expansion2_version1};
  const auto centre=detail::bnw_city_feature_positions(geometry,2,2);
  const auto corner=detail::bnw_city_feature_positions(geometry,0,0);
  std::size_t centre_count=0,corner_count=0;
  for(const auto& p:centre)if(p)++centre_count;
  for(const auto& p:corner)if(p)++corner_count;
  need(centre_count==19&&corner_count==7,"Range2 native hex NULL positions mismatch");
  need(centre[12]==12&&!centre[0]&&!centre[24],"Native centre/dx-major hex range mismatch");
  const auto odd=detail::bnw_city_feature_positions(geometry,2,1);
  need(odd[11]==2&&odd[13]==13,"Native shifted-row hex offset mismatch");
  MapGeometry tiny{{1,1},true,true,saves::GameStateLayout::expansion2_version1};
  const auto duplicate=detail::bnw_city_feature_positions(tiny,0,0);std::size_t duplicates=0;
  for(const auto& p:duplicate)if(p){need(*p==0,"Wrapped source slot unexpectedly different");++duplicates;}
  need(duplicates==7,"Negative-exact-multiple edge or wrapped multiplicity changed");
  geometry.wrap_x=true;
  need(!geometry.plot(-5,0)&&geometry.plot(-1,0)==std::optional<std::int64_t>(4)&&
   !geometry.plot(-2147483647LL,0),"Native coordRange/sentinel behavior changed");
  std::array<bool,25> clearable{};
  for(std::size_t i=0;i<11;++i)clearable[i]=true;
  need(!detail::bnw_city_feature_majority(clearable),"NULL slots removed from25 denominator");
  clearable[11]=true;need(detail::bnw_city_feature_majority(clearable),"Native integer25/2 threshold mismatch");
  refuses([&]{(void)detail::bnw_city_feature_positions(geometry,-1,0);});
  refuses([&]{(void)detail::bnw_city_feature_positions(geometry,INT64_MAX,0);});
  MapGeometry overflow{{INT32_MAX,1},false,false,saves::GameStateLayout::expansion2_version1};
  refuses([&]{(void)detail::bnw_city_feature_positions(overflow,INT32_MAX-1,0);});
  auto vp=geometry;vp.layout=saves::GameStateLayout::vp542_corpus;
  refuses([&]{(void)detail::bnw_city_feature_positions(vp,0,0);});
 }
 {
  content::RulesDatabase pure;
  pure.apply_sql("CREATE TABLE Features(ID INTEGER PRIMARY KEY,Type TEXT);"
   "CREATE TABLE Builds(ID INTEGER PRIMARY KEY,Type TEXT);"
   "CREATE TABLE BuildFeatures(BuildType TEXT,FeatureType TEXT,Remove INTEGER);"
   "INSERT INTO Features VALUES(0,'FEATURE_PURE'); INSERT INTO Builds VALUES(1,'BUILD_PURE');"
   "INSERT INTO BuildFeatures VALUES('BUILD_PURE','FEATURE_PURE',2);"
   "INSERT INTO BuildFeatures VALUES('BUILD_PURE','FEATURE_PURE',0);");
  pure.seal_immutable_preview_fingerprints();
  auto factual=BnwCityEntryFeatureRules::from_rules(pure);factual.require_current();
  need(!factual.feature_clearable(0),"BuildFeatures duplicate last assignment changed");
  refuses([&]{(void)factual.feature_clearable(1);});
  // This cache is factual only and never binds a manufactured live world.
  refuses([&]{pure.apply_sql("DELETE FROM Features");});
  refuses([&]{factual.require_current();});
 }
 const auto unrelated_rules=[](){
  content::RulesDatabase pure;
  pure.apply_sql("CREATE TABLE Features(ID INTEGER PRIMARY KEY,Type TEXT);"
   "CREATE TABLE Builds(ID INTEGER PRIMARY KEY,Type TEXT);"
   "CREATE TABLE BuildFeatures(BuildType TEXT,FeatureType TEXT,Remove INTEGER);"
   "INSERT INTO Features VALUES(0,'FEATURE_PURE');"
   "INSERT INTO Builds VALUES(1,'BUILD_REMOVE');"
   "INSERT INTO Builds VALUES(3,'BUILD_KEEP');"
   "INSERT INTO BuildFeatures VALUES('BUILD_REMOVE','FEATURE_PURE',2);"
   "INSERT INTO BuildFeatures VALUES('BUILD_KEEP','FEATURE_PURE',0);");
  pure.seal_immutable_preview_fingerprints();
  return BnwCityEntryFeatureRules::from_rules(pure);
 }();
 need(unrelated_rules.feature_clearable(0),"Native NULL Build skip/first true Clearable derivation changed");
 // The private guard retains the genuine Impl after this standalone facade
 // dies. It cannot issue a live child because its actual rules SHA differs.
 detail::require_bnw_city_entry_damage_zero_native(0);
 for(const auto damage:{std::int64_t(-1),std::int64_t(1),std::int64_t(INT32_MIN),
  std::int64_t(INT32_MAX),std::int64_t(INT32_MIN)-1,std::int64_t(INT32_MAX)+1})
  refuses([&]{detail::require_bnw_city_entry_damage_zero_native(damage);});
 // Pure native scalar boundaries above never issue an owner or change a world.
 auto selected=host(save,game);need(bool(selected),"Actual immutable host absent");
 auto& session=selected->session();const auto pristine=session.checkpoint();
 const auto feature_rules=BnwCityEntryFeatureRules::from_rules(selected->database());
 std::int64_t owner=-1,id=-1,no_change_owner=-1,no_change_id=-1;
 std::size_t zero=0,positive=0,negative=0,seen=0,no_change=0;
 for(const auto& p:session.state().world.all_players->players){for(const auto& c:p.cities.cities){
  if(seen==24){break;}
  ++seen;const auto damage=value(c,"m_iDamage");
  if(damage==0){
   ++zero;if(owner==-1){owner=p.saved_player_id;id=value(c,"m_iID");}
   if(flags(c)==std::array<bool,3>{false,false,false}){
    ++no_change;if(no_change_owner==-1){no_change_owner=p.saved_player_id;no_change_id=value(c,"m_iID");}
   }
  }
  else{
   const auto before=flags(c);const auto actual_id=value(c,"m_iID");
   if(damage>0)++positive;else ++negative;
   refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(p.saved_player_id,actual_id);});
   need(flags(city(session,p.saved_player_id,actual_id))==before,"Damaged entry refusal changed native flags");
  }
 }if(seen==24)break;}
 need(seen>0,"Actual imported cities absent");
 std::cout<<"Actual DAMAGE0="<<zero<<" positive="<<positive<<" negative="<<negative<<'\n';
 if(!positive)std::cout<<"Actual positive damage: UNOBSERVED; pure native predicate tested\n";
 if(!negative)std::cout<<"Actual negative damage: UNOBSERVED; pure native predicate tested\n";
 if(owner==-1){std::cout<<"Actual DAMAGE0 issuing branch UNOBSERVED\n";return;}
 if(no_change_owner!=-1){owner=no_change_owner;id=no_change_id;}
 else std::cout<<"Actual already-false three-call scene UNOBSERVED\n";
 std::cout<<"Actual DAMAGE0 already-false="<<no_change<<'\n';
 const auto before=flags(city(session,owner,id));
 const auto terrain_rules=BnwPlotTerrainRulesSnapshot::from_rules(selected->database());
 const auto core_before=make_bnw_city_core_checkpoint(session.state().world,session.state().world,selected->identity(),true);
 const auto production_before=make_bnw_city_production_checkpoint(session.state().world,session.state().world,selected->identity(),true);
 const auto source_before=flags(session.source().snapshot.all_players->players.at(
  static_cast<std::size_t>(owner)).cities.cities.front());
 auto generic=LiveSession::from_restored_bnw(session.source(),session.state());
 refuses([&]{(void)generic.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);});
 refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);});
 refuses([&]{(void)generic.begin_bnw_city_entry_damage_zero(owner,id);});
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(INT64_MIN,id);});
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,INT64_MAX);});
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,-1);});
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,id+8192);});
 bool foreign=false;std::thread worker([&]{try{(void)session.begin_bnw_city_entry_damage_zero(owner,id);}catch(const std::exception&){foreign=true;}});
 worker.join();need(foreign,"Foreign thread entered genuine source");
 auto cursor=session.begin_bnw_city_entry_damage_zero(owner,id&8191);
 const auto progress=cursor->progress();cursor->require_current();
 need(progress.before==before&&progress.after==std::array<bool,3>{false,false,false}&&
  flags(city(session,owner,id))==progress.after&&progress.native_calls==3&&
  progress.raw_assignments==static_cast<std::uint8_t>(before[2]?3:2)&&
  progress.city==id&&progress.slot==(id&8191)&&
  progress.phase==BnwCityEntryPhase::feature_surrounded_pending,
  "Three native source reset calls/order/conditional sold store mismatch");
 // Existing25/26 raw domains still validate widths/schema/companions. These
 // bytes are facts only; they cannot serialize this new private once registry.
 const auto core_facts=make_bnw_city_core_checkpoint(session.state().world,session.state().world,selected->identity(),true);
 const auto production_facts=make_bnw_city_production_checkpoint(session.state().world,session.state().world,selected->identity(),true);
 need(!core_facts.bytes.empty()&&!production_facts.bytes.empty(),"Actual raw city component facts unavailable");
 if(no_change){
  need(before==std::array<bool,3>{false,false,false}&&core_before.bytes==core_facts.bytes&&
   production_before.bytes==production_facts.bytes,"Actual no-change three-call scene changed raw component bytes");
 }
 // Actual selected normal Game, genuine DB snapshot, no invented early-zero
 // owner/plot or flag variation. The exact error proves city history is checked
 // before normal plot/query, relation discovery, byte comparison or codecs.
 need(refusal_message([&]{(void)make_bnw_plot_terrain_checkpoint_pair(session,selected->identity(),terrain_rules);})==
  "Plot/terrain pair cannot export city-entry once history","File pair dropped genuine city once history");
 std::optional<LiveSession::Checkpoint> before_feature;bool feature_before_actual=false;
 {
  feature_before_actual=value(city(session,owner,id),"m_bFeatureSurrounded")!=0;
  const auto old_feature=feature_before_actual;
  const auto resets=flags(city(session,owner,id));
  need(refusal_message([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id,unrelated_rules);})==
   "City feature actual rules/import identity differs","Unrelated actual DB issued feature child");
  cursor->require_current();
  refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(INT64_MIN,id,feature_rules);});
  refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,INT64_MAX,feature_rules);});
  refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id+8192,feature_rules);});
  bool wrong_thread=false;
  std::thread t([&]{try{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);}
   catch(const std::exception&){wrong_thread=true;}});
  t.join();need(wrong_thread,"Foreign source thread executed feature child");
  before_feature=session.checkpoint();
  const auto same=session.continue_bnw_city_entry_feature_surrounded(owner,id&8191,feature_rules);
  need(same.get()==cursor.get(),"Feature child replaced genuine retained cursor");cursor->require_current();
  const auto feature=same->progress();
  need(feature.phase==BnwCityEntryPhase::citystrategy_pending&&feature.native_calls==3&&
   feature.raw_assignments==progress.raw_assignments&&feature.feature_positions==25&&
   feature.feature_before==old_feature&&feature.feature_after==(feature.feature_clearable_positions>=12)&&
   feature.feature_assigned==(feature.feature_before!=feature.feature_after)&&
   feature.feature_after==(value(city(session,owner,id),"m_bFeatureSurrounded")!=0)&&
   flags(city(session,owner,id))==resets,"Actual feature setter/order/counts or reset replay mismatch");
  std::cout<<"Actual feature positions=25 NULL="<<int(feature.feature_null_positions)<<
   " clearable="<<int(feature.feature_clearable_positions)<<" assigned="<<feature.feature_assigned<<'\n';
  if(!feature.feature_assigned)std::cout<<"Actual conditional changed-feature store: UNOBSERVED\n";
  refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);});
  // CP has actual feature bytes and private attempted history. Its deliberate
  // quarantine behavior is tested below together with pristine rollback.

 }
 {
  // Detached native-codec negative fixtures only. They do NOT carry the
  // imported private issuer and cannot be used to exercise an issuing scene.
  auto malformed=session.state().world;
  auto& c=malformed.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities.front();
  bool changed=false;for(auto& f:c.core.fields)if(f.name&&std::string_view(f.name)=="m_bDrafted"){
   f.values.at(0)=2;changed=true;
  }
  need(changed,"Pure native malformed flag fixture missing");
  refuses([&]{(void)make_bnw_city_core_checkpoint(malformed,malformed,selected->identity(),true);});
 }
 {
  auto malformed=session.state().world;
  auto& c=malformed.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities.front();
  bool changed=false;for(auto& b:c.production.blocks)if(b.name&&std::string_view(b.name)=="buildings")
   for(auto& f:b.fields)if(f.name&&std::string_view(f.name)=="m_bSoldBuildingThisTurn"){
    f.value=255;changed=true;
   }
  need(changed,"Pure native malformed sold fixture missing");
  refuses([&]{(void)make_bnw_city_production_checkpoint(malformed,malformed,selected->identity(),true);});
 }
 need(!cursor->full_city_turn_ready()&&!cursor->growth_ready()&&!cursor->scheduler_predecessors_ready(),
  "Standalone entry issued full city/growth/scheduler permission");
 const auto foreign_cp=generic.checkpoint();
 refuses([&]{session.restore(foreign_cp);});cursor->require_current();
 need(flags(city(session,owner,id))==progress.after,"Failed foreign CP partially rolled back flags/history");
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,id);});
 cursor.reset(); // The genuine session owns history independently of handles.
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,id&8191);});
 const auto after_entry=session.checkpoint();
 // Keep a second genuine cursor only if this source really contains another
 // zero-damage city. It is used to observe retirement across CP/move, never
 // synthesized to fill an unobserved source scene.
 std::shared_ptr<const BnwCityEntryPending> retained;
 std::size_t second_seen=0;
 for(const auto& p:session.state().world.all_players->players){for(const auto& c:p.cities.cities){
  if(second_seen==24){break;}
  ++second_seen;
  if(p.saved_player_id==owner&&value(c,"m_iID")==id)continue;
  if(value(c,"m_iDamage")==0){retained=session.begin_bnw_city_entry_damage_zero(p.saved_player_id,value(c,"m_iID"));break;}
 }if(retained||second_seen==24)break;}
 // Checkpoints before/after the reset both preserve once history. A rollback
 // may restore original raw flags but must NOT resurrect the setter calls.
 if(before_feature){
  session.restore(*before_feature);
  need((value(city(session,owner,id),"m_bFeatureSurrounded")!=0)==feature_before_actual,
   "Before-child CP did not restore actual feature beforeimage");
  refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);});
 }
 session.restore(pristine);need(flags(city(session,owner,id))==before,"CP raw flag rollback failed");
 refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);});
 need(refusal_message([&]{(void)make_bnw_plot_terrain_checkpoint_pair(session,selected->identity(),terrain_rules);})==
  "Plot/terrain pair cannot export city-entry once history","File pair dropped quarantined city once history");
 if(retained){need(retained->progress().phase==BnwCityEntryPhase::quarantined,"Rollback erased retained history");refuses([&]{retained->require_current();});}
 else std::cout<<"Second genuine DAMAGE0 retained-cursor scene UNOBSERVED\n";
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,id);});
 session.restore(after_entry);need(flags(city(session,owner,id))==progress.after,"CP raw flag facts failed");
 refuses([&]{(void)session.continue_bnw_city_entry_feature_surrounded(owner,id&8191,feature_rules);});
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,id&8191);});
 // A moved facade receives the same retained history, not a new admission.
 LiveSession moved(std::move(session));
 refuses([&]{(void)session.begin_bnw_city_entry_damage_zero(owner,id);});
 refuses([&]{(void)moved.begin_bnw_city_entry_damage_zero(owner,id);});
 refuses([&]{(void)moved.continue_bnw_city_entry_feature_surrounded(owner,id,feature_rules);});
 auto destination=LiveSession::from_restored_bnw(moved.source(),moved.state());
 destination=std::move(moved);refuses([&]{(void)destination.begin_bnw_city_entry_damage_zero(owner,id);});
 // Arbitrary mutable access retires the genuine import owner before its
 // borrower could manufacture a new source or amend issuing operands.
 (void)destination.mutable_state();refuses([&]{(void)destination.begin_bnw_city_entry_damage_zero(owner,id);});
 need(flags(destination.source().snapshot.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities.front())==source_before,
  "Immutable original snapshot flags changed");
 need(identify_file(save).sha256==original,"Original source save changed");
 std::cout<<"PASS actual DAMAGE0 three calls, conditional sold write, index/full once, CP rollback/quarantine, generic/native/thread/move retirement; growth/fullturn unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityEntryPending>);
static_assert(!std::is_copy_constructible_v<BnwCityEntryPending>);
static_assert(!std::is_default_constructible_v<BnwCityEntryCheckpoint>);
int main(int argc,char** argv){try{need(argc==3,"Supply genuine BNW save and shared installation view");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL city entry: "<<e.what()<<'\n';return 1;}}
