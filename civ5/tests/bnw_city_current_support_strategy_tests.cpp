#include "civ5/runtime/bnw_city_current_support_strategy.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/bnw_grand_world_averages.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <string_view>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
using T=BnwSupportCityStrategy;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const BnwCitySupportStrategyUnavailable&){rejected=true;}need(rejected,"Support strategy stale/generic/native operand admitted");}
template<class F>void exact(F&& f,std::string_view reason){bool rejected=false;try{f();}catch(const BnwCitySupportStrategyUnavailable& e){need(std::string_view(e.what())==reason,"Unexpected support strategy refusal");rejected=true;}need(rejected,"Support strategy missing owner substituted false");}
const auto& named(const auto& rows,const char* name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Fixture duplicate field");out=&r;}need(out,"Fixture actual field absent");return *out;}
std::int64_t scalar(const auto& fields,const char* name){const auto& f=named(fields,name);need(f.values.size()==1,"Fixture scalar absent");return f.values.front();}
void pure(){
 using Option=saves::PregameOption;
 const auto noesp=[](std::initializer_list<Option> rows){const std::vector<Option> options(rows);return detail::bnw_culture_saved_no_espionage_option(options);};
 need(!noesp({}).has_value(),"No saved option invented defaultfalse");
 need(noesp({{"GAMEOPTION_NO_ESPIONAGE",1}})==1,"Native exact option absent");
 need(noesp({{"GAMEOPTION_",2},{"GAMEOPTION_NO_ESPIONAGE",1}})==2,"Native prefix/first match lost");
 need(noesp({{"",-1},{"GAMEOPTION_NO_ESPIONAGE",1}})==-1,"Empty CustomOption first match lost");
 need(noesp({{"GAMEOPTION_NO_ESPIONAGE",0},{"GAMEOPTION_NO_ESPIONAGE",1}})==0,"Duplicate option selected last");
 need(!noesp({{"GAMEOPTION_NO_ESPIONAGE_EXTRA",1}}),"Longer saved key matched terminating NUL");
 need(!noesp({{"GAMEOPTION_PITBOSS",1}}),"Wrong string option granted NO_ESPIONAGE");
 const std::string embedded("GAMEOPTION_\0ignored",19);
 need(noesp({{embedded,1},{"GAMEOPTION_NO_ESPIONAGE",0}})==1,"Native first NUL prefix lost");
 const std::string trailing=std::string("GAMEOPTION_NO_ESPIONAGE")+std::string(1,'\0')+std::string(100,'x');
 need(noesp({{trailing,1}})==1,"Post-NUL wire bytes incorrectly exceeded Read64");
 need(noesp({{std::string(63,'x'),1},{"GAMEOPTION_NO_ESPIONAGE",2}})==2,"Native63-byte valid nonmatch refused");
 refuses([&]{(void)noesp({{std::string(64,'x'),1}});});
 refuses([&]{(void)noesp({{"GAMEOPTION_NO_ESPIONAGE",1},{std::string(64,'x'),0}});});
 refuses([&]{(void)noesp({{"GAMEOPTION_NO_ESPIONAGE",1},{"other",INT64_MAX}});});
 refuses([&]{(void)noesp({{"other",INT64_MIN}});});
 for(const auto value:std::array<std::int64_t,5>{-1,0,1,2,INT32_MAX}){const auto actual=noesp({{"GAMEOPTION_NO_ESPIONAGE",value}});need(actual.has_value()&&(*actual==1)==(value==1),"Native Game.isOption equality1 replaced by nonzero");}
 using Four=std::array<std::optional<std::int64_t>,4>;
 need(!detail::bnw_need_tourism_four_terms_math(Four{1,2,3,4}),"Tourism strict10 threshold changed");
 need(detail::bnw_need_tourism_four_terms_math(Four{1,2,3,5}),"Tourism11 positive lost");
 need(!detail::bnw_need_tourism_four_terms_math(Four{-1,-2,-3,-4}),"Tourism negative total clamped");
 refuses([&]{(void)detail::bnw_need_tourism_four_terms_math(Four{INT32_MAX,1,-INT32_MAX,0});});
 refuses([&]{(void)detail::bnw_need_tourism_four_terms_math(Four{INT32_MIN,-1,INT32_MAX,0});});
 refuses([&]{(void)detail::bnw_need_tourism_four_terms_math(Four{INT64_MAX,0,0,0});});
 refuses([&]{(void)detail::bnw_need_tourism_four_terms_math(Four{1,2,{},99});});
 // Even an already-positive prefix cannot skip a later mandatory getter.
 refuses([&]{(void)detail::bnw_need_tourism_four_terms_math(Four{11,0,{},0});});
 using S=detail::BnwGpStrategyMathSlot;std::array<S,2> rows{};
 need(!detail::bnw_good_gp_strategy_math(rows),"Null Info demanded count");rows[0].info_present=true;rows[0].unit_class=-1;need(!detail::bnw_good_gp_strategy_math(rows),"NO_UNITCLASS demanded count");
 auto& r=rows[0];r.unit_class=1;r.count=0;r.rate=3;r.building_rate=0;need(!detail::bnw_good_gp_strategy_math(rows),"Nonpositive change demanded modifiers");r.count=2;r.city_modifier=0;r.player_modifier=0;r.class_kind=0;need(!detail::bnw_good_gp_strategy_math(rows),"600 threshold wrong");r.building_rate=2;need(detail::bnw_good_gp_strategy_math(rows),"800 threshold lost");
 r.building_rate=0;r.city_modifier=25;r.player_modifier=25;need(detail::bnw_good_gp_strategy_math(rows),"City/player modifiers lost");r.city_modifier=r.player_modifier=0;r.class_kind=1;r.trait_scientist_modifier=25;r.class_modifier=25;need(detail::bnw_good_gp_strategy_math(rows),"Scientist two modifiers lost");
 r.class_kind=2;r.class_modifier=0;r.trait_scientist_modifier.reset();need(!detail::bnw_good_gp_strategy_math(rows),"Writer demanded scientist trait");r.class_kind=0;r.count=-1; r.city_modifier.reset();r.player_modifier.reset();need(!detail::bnw_good_gp_strategy_math(rows),"Negative change demanded modifiers");
 r.count=INT32_MAX;refuses([&]{(void)detail::bnw_good_gp_strategy_math(rows);});r.count=8;r.rate=1;r.city_modifier=r.player_modifier=0;r.building_rate=0;rows[1]=r;rows[1].count=INT32_MAX;refuses([&]{(void)detail::bnw_good_gp_strategy_math(rows);}); // no early success at 800
 rows[1]={};r.count=1;r.city_modifier=INT32_MAX;r.player_modifier=1;refuses([&]{(void)detail::bnw_good_gp_strategy_math(rows);});r.city_modifier=INT32_MAX;r.player_modifier=-INT32_MAX;r.class_kind=1;r.trait_scientist_modifier=INT32_MAX;r.class_modifier=1;refuses([&]{(void)detail::bnw_good_gp_strategy_math(rows);});
 using K=detail::BnwKeyScienceMathCity;std::array<K,2> key{{{true,{}},{false,101}}};
 need(!detail::bnw_key_science_strategy_math(-1,{},key,{}),"KeyScience sentinel demanded rate/flavor");
 need(!detail::bnw_key_science_strategy_math(0,100,key,10),"KeyScience self denominator/strict cutoff lost");
 need(detail::bnw_key_science_strategy_math(0,100,key,11),"KeyScience binary32 flavor cutoff lost");
 const std::array<K,3> hole{{{true,{}},{false,101},{false,INT64_MAX,false}}};need(!detail::bnw_key_science_strategy_math(0,100,hole,10),"FLTA null slot counted or demanded rate");
 key[1].science_times100=100;need(detail::bnw_key_science_strategy_math(0,100,key,1),"Equal city science counted better");
 need(!detail::bnw_key_science_strategy_math(0,100,key,0),"Zero flavor admitted equality");need(!detail::bnw_key_science_strategy_math(-2,100,key,-1),"Negative flavor/start semantics changed");
 need(!detail::bnw_key_science_strategy_math(0,100,{},{}),"Empty traversal demanded flavor");
 refuses([&]{(void)detail::bnw_key_science_strategy_math(INT64_MAX,{},key,{});});
 refuses([&]{(void)detail::bnw_key_science_strategy_math(0,INT64_MIN,key,{});});
 key[1].science_times100=INT64_MAX;refuses([&]{(void)detail::bnw_key_science_strategy_math(0,100,key,11);});
 key[1].science_times100=0;refuses([&]{(void)detail::bnw_key_science_strategy_math(0,100,key,INT64_MAX);});
 using W=detail::BnwSpecialistMathRow;const std::vector<W> wire{{{},{}},{1,2},{99,7},{1,4},{2,INT32_MAX},{0,INT32_MAX}};
 const auto dense=detail::bnw_specialist_reset_read_dense_math(4,wire);need(dense==std::vector<std::int32_t>({INT32_MAX,4,INT32_MAX,0}),"Dense Reset holes/unknown/last assignment lost");bool aggregate=false;try{(void)detail::bnw_specialist_reset_read_math(4,-1,wire);}catch(const BnwCitySpecialistCountsUnavailable&){aggregate=true;}need(aggregate,"Per-index capture smuggled aggregate overflow");
 need(!detail::bnw_many_techs_stolen_math(5,0,{}),"Started0 demanded robbed");refuses([&]{(void)detail::bnw_many_techs_stolen_math(INT32_MIN,1,0);});refuses([&]{(void)detail::bnw_many_techs_stolen_math(INT64_MAX,0,{});});
 need(detail::bnw_many_techs_stolen_math(5,5,1),"Positive/zero IEEE infinity lost");need(!detail::bnw_many_techs_stolen_math(5,5,0),"Zero/zero NaN yielded true");need(!detail::bnw_many_techs_stolen_math(4,5,1),"Negative ratio clamped positive");need(detail::bnw_many_techs_stolen_math(4,5,-1),"Native negative/negative float ratio changed");
 const MapGeometry invalid{};need(detail::bnw_good_airlift_strategy_math(invalid,true,{},{},{},{},{},{}),"Capital demanded map");need(detail::bnw_good_airlift_strategy_math(invalid,false,1,2,{},{},{},{}),"Area mismatch demanded coordinates");
 const MapGeometry g{{64,32},false,false,saves::GameStateLayout::expansion2_version1};need(!detail::bnw_good_airlift_strategy_math(g,false,1,1,0,0,20,0),"Distance20 wrong");need(detail::bnw_good_airlift_strategy_math(g,false,1,1,0,0,21,0),"Distance21 lost");const MapGeometry wrap{{64,32},true,false,g.layout};need(!detail::bnw_good_airlift_strategy_math(wrap,false,1,1,0,0,63,0),"Native wrapped distance lost");refuses([&]{(void)detail::bnw_good_airlift_strategy_math(g,false,1,1,INT64_MAX,0,0,0);});
 std::cout<<"PASS detached KeyScience/GP/dense/read-order/native overflow/Many float/Airlift tests; no issuer\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(b.preview.host));need(bool(host),"Actual selected Host absent");return host;}
bool culture_tourism_frontier(std::string_view why){
 return why=="Support NeedCultureBuilding reached additional AND-tech/obsolete/replacement/CanConstruct admission owner unavailable"||
 why=="Support Culture/Tourism missing reached policy flag requires actual native Reset/Read owner"||
 why=="Support Culture/Tourism missing reached HasTech slot requires actual native TEAM Reset/Read owner"||
 why=="Support Culture/Tourism reached actual League member/resolution yield owner unavailable"||
 why=="Support NeedTourismBuilding reached hidden-resource actual normal/debug owner join unavailable"||
 why=="Support NeedTourismBuilding reached SECOND ComputeCultureFromAdjacentImprovement source owner unavailable"||
 why=="Current BaseTourism missing reached policy flag requires actual native Reset/Read owner"||
 why=="Current BaseTourism missing reached HasTech slot requires actual native TEAM Reset/Read owner"||
 why=="Grand tourism GetThemingBonus requires actual theming index/great-work metadata/policy owner"||
 why=="Grand tourism positive LandmarksTourismPercent requires actual GetCultureFromWonders, natural-wonder and improvement queries"||
 why=="Grand tourism GetReligion returned actual beliefs; FaithBuildingTourism/building-class tourism owner required"||
 why=="Grand tourism GetCityTourismModifier requires actual member/active-resolution holy-city owner";
}
std::pair<std::optional<bool>,std::string> culture_tourism_result(const BnwCityCurrentSupportStrategy& q,T trigger){
 try{return {q.value(trigger),{}};}catch(const BnwCitySupportStrategyUnavailable& e){need(culture_tourism_frontier(e.what()),"Unexpected genuine Culture/Tourism failure");return {{},e.what()};}
}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);const auto& s=host->session().state();need(bool(s.world.all_players),"Actual players absent");
 need(s.world.pregame&&s.world.pregame->layout==saves::GameStateLayout::expansion2_version1&&s.world.pregame->slots_version==3&&s.world.pregame->archive_version==6,"Genuine native Pregame option Read descriptor absent");
 const auto option_generation=host->session().state_generation();const auto& actual_options=s.world.pregame->game_options;const auto saved_noesp=detail::bnw_culture_saved_no_espionage_option(actual_options);
 need(host->session().state_generation()==option_generation,"Detached Read witness published option state");
 std::cout<<"Genuine raw NO_ESPIONAGE Read rows="<<actual_options.size()<<" saved_first_match="<<saved_noesp.has_value()<<" saved_enabled="<<(saved_noesp&&*saved_noesp==1)<<"; default/CanConstruct branches counted by retained query, no detached issuance\n";std::array<std::size_t,3> positive{};std::array<std::size_t,2> ct_false{},ct_true{},ct_frontier{};std::size_t seen=0,key_false=0,key_frontier=0,key_read=0,key_positive=0,major_frontier=0;std::int64_t first_owner=-1,first_city=-1;bool first=false;
 for(const auto& p:s.world.all_players->players){for(const auto& c:p.cities.cities){if(seen==24){break;}const auto owner=scalar(c.core.fields,"m_eOwner"),id=scalar(c.core.fields,"m_iID");need(id>=0&&id<=INT32_MAX,"Actual city native ID absent");auto full=inspect_bnw_city_current_support_strategy(host,owner,id),index=inspect_bnw_city_current_support_strategy(host,owner,id&8191);
  for(std::size_t t=0;t<2;++t){const auto trigger=t?T::good_gp_city:T::good_airlift_city;const bool value=full.value(trigger);need(index.value(trigger)==value,"Genuine full/index support predicate differs");if(value){++positive[t];}}
  const auto& ai=named(p.espionage.blocks,"espionage_ai");const auto started=named(ai.fields,"m_iTurnEspionageStarted").value;
  const auto key_query=[&](const auto& query)->std::pair<std::optional<bool>,std::string>{try{return {query.value(T::key_science_city),{}};}catch(const BnwCitySupportStrategyUnavailable& error){const std::string reason=error.what();need(reason=="Current SCIENCE nonzero process requires actual PRODUCTION recursion owner"||reason=="Current PRODUCTION recursive self-process unavailable before base"||reason=="Current SCIENCE older city schema requires native YieldPerReligion Reset owner","Unexpected genuine KeyScience refusal");return {{},reason};}};
  const auto a=key_query(full),b=key_query(index);need(a==b,"Genuine KeyScience full/index result/frontier differs");
  if(started==-1){need(a.first&&!*a.first,"KeyScience sentinel demanded SCIENCE");++key_false;}
  else if(a.first){++key_read;if(*a.first){++key_positive;}}else{++key_frontier;}

  if(started==0||owner<22){const bool value=full.value(T::many_techs_stolen);need(value==index.value(T::many_techs_stolen),"Genuine full/index Many differs");std::optional<std::int64_t> robbed;if(started!=0){robbed=named(named(c.tail.blocks,"espionage").children,"m_aiNumTimesCityRobbed").values.at(static_cast<std::size_t>(owner));}need(value==detail::bnw_many_techs_stolen_math(s.setup.game_turn,started,robbed),"Actual wire Many witness differs");if(value){++positive[2];}}else{exact([&]{(void)full.value(T::many_techs_stolen);},"Support ManyTechsStolen native robbed array owner index outside major22");++major_frontier;full=inspect_bnw_city_current_support_strategy(host,owner,id);}
  for(std::size_t n=0;n<2;++n){const auto trigger=n?T::need_tourism_building:T::need_culture_building;const auto a=culture_tourism_result(full,trigger),b=culture_tourism_result(index,trigger);need(a==b,"Genuine Culture/Tourism full/index value or exact frontier differs");if(!a.first){++ct_frontier[n];}else if(*a.first){++ct_true[n];}else{++ct_false[n];}}
  need(full.value(T::good_airlift_city)==index.value(T::good_airlift_city),"Named frontier retired healthy sibling");
  const auto stale=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;refuses([&]{(void)inspect_bnw_city_current_support_strategy(host,owner,stale);});if(!seen){first_owner=owner;first_city=id;first=full.value(T::good_airlift_city);}++seen;
 }if(seen==24){break;}}
 need(seen>0,"No actual support cities");for(std::size_t n=0;n<2;++n)std::cout<<"Genuine "<<(n?"Tourism":"Culture")<<" false="<<ct_false[n]<<" true="<<ct_true[n]<<" named_frontier="<<ct_frontier[n]<<(ct_true[n]?" positive observed":" positive UNOBSERVED")<<'\n';for(std::size_t t=0;t<positive.size();++t)std::cout<<"Genuine support"<<t<<" positives="<<positive[t]<<(positive[t]?" observed":" UNOBSERVED")<<'\n';std::cout<<"KeyScience sentinelfalse="<<key_false<<" numeric_reads="<<key_read<<" positives="<<key_positive<<(key_positive?" observed":" UNOBSERVED")<<" exact_process_or_schema_frontier="<<key_frontier<<" Many major-index frontier="<<major_frontier<<'\n';
 auto child=inspect_bnw_city_current_base_tourism(host,first_owner,first_city);
 bool child_thread=false;std::thread child_worker([&]{try{(void)child.value();}catch(const BnwCityCurrentBaseTourismUnavailable&){child_thread=true;}});child_worker.join();need(child_thread,"BaseTourism child accepted foreign thread");
 bool child_empty=false;try{(void)inspect_bnw_city_current_base_tourism({},0,0);}catch(const BnwCityCurrentBaseTourismUnavailable&){child_empty=true;}need(child_empty,"BaseTourism accepted generic absent Host");
 auto retained=inspect_bnw_city_current_support_strategy(host,first_owner,first_city);bool read=false,bind=false;std::thread worker([&]{try{(void)retained.value(T::good_airlift_city);}catch(const BnwCitySupportStrategyUnavailable&){read=true;}try{(void)inspect_bnw_city_current_support_strategy(host,first_owner,first_city);}catch(const BnwCitySupportStrategyUnavailable&){bind=true;}});worker.join();need(read&&bind&&retained.value(T::good_airlift_city)==first,"Foreign call poisoned healthy owner");
 refuses([&]{(void)inspect_bnw_city_current_support_strategy({},0,0);});refuses([&]{(void)inspect_bnw_city_current_support_strategy(host,INT64_MAX,first_city);});refuses([&]{(void)inspect_bnw_city_current_support_strategy(host,first_owner,INT64_MIN);});const auto cp=host->session().checkpoint();host->session().restore(cp);bool child_stale=false;try{(void)child.value();}catch(const BnwCityCurrentBaseTourismUnavailable&){child_stale=true;}need(child_stale,"BaseTourism child survived allocation restore");refuses([&]{(void)retained.value(T::good_airlift_city);});auto restored=inspect_bnw_city_current_support_strategy(host,first_owner,first_city);need(restored.value(T::good_airlift_city)==first,"Restore changed current source predicate");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::good_airlift_city);});refuses([&]{(void)inspect_bnw_city_current_support_strategy(host,first_owner,first_city);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_support_strategy(host,first_owner,first_city);});moved.reset();host.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_support_strategy(other,first_owner,first_city);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::good_airlift_city)==first,"Query lost sole real Host/session/DB parent");auto writer=weak.lock();need(bool(writer),"Retained Host absent");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::good_gp_city);});need(identify_file(save).sha256==before,"Original save changed");std::cout<<"PASS genuine support predicates cities="<<seen<<"; no activation/refresh/phase permission\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentSupportStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and shared installation");pure();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
