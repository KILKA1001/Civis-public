#include "civ5/runtime/bnw_city_food_production_order.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Invalid food production getter accepted");}
template<class F>void retired_before_head(F&& f){bool failed=false;try{f();}catch(const std::exception& e){failed=std::string_view(e.what())=="Food production SAME immutable rules scope stale";}check(failed,"Retired rules must decline before malformed borrowed head");}
auto& named(auto& rows,std::string_view name){for(auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Actual raw field unavailable");}
struct Pick{std::size_t p,c;std::int64_t owner,id;};
Pick choose(const runtime::LiveSessionState& s){for(std::size_t p=0;p<s.world.all_players->players.size();++p){const auto& player=s.world.all_players->players[p];if(!player.cities.cities.empty())return {p,0,player.saved_player_id,named(player.cities.cities.front().core.fields,"m_iID").values.front()};}throw std::runtime_error("Selected city unavailable");}
auto& city(auto& s,const Pick& p){return s.world.all_players->players.at(p.p).cities.cities.at(p.c);}
auto& queue(auto& s,const Pick& p){return named(city(s,p).production.blocks,"queue");}
auto& orders(auto& s,const Pick& p){return s.cities.players.at(p.owner).cities.at(p.id).production_orders;}
auto& counter(auto& s,const Pick& p){return named(s.world.all_players->players.at(p.p).core.fields,"m_iMilitaryFoodProductionCount");}
void empty(runtime::LiveSessionState& s,const Pick& p){queue(s,p).children.clear();named(queue(s,p).fields,"wire_count").value=0;orders(s,p)=std::vector<runtime::CityYieldState::ProductionOrder>{};}
void order(runtime::LiveSessionState& s,const Pick& p,std::int64_t kind,const char* type,std::int64_t id){
 const auto hash=content::saved_info_type_hash(type);
 city(s,p).production.context.bnw_city_version=5;
 auto& q=queue(s,p);q.children.resize(1);auto& r=q.children.front();r.index=0;
 r.fields={{"eOrderType",kind},{"iData1_hash",hash},{"iData2",-1},{"bSave",0},{"bRush",0}};named(q.fields,"wire_count").value=1;
 runtime::CityYieldState::ProductionOrder o;o.type=kind;o.item=id;o.secondary=-1;o.source_hash=hash;
 orders(s,p)=std::vector<runtime::CityYieldState::ProductionOrder>{o};
}
void run(const std::filesystem::path& path,const std::filesystem::path& game){
 const auto original=runtime::identify_file(path).sha256;auto boot=ui::bootstrap_selected_bnw_preview(path,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);check(bool(boot.preview.host),"Selected actual host unavailable");
 const auto& host=*boot.preview.host;const auto& base=host.session().state();const auto p=choose(base);
 const auto rules=runtime::BnwCityFoodProductionRules::load(host.database(),host.identity().rules_sha256);
 auto actual=runtime::inspect_bnw_city_food_production_order(base,rules,host.identity().rules_sha256,p.owner,p.id);
 actual.require_same_inputs(base,host.identity().rules_sha256);check(actual.bnw_queue_rules_getter_complete()&&!actual.refreshed_food_yield()&&!actual.full_growth_ready(),"Getter claimed fresh food/growth");
 {
  auto s=base;empty(s,p);counter(s,p).name="unreachable_military_count";
  auto no=runtime::inspect_bnw_city_food_production_order(s,nullptr,"unused",p.owner,p.id);check(!no.food_production()&&!no.read_unit_info()&&!no.read_military_counter(),"Empty head read later producer");no.require_same_inputs(s,"unused");
 }
 for(const auto kind:{1,2,3,4}){
  auto s=base;order(s,p,kind,"IRRELEVANT_NON_TRAIN",-1);counter(s,p).name="unreachable_military_count";
  auto no=runtime::inspect_bnw_city_food_production_order(s,nullptr,"unused",p.owner,p.id);check(!no.food_production()&&!no.read_unit_info()&&!no.read_military_counter(),"NonTRAIN source branch read UnitInfo/player");
 }
 {
  auto s=base;order(s,p,1,"IRRELEVANT_NON_TRAIN",-1);auto& q=queue(s,p);q.children.push_back(q.children.front());q.children.back().index=1;q.children.back().fields.clear();orders(s,p)->push_back(orders(s,p)->front());orders(s,p)->back().type=99;named(q.fields,"wire_count").value=2;
  auto no=runtime::inspect_bnw_city_food_production_order(s,nullptr,"unused",p.owner,p.id);check(!no.food_production(),"Head-only getter read later malformed order");no.require_same_inputs(s,"unused");
 }
 // Finite predicate fixtures on the actual decoded raw/owned queue. The tiny
 // immutable SQL scope tests exact GetBool semantics, not native GC startup.
 content::RulesDatabase db;db.apply_sql("CREATE TABLE Units(ID INTEGER,Type TEXT,Food,MilitaryProduction);INSERT INTO Units VALUES(0,'FIX_FOOD','True',0),(1,'FIX_ARMY',0,1),(3,'FIX_CIVIL',0,0),(4,'FIX_FLOAT',1.0,'TRUE'),(5,'FIX_NULL',NULL,NULL);");
 db.seal_immutable_preview_fingerprints();const auto sha=db.gameplay_fingerprint_sha256();const auto finite=runtime::BnwCityFoodProductionRules::load(db,sha);
 {
  auto s=base;order(s,p,0,"FIX_FOOD",0);counter(s,p).name="unreachable_military_count";
  auto yes=runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);check(yes.food_production()&&yes.read_unit_info()&&!yes.read_military_counter(),"Food UnitInfo did not short circuit military counter");yes.require_same_inputs(s,sha);
  auto moved=std::move(yes);refuses([&]{(void)yes.food_production();});check(moved.food_production(),"Moved getter lost facts");
 }
 for(const auto count:{-1,0,1})for(const auto unit:{1,3,4,5}){
  const char* types[]={"", "FIX_ARMY","", "FIX_CIVIL","FIX_FLOAT","FIX_NULL"};
  auto s=base;order(s,p,0,types[unit],unit);counter(s,p).values.front()=count;
  auto result=runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);
  check(result.read_unit_info()&&result.read_military_counter()&&result.food_production()==(count>0&&(unit==1||unit==4)),"Source Food/GetBool/military>0 branch differs");result.require_same_inputs(s,sha);
  ++counter(s,p).values.front();refuses([&]{result.require_same_inputs(s,sha);});
 }
 {
  auto s=base;order(s,p,0,"MISSING_NATIVE_UNIT_HASH",-1);counter(s,p).name="unreachable_military_count";
  auto no=runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);check(!no.food_production()&&no.read_unit_info()&&!no.read_military_counter(),"NULL UnitInfo read military counter");
  // Native pre-v5 numeric Info holes and out-of-range IDs return NULL.
  for(const auto id:{-1,2,99}){city(s,p).production.context.bnw_city_version=4;auto& r=queue(s,p).children.front();named(r.fields,"iData1_hash").name="iData1";named(r.fields,"iData1").value=id;orders(s,p)->front().source_hash.reset();orders(s,p)->front().item=id;
   check(!runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id).food_production(),"Native nullable/out-of-range UnitInfo differs");named(r.fields,"iData1").name="iData1_hash";
  }
 }
 {
  auto s=base;order(s,p,0,"FIX_ARMY",1);counter(s,p).values.front()=1;
  auto result=runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);
  refuses([&]{result.require_same_inputs(s,"wrong");});
  orders(s,p)->front().item=3;refuses([&]{(void)runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);});orders(s,p)->front().item=1;
  named(queue(s,p).children.front().fields,"bRush").value=1;orders(s,p)->front().rush=true;refuses([&]{result.require_same_inputs(s,sha);});
  bool refused=false;std::thread worker([&]{try{(void)result.food_production();}catch(const std::exception&){refused=true;}});worker.join();check(refused,"Foreign getter thread accepted");
  refuses([&]{(void)runtime::inspect_bnw_city_food_production_order(s,nullptr,sha,p.owner,p.id);});
 }
 {
  auto s=base;order(s,p,0,"FIX_ARMY",1);counter(s,p).values.front()=1;auto result=runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);
  auto nontrain=base;empty(nontrain,p);auto empty_result=runtime::inspect_bnw_city_food_production_order(nontrain,finite,sha,p.owner,p.id);
  refuses([&]{db.apply_sql("UPDATE Units SET Food=1");});refuses([&]{result.require_same_inputs(s,sha);});
  // Detached pure getter-negative variation, never an issuing live session.
  // Poison the reached head after rule retirement. Exact error proves both
  // public factory and retained equality preflight rules before head/world.
  named(queue(s,p).fields,"wire_count").name="malformed_borrowed_head";
  retired_before_head([&]{result.require_same_inputs(s,sha);});
  retired_before_head([&]{(void)runtime::inspect_bnw_city_food_production_order(s,finite,sha,p.owner,p.id);});
  named(queue(nontrain,p).fields,"wire_count").name="malformed_borrowed_empty_head";
  retired_before_head([&]{empty_result.require_same_inputs(nontrain,sha);});
 }
 check(runtime::identify_file(path).sha256==original,"Original save changed");
 std::cout<<"PASS actual BNW queue/rules food-production getter, head-only branches, source GetBool/NULL/military predicates and stale/thread/rules guards; no fresh-yield/growth permission\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodProductionRules>);
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodProductionOrder>);
int main(int argc,char** argv){try{check(argc==3,"Expected existing BNW save and canonical game root");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
