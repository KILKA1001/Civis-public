#include "civ5/runtime/bnw_grand_first_priority.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <climits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){for(auto &r:rows)if(r.name&&name==r.name)return r;throw std::runtime_error("Priority fixture field absent");}
auto &manager(auto &w){return named(w.world.all_players->players.at(0).knowledge.blocks,"flavor_manager");}
void blocked(const auto &q,std::string_view reason){check(!q.category_quoted&&!q.category&&!q.before_random,"Incomplete first priority supplied a result before mandatory query");check(q.blocker.find(reason)!=std::string::npos,q.blocker.c_str());}
void rejects(auto &&f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Flavor source/owned malformed registry accepted");}
}
int main(int argc,char **argv)try{
#ifdef _WIN32
 ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#endif
 check(argc==3||argc==4,"Expected selected save installation [SDKroot]");
 if(argc==4)for(const auto &pin:kBnwGrandPrioritySources)check(identify_file(std::filesystem::path(argv[3])/pin.path).sha256==pin.sha256,"Grand priority native source pin differs");
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Selected preview absent");auto &host=*selected.preview.host;const auto lease=host.session().read_state_lease();const auto &original=*lease;const auto generation=host.session().state_generation();const auto identity=host.identity();const auto native=BnwHappinessSuffixRules::load(host.database());const auto rules=BnwGrandPriorityRules::load(host.database());const auto city_rules=BnwGrandWorldAverageRules::load(host.database());
 for(const auto &entry:rules.strategies())std::cout<<"current Grand Info ID="<<entry.id<<" type="<<entry.type<<"\n";
 const auto first=quote_bnw_grand_first_priority(original,rules,native,host.database(),identity,0,nullptr,&city_rules);
 const auto *culture_flavor=native.types().find("FLAVOR_CULTURE");check(culture_flavor&&culture_flavor->id>=0,"Actual Culture flavor absent");const auto &initial_personality=checked_bnw_personality_flavors(original.world.all_players->players.at(0),original.world.all_players->players.at(0),host.database());
 check(first.strategy==0&&first.strategy_type=="AIGRANDSTRATEGY_CULTURE"&&first.base_quoted&&first.culture_prefix_quoted&&first.culture_initial==initial_personality.at(culture_flavor->id)*20&&first.total_culture_option_anarchy_checked&&first.culture_cities&&first.culture_cities_visited==1&&first.flavor_slots_visited==rules.flavor_slots(),first.blocker.c_str());
 blocked(first,"GetTotalJONSCulturePerTurn");
 std::int64_t expected_base=0;std::size_t expected_nonzero=0;for(std::size_t f=0;f<rules.flavor_slots();++f){const auto weight=rules.strategies()[3].flavors[f];if(weight!=0){++expected_nonzero;expected_base+=weight*initial_personality[f];}}
 const auto quote=quote_bnw_grand_conquest_priority(original,rules,native,host.database(),identity,0);check(quote.category_quoted,quote.blocker.c_str());
 check(quote.strategy==3&&quote.strategy_type=="AIGRANDSTRATEGY_CONQUEST"&&quote.elapsed==0&&quote.base_quoted&&quote.base==expected_base&&quote.category==60&&quote.before_random==expected_base+60&&!quote.nobody_met_query&&!quote.military_ratio_query&&quote.flavor_slots_visited==rules.flavor_slots()&&quote.nonzero_flavors==expected_nonzero&&quote.war_teams_visited==63&&quote.nuke_major_slots_visited==22,"Actual independent ID3 GetBase+Conquest source query differs");
 // Disabled Culture is a genuine native early return after the base getter.
 {auto disabled=original;const auto *culture=native.types().find("VICTORY_CULTURAL");check(culture&&culture->table=="Victories","Actual Culture victory Info missing");disabled.world.pregame->archive.flag_arrays.at("victories")[culture->id]=false;
 const auto q=quote_bnw_grand_first_priority(disabled,rules,native,host.database(),identity,0);check(q.base_quoted&&q.category_quoted&&q.culture_victory_disabled&&q.category==-100&&q.before_random==*q.base-100&&!q.culture_initial&&!q.culture_prefix_quoted,"Disabled Culture did not return before later getters");}
 // Native option/anarchy early returns retain the real base but still
 // require downstream tourism/foreign culture/influence before any JonRand.
 for(int scenario=0;scenario<5;++scenario){auto w=original;
  if(scenario==0){w.world.pregame->game_options.insert(w.world.pregame->game_options.begin(),{"GAMEOPTION_NO_POLICIES",1});w.setup.game_options.insert(w.setup.game_options.begin(),{"GAMEOPTION_NO_POLICIES",1});}
  if(scenario==1)named(w.world.all_players->players.at(0).core.fields,"m_iAnarchyNumTurns").values[0]=1;
  if(scenario==2){w.world.pregame->game_options.insert(w.world.pregame->game_options.begin(),{"GAMEOPTION_NO_POLICIES",1});w.setup.game_options.insert(w.setup.game_options.begin(),{"GAMEOPTION_NO_POLICIES",0});}
  if(scenario==3)named(w.world.teams->teams[0].fields,"era").value=999;
  if(scenario==4)++named(w.world.all_players->players[0].cities.cities[0].core.fields,"m_aiBaseYieldRateFromTerrain").values[4];
  const auto q=quote_bnw_grand_first_priority(w,rules,native,host.database(),identity,0,nullptr,&city_rules);
  if(scenario<2){blocked(q,"GetTourism");check(q.base_quoted&&q.culture_prefix_quoted&&q.own_culture==0&&!q.culture_cities&&q.total_culture_option_anarchy_checked,"No-policies/anarchy skipped native early return or claimed category completion");}
  else blocked(q,scenario==2?"option mirror stale":scenario==3?"team era Info":"terrain stale");
 }
 // Current-world personality is not frozen to original-import values.
 {auto db=host.database().clone_gameplay();db->apply_sql("DELETE FROM AIGrandStrategy_Flavors WHERE AIGrandStrategyType='AIGRANDSTRATEGY_CULTURE'; INSERT INTO AIGrandStrategy_Flavors (AIGrandStrategyType,FlavorType,Flavor) VALUES ('AIGRANDSTRATEGY_CULTURE','FLAVOR_CULTURE',3);");
 const auto r=BnwGrandPriorityRules::load(*db);const auto n=BnwHappinessSuffixRules::load(*db);const auto c=BnwGrandWorldAverageRules::load(*db);auto ident=identity;ident.rules_sha256=r.identity();
 auto w=original;named(manager(w).arrays,"personality_flavors").values.at(culture_flavor->id)=9;
 const auto q=quote_bnw_grand_first_priority(w,r,n,*db,ident,0,nullptr,&c);blocked(q,"source cities complete");check(q.base_quoted&&q.base==27&&q.culture_initial==180&&q.culture_cities&&q.flavor_slots_visited==r.flavor_slots()&&q.nonzero_flavors==1,"Actual slot0 base/current Culture flavor native prefix wrong");}
 std::cout<<"selected actual ID0 base="<<*first.base<<" Culture flavor prefix="<<*first.culture_initial<<" cities="<<*first.culture_cities<<"; independent ID3 base="<<expected_base<<" category="<<*quote.category<<"\n";
 const auto game_rng=original.random.gameplay.state(),map_rng=original.random.map.state();
 for(int scenario=0;scenario<14;++scenario){auto w=original;auto &p=w.world.all_players->players.at(0);auto &d=p.diplomacy.fields;auto &own_team=w.world.teams->teams.at(0);const auto *domination=native.types().find("VICTORY_DOMINATION");check(domination&&domination->table=="Victories","Current domination Info absent");
  if(scenario==0){named(d,"m_paiPersonalityMajorCivApproachBiases").values[5]=2;named(d,"m_paiPersonalityMajorCivApproachBiases").values[6]=99;}
  if(scenario==1)named(own_team.arrays,"at_war").values[1]=1;
  if(scenario==2)named(p.core.fields,"m_bCramped").values[0]=1;
  if(scenario==3){named(own_team.arrays,"has_met").values[1]=1;named(w.world.all_players->players.at(1).core.fields,"m_iNumNukeUnits").values[0]=1;}
  if(scenario==4)named(p.core.fields,"m_iNumNukeUnits").values[0]=1;
  if(scenario==5)w.world.pregame->archive.flag_arrays.at("victories")[domination->id]=false;
  if(scenario==6){auto &v=w.world.pregame->archive.flag_arrays.at("victories");std::fill(v.begin(),v.end(),false);}
  if(scenario==7)w.world.pregame->archive.flag_arrays.erase("victories");
  if(scenario==8)named(own_team.fields,"era").value=999;
  if(scenario==9)named(own_team.arrays,"at_war").values[0]=1;
  if(scenario==10){std::get<saves::Expansion2PrefixObservation>(w.world.game_state.prefix).scalars[1]=60;w.maintenance.elapsed_turns=60;}
  if(scenario==11){named(p.core.fields,"m_bCramped").values[0]=1;++named(w.world.map_plots->plots.at(0).fields,"owner").value;}
  if(scenario==12)named(d,"m_paiPersonalityMajorCivApproachBiases").encoding=saves::DiplomacyEncoding::u8;
  if(scenario==13)named(d,"m_iBoldness").values[0]=INT32_MAX;
  const auto q=quote_bnw_grand_conquest_priority(w,rules,native,host.database(),identity,0);
  if(scenario<=6){check(q.category_quoted,q.blocker.c_str());const auto expected=scenario==0?96:scenario==1?70:scenario==2?80:scenario==3?10:scenario==5?-100:60;check(q.category==expected&&q.before_random==expected_base+expected,"Conquest source conditional/enum/war/cramped/nuke/victory branch differs");if(scenario==2)check(q.cramped_plots_visited==w.world.map_projection->cells.size(),"Positive cramped skipped actual map loop");if(scenario==4)check(!q.nuke_major_slots_visited,"Own positive nuke counter did not short circuit foreign loop");if(scenario==5)check(!q.war_teams_visited&&q.conquest_victory_disabled,"Disabled victory failed native early return");}
  else if(scenario==7)blocked(q,"victories inventory");
  else if(scenario==8)blocked(q,"team era Info");
  else if(scenario==9)blocked(q,"self-war");
  else if(scenario==10)blocked(q,"world military average/GetMilitaryMight calculator owner");
  else if(scenario==11)blocked(q,"raw/live plot owner");
  else if(scenario==12)blocked(q,"bias shape");
  else if(scenario==13)blocked(q,"overflow/width");
  check(w.random.gameplay.state()==game_rng&&w.random.map.state()==map_rng,"Source first category consumed RNG before mandatory first JonRand");
 }
 // Actual native SetFlavors default0 + last row assignment, saved/current
 // personality rather than Leader flavor defaults. Isolated SQL operands.
 auto db=host.database().clone_gameplay();db->apply_sql("DELETE FROM AIGrandStrategy_Flavors WHERE AIGrandStrategyType='AIGRANDSTRATEGY_CONQUEST'; INSERT INTO AIGrandStrategy_Flavors (AIGrandStrategyType,FlavorType,Flavor) VALUES ('AIGRANDSTRATEGY_CONQUEST','FLAVOR_OFFENSE',3);");
 const auto positive_rules=BnwGrandPriorityRules::load(*db);const auto positive_native=BnwHappinessSuffixRules::load(*db);auto positive_id=identity;positive_id.rules_sha256=positive_rules.identity();const auto *offense=positive_native.types().find("FLAVOR_OFFENSE");check(offense&&offense->table=="Flavors","Positive current flavor Info absent");
 auto w=original;auto &personality=named(manager(w).arrays,"personality_flavors").values;personality.at(offense->id)=9;
 auto q=quote_bnw_grand_conquest_priority(w,positive_rules,positive_native,*db,positive_id,0);check(q.category_quoted&&q.nonzero_flavors==1&&q.base==27&&q.category==60&&q.before_random==87,q.blocker.c_str());
 personality.at(offense->id)=INT32_MAX;q=quote_bnw_grand_conquest_priority(w,positive_rules,positive_native,*db,positive_id,0);blocked(q,"overflow/width");
 // Same generalized checks used by first-contact's exact nuke index getter.
 const auto &saved=original.world.all_players->players.at(0);const auto &actual=checked_bnw_personality_flavors(saved,saved,host.database());const auto *nuke=native.types().find("FLAVOR_USE_NUKE");check(nuke&&nuke->id>=0&&nuke->id<static_cast<std::int64_t>(actual.size()),"Current nuke flavor Info/array binding differs");check(actual.at(nuke->id)==named(named(saved.knowledge.blocks,"flavor_manager").arrays,"personality_flavors").values.at(nuke->id),"Generalized getter changed actual nuke flavor value");
 for(int scenario=0;scenario<6;++scenario){auto changed=saved;auto &f=named(changed.knowledge.blocks,"flavor_manager");if(scenario==0)++named(f.arrays,"personality_flavors").values.at(nuke->id);if(scenario==1)++named(f.arrays,"active_flavors").values[0];if(scenario==2)named(f.fields,"flavors_wire_count").value=0;if(scenario==3)named(f.arrays,"personality_flavors").columns=2;if(scenario==4)f.arrays.push_back(named(f.arrays,"personality_flavors"));if(scenario==5)changed.knowledge.layout=saves::GameStateLayout::vp542_corpus;rejects([&]{(void)checked_bnw_personality_flavors(saved,changed,host.database());});}
 check(host.session().state_generation()==generation&&original.random.gameplay.state()==game_rng&&original.random.map.state()==map_rng,"First-priority query published selected session");
 std::cout<<"PASS actual ID0 Culture base/prefix pending total culture; independent ID3 actual base+Conquest60 before RNG;14 source conditional/unknown guards, positive current personality/rules27+60, reused nuke equality/shape checks; query only/no Grand/fullturn readiness\n";
}catch(const std::exception &e){std::cerr<<"Grand first priority FAIL: "<<e.what()<<'\n';return 1;}
