#include "civ5/runtime/bnw_city_food_consumption.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F> void refuses(F&& f,std::string_view wanted={}){bool b=false;try{f();}catch(const std::exception& e){b=true;check(wanted.empty()||std::string_view(e.what()).find(wanted)!=std::string_view::npos,"Wrong source refusal/order");}check(b,"Invalid consumption input accepted");}
auto& named(auto& v,std::string_view name){for(auto& f:v)if(f.name&&std::string_view(f.name)==name)return f;throw std::runtime_error("Selected field unavailable");}
void run(const std::filesystem::path& path){
 const auto original=runtime::identify_file(path).sha256;
 saves::SavedSessionDecodeContext dc;dc.decode_map_regions=true;dc.decode_teams=true;dc.decode_all_players=true;dc.decode_supporting_classes=true;dc.all_players=saves::AllPlayersContext{};
 auto loaded=saves::read_saved_session(path,dc);auto& w=loaded.snapshot;check(w.all_players&&w.all_players->players.size()==64,"Full selected roster unavailable");
 auto it=std::find_if(w.all_players->players.begin(),w.all_players->players.end(),[](const auto& p){return !p.cities.cities.empty();});check(it!=w.all_players->players.end(),"Founded city absent");
 auto& p=*it;auto& c=p.cities.cities.front();const auto owner=p.saved_player_id,id=named(c.core.fields,"m_iID").values.front();
 auto& pop=named(c.core.fields,"m_iPopulation");auto& half=named(p.core.fields,"m_iHalfSpecialistFoodCount");
 auto& counts=named(named(c.management.blocks,"citizens").hashes,"m_aiSpecialistCounts");
 content::RulesDatabase no_specialists;no_specialists.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('FOOD_CONSUMPTION_PER_POPULATION',3);");
 const auto no_defs=no_specialists.cache_global_defines(content::GlobalDefineProfile::bnw);
 // Actual imported city shape with transparent finite policy/Info overlays.
 pop.values.front()=7;half.values.front()=0;
 auto base=runtime::diagnose_bnw_city_food_consumption(w,no_specialists,no_defs,owner,id,1);
 check(base.consumption()==24&&!base.reads_specialists()&&!base.native_gc_cache_owned()&&!base.full_growth_ready(),"Inactive-half source branch or readiness differs");
 base.require_same_raw_inputs(w);
 auto saved_counts=counts;counts.rows.clear();counts.wire_count=0; // unreachable native branch must not require these inputs
 base.require_same_raw_inputs(w);check(runtime::diagnose_bnw_city_food_consumption(w,no_specialists,no_defs,owner,id).consumption()==21,"Inactive half read missing Info/count/default dependency");
 counts=saved_counts;half.values.front()=-1;
 check(runtime::diagnose_bnw_city_food_consumption(w,no_specialists,no_defs,owner,id,-10).consumption()==-9,"Source negative extra unexpectedly clamped");
 content::RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('FOOD_CONSUMPTION_PER_POPULATION',3),('DEFAULT_SPECIALIST',0);CREATE TABLE Specialists(ID,Type);INSERT INTO Specialists VALUES(0,'DEFAULT'),(1,'ONE'),(2,'TWO');");
 const auto defs=db.cache_global_defines(content::GlobalDefineProfile::bnw);
 const auto hash=[](const char* type){return content::saved_info_type_hash(type);};
 counts.rows={{hash("DEFAULT"),{INT32_MAX}},{hash("ONE"),{1}},{hash("TWO"),{1}}};counts.wire_count=3;half.values.front()=1;
 auto q=runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);
 check(q.specialist_total()==2&&q.reduction()==3&&q.consumption()==18&&q.reads_specialists(),"Default exclusion or aggregate-before-/2 source semantics differ");
 q.require_same_raw_inputs(w);
 bool foreign=false;std::thread t([&]{try{q.require_same_raw_inputs(w);}catch(const std::exception&){foreign=true;}});t.join();check(foreign,"Foreign thread currentness accepted");
 counts.rows[1].values.front()=-1;counts.rows[2].values.front()=-2;
 refuses([&]{q.require_same_raw_inputs(w);},"stale");
 auto neg=runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);
 check(neg.specialist_total()==-3&&neg.reduction()==-4&&neg.consumption()==25,"Signed reduction division must truncate toward zero");
 ++pop.values.front();refuses([&]{neg.require_same_raw_inputs(w);},"stale");pop.values.front()=7;
 auto original_rows=counts.rows;counts.rows.pop_back();counts.wire_count=2;
 refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);},"missing raw count");
 counts.rows=original_rows;counts.wire_count=3;counts.rows[1].type_hash=hash("UNKNOWN");refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);},"unresolved");
 counts.rows=original_rows;counts.wire_count=2;refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);},"wire row count");
 counts.wire_count=3;counts.rows[1].values={INT32_MAX};counts.rows[2].values={1};refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);},"int32 overflow");
 counts.rows[1].values={INT32_MAX/2};counts.rows[2].values={0};refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);},"int32 overflow"); // product must fit BEFORE division
 pop.values.front()=INT32_MAX;refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id,1);},"int32 overflow");
 const auto half_name=half.name;half.name="unavailable_policy";
 refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);},"int32 overflow"); // base multiply precedes policy getter
 half.name=half_name;pop.values.front()=7;counts.rows=original_rows;
 content::RulesDatabase holes;holes.apply_sql("CREATE TABLE Specialists(ID,Type);INSERT INTO Specialists VALUES(0,'DEFAULT'),(2,'TWO');");refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,holes,defs,owner,id);},"dense Info span");
 refuses([&]{(void)runtime::diagnose_bnw_city_food_consumption(w,no_specialists,no_defs,owner,id);});
 const auto untouched=named(c.core.fields,"m_iFood").values; (void)runtime::diagnose_bnw_city_food_consumption(w,db,defs,owner,id);
 check(named(c.core.fields,"m_iFood").values==untouched&&runtime::identify_file(path).sha256==original,"Read-only consumption changed food/source save");
 std::cout<<"PASS BNW consumption on decoded city: source branch order/default exclusion/signed aggregate reduction/stale/thread/unknown-slot/overflow guards; native GC and fresh-yield authority unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodConsumptionDiagnostic>);
static_assert(!std::is_copy_constructible_v<runtime::BnwCityFoodConsumptionDiagnostic>);
int main(int argc,char** argv){try{check(argc==2,"Supply selected BNW save");run(argv[1]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL consumption: "<<e.what()<<'\n';return 1;}}
