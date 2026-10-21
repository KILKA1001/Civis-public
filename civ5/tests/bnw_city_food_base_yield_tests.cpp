#include "civ5/runtime/bnw_city_food_base_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f,std::string_view expected={}){bool failed=false;try{f();}catch(const std::exception& e){failed=true;check(expected.empty()||std::string_view(e.what()).find(expected)!=std::string_view::npos,"Wrong food-base source refusal order");}check(failed,"Invalid food-base getter accepted");}
auto& named(auto& rows,std::string_view name){for(auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Selected raw field unavailable");}
struct Pick{std::size_t p,c;std::int64_t owner,id;};
Pick choose(const runtime::LiveSessionState& s){for(std::size_t p=0;p<s.world.all_players->players.size();++p){const auto& player=s.world.all_players->players[p];if(!player.cities.cities.empty())return {p,0,player.saved_player_id,named(player.cities.cities.front().core.fields,"m_iID").values.front()};}throw std::runtime_error("Actual founded city missing");}
auto& raw(auto& s,const Pick& p){return s.world.all_players->players.at(p.p).cities.cities.at(p.c);}
constexpr const char* names[]={"m_aiBaseYieldRateFromTerrain","m_aiBaseYieldRateFromBuildings","m_aiBaseYieldRateFromSpecialists","m_aiBaseYieldRateFromMisc","m_aiBaseYieldRateFromReligion"};
void set(runtime::LiveSessionState& s,const Pick& p,const std::array<std::int64_t,5>& values){
 auto& components=s.cities.players.at(p.owner).cities.at(p.id).yields[0];
 for(std::size_t n=0;n<5;++n)named(raw(s,p).core.fields,names[n]).values.at(0)=values[n];
 components.terrain=values[0];components.buildings=values[1];components.specialists=values[2];components.misc=values[3];components.religion=values[4];
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=runtime::identify_file(save).sha256;auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);check(bool(boot.preview.host),"Actual BNW host unavailable");
 const auto& base=boot.preview.host->session().state();const auto p=choose(base);
 auto actual=runtime::inspect_bnw_city_food_base_yield(base,p.owner,p.id);actual.require_same_inputs(base);
 std::int64_t total=0;for(const auto value:actual.components())total+=value;
 check(total==actual.value()&&actual.base_getter_complete()&&!actual.refreshed_food_yield()&&!actual.full_growth_ready(),"Actual imported getter sum/readiness mismatch");
 for(const auto values:{std::array<std::int64_t,5>{3,5,7,-2,11},{0,0,0,0,0},{-3,-5,7,2,-11},{INT32_MAX,0,0,0,0},{INT32_MIN,0,0,0,0}}){
  auto s=base;set(s,p,values);auto q=runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);std::int64_t expected=0;for(auto n:values)expected+=n;check(q.value()==expected,"Native base-food finite sum differs");q.require_same_inputs(s);
  auto indexed=runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id&8191);check(indexed.value()==expected,"Index-only city handle changed base-food getter");
  named(raw(s,p).core.fields,"m_iFood").name="unreachable_stock";named(raw(s,p).core.fields,"m_iPopulation").name="unreachable_population";
  s.cities.players.at(p.owner).cities.at(p.id).production_orders.reset();
  q.require_same_inputs(s);check(runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id).value()==expected,"Base getter read growth/order operands");
 }
 for(const auto values:{std::array<std::int64_t,5>{INT32_MAX,1,-1,0,0},{INT32_MIN,-1,1,0,0},{std::int64_t(INT32_MAX)+1,0,0,0,0}}){auto s=base;set(s,p,values);refuses([&]{(void)runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);},"overflow");}
 {
  auto s=base;set(s,p,{INT32_MAX,1,-1,0,0});named(raw(s,p).core.fields,names[2]).name="unreachable_specialists";
  refuses([&]{(void)runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);},"overflow");
 }
 {
  auto s=base;set(s,p,{1,2,3,4,5});auto q=runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);
  ++named(raw(s,p).core.fields,names[0]).values[0];refuses([&]{q.require_same_inputs(s);},"raw/owned");
  set(s,p,{2,2,3,4,5});refuses([&]{q.require_same_inputs(s);},"retained");
  auto moved=std::move(q);refuses([&]{(void)q.value();},"moved");check(moved.value()==15,"Moved result lost copied fact");
  bool denied=false;std::thread worker([&]{try{(void)moved.value();}catch(const std::exception&){denied=true;}});worker.join();check(denied,"Foreign food-base result thread accepted");
 }
 {
  auto s=base;set(s,p,{0,0,0,0,0});s.cities.players.at(p.owner).cities.at(p.id).yields.erase(0);
  check(runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id).value()==0,"Actual raw zero sparse components were rejected");
  named(raw(s,p).core.fields,names[0]).values[0]=1;refuses([&]{(void)runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);});
 }
 {
  auto s=base;named(raw(s,p).core.fields,names[0]).wire_count=5;refuses([&]{(void)runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);});
  s=base;s.cities.players.at(p.owner).cities.at(p.id).source_core.reset();refuses([&]{(void)runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);});
  s=base;s.cities.yield_slots=5;refuses([&]{(void)runtime::inspect_bnw_city_food_base_yield(s,p.owner,p.id);});
 }
 check(runtime::identify_file(save).sha256==original,"Original Civ5Save changed");actual.require_same_inputs(base);
 std::cout<<"PASS actual decoded BNW native-width source-order five-component food base yield, raw/owned/currentness/overflow guards; no fresh-yield/growth permission\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodBaseYield>);
int main(int argc,char** argv){try{check(argc==3,"Expected selected BNW save and canonical shared installation root");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
