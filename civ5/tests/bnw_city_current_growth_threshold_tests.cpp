#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/bnw_city_growth_threshold_math.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <initializer_list>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityCurrentGrowthThresholdUnavailable&){denied=true;}need(denied,"Generic/retired/foreign current growth query accepted");}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Duplicate actual fixture field");out=&r;}
 need(out,"Actual fixture field absent");return *out;
}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Actual fixture native integer unavailable");return static_cast<std::int32_t>(n);}
std::int32_t city_value(const saves::SavedCityObservation& c,const char* field){
 const auto& v=named(c.core.fields,field);need(v.values.size()==1,"Actual city fixture scalar unavailable");return i32(v.values.front());
}
std::int32_t rule(const content::RulesDatabase& db,const char* table,const char* column,std::int64_t id){
 std::optional<std::int64_t> value;
 const auto report=db.query_rows({std::string("SELECT \"")+column+"\" FROM \""+table+"\" WHERE ID = ? LIMIT 1",{id}},
  [&](const auto& row){value=row.integer_at(0);},1);
 need(report.all_succeeded()&&value,"Actual selected rules row absent");return i32(*value);
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game,std::int64_t preview=0){
 auto result=ui::bootstrap_selected_bnw_preview(save,game,preview,true);
 if(result.preview.blocker)throw std::runtime_error(result.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.preview.host));need(bool(host),"Actual immutable selected host absent");return host;
}
void saved_pitboss_math_tests(){
 using saves::PregameOption;
 const auto read=[](std::initializer_list<PregameOption> values){
  return detail::bnw_growth_saved_pitboss_option(std::span<const PregameOption>(values.begin(),values.size()));};
 need(!read({}),"Empty saved options fabricated a match");
 need(read({{"GAMEOPTION_PITBOSS",0},{"GAMEOPTION_PITBOSS",1}})==0,"Native option first-match order changed");
 need(read({{"GAMEOPTION_PIT",-2},{"GAMEOPTION_PITBOSS",0}})==-2,"Native strncmp prefix match lost");
 need(read({{"",INT32_MIN},{"GAMEOPTION_PITBOSS",1}})==INT32_MIN,"Native empty-name match lost");
 need(read({{std::string("GAMEOPTION_PITBOSS\0suffix",25),INT32_MAX}})==INT32_MAX,
  "Native FString/CustomOption first-NUL truncation lost");
 need(!read({{"GAMEOPTION_PITBOSS_EXTRA",1},{"GAMEOPTION_OTHER",1}}),"Native longer key falsely matched");
 refuses([&]{(void)read({{std::string(64,'x'),0}});});
 refuses([&]{(void)read({{"GAMEOPTION_PITBOSS",static_cast<std::int64_t>(INT32_MAX)+1}});});
 refuses([&]{(void)read({{"GAMEOPTION_PITBOSS",0},{"OTHER",static_cast<std::int64_t>(INT32_MIN)-1}});});
 need(read({{std::string("GAMEOPTION_PITBOSS\0",19)+std::string(90,'x'),0}})==0,
  "Unreachable wire tail changed native first-NUL copy");
}
bool expected_source_human(const LiveSessionState& s,const content::RulesDatabase& db,std::int64_t id){
 const auto slot=static_cast<std::size_t>(id);const auto& preg=*s.world.pregame;
 const auto status=preg.archive.integer_arrays.at("slot_status").at(slot);
 if(status==3)return true;
 if(status==4)return preg.archive.integer_arrays.at("slot_claims").at(slot)==2;
 if(status!=0)return false;
 const auto type=preg.archive.integers.at("game_type");if(type==2)return true;
 // Independent genuine DB/raw-source witness; ResearchContext.pitboss may
 // omit dead OPEN lookups and is not authority for native isHuman here.
 auto saved=detail::bnw_growth_saved_pitboss_option(preg.game_options);
 std::optional<std::int64_t> value;
 if(saved)value=*saved;
 else{
  const auto report=db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",
   {std::string("GAMEOPTION_PITBOSS")}},[&](const auto& row){value=row.integer_at(0);},1);
  need(report.all_succeeded(),"Actual PitBoss DB witness failed");
 }
 return (value&&i32(*value)!=0)||type==3;
}
bool actual_alive(const LiveSessionState& s,const saves::SavedPlayerObservation& p){
 const auto& f=named(p.core.fields,"m_bAlive");
 need(f.encoding==saves::PlayerCoreEncoding::boolean&&f.values.size()==1,"Actual AI fixture alive encoding unavailable");
 const auto value=i32(f.values.front());need((value==0||value==1)&&s.setup.players.at(p.saved_player_id).alive==(value!=0),
  "Actual AI fixture raw/owned alive differs");return value!=0;
}
std::int32_t actual_team(const LiveSessionState& s,std::int64_t actor){
 need(actor>=0&&actor<64,"Actual AI fixture player domain unavailable");
 const auto value=i32(s.world.pregame->archive.integer_arrays.at("team_types").at(static_cast<std::size_t>(actor)));
 need(value>=0&&value<64&&s.setup.players.at(actor).team==value,"Actual AI fixture raw/owned team differs");return value;
}
void run_required_ai(const std::filesystem::path& save,const std::filesystem::path& game){
 saved_pitboss_math_tests();const auto original=identify_file(save).sha256;auto host=boot(save,game);
 const auto& state=host->session().state();const auto& world=state.world;
 need(world.all_players&&world.pregame&&world.teams&&world.all_players->players.size()==64,
  "Actual AI fixture full native roster unavailable");
 const auto& preg=*world.pregame;const auto& db=host->database();
 const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
 const auto integer=[&](const char* name){const auto* d=defines.find(name);
  need(d&&d->kind==content::GlobalDefineKind::integer&&d->lookup.found&&d->lookup.error.empty()&&
   std::holds_alternative<std::int64_t>(d->lookup.value),"Actual AI integer Define unavailable");
  return i32(std::get<std::int64_t>(d->lookup.value));};
 const auto floating=[&](const char* name){const auto* d=defines.find(name);
  need(d&&d->kind==content::GlobalDefineKind::source_float&&d->lookup.found&&d->lookup.error.empty()&&
   std::holds_alternative<double>(d->lookup.value),"Actual AI source-float Define unavailable");
  const auto value=std::get<double>(d->lookup.value);const auto result=static_cast<float>(value);
  need(static_cast<double>(result)==value,"Actual AI cached Define is not exact binary32");return result;};
 const auto base=integer("BASE_CITY_GROWTH_THRESHOLD"),minor_percent=integer("MINOR_CIV_GROWTH_PERCENT");
 const auto multiplier=floating("CITY_GROWTH_MULTIPLIER"),exponent=floating("CITY_GROWTH_EXPONENT");
 const auto speed=rule(db,"GameSpeeds","GrowthPercent",preg.archive.integers.at("game_speed"));
 const auto start=rule(db,"Eras","GrowthPercent",preg.archive.integers.at("era"));
 const auto saved_pitboss=detail::bnw_growth_saved_pitboss_option(preg.game_options);
 std::optional<std::int64_t> db_pitboss;
 const auto report=db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",
  {std::string("GAMEOPTION_PITBOSS")}},[&](const auto& row){db_pitboss=row.integer_at(0);},1);
 need(report.all_succeeded(),"Actual AI diagnostic PitBoss DB read failed");
 std::cout<<"Actual AI growth fixture gameType="<<preg.archive.integers.at("game_type")
  <<", savedOptions="<<preg.game_options.size()<<", savedPitBossMatch="<<bool(saved_pitboss)
  <<", savedPitBossValue="<<(saved_pitboss?std::to_string(*saved_pitboss):"absent")
  <<", dbPitBossValue="<<(db_pitboss?std::to_string(i32(*db_pitboss)):"absent")<<'\n';
 std::size_t alive_players=0,alive_ai=0,ai_candidates=0,admitted_ai=0;
 for(const auto& player:world.all_players->players){
  const auto actor=player.saved_player_id;need(actor>=0&&actor<64,"Actual AI fixture raw player ID out of domain");
  const auto slot=static_cast<std::size_t>(actor);const bool alive=actual_alive(state,player);
  const bool human=expected_source_human(state,db,actor);
  std::cout<<"Actual AI growth roster player="<<actor<<", rawAlive="<<alive
   <<", rawStatus="<<preg.archive.integer_arrays.at("slot_status").at(slot)
   <<", rawClaim="<<preg.archive.integer_arrays.at("slot_claims").at(slot)
   <<", sourceHuman="<<human<<", cityRows="<<player.cities.cities.size()<<'\n';
  if(alive)++alive_players;
  if(alive&&!human&&actor!=63)++alive_ai;
 }
 // Publish the whole actual roster before a candidate's numeric query can
 // fail, so an early source frontier never hides later raw city counts.
 std::cout<<"Actual AI growth roster totals alive="<<alive_players<<", aliveAI="<<alive_ai<<std::endl;
 for(const auto& player:world.all_players->players){
  const auto actor=player.saved_player_id;const auto slot=static_cast<std::size_t>(actor);
  if(!actual_alive(state,player)||expected_source_human(state,db,actor)||actor==63)continue;
  for(const auto& city:player.cities.cities){
   const auto id=city_value(city,"m_iID"),population=city_value(city,"m_iPopulation");
   ++ai_candidates;
   // Every genuine candidate must succeed; no source frontier is counted as a
   // numeric observation. All city rows are visited, without a fixture cap.
   const auto q=inspect_bnw_city_current_growth_threshold(host,actor,id);
   const auto index=inspect_bnw_city_current_growth_threshold(host,actor,id&8191);
   const bool minor=preg.archive.flag_arrays.at("minor_nation_civs").at(slot);
   bool teammate=false;
   for(const auto& other:world.all_players->players){
    const auto other_actor=other.saved_player_id;
    // SDK human BEFORE alive, then candidate team BEFORE actor team.
    if(expected_source_human(state,db,other_actor)&&actual_alive(state,other)&&
       actual_team(state,other_actor)==actual_team(state,actor)){teammate=true;break;}
   }
   BnwCityGrowthThresholdMathInputs operands{population,base,multiplier,exponent,minor,minor_percent,
    speed,start,false,teammate,false,0,0,0};
   if(!teammate){
    const auto handicap=named(world.game_state.controls,"handicap").value;
    operands.game_handicap_ai_growth_percent=rule(db,"HandicapInfos","AIGrowthPercent",handicap);
    operands.game_handicap_ai_per_era_modifier=rule(db,"HandicapInfos","AIPerEraModifier",handicap);
    const auto team=actual_team(state,actor);
    operands.player_current_era=i32(named(world.teams->teams.at(static_cast<std::size_t>(team)).fields,"era").value);
   }
   const auto expected=bnw_city_growth_threshold_math(operands);
   need(q.value()==expected&&index.value()==expected,"Actual AI Game/TEAM/Define/native full-index numeric binding differs");
   ++admitted_ai;
   std::cout<<"Actual AI growth numeric player="<<actor<<", city="<<id<<", population="<<population
    <<", minor="<<minor<<", humanTeammate="<<teammate<<", value="<<expected<<'\n';
  }
 }
 std::cout<<"Actual AI growth totals alive="<<alive_players<<", aliveAI="<<alive_ai
  <<", candidates="<<ai_candidates<<", admitted="<<admitted_ai<<std::endl;
 need(ai_candidates>0&&admitted_ai==ai_candidates,"Required real AI fixture lacks complete actual AI city numeric bindings");
 need(identify_file(save).sha256==original,"Original actual AI save changed");
 std::cout<<"PASS required real AI growth numeric bindings for ALL actual AI cities; no growth/effect permission\n";
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 saved_pitboss_math_tests();
 const auto original=identify_file(save).sha256;auto host=boot(save,game);
 const auto& session=host->session();const auto& state=session.state();const auto& world=state.world;
 need(world.all_players&&world.pregame&&world.teams,"Actual selected source incomplete");
 const auto& raw=world.all_players->players.at(0).cities.cities.at(0);
 const auto id=city_value(raw,"m_iID"),population=city_value(raw,"m_iPopulation");
 need(id==8192&&population==1&&world.pregame->archive.integer_arrays.at("slot_status").at(0)==3,
  "Genuine selected capital/population/human fixture changed");
 const auto defines=host->database().cache_global_defines(content::GlobalDefineProfile::bnw);
 const auto* base=defines.find("BASE_CITY_GROWTH_THRESHOLD");
 const auto* multiplier=defines.find("CITY_GROWTH_MULTIPLIER");const auto* exponent=defines.find("CITY_GROWTH_EXPONENT");
 const auto* minor_percent=defines.find("MINOR_CIV_GROWTH_PERCENT");
 need(base&&multiplier&&exponent&&minor_percent&&std::get<std::int64_t>(base->lookup.value)==15&&
  std::get<std::int64_t>(minor_percent->lookup.value)==150&&
  std::get<double>(multiplier->lookup.value)==8&&std::get<double>(exponent->lookup.value)==1.5,
  "Actual activated growth Define fixture changed");
 const auto speed=world.pregame->archive.integers.at("game_speed"),start=world.pregame->archive.integers.at("era");
 const auto speed_percent=rule(host->database(),"GameSpeeds","GrowthPercent",speed);
 const auto era_percent=rule(host->database(),"Eras","GrowthPercent",start);
 need(speed_percent>0&&speed_percent<=300&&era_percent>0&&era_percent<=100,"Actual fixture percentage range changed");
 const auto expected=(15*speed_percent/100)*era_percent/100;
 need(expected>0,"Actual human growth fixture no longer admits the hand-derived threshold");
 auto full=inspect_bnw_city_current_growth_threshold(host,0,id);
 auto index=inspect_bnw_city_current_growth_threshold(host,0,0);
 need(full.value()==expected&&index.value()==expected,"Genuine human threshold/native FLTA index vs fullID differs");
 // Preview independence requires a real nonhuman actor, not an AI city.
 // This turn0 scenario has only the human city; AI numeric coverage belongs to
 // the separate --require-ai invocation and is never claimed here.
 std::int64_t actual_preview=-1;
 for(const auto& player:world.all_players->players){
  const auto actor=player.saved_player_id;
  if(actual_alive(state,player)&&actor!=63&&!expected_source_human(state,host->database(),actor)){
   actual_preview=actor;break;
  }
 }
 need(actual_preview>=0,"Actual selected fixture lacks a live nonhuman preview actor");
 // Foreign construction and reads fail BEFORE the immutable DB or world is used.
 bool read_denied=false,factory_denied=false;
 std::thread worker([&]{try{(void)full.value();}catch(const BnwCityCurrentGrowthThresholdUnavailable&){read_denied=true;}
  try{(void)inspect_bnw_city_current_growth_threshold(host,0,id);}catch(const BnwCityCurrentGrowthThresholdUnavailable&){factory_denied=true;}});
 worker.join();need(read_denied&&factory_denied&&full.value()==expected,"Foreign request changed genuine owner or reached a borrow");
 refuses([&]{(void)inspect_bnw_city_current_growth_threshold({},0,id);});
 refuses([&]{(void)inspect_bnw_city_current_growth_threshold(host,-1,id);});
 refuses([&]{(void)inspect_bnw_city_current_growth_threshold(host,0,INT64_MAX);});
 const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
 refuses([&]{(void)full.value();});refuses([&]{(void)index.value();});
 auto restored=inspect_bnw_city_current_growth_threshold(host,0,id);need(restored.value()==expected,"Healthy replacement changed current threshold");
 // Both a generic DTO replacement and a moved-from actual facade are rejecting
 // inputs, never synthetic authorities. Keep real moved source alive for checks.
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());
 auto moved=std::make_shared<LiveSession>(std::move(host->session()));
 refuses([&]{(void)restored.value();});refuses([&]{(void)inspect_bnw_city_current_growth_threshold(host,0,id);});
 host->session()=std::move(generic);
 refuses([&]{(void)inspect_bnw_city_current_growth_threshold(host,0,id);});
 moved.reset();host.reset();
 // A second genuine host with a nonhuman preview actor still queries actual
 // player0. Its strong parent is retained solely by the genuine query.
 auto other=boot(save,game,actual_preview);need(other->active_player_id()==actual_preview,"Actual alternate preview fixture absent");
 auto strong=inspect_bnw_city_current_growth_threshold(other,0,id);need(strong.value()==expected,"Preview actor altered the actual city's rules");
 const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();
 need(!weak.expired()&&strong.value()==expected,"Genuine query lost its actual host/DB/session parents");
 auto retained=weak.lock();need(bool(retained),"Strong real host unexpectedly released");
 // Attempted writer is a negative on the REAL sealed DB. No row changes or
 // synthetic rules authority are supplied. Existing immutable barrier poisons it.
 bool writer_denied=false;try{const_cast<content::RulesDatabase&>(retained->database()).apply_sql("UPDATE Defines SET Value = Value WHERE Name = 'BASE_CITY_GROWTH_THRESHOLD'");}
 catch(const std::exception&){writer_denied=true;}
 need(writer_denied,"Actual immutable writer barrier accepted mutation");refuses([&]{(void)strong.value();});
 refuses([&]{(void)inspect_bnw_city_current_growth_threshold(retained,0,id);});
 need(identify_file(save).sha256==original,"Original selected save changed");
 std::cout<<"PASS genuine turn0 human growth threshold, FLTA, preview independence, thread/generation/move/generic/strong parents/rules poisoning; AI numeric coverage is separate --require-ai; no growth/effect permission\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentGrowthThreshold>);
int main(int argc,char** argv){try{
 if(argc==4&&std::string_view(argv[1])=="--require-ai")run_required_ai(argv[2],argv[3]);
 else{need(argc==3,"Expected selected save and installation, or --require-ai save installation");run(argv[1],argv[2]);}
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
