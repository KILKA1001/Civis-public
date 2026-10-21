#include "civ5/runtime/bnw_city_current_food_consumption.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f,std::string_view text={}){bool failed=false;try{f();}catch(const runtime::BnwCityCurrentFoodConsumptionUnavailable& e){failed=true;check(text.empty()||std::string_view(e.what()).find(text)!=std::string_view::npos,"Wrong current food consumption refusal");}check(failed,"Current food consumption accepted unavailable inputs");}
auto& named(auto& rows,std::string_view n){for(auto& f:rows)if(f.name&&std::string_view(f.name)==n)return f;throw std::runtime_error("Selected raw field unavailable");}
struct Pick{std::size_t p,c;std::int64_t owner,id;};
Pick choose(const runtime::LiveSessionState& s){for(std::size_t p=0;p<s.world.all_players->players.size();++p){const auto& player=s.world.all_players->players[p];if(!player.cities.cities.empty()&&named(player.core.fields,"m_iHalfSpecialistFoodCount").values.front()<=0)return {p,0,player.saved_player_id,named(player.cities.cities.front().core.fields,"m_iID").values.front()};}throw std::runtime_error("Actual city without half-specialist policy absent");}
auto& raw(auto& s,const Pick& p){return s.world.all_players->players.at(p.p).cities.cities.at(p.c);}
auto& half(auto& s,const Pick& p){return named(s.world.all_players->players.at(p.p).core.fields,"m_iHalfSpecialistFoodCount");}
auto& counts(auto& s,const Pick& p){return named(named(raw(s,p).management.blocks,"citizens").hashes,"m_aiSpecialistCounts");}
void population(runtime::LiveSessionState& s,const Pick& p,std::int64_t n){named(raw(s,p).core.fields,"m_iPopulation").values.front()=n;s.cities.players.at(p.owner).cities.at(p.id).population=n;}
void finite(content::RulesDatabase& db,bool specialists){db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('FOOD_CONSUMPTION_PER_POPULATION',3);");if(specialists)db.apply_sql("INSERT INTO Defines VALUES('DEFAULT_SPECIALIST',0);CREATE TABLE Specialists(ID,Type);INSERT INTO Specialists VALUES(0,'DEFAULT'),(1,'ONE'),(2,'TWO');");db.seal_immutable_preview_fingerprints();}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=runtime::identify_file(save).sha256;auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);check(bool(boot.preview.host),"Actual immutable BNW host unavailable");
 const auto& host=*boot.preview.host;const auto& base=host.session().state();const auto p=choose(base);const auto& db=host.database();const auto sha=host.identity().rules_sha256;
 auto actual=runtime::inspect_bnw_city_current_food_consumption(base,db,sha,p.owner,p.id);actual.require_same_inputs(base,sha);
 const auto defs=db.cache_global_defines(content::GlobalDefineProfile::bnw);const auto* food=defs.find("FOOD_CONSUMPTION_PER_POPULATION");check(food&&food->lookup.found,"Actual per-population define unavailable");
 check(actual.value()==base.cities.players.at(p.owner).cities.at(p.id).population*std::get<std::int64_t>(food->lookup.value)&&actual.consumption_getter_complete()&&!actual.native_gc_cache_owned()&&!actual.refreshed_food_yield()&&!actual.full_growth_ready(),"Actual current BNW consumption/readiness differs");
 check(runtime::inspect_bnw_city_current_food_consumption(base,db,sha,p.owner,p.id&8191).value()==actual.value(),"Native index-only handle changed consumption");
 refuses([&]{(void)runtime::inspect_bnw_city_current_food_consumption(base,db,"wrong",p.owner,p.id);},"identity mismatch");
 {
  auto s=base;++s.cities.players.at(p.owner).cities.at(p.id).population;refuses([&]{actual.require_same_inputs(s,sha);},"raw/owned");
  population(s,p,actual.population_with_extra()+1);refuses([&]{actual.require_same_inputs(s,sha);},"retained");
  s=base;half(s,p).values.front()=1;refuses([&]{actual.require_same_inputs(s,sha);},"stale");
  s=base;s.cities.players.at(p.owner).cities.at(p.id).source_core.reset();refuses([&]{(void)runtime::inspect_bnw_city_current_food_consumption(s,db,sha,p.owner,p.id);});
 }
 {
  auto s=base;population(s,p,7);half(s,p).values.front()=0;content::RulesDatabase small;finite(small,false);const auto id=small.gameplay_fingerprint_sha256();
  counts(s,p).rows.clear();counts(s,p).wire_count=0;named(raw(s,p).core.fields,"m_iFood").name="unreachable_stock";
  named(raw(s,p).core.fields,"m_aiBaseYieldRateFromTerrain").name="unreachable_yield";s.cities.players.at(p.owner).cities.at(p.id).production_orders.reset();
  auto q=runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id,1);check(q.value()==24&&!q.reads_specialists(),"Inactive policy read unreachable counts/Info/yield/order");q.require_same_inputs(s,id);
  half(s,p).values.front()=-1;check(runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id,-10).value()==-9,"Negative extra or inactive policy was clamped");
  auto moved=std::move(q);refuses([&]{(void)q.value();},"moved");check(moved.value()==24,"Moved current consumption lost value");
  bool denied=false;std::thread t([&]{try{(void)moved.value();}catch(const runtime::BnwCityCurrentFoodConsumptionUnavailable&){denied=true;}});t.join();check(denied,"Foreign thread consumed rules guard");
 }
 {
  auto s=base;population(s,p,7);half(s,p).values.front()=1;content::RulesDatabase small;finite(small,true);const auto id=small.gameplay_fingerprint_sha256();auto& rows=counts(s,p);
  const auto hash=[](const char* n){return content::saved_info_type_hash(n);};rows.rows={{hash("DEFAULT"),{INT32_MAX}},{hash("ONE"),{1}},{hash("TWO"),{1}}};rows.wire_count=3;
  auto q=runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id);check(q.specialist_total()==2&&q.reduction()==3&&q.value()==18&&q.reads_specialists(),"Default exclusion/aggregate before division differs");q.require_same_inputs(s,id);
  rows.rows[1].values={-1};rows.rows[2].values={-2};refuses([&]{q.require_same_inputs(s,id);},"stale");auto neg=runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id);check(neg.reduction()==-4&&neg.value()==25,"Signed odd reduction division differs");
  rows.rows.pop_back();rows.wire_count=2;refuses([&]{(void)runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id);},"missing raw count");
  rows.rows.push_back({hash("TWO"),{0}});rows.wire_count=3;rows.rows[1].values={INT32_MAX/2};refuses([&]{(void)runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id);},"overflow");
  population(s,p,INT32_MAX);half(s,p).name="unreachable_policy";refuses([&]{(void)runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id);},"overflow");
 }
 {
  auto s=base;population(s,p,7);half(s,p).values.front()=0;content::RulesDatabase unsealed;unsealed.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('FOOD_CONSUMPTION_PER_POPULATION',2);");
  refuses([&]{(void)runtime::inspect_bnw_city_current_food_consumption(s,unsealed,unsealed.gameplay_fingerprint_sha256(),p.owner,p.id);},"immutable");
  content::RulesDatabase small;finite(small,false);const auto id=small.gameplay_fingerprint_sha256();auto q=runtime::inspect_bnw_city_current_food_consumption(s,small,id,p.owner,p.id);
  try{small.apply_sql("UPDATE Defines SET Value=4;");}catch(const std::exception&){}refuses([&]{(void)q.value();},"rules stale");
 }
 actual.require_same_inputs(base,sha);check(runtime::identify_file(save).sha256==original,"Original selected save changed");
 std::cout<<"PASS actual decoded BNW-owned current foodConsumption, same immutable rules/raw/owned binding, source specialist branches/native overflow/stale/thread guards; no fresh yield/full growth permission\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityCurrentFoodConsumption>);
int main(int argc,char** argv){try{check(argc==3,"Expected selected BNW save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
