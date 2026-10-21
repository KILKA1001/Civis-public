#include "../src/runtime/bnw_grand_logging_diagnostic.hpp"
#include <fstream>
#include <iostream>
#include <limits>
using namespace civ5::runtime::diagnostic;
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
void tests(){
 const auto observe=[](int v)->std::optional<ModeledGcLoggingByte>{if(v<0)return {};return ModeledGcLoggingByte{static_cast<std::uint8_t>(v)};};
 const auto a=grand_log_predicates(observe(0),observe(-1));need(a.outcome==GrandLogPredicate::modeled_logging_zero_return&&a.attempted==1&&a.read==1,"disabled logging did not short circuit absent AI observation");
 const auto b=grand_log_predicates(observe(1),observe(-1));need(b.outcome==GrandLogPredicate::missing_ai_logging&&b.read==1,"missing AI observation was invented");
 need(actual_grand_log_frontier()==GrandLogActualFrontier::genuine_gc_logging_lifecycle_owner_required,"modeled branch granted actual owner");
 ModeledGrandLogInputs i;i.elapsed=7;i.active=1;i.civilization={TextObservation::native_value,"Gandhi"};i.split=ModeledGcLoggingByte{0};
 i.strategy_types={ModeledLogText{TextObservation::native_value,"CONQUEST"},ModeledLogText{TextObservation::native_value,"CULTURE"},ModeledLogText{TextObservation::native_null,{}},ModeledLogText{TextObservation::native_value,"SPACESHIP"}};
 i.priorities={0,-1,25,2147483647};i.pressure_changes={-7,0,-19,std::numeric_limits<std::int32_t>::min()};
 const auto g=grand_strategy_log_plan(i);need(g.filename=="GrandStrategyAI_Log.csv"&&g.get_log_calls==1&&g.messages==std::vector<std::string>{"007, Gandhi, CONQUEST, 0, -7","007, Gandhi, *** CULTURE, -1, 0","007, Gandhi, Unknown Type, 25, -19","007, Gandhi, SPACESHIP, 2147483647, -2147483648"},"source Grand formatting golden differs");
 const auto s=new_grand_specialization_log_plan(i);need(s.filename=="CitySpecializationLog.csv"&&s.messages==std::vector<std::string>{"007, Gandhi, Update: New grand strategy: CULTURE"},"source specialization formatting golden differs");
 i.split=ModeledGcLoggingByte{1};i.active=-1;need(new_grand_specialization_log_plan(i).messages[0]=="007, Gandhi, ","NULL active Info did not preserve empty type string");need(grand_strategy_log_plan(i).filename=="GrandStrategyAI_Log_Gandhi.csv","Grand split source filename differs");need(new_grand_specialization_log_plan(i).filename=="CitySpecializationLog_Gandhi.csv","specialization split source filename differs");
 i.active=2;need(new_grand_specialization_log_plan(i).messages[0]=="007, Gandhi, ","NULL Info source emitted invented type");
 i.civilization={TextObservation::native_null,{}};i.elapsed=-1;need(new_grand_specialization_log_plan(i).messages[0]=="-01, , ","source NULL-description or signed03d differs");
 i.civilization={};bool refused=false;try{(void)grand_strategy_log_plan(i);}catch(const std::runtime_error&){refused=true;}need(refused,"unknown description replaced by empty");
 i.civilization={TextObservation::native_value,"Gandhi"};i.split.reset();refused=false;try{(void)grand_strategy_log_plan(i);}catch(const std::runtime_error&){refused=true;}need(refused,"unknown split replaced by false");
 i.split=ModeledGcLoggingByte{0};i.strategy_types[0]={};refused=false;try{(void)grand_strategy_log_plan(i);}catch(const std::runtime_error&){refused=true;}need(refused,"unknown Info replaced by NULL fallback");
}
}
int main(int argc,char**argv)try{
 tests();unsigned count=0;
 if(argc==2){std::ifstream f(argv[1]);need(bool(f),"source oracle fixture file absent");unsigned id,kind,expected,attempts,reads;int logging,ai;while(f>>id>>kind>>logging>>ai>>expected>>attempts>>reads){(void)id;need(kind<2,"unknown native logger");const auto flag=[](int v)->std::optional<ModeledGcLoggingByte>{if(v<0)return {};if(v>255)throw std::runtime_error("source flag byte width differs");return ModeledGcLoggingByte{static_cast<std::uint8_t>(v)};};const auto q=grand_log_predicates(flag(logging),flag(ai));need(static_cast<unsigned>(q.outcome)==expected&&q.attempted==attempts&&q.read==reads,"diagnostic predicate differs from actual original CPU oracle");++count;}need(f.eof()&&count>0,"source fixture parsing/count invalid");}
 else need(argc==1,"optional original fixture file only");
 std::cout<<"PASS Grand logging source format/refusal goldens; original CPU predicate cases "<<count<<"; actual logging owner unavailable\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
