#include "civ5/runtime/bnw_city_current_strategy_eligibility.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityCurrentStrategyEligibilityUnavailable&){denied=true;}need(denied,"Invalid/retired strategy read or reached native arithmetic accepted");}
const auto& named(const auto& rows,std::string_view name){using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;for(const auto& r:rows)if(r.name&&name==r.name){need(!out,"Duplicate actual fixture source field");out=&r;}need(out,"Actual fixture source field missing");return *out;}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Actual fixture native32 unavailable");return static_cast<std::int32_t>(n);}
void math(){
 using O=BnwCityStrategyTriggerObligation;
 BnwCityStrategyEligibilityMathInputs p{false,false,false,false};
 need(bnw_city_strategy_eligibility_math(p)==O::start,"Unused/no-tech strategy should test start");
 p.no_minor=true;need(bnw_city_strategy_eligibility_math(p)==O::start,"Major incorrectly excluded by NoMinor");
 p.minor=true;p.using_strategy=true;p.check_turn_count=1;p.game_turn=INT32_MIN;p.turn_adopted=INT32_MAX;
 need(bnw_city_strategy_eligibility_math(p)==O::skipped_for_minor,"Minor exclusion failed to short-circuit IsUsing/arithmetic");
 p={false,false,false,false};p.prerequisite=4;p.obsolete=7;
 refuses([&]{(void)bnw_city_strategy_eligibility_math(p);});
 p.has_tech[4]=false;need(bnw_city_strategy_eligibility_math(p)==O::none,"Missing prerequisite must skip obsolete HasTech");
 p.has_tech[4]=true;refuses([&]{(void)bnw_city_strategy_eligibility_math(p);});
 p.has_tech[7]=false;need(bnw_city_strategy_eligibility_math(p)==O::start,"Known prerequisite/nonobsolete must test start");
 p.has_tech[7]=true;need(bnw_city_strategy_eligibility_math(p)==O::none,"Obsolete strategy incorrectly tests start");
 p={false,false,true,true};p.prerequisite=9;p.obsolete=10;p.check_turn_count=1;p.game_turn=INT32_MIN;p.turn_adopted=INT32_MAX;
 need(bnw_city_strategy_eligibility_math(p)==O::none,"Permanent using strategy consumed unused tech/adoption arithmetic");
 p.permanent=false;p.check_turn_count=0;need(bnw_city_strategy_eligibility_math(p)==O::none,"Check count0 must skip subtraction/minimum");
 p.check_turn_count=-1;need(bnw_city_strategy_eligibility_math(p)==O::none,"Negative check count must skip modulo entirely");
 p.check_turn_count=1;refuses([&]{(void)bnw_city_strategy_eligibility_math(p);});
 p.game_turn=INT32_MAX;p.turn_adopted=-1;refuses([&]{(void)bnw_city_strategy_eligibility_math(p);});
 p.game_turn=3;p.turn_adopted=0;p.check_turn_count=2;p.minimum_turns=INT32_MAX;
 need(bnw_city_strategy_eligibility_math(p)==O::none,"Unscheduled end consumed minimum addition");
 p.check_turn_count=1;p.minimum_turns=4;need(bnw_city_strategy_eligibility_math(p)==O::none,"Minimum execution turns not honored");
 p.game_turn=4;need(bnw_city_strategy_eligibility_math(p)==O::end,"Exact minimum boundary must test end");
 p.game_turn=0;p.turn_adopted=2;p.minimum_turns=0;p.check_turn_count=2;
 need(bnw_city_strategy_eligibility_math(p)==O::end,"Native signed negative modulo exact-zero behavior differs");
 p.game_turn=INT32_MIN;p.turn_adopted=0;p.check_turn_count=1;
 need(bnw_city_strategy_eligibility_math(p)==O::end,"Representable INT32_MIN subtraction/mod1 refused");
 p.game_turn=INT32_MAX;p.turn_adopted=INT32_MAX-1;p.minimum_turns=2;p.check_turn_count=1;
 refuses([&]{(void)bnw_city_strategy_eligibility_math(p);});
 p.check_turn_count=2;need(bnw_city_strategy_eligibility_math(p)==O::none,"Minimum sum overflow consumed before scheduled-end short-circuit");
 p.check_turn_count=1;p.minimum_turns=-1;need(bnw_city_strategy_eligibility_math(p)==O::end,"Nonpositive minimum should not add/read cutoff");
 p={false,false,false,false};p.prerequisite=5;p.obsolete=5;p.has_tech[5]=true;
 need(bnw_city_strategy_eligibility_math(p)==O::none,"Same native HasTech slot received contradictory prereq/obsolete facts");
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game,std::int64_t preview=0){auto b=ui::bootstrap_selected_bnw_preview(save,game,preview,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(b.preview.host));need(bool(host),"Actual selected immutable host absent");return host;}
std::map<std::int32_t,std::int64_t> actual_hashes(const saves::TeamHashedArray& a,const content::InfoTypeCatalog& types){
 std::map<std::int32_t,std::int64_t> out;
 for(const auto& r:a.rows){if(!r.type_hash)continue;const auto* t=types.find_hash(r.type_hash);if(!t)continue;need(t->table=="AICityStrategies"&&r.values.size()==1,"Actual strategy fixture hash mapping unavailable");out[i32(t->id)]=r.values[0];}return out;
}
void actual(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=identify_file(save).sha256;std::int64_t alternate=-1;
 {
 auto host=boot(save,game);const auto& s=host->session().state();need(s.world.pregame&&s.world.all_players,"Actual strategy fixture source missing");
 for(const auto& [actor,p]:s.setup.players)if(actor>0&&actor<63&&p.alive){alternate=actor;break;}need(alternate>0,"Actual alternate preview actor absent");
 const auto& city=s.world.all_players->players.at(0).cities.cities.at(0);const auto id=i32(named(city.core.fields,"m_iID").values.at(0));need(id==8192,"Actual selected native capital fixture changed");
 auto full=inspect_bnw_city_current_strategy_eligibility(host,0,id);auto index=inspect_bnw_city_current_strategy_eligibility(host,0,0);
 const auto records=full.records(),bare=index.records();need(!records.empty()&&records.size()==bare.size(),"Actual strategy collection or native index resolution absent");
 const auto& block=named(city.management.blocks,"strategy");const auto used=actual_hashes(named(block.hashes,"m_pabUsingCityStrategy"),host->types());const auto adopted=actual_hashes(named(block.hashes,"m_paiTurnCityStrategyAdopted"),host->types());
 std::map<std::int32_t,BnwCityStrategyEligibilityMathInputs> expected;
 const bool minor=s.world.pregame->archive.flag_arrays.at("minor_nation_civs").at(0);
 const auto team=s.world.pregame->archive.integer_arrays.at("team_types").at(0);const TeamTechnologies* techs=nullptr;for(const auto& t:s.technologies)if(t.team()==team)techs=&t;need(techs,"Actual fixture TEAM techs unavailable");
 const auto report=host->database().query_rows({"SELECT ID,Type,NoMinorCivs,Permanent,TechPrereq,TechObsolete,CheckTriggerTurnCount,MinimumNumTurnsExecuted FROM AICityStrategies ORDER BY ID",{}},[&](const auto& row){
  const auto strategy=i32(row.integer_at(0));need(row.text("Type").has_value(),"Actual strategy Type missing");
  const auto tech=[&](const char* column){const auto text=row.text(column);const auto* t=text?host->types().find(*text):nullptr;return t?i32(t->id):std::int32_t(-1);};
  BnwCityStrategyEligibilityMathInputs p{minor,row.boolean("NoMinorCivs"),false,row.boolean("Permanent")};
  p.prerequisite=tech("TechPrereq");p.obsolete=tech("TechObsolete");p.check_turn_count=i32(row.integer("CheckTriggerTurnCount"));p.minimum_turns=i32(row.integer("MinimumNumTurnsExecuted"));p.game_turn=i32(s.world.pregame->archive.integers.at("game_turn"));
  if(!(p.minor&&p.no_minor)){need(used.contains(strategy),"Actual selected using array lacks explicit strategy row");p.using_strategy=used.at(strategy)!=0;}
  if(p.using_strategy&&!p.permanent&&p.check_turn_count>0)need(adopted.contains(strategy),"Actual reached adoption getter lacks explicit saved row");
  if(adopted.contains(strategy))p.turn_adopted=i32(adopted.at(strategy));
  if(p.prerequisite!=-1)p.has_tech[p.prerequisite]=techs->has_tech(p.prerequisite);
  if(p.obsolete!=-1)p.has_tech[p.obsolete]=techs->has_tech(p.obsolete);
  need(expected.emplace(strategy,std::move(p)).second,"Actual rules duplicate strategy ID");
 });need(report.all_succeeded()&&expected.size()==records.size(),"Actual selected independent rule binding incomplete");
 std::size_t starts=0,ends=0;
 for(std::size_t n=0;n<records.size();++n){const auto& r=records[n];need(r.native_id==static_cast<std::int32_t>(n)&&r.native_id==bare[n].native_id&&r.type==bare[n].type&&r.obligation==bare[n].obligation,"Native strategy loop order/full vs FLTA slot differs");need(r.obligation==bnw_city_strategy_eligibility_math(expected.at(r.native_id)),"Actual query GAME turn/strategy/technology predicates differ");
  if(r.obligation==BnwCityStrategyTriggerObligation::skipped_for_minor)need(!r.using_strategy,"Minor exclusion invented a using value");
  else need(r.using_strategy&&*r.using_strategy==expected.at(r.native_id).using_strategy,"Actual using source fact differs");
  starts+=r.obligation==BnwCityStrategyTriggerObligation::start;ends+=r.obligation==BnwCityStrategyTriggerObligation::end;
 }
 bool read_denied=false,factory_denied=false;std::thread worker([&]{try{(void)full.records();}catch(const BnwCityCurrentStrategyEligibilityUnavailable&){read_denied=true;}try{(void)inspect_bnw_city_current_strategy_eligibility(host,0,id);}catch(const BnwCityCurrentStrategyEligibilityUnavailable&){factory_denied=true;}});worker.join();need(read_denied&&factory_denied&&full.records().size()==records.size(),"Foreign strategy query affected actual immutable owner");
 refuses([&]{(void)inspect_bnw_city_current_strategy_eligibility({},0,id);});refuses([&]{(void)inspect_bnw_city_current_strategy_eligibility(host,-1,id);});refuses([&]{(void)inspect_bnw_city_current_strategy_eligibility(host,0,INT64_MAX);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)full.records();});refuses([&]{(void)index.records();});auto fresh=inspect_bnw_city_current_strategy_eligibility(host,0,id);need(fresh.records().size()==records.size(),"Healthy replacement lost current eligibility");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)fresh.records();});refuses([&]{(void)inspect_bnw_city_current_strategy_eligibility(host,0,id);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_strategy_eligibility(host,0,id);});
 std::cout<<"actual strategy bindings="<<records.size()<<", start obligations="<<starts<<", end obligations="<<ends<<"; triggers unexecuted\n";
 }
 // The first block and all its query/VM/session/DB aliases are GONE. Only this
 // query may retain second Host parents after the caller drops its strong host.
 auto parent=boot(save,game,alternate);need(parent->active_player_id()==alternate,"Actual alternate viewer was not selected");auto retained_query=inspect_bnw_city_current_strategy_eligibility(parent,0,0);const auto count=retained_query.records().size();std::weak_ptr<ui::SelectedBnwPreviewHost> weak(parent);parent.reset();need(!weak.expired()&&retained_query.records().size()==count,"Actual strong parents not retained by query");
 auto actual_parent=weak.lock();need(bool(actual_parent),"Actual retained Host disappeared");bool denied=false;try{const_cast<content::RulesDatabase&>(actual_parent->database()).apply_sql("UPDATE AICityStrategies SET Type=Type WHERE ID=0");}catch(const std::exception&){denied=true;}need(denied,"Actual immutable rules writer accepted");refuses([&]{(void)retained_query.records();});refuses([&]{(void)inspect_bnw_city_current_strategy_eligibility(actual_parent,0,0);});
 need(identify_file(save).sha256==original,"Original save changed");
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentStrategyEligibility>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected SAVE and shared installation");math();actual(argv[1],argv[2]);std::cout<<"PASS current genuine strategy eligibility and native arithmetic boundaries; no trigger/Lua/flavor/RNG/CityEntry execution\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
