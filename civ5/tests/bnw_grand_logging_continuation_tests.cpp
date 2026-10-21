#include "civ5/runtime/bnw_grand_logging_continuation.hpp"
#include <vector>
#include <stdexcept>
#include <iostream>
#include <type_traits>
using namespace civ5::runtime;
using namespace civ5::runtime::grand_logging_detail;
namespace {
unsigned checks=0;
void need(bool b,const char*s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void refuses(F f,const char*s){bool failed=false;try{f();}catch(const std::exception&){failed=true;}need(failed,s);}
struct Fixture {
 std::shared_ptr<Journal> j=std::make_shared<Journal>(true,1,std::array<std::int32_t,4>{3,-1,25,90},std::array<std::int32_t,4>{-7,0,-19,-42},true);
 std::shared_ptr<Source> s=std::make_shared<Source>();
 std::vector<std::string> events,files,rows;
 unsigned flags=0,elapsed=0,info=0;
 bool logging=true,ai=true,split=false,current=true;
 std::optional<std::size_t> unknown_row,throw_row;
 std::function<void()> msg_hook,current_hook;
 Fixture(){
  s->current=[this]{if(current_hook)current_hook();return current;};
  s->flag=[this](Kind k,Flag f)->std::optional<std::uint8_t>{
   ++flags;events.push_back(std::string(k==Kind::city?"C":"G")+(f==Flag::logging?"logging":f==Flag::ai?"ai":"split"));
   return static_cast<std::uint8_t>(f==Flag::logging?logging:f==Flag::ai?ai:split);
  };
  s->construct_strings=[this](Kind k){events.push_back(k==Kind::city?"Cstrings":"Gstrings");};
  s->destroy_strings=[this](Kind k){events.push_back(k==Kind::city?"Ccleanup":"Gcleanup");};
  s->civilization=[this](Kind k)->std::optional<Text>{events.push_back(k==Kind::city?"Cciv":"Gciv");return Text{TextKind::bytes,"Gandhi"};};
  s->get_log=[this](Kind k,const std::string&file){
   events.push_back(k==Kind::city?"Cgetlog":"Ggetlog");files.push_back(file);
   auto receiver=std::make_shared<Receiver>();
   receiver->msg=[this,k](const std::string&line){
    events.push_back(k==Kind::city?"Cmsg":"Gmsg");rows.push_back(line);
    if(msg_hook)msg_hook();
    if(k==Kind::grand&&throw_row&&j->progress().confirmed_grand_messages==*throw_row)throw std::runtime_error("sink wrote before failure");
   };return receiver;
  };
  s->strategy_count=[this]()->std::optional<std::int32_t>{events.push_back("Gcount");return 4;};
  s->elapsed=[this](Kind k,std::size_t)->std::optional<std::int32_t>{++elapsed;events.push_back(k==Kind::city?"Celapsed":"Gelapsed");return 7;};
  s->strategy_type=[this](Kind k,std::int32_t slot)->std::optional<Text>{
   ++info;events.push_back(k==Kind::city?"Cinfo":"Ginfo");
   if(k==Kind::grand&&unknown_row&&static_cast<std::size_t>(slot)==*unknown_row)return {};
   const std::array<std::string,4>types{"CONQUEST","CULTURE","SCIENCE","SPACESHIP"};
   if(slot<0)return Text{TextKind::native_null,{}};
   return Text{TextKind::bytes,types.at(static_cast<std::size_t>(slot))};
  };
 }
 Outcome run(){return resume_diagnostic(j,s);}
};
void tests(){
 static_assert(!std::is_default_constructible_v<BnwGrandLoggingSourceOwner>);
 static_assert(!std::is_copy_constructible_v<BnwGrandLoggingSourceOwner>);
 static_assert(!std::is_default_constructible_v<BnwGrandLoggingContinuation>);
 static_assert(!std::is_copy_constructible_v<Journal>);
 {
  Fixture f;need(f.run()==Outcome::conditional_loggers_returned,"ordered city/Grand did not finish");
  need(f.files==std::vector<std::string>{"CitySpecializationLog.csv","GrandStrategyAI_Log.csv"},"file order differs");
  need(f.rows==std::vector<std::string>{"007, Gandhi, Update: New grand strategy: CULTURE","007, Gandhi, CONQUEST, 3, -7","007, Gandhi, *** CULTURE, -1, 0","007, Gandhi, SCIENCE, 25, -19","007, Gandhi, SPACESHIP, 90, -42"},"retained pressure/active format differs");
  need(f.events==std::vector<std::string>{"Clogging","Cai","Cstrings","Cciv","Csplit","Cgetlog","Celapsed","Cinfo","Cmsg","Ccleanup","Glogging","Gai","Gstrings","Gciv","Gsplit","Ggetlog","Gcount","Gelapsed","Ginfo","Gmsg","Gelapsed","Ginfo","Gmsg","Gelapsed","Ginfo","Gmsg","Gelapsed","Ginfo","Gmsg","Gcleanup"},"source callback ordering differs");
  auto p=f.j->progress();need(p.city_returned&&p.reason0_returned&&p.grand_returned&&p.confirmed_city_messages==1&&p.confirmed_grand_messages==4,"return/count ledger differs");
  const auto n=f.events.size();refuses([&]{f.run();},"completed source replay accepted");need(f.events.size()==n,"replay invoked source");
 }
 {
  Fixture f;f.logging=false;f.s->civilization={};f.s->get_log={};f.s->strategy_count={};
  need(f.run()==Outcome::conditional_loggers_returned,"disabled branch required unvisited sink");
  need(f.events==std::vector<std::string>{"Clogging","Glogging"}&&f.rows.empty(),"logging false read AI/sink");
  need(f.j->progress().reason0_returned,"disabled city logger skipped reason0 return");
 }
 {
  Fixture f;f.ai=false;need(f.run()==Outcome::conditional_loggers_returned,"AI disabled did not return");
  need(f.events==std::vector<std::string>{"Clogging","Cai","Glogging","Gai"},"AI short circuit touched strings");
 }
 {
  Fixture f;f.s->flag={};need(f.run()==Outcome::dependency_pending,"unknown flags not pending");
  need(f.events.empty()&&!f.j->progress().quarantined,"unknown flags emitted or poisoned");
 }
 {
  Fixture f;f.unknown_row=2;need(f.run()==Outcome::dependency_pending,"late unknown Info was invented");
  auto p=f.j->progress();need(p.confirmed_city_messages==1&&p.confirmed_grand_messages==2&&p.get_log_attempts==2&&!p.quarantined&&p.reason0_returned,"late dependency lost prefix");
  const auto n=f.rows.size();f.unknown_row.reset();need(f.run()==Outcome::conditional_loggers_returned,"same source cannot resume dependency");
  need(f.files.size()==2&&f.rows.size()==n+2&&f.j->progress().msg_attempts==5,"resume replayed prior log/file");
 }
 {
  Fixture f;f.throw_row=1;refuses([&]{f.run();},"sink post-output failure accepted");
  const auto p=f.j->progress();need(p.quarantined&&p.external_effect_in_flight&&p.confirmed_grand_messages==1&&p.msg_attempts==3&&f.rows.size()==3,"uncertain output falsely confirmed/rolled back");
  auto n=f.rows.size();f.throw_row.reset();refuses([&]{f.run();},"uncertain output replay accepted");need(f.rows.size()==n,"quarantined replay redelivered");
 }
 {
  Fixture f;f.s->get_log={};need(f.run()==Outcome::dependency_pending,"missing sink not pending");
  need(f.j->progress().get_log_attempts==0&&!f.j->progress().reason0_returned,"missing GetLog advanced reason0");
 }
 {
  Fixture f;f.s->get_log=[](Kind,const std::string&)->std::shared_ptr<const Receiver>{return {};};
  refuses([&]{f.run();},"NULL actual receiver was a successful skip");need(f.j->progress().quarantined&&f.j->progress().get_log_attempts==1,"NULL effect replayable");
 }
 {
  Fixture f;f.s->destroy_strings={};need(f.run()==Outcome::dependency_pending,"missing cleanup skipped");
  need(f.j->progress().confirmed_city_messages==1&&!f.j->progress().reason0_returned&&f.rows.size()==1,"reason switch before cleanup");
  f.s->destroy_strings=[](Kind){};need(f.run()==Outcome::conditional_loggers_returned&&f.rows.size()==5,"cleanup resume replayed city output");
 }
 {
  Fixture f;bool nested=false;f.msg_hook=[&]{refuses([&]{f.run();},"nested callback accepted");nested=true;};
  refuses([&]{f.run();},"caught sink reentry published success");need(nested&&f.j->progress().quarantined&&f.rows.size()==1,"reentry prefix not quarantined");
 }
 {
  Fixture f;bool nested=false;f.current_hook=[&]{if(nested)return;nested=true;refuses([&]{f.run();},"source currentness recursion accepted");};
  refuses([&]{f.run();},"caught currentness recursion escaped guard");need(f.j->progress().quarantined&&f.rows.empty(),"currentness recursion emitted log");
 }
 {
  Fixture f;f.unknown_row=0;need(f.run()==Outcome::dependency_pending,"source prefix missing");
  auto other=std::make_shared<Source>(*f.s);refuses([&]{resume_diagnostic(f.j,other);},"identical-valued replacement source accepted");
  need(!f.j->progress().quarantined,"preflight replacement damaged original prefix");
 }
 {
  Fixture f;f.current=false;refuses([&]{f.run();},"stale source accepted");need(f.rows.empty()&&f.j->progress().quarantined,"stale source effects");
 }
 {
  Fixture f;f.s->strategy_type=[](Kind,std::int32_t)->std::optional<Text>{return Text{TextKind::native_null,{}};};
  need(f.run()==Outcome::conditional_loggers_returned&&f.rows.front()=="007, Gandhi, ","NULL active Info fallback differs");
  need(f.rows[1]=="007, Gandhi, Unknown Type, 3, -7","NULL Grand Info fallback differs");
 }
 {
  Fixture f;f.split=true;f.s->civilization=[](Kind)->std::optional<Text>{return Text{TextKind::bytes,std::string("Gandhi\0junk",11)};};
  need(f.run()==Outcome::conditional_loggers_returned&&f.files[0]=="CitySpecializationLog_Gandhi.csv"&&f.files[1]=="GrandStrategyAI_Log_Gandhi.csv","native C-string/split filename differs");
 }
 {
  Fixture f;f.j=std::make_shared<Journal>(false,-1,std::array<std::int32_t,4>{},std::array<std::int32_t,4>{});f.s->strategy_count=[]()->std::optional<std::int32_t>{return 0;};
  need(f.run()==Outcome::conditional_loggers_returned&&f.files.size()==1&&f.rows.empty()&&!f.j->progress().reason0_returned,"source count0/reason-free Grand differs");
 }
 {
  Fixture f;bool refused=false;std::thread t([&]{try{f.run();}catch(const std::exception&){refused=true;}});t.join();
  need(refused&&!f.j->progress().quarantined&&f.run()==Outcome::conditional_loggers_returned,"foreign thread touched origin state");
 }
 {
  Fixture f;const auto weak_j=std::weak_ptr<Journal>(f.j);const auto weak_s=std::weak_ptr<Source>(f.s);
  f.msg_hook=[&]{f.j.reset();f.s.reset();};
  need(f.run()==Outcome::conditional_loggers_returned&&f.rows.size()==5,"external last-owner release destroyed executing journal/source");
  need(weak_j.expired()&&weak_s.expired(),"last-owner fixture leaked journal/source");
 }
 {
  Fixture f;f.s->construct_strings=[&](Kind){f.events.push_back("ctor side effect");throw std::runtime_error("ctor failed");};
  refuses([&]{f.run();},"string construction exception accepted");
  need(f.j->progress().quarantined&&f.j->progress().string_effect_attempts==1&&f.files.empty(),"construction effect retriable/advanced");
 }
 {
  Fixture f;f.s->get_log=[&](Kind,const std::string&)->std::shared_ptr<const Receiver>{f.files.push_back("created before exception");throw std::runtime_error("open failed after creation");};
  refuses([&]{f.run();},"GetLog uncertain effect accepted");
  const auto n=f.files.size();refuses([&]{f.run();},"GetLog uncertain effect replayed");
  need(f.files.size()==n&&f.j->progress().get_log_attempts==1,"GetLog side effect rollback/replay");
 }
 {
  Fixture f;f.s->destroy_strings=[](Kind){throw std::runtime_error("cleanup failed");};
  refuses([&]{f.run();},"cleanup exception completed city");
  need(f.j->progress().confirmed_city_messages==1&&!f.j->progress().reason0_returned&&f.j->progress().quarantined,"cleanup failure forgot output/advanced reason");
 }
 {
  // Typed lifetime MOCK only: no production capability/factory is forged.
  struct OwnedTestEpoch {unsigned*destructions;~OwnedTestEpoch(){++*destructions;}};
  Fixture f;unsigned destructions=0;
  auto external_epoch=std::make_shared<OwnedTestEpoch>();external_epoch->destructions=&destructions;
  const auto expected=external_epoch.get();const std::weak_ptr<const OwnedTestEpoch> weak_epoch=external_epoch;
  f.s->current=[retained=std::shared_ptr<const OwnedTestEpoch>(external_epoch),expected]{return retained.get()==expected;};
  f.msg_hook=[&]{external_epoch.reset();need(!weak_epoch.expired(),"external epoch drop destroyed source-owned epoch during Msg");};
  need(f.run()==Outcome::conditional_loggers_returned&&destructions==0,"source-owned epoch did not survive all callback postchecks");
  f.j.reset();f.s.reset();
  need(weak_epoch.expired()&&destructions==1,"source epoch ownership leaked after final release");
 }
 refuses([]{BnwGrandLoggingContinuation::resume({});},"missing genuine production continuation accepted");
}
}
int main()try{tests();std::cout<<"PASS "<<checks<<" ordered Grand logging fixture checks; actual startup/sink capability unavailable; zero production receipts\n";}
catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}


