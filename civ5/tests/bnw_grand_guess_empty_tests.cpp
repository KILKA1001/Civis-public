#include "civ5/runtime/bnw_grand_guess_empty.hpp"
#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <type_traits>
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){for(auto &r:rows)if(r.name&&name==r.name)return r;throw std::runtime_error("Guess fixture source field missing");}
auto &grand(auto &w){return named(w.world.all_players->players.at(0).strategy.blocks,"grand_strategy_ai");}
auto &tech(auto &w){for(auto &t:w.technologies)if(t.team()==0)return t;throw std::runtime_error("Guess fixture current tech owner missing");}
void rejects(auto &&f,std::string_view required={}){bool caught=false;std::string why;try{f();}catch(const std::exception &e){caught=true;why=e.what();}check(caught,"Incomplete/stale Guess produced private completion");if(!required.empty())check(why.find(required)!=std::string::npos,why.c_str());}
}
int main(int argc,char **argv)try{
#ifdef _WIN32
 ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#endif
 static_assert(!std::is_default_constructible_v<BnwGrandWorldAverageJournal>);
 static_assert(!std::is_default_constructible_v<BnwGrandGuessEmptyJournal>);
 check(argc==3||argc==4,"Expected selected save installation [SDKroot]");
 if(argc==4)for(const auto &pin:kBnwGrandGuessEmptySources)check(identify_file(std::filesystem::path(argv[3])/pin.path).sha256==pin.sha256,"Grand DoGuess source pin differs");
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Selected preview unavailable");auto &host=*selected.preview.host;
 const auto lease=host.session().read_state_lease();const auto &original=*lease;const auto generation=host.session().state_generation();check(host.active_player_id()==0,"Fixture requires actual Gandhi");
 const auto identity=host.identity();const auto native=BnwHappinessSuffixRules::load(host.database());const auto rules=BnwGrandWorldAverageRules::load(host.database());
 // Explicit isolated current-registry/post-deactivation test operands. This
 // does not restore stock Lua or claim genuine deactivation/activation entry.
 auto w=original;for(auto &p:w.world.all_players->players)named(p.core.fields,"m_bTurnActive").values[0]=0;
 auto &g=grand(w);named(g.fields,"active_strategy").value=1;named(g.fields,"turns_since_active_set").value=7;
 named(g.arrays,"priorities").values={4,9,-1,11};
 auto &guesses=named(g.arrays,"other_player_strategy_guesses").values;auto &confidence=named(g.arrays,"guess_confidence").values;
 guesses[0]=2;confidence[0]=2;guesses[1]=0;confidence[1]=1;guesses[21]=3;confidence[21]=0;
 const auto retained_guesses=guesses,retained_confidence=confidence;
 const auto prime=[&](LiveSessionState &s){auto prefix=stage_bnw_player_do_turn_prefix(s,host.database(),identity,0,0);auto revolts=stage_bnw_player_do_turn_revolts(s,prefix,native,host.database(),identity,0);auto trade=stage_bnw_player_do_turn_trade(s,revolts,native,host.database(),identity,0);auto military=stage_bnw_player_do_turn_military(s,trade,native,host.database(),identity,0);auto average=stage_bnw_grand_military_average(s,trade,military,native,host.database(),identity,0);return std::tuple{std::move(trade),std::move(military),std::move(average)};};
 const auto parents=prime(w);const auto &[trade,military,military_average]=parents;
 const auto averages=stage_bnw_grand_world_average_journal(w,trade,military,military_average,rules,native,host.database(),identity,0);
 check(averages.averages().calculated&&averages.averages().culture_average==0&&averages.averages().tourism_average==0&&averages.averages().tech_average==1,"Sealed source producer did not perform actual selected-city averages");
 const auto before=bnw_grand_native_component_revision(w,0),query=bnw_grand_world_average_inputs_revision(w);const auto game_rng=w.random.gameplay.state(),map_rng=w.random.map.state();
 const auto completed=stage_bnw_grand_guess_empty(w,trade,military,military_average,averages,rules,native,host.database(),identity,0);
 check(completed.player()==0&&completed.inspected().size()==22&&completed.grand_revision()==before&&completed.query_revision()==query,"Source22loop evidence missing");
 std::size_t foreign=0;for(const auto &p:completed.inspected()){if(!p.alive||p.self)check(!p.team&&!p.met,"Source short circuit read dead/self team/met");else{++foreign;check(p.player==1&&p.team==1&&p.met==false,"Selected actual foreign major incorrectly eligible");}}
 check(foreign==1&&named(grand(w).arrays,"other_player_strategy_guesses").values==retained_guesses&&named(grand(w).arrays,"guess_confidence").values==retained_confidence&&bnw_grand_native_component_revision(w,0)==before&&bnw_grand_world_average_inputs_revision(w)==query&&w.random.gameplay.state()==game_rng&&w.random.map.state()==map_rng,"Empty source loop reset native Grand values or mutated query/RNG");
 std::cout<<"source empty22 retained current nondefault guesses/priorities PASS\n";
 // Late stale guards use one accepted average, no formula or scheduler replay.
 for(int scenario=0;scenario<12;++scenario){auto stale=w;auto id=identity;
  if(scenario==0)++named(grand(stale).arrays,"other_player_strategy_guesses").values[1];
  if(scenario==1)++named(grand(stale).arrays,"priorities").values[0];
  if(scenario==2)named(stale.world.teams->teams.at(0).arrays,"has_met").values[1]=1;
  if(scenario==3)stale.setup.players.at(0).policies_resolved=false;
  if(scenario==4)++named(named(stale.world.all_players->players.at(0).cities.cities.at(0).production.blocks,"buildings").fields,"m_iGreatWorksTourismModifier").value;
  if(scenario==5)++named(stale.world.map_plots->plots.at(0).fields,"owner").value;
  if(scenario==6)tech(stale).increment_count(0);
  if(scenario==7)++stale.military_request_counters.at(0).land;
  if(scenario==8)(void)stale.random.gameplay.draw(7);
  if(scenario==9)id.payload_sha256[0]=id.payload_sha256[0]=='0'?'1':'0';
  if(scenario==10)stale.world.map_plots.reset();
  if(scenario==11)stale.world.all_players->players.at(0).cities.cities.at(0).core.layout=saves::GameStateLayout::vp542_corpus;
  const auto sg=stale.random.gameplay.state(),sm=stale.random.map.state();
  rejects([&]{(void)stage_bnw_grand_guess_empty(stale,trade,military,military_average,averages,rules,native,host.database(),id,0);});
  check(stale.random.gameplay.state()==sg&&stale.random.map.state()==sm,"Rejected Guess consumed RNG");
 }
 // Fresh averages cannot bless a malformed Grand native Read component.
 for(int scenario=0;scenario<5;++scenario){auto malformed=w;auto &m=grand(malformed);
  if(scenario==0)named(m.fields,"version").value=2;
  if(scenario==1)named(m.arrays,"other_player_strategy_guesses").values.pop_back();
  if(scenario==2)named(m.arrays,"guess_confidence").values[1]=3;
  if(scenario==3)named(m.fields,"turns_since_active_set").value=-1;
  if(scenario==4)malformed.world.all_players->players.at(0).strategy.dimensions.grand=5;
  if(scenario==4){rejects([&]{(void)stage_bnw_grand_world_average_journal(malformed,trade,military,military_average,rules,native,host.database(),identity,0);},"Military component/input seal stale");continue;}
  const auto fresh=stage_bnw_grand_world_average_journal(malformed,trade,military,military_average,rules,native,host.database(),identity,0);
  rejects([&]{(void)stage_bnw_grand_guess_empty(malformed,trade,military,military_average,fresh,rules,native,host.database(),identity,0);},"Grand guess");
 }
 // Positive eligible branch: no constant priorities or empty-loop shortcut.
 auto eligible=original;for(auto &p:eligible.world.all_players->players)named(p.core.fields,"m_bTurnActive").values[0]=0;
 named(eligible.world.teams->teams.at(0).arrays,"has_met").values[1]=1;
 const auto elapsed=std::get<saves::Expansion2PrefixObservation>(eligible.world.game_state.prefix).scalars.at(1);
 named(eligible.world.all_players->players.at(1).core.fields,"m_iTurnMightRecomputed").values[0]=elapsed; // explicitly supplied cached fixture operand
 const auto eligible_parents=prime(eligible);const auto &[et,em,ea]=eligible_parents;
 const auto eavg=stage_bnw_grand_world_average_journal(eligible,et,em,ea,rules,native,host.database(),identity,0);
 std::string first_type;for(const auto &[key,type]:rules.types().entries()){(void)key;if(type.table=="AIGrandStrategies"&&type.id==0){check(first_type.empty(),"Duplicate current first strategy Info");first_type=type.type;}}check(!first_type.empty(),"Current first strategy Info absent");
 const auto eg=bnw_grand_native_component_revision(eligible,0);const auto er=eligible.random.gameplay.state();
 rejects([&]{(void)stage_bnw_grand_guess_empty(eligible,et,em,ea,eavg,rules,native,host.database(),identity,0);},"first eligible major=1 requires actual source strategy id=0 type="+first_type);

 check(bnw_grand_native_component_revision(eligible,0)==eg&&eligible.random.gameplay.state()==er,"Positive mandatory producer refusal changed arrays/RNG");
 auto wrong_identity=identity;wrong_identity.rules_sha256[0]=wrong_identity.rules_sha256[0]=='0'?'1':'0';rejects([&]{(void)stage_bnw_grand_world_average_journal(w,trade,military,military_average,rules,native,host.database(),wrong_identity,0);},"identity");
 auto absent=w;absent.world.all_players.reset();rejects([&]{(void)bnw_grand_native_component_revision(absent,0);},"unavailable");rejects([&]{(void)bnw_grand_native_component_revision(w,22);},"unavailable");
 check(host.session().state_generation()==generation&&original.random.gameplay.state()==lease->random.gameplay.state(),"Private fixture published original session");
 std::cout<<"PASS sealed actual city average once -> source empty22 Guess; retained nondefault arrays, late-stale/malformed/positive eligible guards; isolated current-registry fixture, no Grand/fullturn/readiness grant\n";
}catch(const std::exception &e){std::cerr<<"Grand empty Guess FAIL: "<<e.what()<<'\n';return 1;}
